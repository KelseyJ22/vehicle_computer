#include <global.h>
#include <math.h>
#include "app_send_data.h"
#include "app_pedals.h"
#include "dev_drive_config.h"
#include "dev_tritium.h"
#include "app_cruise_control.h"

inline void app_cruise_control_updateTime(void);

float app_cruise_control_cruise(void)
{
  float output = 0.0;

  float vel = avg_velocity;
  float error = drive_state.cruiseVelocity - vel;

  uint64_t dt = 1; // Cruise control gets called regularly, amortized to constant.
  
  // Prevents huge iTerm
  drive_state.iTerm += drive_state.KI * error * dt;
  if(drive_state.iTerm > IMAX)
  {
    drive_state.iTerm = IMAX;
  }
  if(drive_state.iTerm < 0)
  {
    drive_state.iTerm = 0;
  }

  output = (drive_state.KP * error) + drive_state.iTerm;

  // coast, don't use braking/regen to slow down
  if (output < 0.0)
  {
    output = 0.0;
  }

  // cap speed at 100% of motor speed
  if (output > 1.0)
  {
    output = 1.0;
  }

  return output;
}