#include <math.h>
#include <string.h>
#include "global.h"
#include "dev_drive_config.h"
#include "dev_tritium.h"
#include "lib_protobuf_data_struct.h"

// stores the state of the 2 Tritium motor controllers.
TritiumStateT tritiums[2] =
{
  {
    // Tritium 0 (left)
    .DCBaseAddr = TRITIUM_0_DCBASE,
    .WSBaseAddr = TRITIUM_0_WSBASE,
  },
  {
    // Tritium 1 (right)
    .DCBaseAddr = TRITIUM_1_DCBASE,
    .WSBaseAddr = TRITIUM_1_WSBASE,
  }
};

volatile float avg_velocity;    // m/s
volatile float avg_rpm;         // rpm
volatile float avg_odometer;    // m


inline bool dev_tritium_assert(bool expression, char *err_message, TritiumStateT *state);
void dev_tritium_motorDriveCommand(float motorVelocity, float motorCurrent, TritiumStateT *state);
uint8_t dev_tritium_getDeviceID(uint32_t stdID);
void dev_tritium_parseMessage(volatile TritiumStateT* state, CanRxMsg* message);

inline float average(float a, float b) {
    return (a + b) / 2;
}

/*
To use regen braking:
- Set motor current < 0
- Set velocity to 0
To stop without regen braking:
- Set motor current to 0
- Set velocity (rpm) to 0
To reverse:
- Set motor current > 0
- Set velocity negative unobtainable
*/

/**
 * Prepares a motor drive command depending on an input for a particular Tritium
 * @param float input_command goal current scaled 0 to 1
 * @param TritiumStateT *state the particular tritium
 * Note: input_command is from [-1, 1] where -1 is hard brake and 1 is full
 * throttle. However, the tritium expects a current value scaled from [0, 1].
 * To express braking, we
 * 1. Set the output_rpm to be 0.
 * 2. Set the current to be abs(input_command)
 */
void dev_tritium_send(float input_command, TritiumStateT* state)
{
  float motor_rpm = 0.0f;
  float motor_current = 0.0f;

  // reset the Tritium if the responses have timed out
  if(time_ms > state->nextResetTime)
  {
    dev_tritium_assert(false, "Timeout, resetting tritium", state);
    state->alive = false;
    dev_tritium_reset(state);
    state->nextResetTime = time_ms + RESET_TIMEOUT;
    return;
  }

  // only drive if the throttle is enabled
  if(dev_tritium_assert((drive_state.enableThrottle), "Throttle is disabled", state))
  {
    return;
  }


  if(input_command > 1.0)
  {
    input_command = 1.0;
  }
  else if(input_command < -1.0)
  {
    input_command = -1.0;
  }
  
  // throttle
  if(input_command >= 0.0f)
  {
    // to reverse, set the RPM negative
    if(drive_state.drive_mode == MANUAL_REVERSE)
    {
      motor_rpm = -TORQUE_CONTROL_RPM;
    }
    else
    {
      motor_rpm = TORQUE_CONTROL_RPM;
    }

    motor_current = input_command;
  }
  // brake
  else
  {
    // don't waste motor current trying to make velocity zero when already close to 0
    if(!drive_state.enableRegen || ((fabsf(avg_velocity) < REGEN_START_VELOCITY)))
    {
      motor_rpm = 0.0f;
      motor_current = 0.0f;
    }
    else
    {
      motor_rpm = 0.0f;
      motor_current = fabsf(input_command);
    }
  }

  // Note: if a motor moves in the opposite direction you expect, run
  // PhasorSense with the wheel spinning forwards!
  dev_tritium_motorDriveCommand(motor_rpm, motor_current, state);
}

/*
 * FreeRTOS task that listens for and handles CAN messages from the Tritiums.
 */
void dev_tritium_messageTask(void *pvParameters)
{  
  while(true)
  {
    CanRxMsg rxMsg;
    // don't use portMAX_DELAY: want to timeout and turn on red error LED
    if(xQueueReceive(CanRx_tritium_queue, &rxMsg, 100) == pdPASS)
    {
      // find out which Tritium sent rxMsg
      uint8_t tritiumID = dev_tritium_getDeviceID(rxMsg.StdId);

      // interpret rxMsg and update internal values
      if(tritiumID == TRITIUM_0_ID)
      {
        dev_tritium_parseMessage(&(tritiums[0]), &rxMsg);
      }
      else if(tritiumID == TRITIUM_1_ID)
      {
        dev_tritium_parseMessage(&(tritiums[1]), &rxMsg);
      }
    }
    
    avg_rpm = tritiums[1].motorVelocity;
    avg_odometer = tritiums[1].odometer;
    
    // LED_TRITIUM == RED
    if (!tritiums[0].alive && !tritiums[1].alive) // both tritiums dead = blink
    {
      Pin_Toggle(LED_TRITIUM);
    }
    else if (!tritiums[0].alive || !tritiums[1].alive) // one tritium dead = on
    {
      Pin_SetHigh(LED_TRITIUM);
    }
    else
    {
      Pin_SetLow(LED_TRITIUM); // both tritiums alive = off
    }
  }
}


/* 
 * Utility to get Tritium ID
 */ 
inline uint8_t dev_tritium_getDeviceID(uint32_t stdID)
{
    return stdID >> 5; // strip off the message ID
}

/**
 * Prepares a Motor Drive Command type message for a tritium
 */
void dev_tritium_motorDriveCommand(float motorVelocity, float motorCurrent, TritiumStateT *state)
{
  // must be a percentage: 0 to 1
  if (dev_tritium_assert((motorCurrent >= 0.0 && motorCurrent <= 1.0), "Current out of bounds", state)) return;

  CanTxMsg txMsg;
  txMsg.StdId = state->DCBaseAddr + DC_OFFSET_DRIVE;
  txMsg.ExtId = 0;
  txMsg.IDE = CAN_ID_STD;
  txMsg.RTR = CAN_RTR_DATA;
  txMsg.DLC = 8;
  
  memcpy(&(txMsg.Data[0]), &motorVelocity, sizeof(float));
  memcpy(&(txMsg.Data[4]), &motorCurrent, sizeof(float));

  CAN_Enqueue(&mainCan, &txMsg, false);
}

/*
 * Send reset messages to the tritiums as required.
 */
void dev_tritium_reset(TritiumStateT* state)
{
  CanTxMsg txMsg;
  txMsg.StdId = state->DCBaseAddr + 25;
  txMsg.ExtId = 0;
  txMsg.IDE = CAN_ID_STD;
  txMsg.RTR = CAN_RTR_DATA;
  txMsg.DLC = 8;

  char* data = "RESETWS";
  
  strncpy((void*)(&(txMsg.Data[0])), data, 8);

  CAN_Enqueue(&mainCan, &txMsg, false);
}

/*
  Parses an incoming CanRxMsg from a tritium, updates the corresponding 
  tritium state, announces new state via Catalog.
 */
void dev_tritium_parseMessage(volatile TritiumStateT* tritium, CanRxMsg* rxMsg)
{
    // Push back time until a reset is required
    tritium->nextResetTime = time_ms + RESET_TIMEOUT;
    uint16_t var = rxMsg->StdId - tritium->WSBaseAddr;
    tritium->alive = true;
  
    Tritium_CanId canId;
    canId.asInt = rxMsg->StdId;

    switch(canId.asFields.msgId){
        case 0: // Identification information
            memcpy((void*)&(tritium->serialNumber), &(rxMsg->Data[4]), 4);
            memcpy((void*)&(tritium->tritiumID), &(rxMsg->Data[0]), 4);
            break;
        case 1: // Status information
            memcpy((void*)&(tritium->rxErrCount), &(rxMsg->Data[7]), 1);
            memcpy((void*)&(tritium->txErrCount), &(rxMsg->Data[6]), 1);
            memcpy((void*)&(tritium->activeMotor), &(rxMsg->Data[4]), 2);
            memcpy((void*)&(tritium->errorFlags), &(rxMsg->Data[2]), 2);
            memcpy((void*)&(tritium->limitFlags), &(rxMsg->Data[0]), 2);
            lib_data_struct_internal.tritium_rx_error_count = tritium->rxErrCount;
            lib_data_struct_internal.has_tritium_rx_error_count = true;
            lib_data_struct_internal.tritium_tx_error_count = tritium->txErrCount;
            lib_data_struct_internal.has_tritium_tx_error_count = true;
            lib_data_struct_internal.tritium_active_motor = tritium->activeMotor;
            lib_data_struct_internal.has_tritium_active_motor = true;
            lib_data_struct_internal.tritium_error_flags = tritium->errorFlags;
            lib_data_struct_internal.has_tritium_error_flags = true;
            lib_data_struct_internal.tritium_limit_flags = tritium->limitFlags;
            lib_data_struct_internal.has_tritium_limit_flags = true;
            // TODO: check if error states are different than previous version
            // of error states, and if so call app_send_data_errorMessage
            // to update telemetry (and bypass 100ms send of the big struct)        
            break;
        case 2: // Bus measurement
            memcpy((void*)&(tritium->busCurrent), &(rxMsg->Data[4]), 4); 
            memcpy((void*)&(tritium->busVoltage), &(rxMsg->Data[0]), 4);
            lib_data_struct_internal.tritium_bus_current = (int)tritium->busCurrent;
            lib_data_struct_internal.has_tritium_bus_current = true;
            lib_data_struct_internal.tritium_bus_voltage = (int)tritium->busVoltage;
            lib_data_struct_internal.has_tritium_bus_voltage = true;
            break;
        case 3: // Velocity measurement
            memcpy((void*)&(tritium->vehicleVelocity), &(rxMsg->Data[4]), 4);
            memcpy((void*)&(tritium->motorVelocity), &(rxMsg->Data[0]), 4);
            
            if(tritium == &(tritiums[1]))
            {
              avg_velocity = tritiums[1].vehicleVelocity * VELOCITY_SCALAR;
              lib_data_struct_internal.average_speed = (int)avg_velocity;
              lib_data_struct_internal.has_average_speed = true;
            }
            break;
        case 4: // Phase current measurement
            memcpy((void*)&(tritium->phaseCurrentB), &(rxMsg->Data[4]), 4);
            memcpy((void*)&(tritium->phaseCurrentC), &(rxMsg->Data[0]), 4);
            lib_data_struct_internal.tritium_phase_current = (int)tritium->phaseCurrentB;
            lib_data_struct_internal.has_tritium_phase_current = true;
            break;
        case 5: // Motor voltage vector measurement
            memcpy((void*)&(tritium->voltageD), &(rxMsg->Data[4]), 4);
            memcpy((void*)&(tritium->voltageQ), &(rxMsg->Data[0]), 4);
            break;
        case 6: // Motor current vector measurement
            memcpy((void*)&(tritium->currentD), &(rxMsg->Data[4]), 4);
            memcpy((void*)&(tritium->currentQ), &(rxMsg->Data[0]), 4);
            break;
        case 7: // Motor back EMF measurement / prediction
            memcpy((void*)&(tritium->emfD), &(rxMsg->Data[4]), 4);
            memcpy((void*)&(tritium->emfQ), &(rxMsg->Data[0]), 4);
            break;
        case 8: // 15v rail measurement
            memcpy((void*)&(tritium->rail15v), &(rxMsg->Data[4]), 4);
            lib_data_struct_internal.tritium_15v_rail = tritium->rail15v;
            lib_data_struct_internal.has_tritium_15v_rail = true;
            break;
        case 9: // 3.3v and 1.9v rail measurement
            memcpy((void*)&(tritium->rail3v3), &(rxMsg->Data[4]), 4);
            memcpy((void*)&(tritium->rail1v9), &(rxMsg->Data[0]), 4);
            lib_data_struct_internal.tritium_3v3 = tritium->rail3v3;
            lib_data_struct_internal.has_tritium_3v3 = true;
            lib_data_struct_internal.tritium_1v9 = tritium->rail1v9;
            lib_data_struct_internal.has_tritium_1v9 = true;
            break;
        case 11: // Power electronics and motor temperature measurement
            memcpy((void*)&(tritium->heatsinkTemp), &(rxMsg->Data[4]), 4);
            memcpy((void*)&(tritium->motorTemp), &(rxMsg->Data[0]), 4);
            lib_data_struct_internal.tritium_motor_temp = tritium->motorTemp;
            lib_data_struct_internal.has_tritium_motor_temp = true;
            lib_data_struct_internal.tritium_heatsink_temp = tritium->heatsinkTemp;
            lib_data_struct_internal.has_tritium_heatsink_temp = true;
            break;
        case 12: // Processor temperature measurement
            memcpy((void*)&(tritium->DSPTemp), &(rxMsg->Data[0]), 4);
            lib_data_struct_internal.tritium_dsp_temp = tritium->DSPTemp;
            lib_data_struct_internal.has_tritium_dsp_temp = true;
            break;
        case 14: // Odometer and coulomb counter measurement
            memcpy((void*)&(tritium->DCbusAmpHours), &(rxMsg->Data[4]), 4);
            memcpy((void*)&(tritium->odometer), &(rxMsg->Data[0]), 4);
            lib_data_struct_internal.tritium_dc_bus_amphour = tritium->DCbusAmpHours;
            lib_data_struct_internal.has_tritium_dc_bus_amphour = true;
            lib_data_struct_internal.tritium_odometer = tritium->odometer;
            lib_data_struct_internal.has_tritium_odometer = true;
            break;
        case 23: // Slip speed measurement
            memcpy((void*)&(tritium->slipSpeed), &(rxMsg->Data[4]), 4);
            break;
        default:
            break;
    }
}

/*
 * Tritium debugging assert function
 */
inline bool dev_tritium_assert(bool expression, char *err_message, TritiumStateT *state)
{
  if (!expression)
  {
#ifdef DEBUG
    printf("Tritium %#x Error:\t%s\n", state->WSBaseAddr, err_message);
#endif
    return true;
  }
  else
    return false;
}