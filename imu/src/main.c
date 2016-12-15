// STM32F4 starter code
// IAR toolchain
// 72MHz sysclock w/8MHz crystal

#include "bsp.h"

#include <canrouter.h>
#include <stdio.h>
#include <stm32f4xx.h>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <pin.h>
#include <catalog.h>

void YourCodeHere(void* pvParameters)
{
  while(true)
  {
    printf("%u\tDERP DERP DERP\r\n", (unsigned long)(BSP_TimeMillis()));
    vTaskDelay(100);
  }
}

int main()
{
  // Startup housekeeping
  RCC_HSICmd(DISABLE);
  BSP_EnableCycleCounter();
  BSP_SetAllAnalog();
  //BSP_PrintStartupInfo();

  // Initialize CAN
  CAN_Config(&can, 500000, 256); // 125kbps, 256 item tx queue
  CAN_InitHwF(&can, 0, 0, true, 0); // HW filter 0 receives all extid packets

  // catInitializeCatalog(&cat, DEVICE_ID);
  // Add entries here
  //xTaskCreate(CatalogReceiveTask, (const signed char*)("CatalogReceiveTask"), 512, &cat, 1, NULL);
  //xTaskCreate(CatalogAnnounceTask, (const signed char *)("CatalogAnnounceTask"), 512, &cat, 1, NULL);
  xTaskCreate(YourCodeHere, (const char *)("YourTask"), 512, NULL, 1, NULL);

  // Start the scheduler
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
  vTaskStartScheduler();

  // Should never get here
  while(true);
}
