#include "global.h"
#include "dev_drive_config.h"
#include "dev_adc.h"
#include "dev_tritium.h"
#include "app_drive.h"
#include "pin.h"
#include "canrouter.h"
#include "app_send_data.h"
#include "app_pedals.h"
#include "lib_protobuf_data_struct.h"

Pin MicSwitch = {GPIOB, &RCC_AHB1PeriphClockCmd, GPIO_Pin_14, GPIO_PinSource14, RCC_AHB1Periph_GPIOB};

// ############################
// ##### GLOBAL VARIABLES #####
// ############################
volatile int64_t time_ms = 0;

// Holds the values read from the ADCs
volatile ADC_Value adc_val;

xTaskHandle DriveTaskHandle;
xTaskHandle TritiumCANTaskHandle;
//xTaskHandle LuminosCANRxTaskHandle;
//xTaskHandle LuminosCANTxTaskHandle;
//xTaskHandle PeriphBoardAnnounceTaskHandle;

// LED blinkenlight to make it clear that our board is alive
void Heartbeat(void* pvParameters)
{
  Pin_ConfigGpioPin(LED_ALIVE, GPIO_Mode_OUT, GPIO_Speed_2MHz, GPIO_OType_PP,
                    GPIO_PuPd_DOWN, false);
  while(true)
  {
    Pin_Toggle(LED_ALIVE);
    vTaskDelay(200);
  }
}


int main()
{
    EnableCycleCounter();

    // Disable unused pins
    //dev_drive_config_setAnalog();

    dev_drive_config_LED_Init(LED);
    dev_drive_config_initMic(&MicSwitch);

    // Turn off all LEDs
    for (uint8_t i=0; i<NUM_LEDS; ++i)
        Pin_SetLow(&LED[i]);

    // Turn off the mic
    Pin_SetLow(&MicSwitch);

    CanRx_tritium_queue = xQueueCreate(CAN_RXLEN, sizeof(CanRxMsg));

    // Initialize CAN
    dev_drive_config_CANsetup();

    // TASKS

#ifdef PEDAL_CONFIG
    xTaskCreate(app_pedals_configurePedals, (const signed char*)("ConfigurePedalsTask"), 1024, NULL, 1, NULL);
#else
    // Set up tasks and start the scheduler

    xTaskCreate(app_drive_driveTask, (const char*)("DriveTask"), 1024, NULL, 1, NULL);
    xTaskCreate(dev_tritium_messageTask, (const char*)("TritiumCANTask"), 1024, NULL, 1, &TritiumCANTaskHandle);
    //xTaskCreate(luminosCANRxTask, (const signed char*)("LuminosCANRxTask"), 512, NULL, 1, &LuminosCANRxTaskHandle);
    //xTaskCreate(luminosCANTxTask, (const signed char*)("LuminosCANTxTask"), 512, NULL, 1, &LuminosCANTxTaskHandle);
    //xTaskCreate(Heartbeat, (const signed char*)("Blink"), 256, NULL, tskIDLE_PRIORITY + 1, NULL);
#endif

    // Start the scheduler
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    vTaskStartScheduler();

    // Should never get here
    while(true);
}