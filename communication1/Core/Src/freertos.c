/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2022 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
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
#include "tool.h"
#include "Modbus.h"
#include "lora.h"
#include "shell_port.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
osThreadId shellHandle;
osThreadId CpuHandle;
osThreadId LoraHandle;
osThreadId IWDGHandle;
osMutexId shellMutexHandle;
osSemaphoreId Cpu_ReciveHandle;
osSemaphoreId Lora_ReciveHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void Shell_Task(void const *argument);
void Cpu_Task(void const *argument);
void Lora_Task(void const *argument);
void IWDG_Task(void const *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize);

/* Hook prototypes */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName);
void vApplicationMallocFailedHook(void);

/* USER CODE BEGIN 4 */
__weak void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName)
{
  /* Run time stack overflow checking is performed if
  configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
  called if a stack overflow is detected. */
  shellPrint(Shell_Object, "@Error:%s is stack overflow!\r\n", pcTaskName);
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
  shellPrint(Shell_Object, "@Error:memory allocation failed!\r\n");
  shellPrint(Shell_Object, "\r\n\r\nOS remaining heap = %dByte, Mini heap = %dByte\r\n",
             xPortGetFreeHeapSize(), xPortGetMinimumEverFreeHeapSize());
  /*Solve the problem of scheduling stuck after running out of memory: reset MCU with Watchdog*/
  for (;;)
    ;
}

static void TIRQ_Handle(void)
{
  /*Interrupt signal generation*/
  static IRQ_Type irq = DisEnable;
  pModbusHandle pd = Modbus_Object;

  if (pd->Slave.pHandle)
  { /*The board model is read or the corresponding interrupt processing request is responded*/
    irq = *(bool *)pd->Slave.pHandle ? DisEnable : (IRQ_Type)(irq ^ DisEnable);
    IRQ_Handle(irq);
  }
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

/**
 * @brief  FreeRTOS initialization
 * @param  None
 * @retval None
 */
void MX_FREERTOS_Init(void)
{
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* definition and creation of shellMutex */
  osMutexDef(shellMutex);
  shellMutexHandle = osMutexCreate(osMutex(shellMutex));

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* definition and creation of Cpu_Recive */
  osSemaphoreDef(Cpu_Recive);
  Cpu_ReciveHandle = osSemaphoreCreate(osSemaphore(Cpu_Recive), 1);

  /* definition and creation of Lora_Recive */
  osSemaphoreDef(Lora_Recive);
  Lora_ReciveHandle = osSemaphoreCreate(osSemaphore(Lora_Recive), 1);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  MX_ShellInit(Shell_Object);
  MX_ModbusInit();
  if (Modbus_Object)
  {
    __HAL_UART_ENABLE_IT(Modbus_Object->huart, UART_IT_IDLE);
    /*DMA buffer must point to an entity address!!!*/
    HAL_UART_Receive_DMA(Modbus_Object->huart, Modbus_Object->Slave.pRbuf, Modbus_Object->Slave.RxSize);
    Modbus_Object->Slave.RxCount = 0U;
  }
  MX_Lora_Init();
  if (Lora_Object)
  {
    __HAL_UART_ENABLE_IT(Lora_Object->huart, UART_IT_IDLE);
    /*DMA buffer must point to an entity address!!!*/
    HAL_UART_Receive_DMA(Lora_Object->huart, Lora_Object->Slave.pRbuf, Lora_Object->Slave.RxSize);
    Lora_Object->Slave.RxCount = 0U;
  }
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of shell */
  osThreadDef(shell, Shell_Task, osPriorityLow, 0, 192);
  shellHandle = osThreadCreate(osThread(shell), (void *)Shell_Object);

  /* definition and creation of Cpu */
  osThreadDef(Cpu, Cpu_Task, osPriorityNormal, 0, 256);
  CpuHandle = osThreadCreate(osThread(Cpu), (void *)Modbus_Object);

  /* definition and creation of Lora */
  osThreadDef(Lora, Lora_Task, osPriorityHigh, 0, 256);
  LoraHandle = osThreadCreate(osThread(Lora), (void *)Lora_Object);

  /* definition and creation of IWDG */
  osThreadDef(IWDG, IWDG_Task, osPriorityRealtime, 0, 64);
  IWDGHandle = osThreadCreate(osThread(IWDG), (void *)Shell_Object);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */
}

/* USER CODE BEGIN Header_Shell_Task */
/**
 * @brief  Function implementing the shell thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_Shell_Task */
void Shell_Task(void const *argument)
{
  /* USER CODE BEGIN Shell_Task */
  // Shell *shell = (Shell *)argument;
  // char data = '\0';
  /* Infinite loop */
  for (;;)
  {
    shellTask((void *)argument);
  }
  /* USER CODE END Shell_Task */
}

/* USER CODE BEGIN Header_Cpu_Task */
/**
 * @brief Function implementing the Cpu thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_Cpu_Task */
void Cpu_Task(void const *argument)
{
  /* USER CODE BEGIN Cpu_Task */
  pModbusHandle mdHandle = (pModbusHandle)argument;
  /* Infinite loop */
  for (;;)
  {
    /*https://www.cnblogs.com/w-smile/p/11333950.html*/
    if ((osOK == osSemaphoreWait(mdHandle->Slave.bSemaphore, osWaitForever)) && mdHandle)
    {
      mdHandle->Mod_Poll(mdHandle);
#if defined(USING_DEBUG)
      shellPrint(Shell_Object, "Slave received a packet of data.\r\n");
#endif
    }
  }
  /* USER CODE END Cpu_Task */
}

/* USER CODE BEGIN Header_Lora_Task */
/**
 * @brief Function implementing the Lora thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_Lora_Task */
void Lora_Task(void const *argument)
{
  /* USER CODE BEGIN Lora_Task */
  pLoraHandle pHandle = (pLoraHandle)argument;
  extern bool g_At_mutex;
  /* Infinite loop */
  for (;;)
  {
    /*https://www.cnblogs.com/w-smile/p/11333950.html*/
    if ((osOK == osSemaphoreWait(pHandle->Slave.bSemaphore, LORA_SCHEDULE_TIMES)))
    {
      if (!g_At_mutex)
        pHandle->Lora_Recive_Poll(pHandle);
#if defined(USING_DEBUG)
      shellPrint(Shell_Object, "Lora received a packet of data.\r\n");
#endif
    }
    else
    {
      if (!g_At_mutex)
        pHandle->Lora_Transmit_Poll(pHandle);
    }
// osDelay(1000);
#if defined(USING_DEBUG)
#if ((configUSE_TRACE_FACILITY == 1) && (configUSE_STATS_FORMATTING_FUNCTIONS > 0) && (configSUPPORT_DYNAMIC_ALLOCATION == 1))
    TaskStatus_t *pTable = CUSTOM_MALLOC(uxTaskGetNumberOfTasks() * sizeof(TaskStatus_t));
    if (pTable)
    {
      vTaskList((char *)pTable);
      shellPrint(Shell_Object, "TaskName\tTaskState Priority   RemainingStack TaskID\r\n");
      shellPrint(Shell_Object, "%s\r\n", pTable);
    }
    CUSTOM_FREE(pTable);
    shellPrint(Shell_Object, "\r\nremaining heap = %d.\r\n", xPortGetFreeHeapSize());
#endif
#endif
    // shellPrint(Shell_Object, "\r\n\r\nOS remaining heap = %dByte, Mini heap = %dByte\r\n",
    //            xPortGetFreeHeapSize(), xPortGetMinimumEverFreeHeapSize());
  }
  /* USER CODE END Lora_Task */
}

/* USER CODE BEGIN Header_IWDG_Task */
/**
 * @brief Function implementing the IWDG thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_IWDG_Task */
void IWDG_Task(void const *argument)
{
  /* USER CODE BEGIN IWDG_Task */
  /* Infinite loop */
  for (;;)
  {
    extern IWDG_HandleTypeDef hiwdg;
    HAL_IWDG_Refresh(&hiwdg);
    /*External interruption*/
    TIRQ_Handle();
    osDelay(120);
  }
  /* USER CODE END IWDG_Task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
