/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                             www.st.com/SLA0044
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "dma.h"
#include "quadspi.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"
#include "usbd_cdc_if.h"
#include "shell_port.h"
#include "mdrtuslave.h"
#include "dwin.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
// GPIO_PinState State = GPIO_PIN_SET;
bool g_oSRunFlag = false;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// EventGroupHandle_t Event_Handle = NULL;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define MASTER_TASK_BIT (0x001 << 0U)
#define RUN_TASK_BIT (0x001 << 1U)
#define WAVE_TASK_BIT (0x001 << 2U)
#define DWIN_TASK_BIT (0x001 << 3U)
#define ADC_TASK_BIT (0x001 << 4U)
#define EVENT_ALL_BIT (MASTER_TASK_BIT)
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
osThreadId ShellHandle;
osThreadId ScreenHandle;
osThreadId RS485Handle;
osThreadId WifiHandle;
osThreadId C4GHandle;
osThreadId MasterHandle;
osThreadId InterruptHandle;
osThreadId controlHandle;
osMessageQId CodeQueueHandle;
osTimerId ReportHandle;
osTimerId ModbusHandle;
osMutexId shellMutexHandle;
osSemaphoreId Recive_Uart1Handle;
osSemaphoreId Recive_Uart3Handle;
osSemaphoreId Recive_Uart4Handle;
osSemaphoreId Recive_Uart5Handle;
osSemaphoreId Recive_Uart7Handle;
osSemaphoreId Code_SignalHandle;
osSemaphoreId Recive_UsbHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void Shell_Task(void const *argument);
void Screen_Task(void const *argument);
void RS485_Task(void const *argument);
void Wifi_Task(void const *argument);
void C4G_Task(void const *argument);
void Master_Task(void const *argument);
void Coding_Task(void const *argument);
void Control_Task(void const *argument);
void Report_Callback(void const *argument);
void Modbus_Callback(void const *argument);

extern void MX_USB_DEVICE_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize);

/* GetTimerTaskMemory prototype (linked to static allocation support) */
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize);

/* Hook prototypes */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName);
void vApplicationMallocFailedHook(void);

/* USER CODE BEGIN 4 */
__weak void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName)
{
  /* Run time stack overflow checking is performed if
  configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
  called if a stack overflow is detected. */
  shellPrint(Shell_Object, "%s is stack overflow!\r\n", pcTaskName);
}
/* USER CODE END 4 */

/* USER CODE BEGIN 5 */
__weak void vApplicationMallocFailedHook(void)
{
  /* vApplicationMallocFailedHook() will only be called if
  configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h. It is a hook
  function that will get called if a call to pvPortMalloc() fails.
  pvPortMalloc() is called internally by the kernel whenever a task, queue,
  timer or semaphore is created. It is also called by various parts of the
  demo application. If heap_1.c or heap_2.c are used, then the size of the
  heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
  FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
  to query the size of free heap space that remains (although it does not
  provide information on how the remaining heap might be fragmented). */
  shellPrint(Shell_Object, "memory allocation failed!\r\n");
}
/* USER CODE END 5 */

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize)
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/* USER CODE BEGIN GET_TIMER_TASK_MEMORY */
static StaticTask_t xTimerTaskTCBBuffer;
static StackType_t xTimerStack[configTIMER_TASK_STACK_DEPTH];

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize)
{
  *ppxTimerTaskTCBBuffer = &xTimerTaskTCBBuffer;
  *ppxTimerTaskStackBuffer = &xTimerStack[0];
  *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
  /* place for user code */
}
/* USER CODE END GET_TIMER_TASK_MEMORY */

/**
 * @brief  FreeRTOS initialization
 * @param  None
 * @retval None
 */
void MX_FREERTOS_Init(void)
{
  /* USER CODE BEGIN Init */
  /* Create event flag group */
  // Event_Handle = xEventGroupCreate();
  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* definition and creation of shellMutex */
  osMutexDef(shellMutex);
  shellMutexHandle = osMutexCreate(osMutex(shellMutex));

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* definition and creation of Recive_Uart1 */
  osSemaphoreDef(Recive_Uart1);
  Recive_Uart1Handle = osSemaphoreCreate(osSemaphore(Recive_Uart1), 1);

  /* definition and creation of Recive_Uart3 */
  osSemaphoreDef(Recive_Uart3);
  Recive_Uart3Handle = osSemaphoreCreate(osSemaphore(Recive_Uart3), 1);

  /* definition and creation of Recive_Uart4 */
  osSemaphoreDef(Recive_Uart4);
  Recive_Uart4Handle = osSemaphoreCreate(osSemaphore(Recive_Uart4), 1);

  /* definition and creation of Recive_Uart5 */
  osSemaphoreDef(Recive_Uart5);
  Recive_Uart5Handle = osSemaphoreCreate(osSemaphore(Recive_Uart5), 1);

  /* definition and creation of Recive_Uart7 */
  osSemaphoreDef(Recive_Uart7);
  Recive_Uart7Handle = osSemaphoreCreate(osSemaphore(Recive_Uart7), 1);

  /* definition and creation of Code_Signal */
  osSemaphoreDef(Code_Signal);
  Code_SignalHandle = osSemaphoreCreate(osSemaphore(Code_Signal), 1);

  /* definition and creation of Recive_Usb */
  osSemaphoreDef(Recive_Usb);
  Recive_UsbHandle = osSemaphoreCreate(osSemaphore(Recive_Usb), 1);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* Create the timer(s) */
  /* definition and creation of Report */
  osTimerDef(Report, Report_Callback);
  ReportHandle = osTimerCreate(osTimer(Report), osTimerPeriodic, NULL);

  /* definition and creation of Modbus */
  osTimerDef(Modbus, Modbus_Callback);
  ModbusHandle = osTimerCreate(osTimer(Modbus), osTimerPeriodic, NULL);

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  osTimerStart(ReportHandle, 1000U);
  osTimerStart(ModbusHandle, 1000U);
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* definition and creation of CodeQueue */
  osMessageQDef(CodeQueue, 16, uint16_t);
  CodeQueueHandle = osMessageCreate(osMessageQ(CodeQueue), NULL);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of Shell */
  osThreadDef(Shell, Shell_Task, osPriorityLow, 0, 256);
  ShellHandle = osThreadCreate(osThread(Shell), (void *)&shell);

  /* definition and creation of Screen */
  osThreadDef(Screen, Screen_Task, osPriorityBelowNormal, 0, 256);
  ScreenHandle = osThreadCreate(osThread(Screen), (void *)Dwin_Object);

  /* definition and creation of RS485 */
  osThreadDef(RS485, RS485_Task, osPriorityLow, 0, 128);
  RS485Handle = osThreadCreate(osThread(RS485), NULL);

  /* definition and creation of Wifi */
  osThreadDef(Wifi, Wifi_Task, osPriorityLow, 0, 256);
  WifiHandle = osThreadCreate(osThread(Wifi), NULL);

  /* definition and creation of C4G */
  osThreadDef(C4G, C4G_Task, osPriorityLow, 0, 256);
  C4GHandle = osThreadCreate(osThread(C4G), NULL);

  /* definition and creation of Master */
  osThreadDef(Master, Master_Task, osPriorityAboveNormal, 0, 256);
  MasterHandle = osThreadCreate(osThread(Master), NULL);

  /* definition and creation of Interrupt */
  osThreadDef(Interrupt, Coding_Task, osPriorityHigh, 0, 128);
  InterruptHandle = osThreadCreate(osThread(Interrupt), NULL);

  /* definition and creation of control */
  osThreadDef(control, Control_Task, osPriorityHigh, 0, 512);
  controlHandle = osThreadCreate(osThread(control), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  MX_ShellInit(Shell_Object);
  MX_ModbusInit();
  if (Slave1_Object)
  {
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
    /*DMA buffer must point to an entity address!!!*/
    HAL_UART_Receive_DMA(&huart1, mdRTU_Recive_Buf(Slave1_Object), MODBUS_PDU_SIZE_MAX);
    __HAL_UART_ENABLE_IT(&huart5, UART_IT_IDLE);
    /*DMA buffer must point to an entity address!!!*/
    HAL_UART_Receive_DMA(&huart5, mdRTU_Recive_Buf(Slave1_Object), MODBUS_PDU_SIZE_MAX);
    __HAL_UART_ENABLE_IT(&huart7, UART_IT_IDLE);
    /*DMA buffer must point to an entity address!!!*/
    HAL_UART_Receive_DMA(&huart7, mdRTU_Recive_Buf(Slave1_Object), MODBUS_PDU_SIZE_MAX);
    Slave1_Object->receiveBuffer->count = 0U;
  }
  if (Slave2_Object)
  {
    __HAL_UART_ENABLE_IT(&huart3, UART_IT_IDLE);
    /*DMA buffer must point to an entity address!!!*/
    HAL_UART_Receive_DMA(&huart3, mdRTU_Recive_Buf(Slave2_Object), MODBUS_PDU_SIZE_MAX);
    Slave2_Object->receiveBuffer->count = 0U;
  }
  MX_DwinInit();
  if (Dwin_Object)
  {
    __HAL_UART_ENABLE_IT(&huart4, UART_IT_IDLE);
    /*DMA buffer must point to an entity address!!!*/
    HAL_UART_Receive_DMA(&huart4, Dwin_Recive_Buf(Dwin_Object), Dwin_Rx_Size(Dwin_Object));
    *Dwin_Object->Uart.pRxCount = 0U;
  }
  /* USER CODE END RTOS_THREADS */
}

/* USER CODE BEGIN Header_Shell_Task */
/**
 * @brief  Function implementing the Shell thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_Shell_Task */
void Shell_Task(void const *argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN Shell_Task */
  g_oSRunFlag = true;
  /* Infinite loop */
  for (;;)
  {
#if defined(USING_USB)
    /*https://www.cnblogs.com/w-smile/p/11333950.html*/
    if (osOK == osSemaphoreWait(Recive_UsbHandle, osWaitForever))
    {
      Shell *shell = (Shell *)argument;
      extern USBD_HandleTypeDef hUsbDeviceFS;
      if (Usb.pRxbuf)
      {
        for (uint8_t *p = Usb.pRxbuf; p < Usb.pRxbuf + Usb.counts;)
          shellHandler(shell, *p++);
        Usb.counts = 0U;
        USBD_CDC_SetRxBuffer(&hUsbDeviceFS, Usb.pRxbuf);
        USBD_CDC_ReceivePacket(&hUsbDeviceFS);
      }
    }
    // osDelay(1000);
#else
    shellTask((void *)argument);
#endif
  }
  /* USER CODE END Shell_Task */
}

/* USER CODE BEGIN Header_Screen_Task */
/**
 * @brief Function implementing the Screen thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_Screen_Task */
void Screen_Task(void const *argument)
{
  /* USER CODE BEGIN Screen_Task */
  /* Infinite loop */
  for (;;)
  {
    /*https://www.cnblogs.com/w-smile/p/11333950.html*/
    if (osOK == osSemaphoreWait(Recive_Uart4Handle, osWaitForever))
    {
      Dwin_Handler(Dwin_Object);
#if defined(USING_DEBUG)
      shellPrint(Shell_Object, "Screen received a packet of data .\r\n");
#endif
    }
  }
  /* USER CODE END Screen_Task */
}

/* USER CODE BEGIN Header_RS485_Task */
/**
 * @brief Function implementing the RS485 thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_RS485_Task */
void RS485_Task(void const *argument)
{
  /* USER CODE BEGIN RS485_Task */
  /* Infinite loop */
  for (;;)
  {
    /*https://www.cnblogs.com/w-smile/p/11333950.html*/
    if (osOK == osSemaphoreWait(Recive_Uart3Handle, osWaitForever))
    {
      Slave2_Object->uartId = 0x03;
      mdRTU_Handler(Slave2_Object);
#if defined(USING_DEBUG)
      shellPrint(Shell_Object, "RS485 received a packet of data .\r\n");
#endif
    }
  }
  /* USER CODE END RS485_Task */
}

/* USER CODE BEGIN Header_Wifi_Task */
/**
 * @brief Function implementing the Wifi thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_Wifi_Task */
void Wifi_Task(void const *argument)
{
  /* USER CODE BEGIN Wifi_Task */
  /* Infinite loop */
  for (;;)
  {
    /*https://www.cnblogs.com/w-smile/p/11333950.html*/
    if (osOK == osSemaphoreWait(Recive_Uart5Handle, osWaitForever))
    {
      Slave1_Object->uartId = 0x05;
      mdRTU_Handler(Slave1_Object);
#if defined(USING_DEBUG)
      shellPrint(Shell_Object, "Wifi received a packet of data .\r\n");
#endif
    }
  }
  /* USER CODE END Wifi_Task */
}

/* USER CODE BEGIN Header_C4G_Task */
/**
 * @brief Function implementing the C4G thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_C4G_Task */
void C4G_Task(void const *argument)
{
  /* USER CODE BEGIN C4G_Task */
  /* Infinite loop */
  for (;;)
  {
    /*https://www.cnblogs.com/w-smile/p/11333950.html*/
    if (osOK == osSemaphoreWait(Recive_Uart7Handle, osWaitForever))
    {
      Slave1_Object->uartId = 0x07;
      mdRTU_Handler(Slave1_Object);
#if defined(USING_DEBUG)
      shellPrint(Shell_Object, "4G received a packet of data .\r\n");
#endif
    }
  }
  /* USER CODE END C4G_Task */
}

/* USER CODE BEGIN Header_Master_Task */
/**
 * @brief Function implementing the Master thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_Master_Task */
void Master_Task(void const *argument)
{
/* USER CODE BEGIN Master_Task */
#define ULONG_MAX 0xFFFFFFFF
  uint32_t target_slave_id = 0;
  /* Infinite loop */
  for (;;)
  {
    /*https://www.cnblogs.com/w-smile/p/11333950.html*/
    if (osOK == osSemaphoreWait(Recive_Uart1Handle, osWaitForever))
    {
      xTaskNotifyWait(0x00,             /* Don't clear any notification bits on entry. */
                      ULONG_MAX,        /* Reset the notification value to 0 on exit. */
                      &target_slave_id, /* Notified value pass out in ulNotifiedValue. */
                      portMAX_DELAY);   /* Block indefinitely. */
      Slave1_Object->uartId = 0x01;
      Slave1_Object->portRTUMasterHandle(Slave1_Object, (mdU8)target_slave_id);
    }
  }
  /* USER CODE END Master_Task */
}

/* USER CODE BEGIN Header_Coding_Task */
/**
 * @brief Function implementing the Interrupt thread.
 * @param argument: Not used
 * @retval None
 */
__inline uint8_t Get_InterruptSite(uint16_t id)
{
#define ERROR_INTERRUPT 0xFF
  uint8_t site = 0x00;

  if ((id == 0U) || (id % 2U))
    return ERROR_INTERRUPT;

  while (id != 0x01)
  {
    id >>= 1U, site++;
  }
  return site;
}
/* USER CODE END Header_Coding_Task */
void Coding_Task(void const *argument)
{
  /* USER CODE BEGIN Coding_Task */
  uint16_t interrupt_id;
  uint8_t interrupt_site;
  static uint16_t first_interrupt = 0xFFFF;
  Slave_IRQTableTypeDef *sp_irq = IRQ;
  /* Infinite loop */
  for (;;)
  { /*It is detected that the slave has issued an interrupt request*/
    if (xQueueReceive(CodeQueueHandle, &interrupt_id, 10) == pdPASS)
    {
#if defined(USING_DEBUG)
      shellPrint(Shell_Object, "Note: Generate an interrupt: %d.\r\n", interrupt_id);
#endif
      /*Separate the interrupt number and count the interrupt source*/
      while (interrupt_id)
      {
        sp_irq->pIRQ->IRQId = interrupt_id;
        interrupt_id &= interrupt_id - 1;
        sp_irq->pIRQ->IRQId -= interrupt_id;
        count++;
#if defined(USING_DEBUG)
        shellPrint(Shell_Object, "Note: irq[%d]_id = 0x%x.\r\n", interrupt_id);
#endif
      }
      interrupt_site = Get_InterruptSite(interrupt_id);
    }

    /*Check if the interrupt source is valid*/
    if (interrupt_id && (interrupt_site != ERROR_INTERRUPT))
    {
      /*Check the first access backplane sign*/
      if (first_interrupt & interrupt_id)
      {
        Slave1_Object->uartId = 0x01;
        Slave1_Object->mdRTUHandleCode11(Slave1_Object, interrupt_site);
        /*Clear the flag after the slave responds correctly*/
        if (xTaskNotify(MasterHandle, interrupt_site, eSetValueWithoutOverwrite) == pdPASS)
        {
          /*Clear first access flag*/
          first_interrupt &= ~interrupt_id;
        }
        /*no response from slave*/
        else
        {
#if defined(USING_DEBUG)
          shellPrint(Shell_Object, "Error: Interrupt Slave 0x%02x No Answer!\r\n",
                     interrupt_site);
#endif
        }
      }
      /*The board is not connected to the backplane for the first time*/
      else
      { /*Interrupt device table is not empty*/
        if (sp_irq->Amount)
        {
        }
      }
    }
    else
    {
#if defined(USING_DEBUG)
      shellPrint(Shell_Object, "Error: Invalid interrupt source: 0x%x, site: %d.\r\n",
                 interrupt_id, interrupt_site);
#endif
    }
    osDelay(1);
  }
  /* USER CODE END Coding_Task */
}

/* USER CODE BEGIN Header_Control_Task */
/**
 * @brief Function implementing the control thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_Control_Task */
void Control_Task(void const *argument)
{
  /* USER CODE BEGIN Control_Task */
  /* Infinite loop */
  for (;;)
  {
    osDelay(1);
  }
  /* USER CODE END Control_Task */
}

/* Report_Callback function */
void Report_Callback(void const *argument)
{
  /* USER CODE BEGIN Report_Callback */
  HAL_GPIO_TogglePin(GPIOB, LED_Pin);
#if defined(USING_DEBUG)
  // shellPrint(Shell_Object, "Report_Callback !\r\n");
  // shellWriteEndLine(Shell_Object, "Report_Callback !\r\n", sizeof("Report_Callback !\r\n"));
#endif
  /* USER CODE END Report_Callback */
}

/* Modbus_Callback function */
void Modbus_Callback(void const *argument)
{
  /* USER CODE BEGIN Modbus_Callback */
#if defined(USING_DEBUG)
  // shellPrint(Shell_Object, "Modbus_Callback !\r\n");
#endif
  /* USER CODE END Modbus_Callback */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
/**
 * @brief  EXTI line detection callback.
 * @param  GPIO_Pin: Specifies the port pin connected to corresponding EXTI line.
 * @retval None
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  /* Prevent unused argument(s) compilation warning */
  // UNUSED(GPIO_Pin);
  uint16_t irq_id = GPIO_Pin;
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  /*There are other interrupt sources*/
  uint16_t irq_flag = __HAL_GPIO_EXTI_GET_IT(GPIO_PIN_All);
  if (irq_flag)
  {
    irq_id |= irq_flag;
    /*Clear all interrupt signals*/
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_All);
  }
  xQueueSendToBackFromISR(CodeQueueHandle, &irq_id, &xHigherPriorityTaskWoken);
  // xQueueSendFromISR(CodeQueueHandle, &GPIO_Pin, &xHigherPriorityTaskWoken);
  /* Now the buffer is empty we can switch context if necessary. */
  if (xHigherPriorityTaskWoken)
  {
    taskENTER_CRITICAL();
    /* Actual macro used here is port specific. */
    osThreadYield();
    taskEXIT_CRITICAL();
  }

  /* NOTE: This function Should not be modified, when the callback is needed,
           the HAL_GPIO_EXTI_Callback could be implemented in the user file
   */
}

/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
