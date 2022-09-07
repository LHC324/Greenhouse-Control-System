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
#include "shell.h"
#include "shell_port.h"
#include "mdrtuslave.h"
#include "at_usr.h"
#include "io_signal.h"
#include "L101.h"
#include "io_uart.h"
#include "at.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
bool g_At = false;
SHELL_EXPORT_VAR(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_VAR_INT), g_at, &g_At, at_cmd);
extern void Free_Mode(Shell *shell, char *pData);
extern bool Check_Mode(ModbusRTUSlaveHandler handler);
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
osThreadId mdbusHandle;
osThreadId read_ioHandle;
osThreadId IWDGHandle;
osMessageQId DDIx_QueueHandle;
osTimerId Timer1Handle;
osMutexId shellMutexHandle;
osSemaphoreId ReciveHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void Shell_Task(void const *argument);
void Mdbus_Task(void const *argument);
void Read_Io_Task(void const *argument);
void IWDG_Task(void const *argument);
void Timer_Callback(void const *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize);

/* GetTimerTaskMemory prototype (linked to static allocation support) */
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize);

/* Hook prototypes */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName);

/* USER CODE BEGIN 4 */
__weak void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName)
{
  /* Run time stack overflow checking is performed if
  configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
  called if a stack overflow is detected. */
  shellPrint(&shell, "%s is stack overflow!\r\n", pcTaskName);
}
/* USER CODE END 4 */

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

  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* definition and creation of shellMutex */
  osMutexDef(shellMutex);
  shellMutexHandle = osMutexCreate(osMutex(shellMutex));

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* definition and creation of Recive */
  osSemaphoreDef(Recive);
  ReciveHandle = osSemaphoreCreate(osSemaphore(Recive), 1);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* Create the timer(s) */
  /* definition and creation of Timer1 */
  osTimerDef(Timer1, Timer_Callback);
  Timer1Handle = osTimerCreate(osTimer(Timer1), osTimerPeriodic, NULL);

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* definition and creation of DDIx_Queue */
  osMessageQDef(DDIx_Queue, 16, uint16_t);
  DDIx_QueueHandle = osMessageCreate(osMessageQ(DDIx_Queue), NULL);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of shell */
  osThreadDef(shell, Shell_Task, osPriorityLow, 0, 256);
  shellHandle = osThreadCreate(osThread(shell), (void *)&shell);

  /* definition and creation of mdbus */
  osThreadDef(mdbus, Mdbus_Task, osPriorityNormal, 0, 256);
  mdbusHandle = osThreadCreate(osThread(mdbus), NULL);

  /* definition and creation of read_io */
  osThreadDef(read_io, Read_Io_Task, osPriorityAboveNormal, 0, 256);
  read_ioHandle = osThreadCreate(osThread(read_io), &DDIx);

  /* definition and creation of IWDG */
  osThreadDef(IWDG, IWDG_Task, osPriorityRealtime, 0, 64);
  IWDGHandle = osThreadCreate(osThread(IWDG), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  osTimerStart(Timer1Handle, MDTASK_SENDTIMES);
  /*Suspend shell task*/
#if defined(USING_L101)
  osThreadSuspend(shellHandle);
#endif
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
  // HAL_NVIC_DisableIRQ(TIM1_UP_IRQn);
  HAL_NVIC_DisableIRQ(TIM3_IRQn);
  HAL_NVIC_DisableIRQ(TIM4_IRQn);
  HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
  // HAL_NVIC_SetPriority(TIM1_UP_IRQn, 8, 0);
  HAL_NVIC_SetPriority(TIM3_IRQn, 6, 0);
  HAL_NVIC_SetPriority(TIM4_IRQn, 7, 0);
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 7, 0);
  // HAL_NVIC_EnableIRQ(TIM1_UP_IRQn);
  HAL_NVIC_EnableIRQ(TIM3_IRQn);
  HAL_NVIC_EnableIRQ(TIM4_IRQn);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
#if !defined(USING_ASHINING)
#if !defined(USING_TRANSPARENT_MODE)
  At_GetSlaveId();
#endif
#endif
  DDIx.pQueue = DDIx_QueueHandle;
  /* Infinite loop */
  for (;;)
  {
#if defined(USING_DEBUG)
    // shellPrint(&shell, "shell_Task is running !\r\n");
    // osDelay(1000);
#endif
    shellTask((void *)argument);
  }
  /* USER CODE END Shell_Task */
}

/* USER CODE BEGIN Header_Mdbus_Task */
/**
 * @brief Function implementing the mdbus thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_Mdbus_Task */
void Mdbus_Task(void const *argument)
{
  /* USER CODE BEGIN Mdbus_Task */
  /* Infinite loop */
  for (;;)
  {
    /*https://www.cnblogs.com/w-smile/p/11333950.html*/
    if (osOK == osSemaphoreWait(ReciveHandle, osWaitForever))
    {
#if defined(USING_L101)
      Check_Mode(Master_Object) ? mdRTU_Handler(Master_Object) : Shell_Mode();
#else
      extern bool g_LoraMutex;
      if (!g_LoraMutex)
      {
        mdRTU_Handler(Master_Object);
#endif
#if defined(USING_DEBUG)
      shellWriteEndLine(&shell, "Received a data!\r\n", 19U);
#endif
    }
  }

#if defined(USING_DEBUG)
//     shellPrint(&shell, "Master_Object = 0x%p, ptick = 0x%p\r\n", Master_Object,
//                Master_Object->portRTUTimerTick);
//     osDelay(1000);
#endif

  // osDelay(10);
}
/* USER CODE END Mdbus_Task */
}

/* USER CODE BEGIN Header_Read_Io_Task */
/**
 * @brief Function implementing the read_io thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_Read_Io_Task */
void Read_Io_Task(void const *argument)
{
  /* USER CODE BEGIN Read_Io_Task */
#if !defined(USING_REPEATER_MODE)
  DDIx_HandleTypeDef *pDDIx = (DDIx_HandleTypeDef *)argument;
#endif
  // uint16_t data = 0;
  /* Infinite loop */
  for (;;)
  {
    // Io_Digital_Handle();
    // Io_Analog_Handle();
    // osDelay(50);

    // if (xQueueReceive(DDIx_QueueHandle, &data, osWaitForever) == pdPASS)
    // {
    //   /* xRxedStructure now contains a copy of xMessage. */
    //   Io_Digital_Handle(pDDIx->site, (uint8_t *)&data);
    //   pDDIx->site = 0;
    //   pDDIx->bits = 0;
    // }
#if !defined(USING_REPEATER_MODE)
    Io_Digital_Handle((uint8_t *)&pDDIx->bits);
    pDDIx->bits = 0;
    Io_Analog_Handle();
#endif
    osDelay(50);
  }
  /* USER CODE END Read_Io_Task */
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
    osDelay(100);
  }
  /* USER CODE END IWDG_Task */
}

/* Timer_Callback function */
void Timer_Callback(void const *argument)
{
  /* USER CODE BEGIN Timer_Callback */
  // Master_Poll();
  /* USER CODE END Timer_Callback */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
