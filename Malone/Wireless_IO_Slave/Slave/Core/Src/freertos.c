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
#include "usart.h"
#include "shell.h"
#include "shell_port.h"
#include "mdrtuslave.h"
#include "io_signal.h"
#include "at.h"
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
osThreadId modbusHandle;
osThreadId io_outputHandle;
osThreadId IWDGHandle;
osTimerId Timer1Handle;
osMutexId shellMutexHandle;
osSemaphoreId ReciveHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
// GPIO_PinState g_State = GPIO_PIN_SET;
bool g_Timerout_Flag = false;
static uint32_t g_Counter = 0;
/* USER CODE END FunctionPrototypes */

void Shell_Task(void const *argument);
void Modbus_Task(void const *argument);
void Io_Output_Task(void const *argument);
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

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of shell */
  osThreadDef(shell, Shell_Task, osPriorityLow, 0, 256);
  shellHandle = osThreadCreate(osThread(shell), (void *)&shell);

  /* definition and creation of modbus */
  osThreadDef(modbus, Modbus_Task, osPriorityBelowNormal, 0, 128);
  modbusHandle = osThreadCreate(osThread(modbus), NULL);

  /* definition and creation of io_output */
  osThreadDef(io_output, Io_Output_Task, osPriorityNormal, 0, 128);
  io_outputHandle = osThreadCreate(osThread(io_output), NULL);

  /* definition and creation of IWDG */
  osThreadDef(IWDG, IWDG_Task, osPriorityRealtime, 0, 64);
  IWDGHandle = osThreadCreate(osThread(IWDG), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  osTimerStart(Timer1Handle, RELAY_CLOSE_TIMES);
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
  HAL_NVIC_DisableIRQ(TIM3_IRQn);
  HAL_NVIC_DisableIRQ(TIM4_IRQn);
  HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
  HAL_NVIC_SetPriority(TIM3_IRQn, 6, 0);
  HAL_NVIC_SetPriority(TIM4_IRQn, 7, 0);
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 7, 0);
  HAL_NVIC_EnableIRQ(TIM3_IRQn);
  HAL_NVIC_EnableIRQ(TIM4_IRQn);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
  At_GetSlaveId();
  /* Infinite loop */
  for (;;)
  {
    shellTask((void *)argument);
  }
  /* USER CODE END Shell_Task */
}

/* USER CODE BEGIN Header_Modbus_Task */
/**
 * @brief Function implementing the modbus thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_Modbus_Task */
void Modbus_Task(void const *argument)
{
  /* USER CODE BEGIN Modbus_Task */
  /* Infinite loop */
  for (;;)
  {
    /*https://www.cnblogs.com/w-smile/p/11333950.html*/
    if (osOK == osSemaphoreWait(ReciveHandle, osWaitForever))
    {
      g_Timerout_Flag = false;
      g_Counter = 0;
      extern bool g_LoraMutex;
      if (!g_LoraMutex)
      {
        mdRTU_Handler();
      }
//		shellPrint(&shell, "buf is %s \r\n", mdhandler->receiveBuffer->buf);
// Usart3_Printf("%s\r\n", mdhandler->receiveBuffer->buf);
#if defined(USING_DEBUG)
      // shellPrint(&shell,"Mdbus_Task is running !\r\n");
#endif
    }
  }
  /* USER CODE END Modbus_Task */
}

/* USER CODE BEGIN Header_Io_Output_Task */
/**
 * @brief Function implementing the io_output thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_Io_Output_Task */
void Io_Output_Task(void const *argument)
{
  /* USER CODE BEGIN Io_Output_Task */
  /* Infinite loop */
  for (;;)
  {
    /*Dog feed signal*/
    // g_State ^= GPIO_PIN_SET;
    // HAL_GPIO_WritePin(GPIOA, WDI_Pin, g_State);
    HAL_GPIO_TogglePin(GPIOA, WDI_Pin);
#if !defined(AS_REPEATER)
    Io_Digital_Output(g_Timerout_Flag);
#endif
    osDelay(50);
  }
  /* USER CODE END Io_Output_Task */
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
  // Usart3_Printf("hello world !\r\n");
  //	shellPrint(&shell, "hello world !\r\n");
#define MAX_COUNTS 10U

  if (++g_Counter > MAX_COUNTS)
  {
    g_Counter = 0U;
    g_Timerout_Flag = true;
  }
  /* USER CODE END Timer_Callback */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
