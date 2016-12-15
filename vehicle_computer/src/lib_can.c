//#include "luminos_can.h"
#include "global.h"
#include "app_send_data.h"
//#include "buttons_interface.h"
#include "dev_tritium.h"
#include "app_pedals.h"

bool prevBlinkOn = false;

// Writes driver control state to other boards
void app_CAN_txTask(void *pvParameters)
{
  while(true)
  {
    if((drive_state.turn[LEFT] || drive_state.turn[RIGHT]) &&
       (drive_state.blinkOn != prevBlinkOn))
    {
      app_send_data_lightsCommands(); // TODO change name
      app_send_data_buttonBoard(); // TODO change name
      prevBlinkOn = drive_state.blinkOn;
    }
    
    vTaskDelay(30);
  }
}

// Gets other boards' states
void app_CAN_rxTask(void *pvParameters)
{
  static CanRxMsg rxMsg;
  
  while(true)
  { 
    // Respond to BMS and button board CAN
    portBASE_TYPE res = xQueueReceive(LuminosCANRxQueue, &rxMsg, 1000);
    uint8_t devID = getDevID(rxMsg.ExtId);
    uint8_t varID = getVarID(rxMsg.ExtId);
    bool data_is_bool_type = (rxMsg.DLC == sizeof(bool)) ? true : false;
    bool data_is_int32_type = (rxMsg.DLC == sizeof(uint32_t)) ? true : false;
    bool is_enable = *((bool*)rxMsg.Data);
    
    if((res == pdPASS) && isCat(rxMsg.ExtId))
    {
      // expect enable throttle and enable regen messages from BMS
      if((devID == BMS_ID) && data_is_bool_type)
      {
        if(varID == BMS_VAR_ENABLE_THROTTLE)
          drive_state.enableThrottle = is_enable;
        else if(varID == BMS_VAR_ENABLE_REGEN)
          drive_state.enableRegen = is_enable;
      }
      else if((devID == DEVICE_ID) && data_is_int32_type)
      {
        // TODO: Register callback function
        buttonsRespond(*((uint32_t*)(rxMsg.Data)));
        
        // Update headlights and button board LEDs
        app_send_data_lightsCommands();
        app_send_data_buttonBoard();
      } 
    }
  }
}