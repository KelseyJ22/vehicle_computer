#pragma once

// Set up LEDs with the correct pins
void dev_drive_config_LED_Init(Pin *LED);
void dev_drive_config_initMic(Pin *MicSwitch);
void dev_drive_config_CANsetup(void);


// LED Pins
#define NUM_LEDS 4
#define LED_ALIVE (&LED[2])    // green
#define LED_TRITIUM (&LED[0])   // red
#define LED_THROTTLE (&LED[1])  // blue
#define LED_REVERSE (&LED[3])   // white

extern Pin LED[NUM_LEDS];

// Set all GPIO banks to analog in and disabled to save power
void dev_drive_config_setAllAnalog();

// ############################
// ##### GLOBAL VARIABLES #####
// ############################

extern volatile uint64_t grossCycleCount;

// ###################
// ##### M4 CORE #####
// ###################

#define CYCCNT (*(volatile const uint32_t*)0xE0001004)

#define DWT_CTRL (*(volatile uint32_t*)0xE0001000)
#define DWT_CTRL_CYCEN 0x00000001

#define SCB_DEMCR (*(volatile uint32_t*)0xE000EDFC)
#define SCB_DEMCR_TRCEN 0x01000000

// ###################################
// ##### GLOBAL HELPER FUNCTIONS #####
// ###################################

// Compute the total number of elapsed clock cycles since boot
uint64_t TotalClockCycles(void);

// Compute the number of milliseconds elapsed since boot
uint64_t TimeMillis(void);

// M3/M4 core configuration to turn on cycle counting since boot
void EnableCycleCounter(void);

// Print out some clock information
void PrintStartupInfo();