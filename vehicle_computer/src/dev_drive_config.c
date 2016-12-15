#include "catalog.h"
#include "global.h"
#include "dev_drive_config.h"

xQueueHandle LuminosCANRxQueue;
xQueueHandle CanTxQueue;

// Incoming catalog messages
xQueueHandle CatalogRxQueue;
// Incoming tritium messages
xQueueHandle CanRx_tritium_queue;

// LED Pins
Pin LED[] = {
    {GPIOD, &RCC_AHB1PeriphClockCmd, GPIO_Pin_12, GPIO_PinSource12, RCC_AHB1Periph_GPIOD},
    {GPIOD, &RCC_AHB1PeriphClockCmd, GPIO_Pin_13, GPIO_PinSource13, RCC_AHB1Periph_GPIOD},
    {GPIOD, &RCC_AHB1PeriphClockCmd, GPIO_Pin_14, GPIO_PinSource14, RCC_AHB1Periph_GPIOD},
    {GPIOD, &RCC_AHB1PeriphClockCmd, GPIO_Pin_15, GPIO_PinSource15, RCC_AHB1Periph_GPIOD}
};

Can mainCan = {
    .canPeriph = CAN2,
    .clock = RCC_APB1Periph_CAN2,
    .rccfunc = &(RCC_APB1PeriphClockCmd),
    .canTx = {
        .port = GPIOB,
        .pin = GPIO_Pin_13,
        .pinsource = GPIO_PinSource13,
        .clock = RCC_AHB1Periph_GPIOB,
        .rccfunc = &(RCC_AHB1PeriphClockCmd),
        .af = GPIO_AF_CAN2
    },
    .canRx = {
        .port = GPIOB,
        .pin = GPIO_Pin_12,
        .pinsource = GPIO_PinSource12,
        .clock = RCC_AHB1Periph_GPIOB,
        .rccfunc = &(RCC_AHB1PeriphClockCmd),
        .af = GPIO_AF_CAN2
    },
    .initialized = false,
    .nextSubscriber = NULL
};

void dev_drive_config_LED_Init(Pin *LED)
{
    // Configure the LEDs
    for(int a = 0; a < 4; a++)
        Pin_ConfigGpioPin(&(LED[a]), GPIO_Mode_OUT, GPIO_Speed_2MHz,
                          GPIO_OType_PP, GPIO_PuPd_DOWN, false);
}

void dev_drive_config_initMic(Pin *MicSwitch) {
    // Configure the microphone switch (technically also an LED)
    Pin_ConfigGpioPin(MicSwitch, GPIO_Mode_OUT, GPIO_Speed_2MHz,
                      GPIO_OType_PP, GPIO_PuPd_DOWN, false);
}

#define CAN_BAUD 500000
#define CAN_QUEUE_LEN 256

void dev_drive_config_CANsetup(void)
{
  // start CAN1 clock - to use CAN2 on its own, both clocks must be running
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);
  CAN_Config(&mainCan, CAN_BAUD, CAN_QUEUE_LEN);

  // use the same queue for Catalog and CAN router
  CanTxQueue = mainCan.txQueue;

  // extended id filer
  CAN_InitHwF(&mainCan, 0x0, 0x0, true, 14); // CAN2 filters start at 14

  // standard id filter
  CAN_InitHwF(&mainCan, 0x0, 0x0, false, 15);

  // Subscribe to a type of message ID
// match = ((packetId & mask)==(id))
// Matching packets will be put in to the returned queue.

  LuminosCANRxQueue = xQueueCreate(CAN_RXLEN, sizeof(CanRxMsg));

  // TODO: Make this a set value variable in button board, register callback in DC
  uint32_t buttonStateId = assembleID(DEVICE_ID, VAR_BUTTON_STATE, 0, CAT_OP_SETVALUE);
  CAN_Subscribe(&mainCan, buttonStateId, buttonStateId, true, LuminosCANRxQueue);
  
  // Subscribe to BMS messages about regen and throttle enable/disable because
  // BMS uses CAT_OP_VALUE instead of CAT_OP_SETVALUE.
  uint32_t throttleEnId = assembleID(BMS_ID, BMS_VAR_ENABLE_THROTTLE, 0, CAT_OP_VALUE);
  CAN_Subscribe(&mainCan, throttleEnId, throttleEnId, true, LuminosCANRxQueue);

  uint32_t regenEnId = assembleID(BMS_ID, BMS_VAR_ENABLE_REGEN, 0, CAT_OP_VALUE);
  CAN_Subscribe(&mainCan, regenEnId, regenEnId, true, LuminosCANRxQueue);


  // subscribe incoming Tritium messages (all standard ID messages)
  CAN_Subscribe(&mainCan, 0, 0, false, CanRx_tritium_queue);
}

// Set all GPIO banks to analog in and disabled to save power
void dev_drive_config_setAllAnalog()
{
    // Data structure to represent GPIO configuration information
    GPIO_InitTypeDef GPIO_InitStructure;

    // Enable the peripheral clocks on all GPIO banks (needed during configuration)
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB |
        RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD | RCC_AHB1Periph_GPIOE |
        RCC_AHB1Periph_GPIOF | RCC_AHB1Periph_GPIOG | RCC_AHB1Periph_GPIOH |
        RCC_AHB1Periph_GPIOI, ENABLE);

    // Set all GPIO banks to analog inputs
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    GPIO_Init(GPIOE, &GPIO_InitStructure);
    GPIO_Init(GPIOF, &GPIO_InitStructure);
    GPIO_Init(GPIOG, &GPIO_InitStructure);
    GPIO_Init(GPIOH, &GPIO_InitStructure);
    GPIO_Init(GPIOI, &GPIO_InitStructure);

    // Do these separately so we don't break jtag
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All & ~GPIO_Pin_13 & ~GPIO_Pin_14 & ~GPIO_Pin_15;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All & ~GPIO_Pin_3 & ~GPIO_Pin_4;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // Disable the clocks to save power
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB |
        RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD | RCC_AHB1Periph_GPIOE |
        RCC_AHB1Periph_GPIOF | RCC_AHB1Periph_GPIOG | RCC_AHB1Periph_GPIOH |
        RCC_AHB1Periph_GPIOI, DISABLE);
}

// ##########################
// ##### FREERTOS HOOKS #####
// ##########################

volatile uint64_t grossCycleCount = 0;

// Hook to increment the system timer
void vApplicationTickHook(void){
    static uint32_t lastCycleCount = 0;
    uint32_t currentCycleCount = CYCCNT;

    if(currentCycleCount < lastCycleCount){
        grossCycleCount += ULONG_MAX;
    }

    lastCycleCount = currentCycleCount;

    time_ms++;
    blink_timer++;
    if (blink_timer % 500 == 0)
        drive_state.blinkOn = !drive_state.blinkOn;
}

// Compute the total number of elapsed clock cycles since boot
uint64_t TotalClockCycles(void){
    // Define the order of volatile accesses by creating a temporary variable
    uint64_t bigPart = grossCycleCount;
    return bigPart + CYCCNT;
}

// Compute the number of milliseconds elapsed since boot
uint64_t BSP_TimeMillis(void){
    return TotalClockCycles() * 1000 / SystemCoreClock;
}

// Hook to execute whenever entering the idle task
// This is a good place for power-saving code.
void vApplicationIdleHook(void)
{
  __WFI();
}

// Hook to handle stack overflow
void vApplicationStackOverflowHook(xTaskHandle xTask,
                                   signed portCHAR *pcTaskName){}

void vApplicationMallocFailedHook(){}

// M3/M4 core configuration to turn on cycle counting since boot
void EnableCycleCounter() {
    // Trace enable, which in turn enables the DWT_CTRL register
    SCB_DEMCR |= SCB_DEMCR_TRCEN;
	// Start the CPU cycle counter (read with CYCCNT)
    DWT_CTRL |= DWT_CTRL_CYCEN;
}

// Print out some clock information
void PrintStartupInfo() {
    // Get some clock information
    RCC_ClocksTypeDef RCC_ClocksStatus;
    RCC_GetClocksFreq(&RCC_ClocksStatus);

#ifdef DEBUG
    printf("Driver Controls\r\n");
    printf("SYSCLK at %u hertz.\r\n", RCC_ClocksStatus.SYSCLK_Frequency);
    printf("HCLK at %u hertz.\r\n", RCC_ClocksStatus.HCLK_Frequency);
    printf("PCLK1 at %u hertz.\r\n", RCC_ClocksStatus.PCLK1_Frequency);
    printf("PCLK2 at %u hertz.\r\n\r\n", RCC_ClocksStatus.PCLK2_Frequency);
#endif
}