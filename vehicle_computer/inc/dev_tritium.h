#pragma once
#include <stm32f4xx.h>
#include <stdbool.h>
#include "canrouter.h"
#include <global.h>

#define REGEN_START_VELOCITY (0.003) // speed when the regen starts (m/s)
#define VELOCITY_SCALAR (1.95) // Hack to get more accurate speedometer


#define TORQUE_CONTROL_RPM (100000)
#define RESET_TIMEOUT (1500)

// tritium message IDs
#define TRITIUM_0_ID            0x020
#define TRITIUM_0_WSBASE        0x400
#define TRITIUM_0_DCBASE        0x500

#define TRITIUM_1_ID            0x021
#define TRITIUM_1_WSBASE        0x420
#define TRITIUM_1_DCBASE        0x520


// CAN bus Constants for Tritium Communication
#define DC_OFFSET_DRIVE 1

// Hack to get into ballpark of correct velocity
#define VEL_SCALE   (1.25f)

// Struct to unpack Tritium's standard CAN ID format
typedef __packed struct Tritium_CanId_Bf {
    unsigned int msgId:5;
    unsigned int devId:6;
    unsigned int padding:21;
} Tritium_CanIdBf;

typedef union Tritium_CanId_T {
    uint32_t asInt;
    Tritium_CanIdBf asFields;
} Tritium_CanId;

// Struct to unpack error flags
typedef __packed struct Tritium_ErrFlags_T {
    unsigned int hardwareOverCurrent:1;
    unsigned int softwareOverCurrent:1;
    unsigned int dcBusOverVoltage:1;
    unsigned int badMotorPositionSequence:1;
    unsigned int watchdogReset:1;
    unsigned int configReadError:1;
    unsigned int supplyUnderVoltage:1;
    unsigned int desaturationFault:1;
    unsigned int padding:8;
} Tritium_ErrFlags;

// Struct to unpack limit flags
typedef __packed struct Tritium_LimitFlags_T {
    unsigned int outputVoltagePwm:1;
    unsigned int motorCurrent:1;
    unsigned int velocity:1;
    unsigned int busCurrent:1;
    unsigned int busVoltageUpperLimit:1;
    unsigned int busVoltageLowerLimit:1;
    unsigned int motorTemperature:1;
    unsigned int padding:9;
} Tritium_LimitFlags;

typedef struct
{
    uint64_t nextResetTime;

    bool alive; // whether or not we can hear back from the Tritium

    uint16_t DCBaseAddr;    // commands (motor drive, reset) are issued with this address
    uint16_t WSBaseAddr;    // responses come with this address

    uint32_t serialNumber;
    char tritiumID[4];
    
    uint8_t rxErrCount;
    uint8_t txErrCount;

    uint16_t activeMotor;
    uint16_t errorFlags;
    uint16_t limitFlags;

    float busCurrent; // A
    float busVoltage; // V
    
    float vehicleVelocity; // tritium tries to calculate m/s
    float motorVelocity; // tritium tries to calculate rpm

    float phaseCurrentB;
    float phaseCurrentC;
    float voltageD;
    float voltageQ;
    float currentD;
    float currentQ;
    float emfD;
    float emfQ;
    float rail15v;
    float rail3v3;
    float rail1v9;
    
    float heatsinkTemp;
    float motorTemp;
    float DSPTemp;

    float DCbusAmpHours; // A*hr
    float odometer; // tritium tries to calculate m
    
    float slipSpeed;
} TritiumStateT;

extern TritiumStateT tritiums[2];

extern volatile float avg_velocity;    // tritium tries to calculate m/s
extern volatile float avg_rpm;         // tritium tries to calculate rpm
extern volatile float avg_odometer;    // tritium tries to calculate m

void dev_tritium_messageTask(void *pvParameters);
void dev_tritium_send(float command, TritiumStateT* state);
void dev_tritium_reset(TritiumStateT* state);