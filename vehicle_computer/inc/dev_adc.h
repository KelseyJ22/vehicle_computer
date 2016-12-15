#pragma once
#include <stm32f4xx.h>

#define ADC_RATE 50  // Hz.
#define ADC_BUFF_SIZE 2

typedef struct {
  // 0 is history, 1 is current value
  uint16_t brake[ADC_BUFF_SIZE];
  uint16_t throttle[ADC_BUFF_SIZE];

  uint16_t brake_avg;
  uint16_t throttle_avg;

} ADC_Value;

void dev_adc_setup();
void dev_adc_initVal(volatile ADC_Value *adc);
void dev_adc_poll(volatile ADC_Value *adc);

// pedal configuration
void ConfigureBrake();
void ConfigureThrottle();