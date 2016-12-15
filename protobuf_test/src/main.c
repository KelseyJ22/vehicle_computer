// STM32F4 LED test
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
#include <test.pb.h>
#include <lib_pb.h>

void LedTest(void* pvParameters)
{

  /* Configure the LEDs */
  
  while(true)
  {
    /* Wait one second */
    vTaskDelay(1000);
  }
}

void ProtobufTest(void* pvParameters) {
  // In our case, we would send buffer over ethernet
  char buffer[128];
  size_t message_length;
  uint32_t i = 0;
  
  while (true) {
    TestMessage message = TestMessage_init_default;
    message.a = i;
    message.has_b = true;
    message.b = 2 * i;
    message.has_c = true;
    message.c = 3 * i;
    message.has_d = true;
    message.d = 4 * i;
    
    message_length = SerializeProtobuf(&message, TestMessage_fields, buffer, sizeof(buffer));
    
    // Send buffer and message_length over the wire...
    
    TestMessage received = TestMessage_init_default;
    bool status = DeserializeProtobuf(&received, TestMessage_fields, buffer, message_length);
    if (status) {
      if (i % 1000 == 0) {
        printf("Received: %i %i %i %i\n", received.a, received.b, received.c, received.d);
        uint8_t* curr_pointer = ((uint8_t*) &received);
        
        for (int j = 0; j < 4; ++j) {
          pb_field_t field = TestMessage_fields[j];
          curr_pointer += field.data_offset;
          printf("Value: %i\n", *((uint32_t *) curr_pointer));
          curr_pointer += field.data_size;
        }
      }
    } else {
      printf("Error\n");
    }
    
    i += 1;
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
  
  /*This is were tasks get created
   *they allow us to run several functions virtually in parallel.
   *Right now only Led_Test is running
  */
  //xTaskCreate(CatalogReceiveTask, (const signed char*)("CatalogReceiveTask"), 512, &cat, 1, NULL);
  //xTaskCreate(CatalogAnnounceTask, (const signed char *)("CatalogAnnounceTask"), 512, &cat, 1, NULL);
  xTaskCreate(LedTest, (const char *)("LedTest"), 512, NULL, 1, NULL);
  xTaskCreate(ProtobufTest, (const char *)("ProtobufTest"), 512, NULL, 1, NULL);

  // Start the scheduler
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
  vTaskStartScheduler();

  // Should never get here
  while(true);
}

