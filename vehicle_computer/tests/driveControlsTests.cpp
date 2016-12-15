extern "C"
{
  #include "app_drive.h"
  #include "app_cruise_control.h"
}

#include "CppUTest/TestHarness.h"

TEST_GROUP(DriveControls)
{
	void setup() {}
  void teardown() {}
};

TEST(DriveControls, ManualDrive)
{
  drive_state.drive_mode = CRUISE_CONTROL;
  // set input so checkBrake() will return True
	float output = app_drive_drive(input);
  // want drive_state.drive_mode == MANUAL_FORWARD
  
  // set input so checkThrottle will return True (and checkBrake() false)
  output = app_drive_drive(input);
  // check that output == app_pedals_throttleResponse(input->throttle)
  
  drive_state.drive_mode = MANUAL_FORWARD; 
  // set input so checkThrottle will return True (and checkBrake() false)
  output = app_drive_drive(input);
  // check that output == app_pedals_throttleResponse(input->throttle)
  
  drive_state.drive_mode = MANUAL_REVERSE;
  // set input so checkThrottle will return True (and checkBrake() false)
  output = app_drive_drive(input);
  // check that output == app_pedals_throttleResponse(input->throttle)
  
  // set so app_pedals_brakeUnplugged() will return True
  output = app_drive_drive(input);
  // check that output == 0.0f

  // set so app_pedals_throttleExceedMax() will return True
  output = app_drive_drive(input);
  // check that output == 0.0f
}

TEST(DriveControls, CruiseControl)
{
	drive_state.drive_mode = CRUISE_CONTROL;
  output = app_drive_drive(input);
  // check that output == (drive_state.KP * (drive_state.cruiseVelocity - average_velocity)) + drive_state.iTerm;
}