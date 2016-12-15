#pragma once
#include <stm32f4xx.h>
#include <stdbool.h>
#include <global.h>

/* Hardcoded pedal calibration constants here. 
  TO CALIBRATE: Uncomment #define PEDAL_CONFIG 1 in global.h*/

// Regen braking [nothing, full strength], corresponds to 
// [BRAKE_PEDAL_MAX, BRAKE_PEDAL_REGEN_MIN]
#define BRAKE_PEDAL_MECHANICAL_MIN  (377) // Furthest depressed
#define BRAKE_PEDAL_REGEN_MIN       (1465) // Furthest depressed, until regen
#define BRAKE_PEDAL_MAX             (2655) // Not depressed

#define THROTTLE_PEDAL_MIN  (2915)     // Not depressed
#define THROTTLE_PEDAL_MAX  (3695)    // Furthest depressed

/* Pedal scale, error, and deadzone constants */
#define BRAKE_REGEN_SCALE   (1.0f/(BRAKE_PEDAL_MAX - BRAKE_PEDAL_REGEN_MIN))
#define THROTTLE_SCALE      (1.0f/(THROTTLE_PEDAL_MAX - THROTTLE_PEDAL_MIN))

// These pedal values are considered the extremes that the driver controls will 
// accept. More extreme values are considered pedal malfunctions like brake wire
// disconnecting. If a pedal malfunction is detected, 0 is sent to the Tritiums
// for safety. We currently have a 30% buffer zone.
#define THROTTLE_PEDAL_ERROR_MAX ((int)(THROTTLE_PEDAL_MAX + (THROTTLE_PEDAL_MAX * 0.30f)))
// Below this value, brake is unplugged. 
// [ERROR_MIN, BRAKE_PEDAL_REGEN_MIN] = out of range, but regen
// [BRAKE_PEDAL_REGEN_MIN, BRAKE_PEDAL_MAX] = in range, regen
// Above BRAKE_PEDAL_MAX is error, do not regen nor throttle.
#define BRAKE_PEDAL_ERROR_MIN ((int)(100))

#define BRAKE_DEADZONE      ((BRAKE_PEDAL_MAX - BRAKE_PEDAL_MECHANICAL_MIN) * 0.15)
#define THROTTLE_DEADZONE   ((THROTTLE_PEDAL_MAX - THROTTLE_PEDAL_MIN) * 0.15)

/* Cruise control constants */
#define KP_DEFAULT (0.10) //constants for Cruise PI loop
#define KI_DEFAULT (0.0003)

#define IMAX  (0.6)

#define CRUISE_INC_DEFAULT (0.277778) // tritium vehicle velocity (m/s)
    
#define MAX_START_REVERSE_VELOCITY (2.2352) // 5 mph

#define MAX_START_REVERSE_VELOCITY (2.2352) // 5 mph

extern int blink_timer;
    
typedef enum
{
  MANUAL_FORWARD,
  MANUAL_REVERSE,
  CRUISE
} DriveModeType_t;

typedef struct
{
  DriveModeType_t drive_mode;
  
  bool brake;
  bool tritiums_on;
  
  bool turn[2]; // 0 = left, 1 = right
  bool blinkOn;
  bool headlights;
  uint32_t buttonState;

  bool enableThrottle;
  bool enableRegen;
  bool messageReceived;
  
  float cruiseVelocity;
  float KP;
  float KI;
  float iTerm;

} DriveStateT;

extern DriveStateT drive_state;

typedef struct
{
  uint32_t brake;
  uint32_t throttle;
} DriveInputT;

extern DriveInputT raw_drive_input;

DriveStateT InitDriveState(void);

//inline bool app_pedals_checkThrottle(DriveInputT *input);
//inline bool app_pedals_checkBrake(DriveInputT *input);
void app_pedals_applyDeadzone(DriveInputT *input);
float app_pedals_brakeResponse(float input);
float app_pedals_throttleResponse(float input);
inline void app_pedals_updateHistory();
bool app_pedals_brakeUnplugged(DriveInputT *input);
bool app_pedals_throttleExceedMax(DriveInputT *input);