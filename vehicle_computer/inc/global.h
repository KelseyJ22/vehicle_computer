#pragma once

// Uncomment this when debugging
//#define DEBUG 1

// Uncomment this to run driver controls in pedal calibration mode.
// Make sure you have terminal I/O in IAR opened. When calibration
// is finished, a finish message will appear.

//#define PEDAL_CONFIG 1


// ##########################
// ##### GLOBAL DEFINES #####
// ##########################

#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define BOUND(a, b, c)	(MIN(b, MAX(a, c)))

// ####################
// ##### INCLUDES #####
// ####################

// STM32 peripheral library files
#include <stm32f4xx.h>

// ANSI C library files
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Scheduler header files
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

// Local definitions
#include "lis331.h"
#include "l3g4200.h"
#include "ms5803.h"
#include "dev_drive_config.h"
#include "dev_adc.h"
//#include "catalog.h"
#include "pin.h"
#include "canrouter.h"
#include "app_pedals.h"

// ############################
// ##### GLOBAL VARIABLES #####
// ############################

// CAN
extern Can mainCan;

// Outgoing can messages
extern xQueueHandle CanTxQueue;

// All incoming messages
extern xQueueHandle LuminosCANRxQueue;
// Incoming messages from Tritium
extern xQueueHandle CanRx_tritium_queue;

extern volatile float avg_velocity; // tritium tries to calculate m/s
extern volatile float avg_rpm;      // tritium tries to calculate rpm
extern volatile float avg_odometer; // tritium tries to calculate m

// Holds the values read from the ADCs
extern volatile ADC_Value adc_val;

extern volatile int64_t time_ms;

#define LEFT    0
#define RIGHT   1

#define CAN_TXLEN		128
#define CAN_RXLEN		64

extern Pin MicSwitch;

// #############################
// ##### CATALOG VARIABLES #####
// #############################


// Self
#define DEVICE_ID               0x04

// other boards on CAN
#define BUTTON_BOARD_ID         0x02
#define BMS_ID                  0x01

// control variables
#define VAR_ENABLE_THROTTLE     0x20
#define VAR_ENABLE_REGEN        0x21
#define VAR_BRAKING             0x22
#define VAR_DRIVE_MODE          0x23

#define VAR_ENABLE_MESSAGE      0x24
#define VAR_BRAKING_VOLTAGE     0x25
#define VAR_THROTTLE_VOLTAGE    0x26

// tritium variables
#define VAR_SPEED               0x30
#define VAR_RPM                 0x31
#define VAR_ODOMETER            0x32
#define VAR_MOTORTEMP_0         0x33
#define VAR_MOTORTEMP_1         0x34
#define VAR_ALIVE_0             0x35
#define VAR_ALIVE_1             0x36
#define VAR_BUS_CURRENT_0       0x37
#define VAR_BUS_CURRENT_1       0x38
#define VAR_TRIT_HS_TEMP_0      0x39
#define VAR_TRIT_HS_TEMP_1      0x3A
#define VAR_ERRORFLAGS_0        0x3B
#define VAR_ERRORFLAGS_1        0x3C
#define VAR_LIMITFLAGS_0        0x3D
#define VAR_LIMITFLAGS_1        0x3F
#define VAR_TRIT_RX_ERR_0       0x60
#define VAR_TRIT_RX_ERR_1       0x61
#define VAR_TRIT_TX_ERR_0       0x62
#define VAR_TRIT_TX_ERR_1       0x63
#define VAR_TRIT_BUS_VOLTAGE_0  0x64
#define VAR_TRIT_BUS_VOLTAGE_1  0x65
#define VAR_TRIT_15V_VOLTAGE_0  0x66
#define VAR_TRIT_15V_VOLTAGE_1  0x67
#define VAR_TRIT_3d3V_0         0x68
#define VAR_TRIT_3d3V_1         0x69
#define VAR_TRIT_1d9V_0         0x6A
#define VAR_TRIT_1d9V_1         0x6B
#define VAR_TRIT_DSP_TEMP_0     0x6C
#define VAR_TRIT_DSP_TEMP_1     0x6D
#define VAR_TRIT_AMPHR_0        0x6E
#define VAR_TRIT_AMPHR_1        0x6F
#define VAR_PHASE_CURRENTB_0    0x70
#define VAR_PHASE_CURRENTB_1    0x71
#define VAR_TRITIUMS_ON         0x80

// cruise variables
#define VAR_CRUISE_KP           0x40
#define VAR_CRUISE_KI           0x41
#define VAR_CRUISE_SPEED        0x42
#define VAR_CRUISE_INC          0x44
#define VAR_CRUISE_ITERM        0x45

#define VAR_BUTTON_STATE        0x50

#define VAR_TIME                0x51

// messages from Button Board
#define BUTTON_CRUISE_UP_MASK   (1 << 3)
#define BUTTON_CRUISE_DOWN_MASK (1 << 2)
#define BUTTON_REVERSE_MASK     (1 << 7)
#define BUTTON_L_TURN_MASK      (1 << 0)
#define BUTTON_R_TURN_MASK      (1 << 1)
#define BUTTON_HEADLIGHT_MASK   (1 << 6)
#define BUTTON_RADIO_MASK       (1 << 5)
#define BUTTON_POWERSAVE_MASK   (1 << 4)

// messages to write to Button Board
#define BUTTON_VAR_LEDS         0x36

// messages from BMS
#define BMS_VAR_ENABLE_REGEN    0x41
#define BMS_VAR_ENABLE_THROTTLE 0x42