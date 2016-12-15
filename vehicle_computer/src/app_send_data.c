/* Logic to send and receive messages over our new catalog */

/* Harrison and Gawan */

// send to telemetry --> app_send_data_broadcastTelemetry()
// send to BMS --> app_send_data_BMS()
// send to lights --> app_send_data_lightsCommands()
// send to button board --> app_send_data_buttonBoard()

#include "app_send_data.h"
#include "app_pedals.h"

// shared with the light boards
#define RR_REAR_LIGHT_ID 0x5
#define RR_FRONT_LIGHT_ID 0x03

#define RR_LIGHTS_MSG_VID 0xEE
#define RHL_IND 0
#define LHL_IND 1
#define LTURN_IND 2
#define RTURN_IND 3
#define LBRAKE_IND 4
#define CBRAKE_IND 5
#define RBRAKE_IND 6

typedef enum
{
  BRIGHTNESS_OFF,
  BRIGHTNESS_LOW,
  BRIGHTNESS_MED,
  BRIGHTNESS_HIGH,
  BRIGHTNESS_MAX
} BrightnessType_t;


/* lightsSendMessage()
 *
 * Checks the local state of the lights, then assembles and sends out a packet
 * to each light board telling it which lighs should be on.  This function also
 * manages blinking the physical turn signals.
 */
void app_send_data_lightsCommands(void)
{
  uint32_t lights_power = 0; // Up to 10 lights with 2^3 brightnesss each.
  if(drive_state.headlights)
  {
    lights_power |= ((BRIGHTNESS_HIGH << (RHL_IND*3)) |
                     (BRIGHTNESS_HIGH << (LHL_IND*3)) |
                     (BRIGHTNESS_LOW << (LBRAKE_IND*3)) | 
                     (BRIGHTNESS_LOW << (RBRAKE_IND*3)));
  }

  if(drive_state.blinkOn)
  {
    if(drive_state.turn[RIGHT])
    {
      lights_power |= (BRIGHTNESS_MED << (RTURN_IND*3));
    }
    if(drive_state.turn[LEFT])
    {
      lights_power |= (BRIGHTNESS_MED << (LTURN_IND*3));
    }
  }
  // When braking, brake lights should be bright, even if headlights are on.
  if(drive_state.brake)
  {
    lights_power |= ((BRIGHTNESS_MED << (LBRAKE_IND*3)) | 
                     (BRIGHTNESS_MED << (CBRAKE_IND*3)) |
                     (BRIGHTNESS_MED << (RBRAKE_IND*3)));
  }
  
  // TODO update this for ethernet
  //static CanTxMsg msg;
  //msg.StdId = 0;
 // msg.IDE = CAN_ID_EXT;
 // msg.RTR = CAN_RTR_DATA;
 // msg.DLC = sizeof(uint32_t);
  // Fill data
  //memcpy(msg.Data, (void*)(&lights_power), sizeof(lights_power));

  // Send to both front and rear light boards
 // msg.ExtId = assembleID(RR_FRONT_LIGHT_ID, RR_LIGHTS_MSG_VID, 0, CAT_OP_SETVALUE);
  //CAN_Enqueue(&mainCan, &msg, false);

  //msg.ExtId = assembleID(RR_REAR_LIGHT_ID, RR_LIGHTS_MSG_VID, 0, CAT_OP_SETVALUE);
  //CAN_Enqueue(&mainCan, &msg, false);
}

/* buttonBoardLEDsSendMessage()
 *
 * Announce to the button board which of its LEDs should be on.
 */
void app_send_data_buttonBoard(void) // TODO update this logic
{
    static uint32_t leds = 0;
    leds = 0;
    if (drive_state.drive_mode == CRUISE) leds |= BUTTON_CRUISE_UP_MASK;
    if (drive_state.drive_mode == CRUISE) leds |= BUTTON_CRUISE_DOWN_MASK;
    if (drive_state.drive_mode == MANUAL_REVERSE) leds |= BUTTON_REVERSE_MASK;
    if (drive_state.turn[LEFT] && drive_state.blinkOn) leds |= BUTTON_L_TURN_MASK;
    if (drive_state.turn[RIGHT] && drive_state.blinkOn) leds |= BUTTON_R_TURN_MASK;
    if (drive_state.headlights) leds |= BUTTON_HEADLIGHT_MASK;
    //if (drive_state.buttonState & BUTTON_RADIO_MASK) leds |= BUTTON_RADIO_MASK;
      
    // TODO update this to ethernet
    //static CanTxMsg msg;
    //msg.StdId = 0;
    //msg.IDE = CAN_ID_EXT;
    //msg.RTR = CAN_RTR_DATA;
    //msg.ExtId = assembleID(BUTTON_BOARD_ID, BUTTON_VAR_LEDS, 0, CAT_OP_SETVALUE);
    //memcpy(msg.Data, &leds, sizeof(uint32_t));
    //msg.DLC = sizeof(uint32_t);
    //CAN_Enqueue(&mainCan, &msg, false);
}