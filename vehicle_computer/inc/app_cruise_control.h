#pragma once
#include <stm32f4xx.h>
#include <stdbool.h>
#include <global.h>

#define IMAX  (0.6)

/* Cruise control constants */
#define KP_DEFAULT (0.10) //constants for Cruise PI loop
#define KI_DEFAULT (0.0003)

extern DriveStateT drive_state;

float app_cruise_control_cruise();