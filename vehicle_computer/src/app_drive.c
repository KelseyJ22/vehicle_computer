#include <global.h>
#include <math.h>
#include "app_send_data.h"
#include "dev_drive_config.h"
#include "dev_tritium.h"
#include "dev_adc.h"
#include "global.h"
#include "app_pedals.h"
#include "app_cruise_control.h"
#include "app_drive.h"
#include "lib_protobuf_data_struct.h"

inline bool app_drive_(DriveInputT *input);
inline bool app_drive_checkThrottle(DriveInputT *input);
inline bool app_drive_checkBrake(DriveInputT *input);
float app_drive_handlePedals(int64_t last_active_time, DriveInputT input);
void app_drive_setLEDs(float tritium_command);

// tritiums will turn off after 60 seconds of inactivity
#define TRITIUM_IDLE_TIME_MS (60 * 1000)

/*
 * Based on the current drive state and tritium commands, set the correct LEDs.
 */
void app_drive_setLEDs(float tritium_command)
{
    if(drive_state.drive_mode == MANUAL_REVERSE)
    {
      Pin_SetHigh(LED_REVERSE); // LED_REVERSE == WHITE
    }
    else
    {
      Pin_SetLow(LED_REVERSE);
    }
 
    if (tritium_command > 0.0f)
    {
      Pin_SetHigh(LED_THROTTLE);
    }
    else if(tritium_command < 0.0f)
    {
      Pin_Toggle(LED_THROTTLE);
    }
    else
    {
      Pin_SetLow(LED_THROTTLE);
    }
}


/*
 * Reads ADCs and computes the proper drive state, drive mode, and tritium command
 * accordingly, as well as calling a utility function to set appropriate LEDs.
 */
float app_drive_handlePedals(int64_t last_active_time, DriveInputT input)
{
    // read pedal position from ADCs
    dev_adc_poll(&adc_val);

    input.brake = adc_val.brake[ADC_BUFF_SIZE - 1];
    input.throttle = adc_val.throttle[ADC_BUFF_SIZE - 1];

    // note: call applyDeadzone() before doing anything with input, like calling
    // BrakeResponse, ThrottleResponse, checkBrake for consistent results
    app_pedals_applyDeadzone(&input);

    // generate a command based on pedal position
    bool brake_old = drive_state.brake;
    float tritium_command = app_drive_drive(&input);
    bool brake_new = drive_state.brake;

    if (brake_old != brake_new)
    {
      app_send_data_lightsCommands(); // Announce brake for lightboard
      app_send_data_buttonBoard(); // Brake may have kicked out of cruise, update bb
    }
    app_drive_setLEDs(tritium_command);
    
    // if idle, turn off power to Tritiums to save power.
    bool tritiums_on_old = drive_state.tritiums_on;
    if(app_pedals_throttleResponse(input.throttle) || app_pedals_brakeResponse(input.brake) ||
        (drive_state.drive_mode == CRUISE))
    {
      drive_state.tritiums_on = true;
      last_active_time = time_ms;
    }
    else if (time_ms - last_active_time > TRITIUM_IDLE_TIME_MS) {
      drive_state.tritiums_on = false;
    }
    bool tritiums_on_new = drive_state.tritiums_on;
    
    if (tritiums_on_old != tritiums_on_new)
    {
      lib_data_struct_internal.tritiums_on = true;
      lib_data_struct_internal.has_tritiums_on = true;
    }
    return tritium_command;
}


/**
 * Reads pedal positions and generates a command for the tritiums.
 * Cruise is handled internally in the Drive module.
 */
void app_drive_driveTask(void* pvParameters)
{
  dev_adc_setup();
  dev_adc_initVal(&adc_val);
  DriveInputT input =
  {
    .brake = 0,
    .throttle = 0
  };
  // send reset messages to both tritiums
  dev_tritium_reset(&tritiums[0]);
  dev_tritium_reset(&tritiums[1]);
  
  while(true)
  {
    dev_adc_poll(&adc_val);
    input.throttle = adc_val.throttle[ADC_BUFF_SIZE - 1];
    raw_drive_input.brake = input.brake;
    raw_drive_input.throttle = input.throttle;
    app_pedals_applyDeadzone(&input);

    // only allow the car to start when the pedal is not depressed
    if(app_drive_checkThrottle(&input))
    {
      vTaskDelay(40);
      continue;
    }
    else
    {
      break;
    }
  }

  int64_t last_active_time = time_ms;

  while(true)
  {
    float tritium_command = app_drive_handlePedals(last_active_time, input);
    dev_tritium_send(tritium_command, &tritiums[0]);
    dev_tritium_send(tritium_command, &tritiums[1]);

    // don't wait longer than 250ms, or the tritium will coast
    // (matched with 1/ADC_RATE)
    vTaskDelay(40);
  }
}


/**
 * Returns a float that will be passed on to the Tritium module.
 * It will be roughly between -1 and 1, where -1 is hard brake and 1 is full throttle.
 * Output depends on the control states: braking, reversing, cruise, and input throttle and brake.
 */
float app_drive_drive(DriveInputT *input)
{
  float output = 0.0f;

  if(drive_state.drive_mode == MANUAL_REVERSE)
  {
    output = app_drive_manual(input);
  }
  else if(drive_state.drive_mode == CRUISE)
  {
    // kick out of cruise if they press the brake
    if(app_drive_checkBrake(input))
    {
      drive_state.drive_mode = MANUAL_FORWARD;
      drive_state.iTerm = 0.0f;
      drive_state.cruiseVelocity = 0.0f;
      output = app_drive_manual(input);
      lib_data_struct_internal.drive_mode = MANUAL_FORWARD;
      lib_data_struct_internal.has_drive_mode = true;
      lib_data_struct_internal.cruise_speed = 0;
      lib_data_struct_internal.has_cruise_speed = false;
      lib_data_struct_internal.cruise_on = false;
      lib_data_struct_internal.has_cruise_on = true;
    }
    // don't change cruise mode or cruise velocity, but manually speed up
    else if(app_drive_checkThrottle(input))
    {
      output = app_drive_manual(input);
    }
    else
    {
      output = app_cruise_control_cruise();
    }
  }
  else if(drive_state.drive_mode == MANUAL_FORWARD)
  {
    output = app_drive_manual(input);
  }

  // berify sane pedal values, i.e. values that are not extremely out of range;
  // make sure a disconnected brake cable does not cause sudden
  // regen braking. Throttle should still work.
  if(app_pedals_brakeUnplugged(input))
  {
    output = MAX(0.0f, output);
  }
  if(app_pedals_throttleExceedMax(input))
  {
    output = 0.0f; // prevent further acceleration beyond max depressed
  }

  // brakes are on if value sent to Tritiums are negative
  if(output < 0.0f)
  {
    drive_state.brake = true;
  }
  else
  {
    drive_state.brake = false;
  }

  return output;
}

/*
 * Manual drive -- check pedals and respond accordingly
 */
float app_drive_manual(DriveInputT *input)
{
  // default 0
  float output = 0.0;

  if(app_drive_checkBrake(input)){
    output = app_pedals_brakeResponse(input->brake);
  }
  else{
    output = app_pedals_throttleResponse(input->throttle);
  }

  return output;
}

/* Throttle pedal goes from low to high */
inline bool app_drive_checkThrottle(DriveInputT *input)
{
  return (input->throttle > THROTTLE_PEDAL_MIN) ? true : false;
}

/* Brake pedal goes from high to low */
inline bool app_drive_checkBrake(DriveInputT *input)
{
  return (input->brake < BRAKE_PEDAL_MAX) ? true : false;
}