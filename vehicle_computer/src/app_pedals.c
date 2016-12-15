#include <global.h>
#include <math.h>
#include "app_send_data.h"
#include "dev_drive_config.h"
#include "dev_tritium.h"
#include "dev_adc.h"
#include "global.h"
#include "app_pedals.h"
#include "app_cruise_control.h"

// tritiums will turn off after 60 seconds of inactivity
#define TRITIUM_IDLE_TIME_MS (60 * 1000)

int blink_timer = 0;

DriveStateT drive_state =
{
  .drive_mode = MANUAL_FORWARD,
  .tritiums_on = true,
  .brake = false,
  .turn = {false, false},
  .blinkOn = false,
  .headlights = false,

  .buttonState = 0,

  .enableThrottle = false,
  .enableRegen = false,
  .messageReceived = true,

  .cruiseVelocity = 0.0f,
  .KP = KP_DEFAULT,   // cruise P constant
  .KI = KI_DEFAULT,    // cruise I constant
  .iTerm = 0.0f
};

DriveInputT raw_drive_input =
{
  .brake = 0,
  .throttle = 0
};

/*
 * Pedal calibration/config routine -- requires user interaction to find and record
 * the greatest and least values for each pedal, which can then be used to translate
 * ADC readings during operation. NOTE: this includes printf statements, which are
 * not compatible with normal (no computer attached) operation, so make sure to
 * disable this routine before attempting to drive.
 */
void app_pedals_configurePedals(void *pvParameters)
{
    // 0 is lower limit, 1 is upper limit
    uint16_t throttle_min = 0xFFFF;
    uint16_t throttle_max = 0x0;
    uint16_t brake_min = 0xFFFF;
    uint16_t brake_max = 0x0;

    dev_adc_setup();
    dev_adc_initVal(&adc_val);

    printf("Initializing low pass filter, please wait...\n");
    
    // sample for 2 seconds to init the low pass filter
    uint64_t start_time = time_ms;
    while(time_ms < start_time + 2000)
    {
        dev_adc_poll(&adc_val); 
    }

    printf("Starting pedal configuration. Mash pedals now!\n");

    // sample for 15 seconds, finding global min, max pedal values
    start_time = time_ms;
    while(time_ms < start_time + 15000)
    {
        dev_adc_poll(&adc_val);

        uint16_t brake = adc_val.brake[ADC_BUFF_SIZE - 1];
        uint16_t throttle = adc_val.throttle[ADC_BUFF_SIZE - 1];

        // print out current pedal values every 500ms, because printfs are slow
        if(time_ms % 500 == 0)
          printf("cur_brake: %d\tcur_throttle: %d\n", brake, throttle);

        if(throttle < throttle_min)
        {
          throttle_min = throttle;
        }
        if(throttle > throttle_max)
        {
          throttle_max = throttle;
        }

        if(brake < brake_min)
        {
          brake_min = brake;
        }
        if(brake > brake_max)
        {
          brake_max = brake;
        }
    }
    
    printf("Calibration done! Don't forget to calibrate BRAKE_PEDAL_REGEN_MIN\n");
    printf("min brake: %d\n", brake_min);
    printf("max brake: %d\n", brake_max);
    printf("min throttle: %d\n", throttle_min);   
    printf("max throttle: %d\n", throttle_max);
}


/* Applies a deadzone to pedal inputs for consistent results */
void app_pedals_applyDeadzone(DriveInputT *input)
{
  // BRAKE_PEDAL_MAX means not depressed
  if(abs(BRAKE_PEDAL_MAX - input->brake) < BRAKE_DEADZONE)
  {
    input->brake = BRAKE_PEDAL_MAX;
  }

  // THROTTLE_PEDAL_MIN means not depressed
  if(abs(THROTTLE_PEDAL_MIN - input->throttle) < THROTTLE_DEADZONE)
  {
    input->throttle = THROTTLE_PEDAL_MIN;
  }
}


/*
 * Checks the ADC for brakes and converts to usable value
 */
float app_pedals_brakeResponse(float input)
{
  // prescale to [0,1], then square the output
  float output = (BRAKE_PEDAL_MAX - input) * BRAKE_REGEN_SCALE; // brake pedal is high to low

  // X^4 response
  output = powf(output, 2.0);

  // brake values are negative
  output = output * -1.0;

  // pedals may be calibrated incorrectly/out-of-range.
  // note: output will be < -1.0 if pedal < BRAKE_PEDAL_REGEN_MIN
  // (this is by design and is normal if pedal also < BRAKE_PEDAL_MECHANICAL_MIN)
  if(output > 0.0f)
  {
    output = 0.0f;
    // TODO send error
  }
  return output;
}

/*
 * Checks ADC for throttle and converts to usable value.
 */
float app_pedals_throttleResponse(float input)
{
  // prescale to [0,1], then square the output
  float output = (input - THROTTLE_PEDAL_MIN) * THROTTLE_SCALE; // throttle pedal is low to high

  // throttle values are positive (and zero).
  if(output < 0.0f) // Pedals may be calibrated incorrectly/out-of-range
  {
    output = 0.0f;
    // TODO: Send error
  }

  // X^2 response
  output = powf(output, 2.0);
  return output;
}


/*
 * Returns whether the brake value is unreasonable (indicating unplugged)
 */
bool app_pedals_brakeUnplugged(DriveInputT *input)
{
  // past furthest depressed, likely unplugged
  if (input->brake < BRAKE_PEDAL_ERROR_MIN)
  {
    return true;
  }
  else
  {
    return false;
  }
}


/*
 * Returns whether the throttle value is unreasonable (indicating error) 
 */
bool app_pedals_throttleExceedMax(DriveInputT *input)
{
   // past furthest depressed.
  if (input->throttle > THROTTLE_PEDAL_ERROR_MAX)
  {
    return true;
  }
  else
  {
    return false;
  }
}