#pragma once
#include <stm32f4xx.h>
#include <stdbool.h>
#include <global.h>

void app_drive_driveTask(void* pvParameters);

float app_drive_drive(DriveInputT *input);
float app_drive_manual(DriveInputT *input);

extern DriveStateT drive_state;

extern DriveInputT raw_drive_input;