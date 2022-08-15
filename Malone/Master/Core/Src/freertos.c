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
#include "Flash.h"
#include "lte.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
// GPIO_PinState State = GPIO_PIN_SET;
bool g_oSRunFlag = false;
Save_HandleTypeDef Save_Flash;
Save_User *puser = &Save_Flash.User;
// const uint8_t para_flash_area[128 * 1024 * 2U] __attribute__((at(UPDATE_SAVE_ADDRESS)));
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// EventGroupHandle_t Event_Handle = NULL;
Save_Param Save_InitPara = {
    .crc16 = 0xF168,
    .Ptank_max = 4.0F,
    .Ptank_min = 0.0F,
    .Pvap_outlet_max = 4.0F,
    .Pvap_outlet_min = 0.0F,
    .Pgas_soutlet_max = 4.0F,
    .Pgas_soutlet_min = 0.0F,
    .Ltank_max = 20.0F,
    .Ltank_min = 0.0F,
    .Ptoler_upper = 100.0F,
    .Ptoler_lower = 0.0F,
    .Ltoler_upper = 2.0F,
    // .Ltoler_lower = 0.5F,
    .PStank_supplement = 1.3F,
    .PSspf_start = 2.0F,
    .PSspf_stop = 1.8F,
    .PSvap_outlet_Start = 1.2F,
    .PSvap_outlet_stop = 1.1F,
    .Pback_difference = 1.8F,
    .Ptank_difference = 1.6F,
    .PPvap_outlet_Start = 2.1F,
    .PPvap_outlet_stop = 1.6F,
    .PPspf_start = 2.1F,
    .PPspf_stop = 1.9F,
    .Ptank_limit = 1.2F,
    .Ltank_limit = 2.0F,
    .Htank = 0.1F,
    .Rtank = 1.25F,
    .LEtank = 11.14F,
    .Dtank = 1.977F,

    .Error_Code = 0x0000,
    .User_Name = 0x07E6,
    .User_Code = 0x0522,
    .System_Version = SYSTEM_VERSION(),
    .System_Flag = 0xFFFFFFFF,
    .Slave_IRQ_Table.IRQ_Table_SetFlag = 0xFFFFFFFF,
    // .Update = 0xFFFFFFFF,
};
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
osThreadId TransimtHandle;
osThreadId TreportHandle;
osMessageQId CodeQueueHandle;
osMessageQId UserQueueHandle;
osMessageQId SureQueueHandle;
osTimerId ReportHandle;
osTimerId ModbusHandle;
osMutexId shellMutexHandle;
osSemaphoreId Recive_CpuHandle;
osSemaphoreId Recive_Rs485Handle;
osSemaphoreId Recive_Rs232Handle;
osSemaphoreId Recive_WifiHandle;
osSemaphoreId Recive_LteHandle;
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
void IRQ_Task(void const *argument);
void Control_Task(void const *argument);
void Transimt_Task(void const *argument);
void Report_Task(void const *argument);
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
  shellPrint(Shell_Object, "@Error:memory allocation failed!\r\n");
  shellPrint(Shell_Object, "\r\n\r\nOS remaining heap = %dByte, Mini heap = %dByte\r\n",
             xPortGetFreeHeapSize(), xPortGetMinimumEverFreeHeapSize());
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

/**
 * @brief	Background setting parameters will be written to the Modbus holding register
 * @details
 * @param	ps Point to the address of the first background parameter
 * @retval	None
 */
void Param_WriteBack(Save_HandleTypeDef *ps)
{
  // uint16_t len = sizeof(Save_Param) - sizeof(uint32_t) - sizeof(ps->Param.crc16);
  ps->Param.System_Flag = (*(__IO uint32_t *)UPDATE_SAVE_ADDRESS);
  ps->Param.System_Version = SYSTEM_VERSION();
  /*Parameters are written to the mdbus hold register*/
  mdSTATUS ret = mdRTU_WriteHoldRegs(Slave1_Object, PARAM_BASE_ADDR,
                                     GET_PARAM_SITE(Save_Param, Slave_IRQ_Table, uint16_t),
                                     (mdU16 *)&ps->Param);
  if (ret == mdFALSE)
  {
#if defined(USING_DEBUG)
    shellPrint(Shell_Object, "Parameter write to hold register failed!\r\n");
#endif
  }
}

/**
 * @brief	Report screen background parameters
 * @details
 * @param	pd:Dewin obj
 * @param sp:data pointer
 * @retval	NULL
 */
void Report_Backparam(pDwinHandle pd, Save_Param *sp)
{
  uint32_t actual_size = offsetof(Save_Param, Error_Code); // 29*szieof(float)
#if defined(USING_FREERTOS)
  float *pdata = (float *)CUSTOM_MALLOC(actual_size);
  if (!pdata)
    goto __exit;
#else
  float pdata[sizeof(Save_Param) - 1U];
#endif

  memcpy(pdata, sp, actual_size);
  for (float *p = pdata; p < pdata + actual_size / sizeof(float); p++)
  {
#if defined(USING_DEBUG)
    shellPrint(Shell_Object, "sp = %p,p = %p, *p = %.3f\r\n", sp, p, *p);
#endif
    Endian_Swap((uint8_t *)p, 0U, sizeof(float));
  }
  pd->Dw_Write(pd, PARAM_SETTING_ADDR, (uint8_t *)pdata, actual_size);

__exit:
  CUSTOM_FREE(pdata);
}

/**
 * @brief	Determine how the 4G module works
 * @details
 * @param	handler:modbus master/slave handle
 * @retval	true：MODBUS;fasle:shell
 */
bool Check_Mode(ModbusRTUSlaveHandler handler)
{
  ReceiveBufferHandle pB = handler->receiveBuffer;

  return (!!((pB->count == 1U) && (pB->buf[0] == ENTER_CODE)));
}

/**
 * @brief	Remote OTA
 * @details
 * @param handler:modbus master/slave handle
 * @param
 * @retval	NULL
 */
void OTA_Update(ModbusRTUSlaveHandler handler)
{
  uint32_t update_flag = 0;
  if (Check_Mode(handler))
  {
#if defined(USING_DEBUG)
    shellPrint(Shell_Object, "About to enter upgrade mode .......\r\n");
#endif
    /*Switch to the upgrade page*/
    if (Dwin_Object)
    {
#define Update_Page 0x11
      Dwin_Object->Dw_Page(Dwin_Object, Update_Page);
    }

    update_flag = (*(__IO uint32_t *)UPDATE_SAVE_ADDRESS);

    if (((update_flag & 0xFFFF0000) >> 16U) == UPDATE_APP1)
    {
      update_flag = (((uint32_t)UPDATE_APP2 << 16U) | UPDATE_CMD);
    }
    else
    {
      update_flag = (((uint32_t)UPDATE_APP1 << 16U) | UPDATE_CMD);
    }
    // update_flag = ((*(__IO uint32_t *)UPDATE_SAVE_ADDRESS) & 0xFFFF0000) | UPDATE_CMD;
    taskENTER_CRITICAL();
    FLASH_Write(UPDATE_SAVE_ADDRESS, (uint32_t *)&update_flag, sizeof(update_flag));
    taskEXIT_CRITICAL();
#define __RESET_SYSTEM
    {
      __set_FAULTMASK(1);
      NVIC_SystemReset();
    }
  }
  else
  {
    mdRTU_Handler(Slave1_Object);
  }
}

/**
 * @brief	Read the status of 4G module pins (work, net, link)
 * @details	13. Pelink, 15 hardware connection
 *          Affected by the size end, it only needs to be placed at the lower 8bit
 * @param	None
 * @retval	Three lamp states (8-10bit)
 */
static uint8_t Read_ATx_State(void)
{
#define PINX_NUM 5U
  GPIO_TypeDef *pGPIOx;
  uint16_t GPIO_Pinx;
  uint8_t status = 0, bit = 0;

  for (uint8_t i = 0; i < PINX_NUM; i++)
  {
    pGPIOx = i <= 2U ? LTE_LINK_GPIO_Port : WIFI_LINK_GPIO_Port;
    GPIO_Pinx = i > 1U ? (i == 2U ? LTE_LINK_Pin : (i == 3U ? WIFI_READY_Pin : WIFI_LINK_Pin)) : LTE_NET_Pin;
    /*The 1.8V level of link and net of 4G module matches
    with the 3.3 level of MCU, resulting in effective level reversal.*/
    bit = (uint32_t)(pGPIOx->IDR & GPIO_Pinx) ? 0U : 1U;
    status |= (uint8_t)(bit << i);
  }
#if defined(USING_DEBUG)
  // shellPrint(Shell_Object, "LTE_Status = 0x%x.\r\n", status);
#endif
  return status;
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
  Save_HandleTypeDef *ps = &Save_Flash;
  MX_FLASH_Init();
  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* definition and creation of shellMutex */
  osMutexDef(shellMutex);
  shellMutexHandle = osMutexCreate(osMutex(shellMutex));

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* definition and creation of Recive_Cpu */
  osSemaphoreDef(Recive_Cpu);
  Recive_CpuHandle = osSemaphoreCreate(osSemaphore(Recive_Cpu), 1);

  /* definition and creation of Recive_Rs485 */
  osSemaphoreDef(Recive_Rs485);
  Recive_Rs485Handle = osSemaphoreCreate(osSemaphore(Recive_Rs485), 1);

  /* definition and creation of Recive_Rs232 */
  osSemaphoreDef(Recive_Rs232);
  Recive_Rs232Handle = osSemaphoreCreate(osSemaphore(Recive_Rs232), 1);

  /* definition and creation of Recive_Wifi */
  osSemaphoreDef(Recive_Wifi);
  Recive_WifiHandle = osSemaphoreCreate(osSemaphore(Recive_Wifi), 1);

  /* definition and creation of Recive_Lte */
  osSemaphoreDef(Recive_Lte);
  Recive_LteHandle = osSemaphoreCreate(osSemaphore(Recive_Lte), 1);

  /* definition and creation of Code_Signal */
  osSemaphoreDef(Code_Signal);
  Code_SignalHandle = osSemaphoreCreate(osSemaphore(Code_Signal), 1);

  /* definition and creation of Recive_Usb */
  osSemaphoreDef(Recive_Usb);
  Recive_UsbHandle = osSemaphoreCreate(osSemaphore(Recive_Usb), 1);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
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
    __HAL_UART_ENABLE_IT(&huart2, UART_IT_IDLE);
    /*DMA buffer must point to an entity address!!!*/
    HAL_UART_Receive_DMA(&huart2, mdRTU_Recive_Buf(Slave1_Object), MODBUS_PDU_SIZE_MAX);
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
  FLASH_Read(PARAM_SAVE_ADDRESS, &ps->Param, sizeof(Save_Param));
  // HAL_FLASH_Unlock();
  // if (__HAL_FLASH_GET_FLAG(FLASH_SR_SNECCERR))
  // {
  //   __HAL_FLASH_CLEAR_FLAG(FLASH_SR_SNECCERR);
  // }
  // HAL_FLASH_Lock();
  /*Must be 4 byte aligned!!!*/
  uint16_t crc16 = Get_Crc16((uint8_t *)&ps->Param, sizeof(Save_Param) - sizeof(ps->Param.crc16), 0xFFFF);
#if defined(USING_DEBUG)
  // shellPrint(Shell_Object, "ps->Param.flag = 0x%x\r\n", ps->Param.flag);
  shellPrint(Shell_Object, "ps->Param.crc16 = 0x%x, crc16 = 0x%x.\r\n", ps->Param.crc16, crc16);
#endif
  if (crc16 != ps->Param.crc16)
  {
    Save_InitPara.crc16 = Get_Crc16((uint8_t *)&Save_InitPara, sizeof(Save_Param) - sizeof(Save_InitPara.crc16), 0xFFFF);
    memcpy(&ps->Param, &Save_InitPara, sizeof(Save_Param));
    FLASH_Write(PARAM_SAVE_ADDRESS, (uint32_t *)&Save_InitPara, sizeof(Save_Param));
#if defined(USING_DEBUG)
    shellPrint(Shell_Object, "Save_InitPara.crc16 = 0x%x,First initialization of flash parameters!\r\n", Save_InitPara.crc16);
#endif
    /*Initial 4G module or Wifi*/
    MX_AtInit();
    if (Lte_Object)
    {
      Lte_Object->AT_SetDefault(Lte_Object);
      Lte_Object->Free_AtObject(&Lte_Object);
    }
    if (Wifi_Object)
    {
      Wifi_Object->AT_SetDefault(Wifi_Object);
      Wifi_Object->Free_AtObject(&Wifi_Object);
    }
  }
  /*Check whether the board information has been initialized*/
  if (ps->Param.Slave_IRQ_Table.IRQ_Table_SetFlag != SAVE_SURE_CODE)
  {
#if defined(USING_DEBUG)
    shellPrint(Shell_Object, "Please initialize board information!\r\n");
#endif
#define CONFIG_CARD_INFO_PAGE 29U
    /*The screen forcibly jumps to the image page of the board information configuration*/
    Dwin_Object->Dw_Page(Dwin_Object, CONFIG_CARD_INFO_PAGE);
  }
  else
  {
    if (IRQ_Table.pReIRQ && IRQ_Table.pIRQ)
    {
      /*Copy flash parameters to the application area*/
      // memcpy(IRQ_Table.pReIRQ, ps->Param.Slave_IRQ_Table.ReIRQ, sizeof(ps->Param.Slave_IRQ_Table.ReIRQ));
      memcpy(IRQ_Table.pIRQ, ps->Param.Slave_IRQ_Table.IRQ, sizeof(ps->Param.Slave_IRQ_Table.IRQ));
      IRQ_Table.TableCount = ps->Param.Slave_IRQ_Table.TableCount;
#if defined(USING_DEBUG)
      shellPrint(Shell_Object, "\r\nIRQ_Table Information:\r\nSlaveId\t\tPriority\t\tType\t\tNumber\r\n");
      for (uint8_t i = 0; i < CARD_NUM_MAX; i++)
      {
        shellPrint(Shell_Object, "0x%x\t\t0x%x\t\t\t0x%x\t\t%d\r\n", IRQ_Table.pIRQ[i].SlaveId, IRQ_Table.pIRQ[i].Priority,
                   IRQ_Table.pIRQ[i].TypeCoding, IRQ_Table.pIRQ[i].Number);
      }
#endif
    }
  }
  /*Parameters are stored in the holding register*/
  // Param_WriteBack(ps);
  /*Turn off the global interrupt in bootloader, and turn it on here*/
  // __set_FAULTMASK(0);
  /*Solve the problem that the background data cannot be received due to the unstable power supply when the Devon screen is turned on*/
  // HAL_Delay(2000);
  // Report_Backparam(Dwin_Object, &ps->Param);
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

#define REPORRT_TIMERS 800U
  osTimerStart(ReportHandle, REPORRT_TIMERS);
  osTimerStart(ModbusHandle, 1000U);
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* definition and creation of CodeQueue */
  osMessageQDef(CodeQueue, 16, uint16_t);
  CodeQueueHandle = osMessageCreate(osMessageQ(CodeQueue), NULL);

  /* definition and creation of UserQueue */
  osMessageQDef(UserQueue, 4, Save_User);
  UserQueueHandle = osMessageCreate(osMessageQ(UserQueue), NULL);

  /* definition and creation of SureQueue */
  osMessageQDef(SureQueue, 16, IRQ_Request);
  SureQueueHandle = osMessageCreate(osMessageQ(SureQueue), NULL);

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
  RS485Handle = osThreadCreate(osThread(RS485), (void *)Slave2_Object);

  /* definition and creation of Wifi */
  osThreadDef(Wifi, Wifi_Task, osPriorityLow, 0, 256);
  WifiHandle = osThreadCreate(osThread(Wifi), (void *)Slave1_Object);

  /* definition and creation of C4G */
  osThreadDef(C4G, C4G_Task, osPriorityLow, 0, 256);
  C4GHandle = osThreadCreate(osThread(C4G), (void *)Slave1_Object);

  /* definition and creation of Master */
  osThreadDef(Master, Master_Task, osPriorityHigh, 0, 256);
  MasterHandle = osThreadCreate(osThread(Master), (void *)Slave1_Object);

  /* definition and creation of Interrupt */
  osThreadDef(Interrupt, IRQ_Task, osPriorityRealtime, 0, 256);
  InterruptHandle = osThreadCreate(osThread(Interrupt), (void *)&IRQ_Table);

  /* definition and creation of control */
  osThreadDef(control, Control_Task, osPriorityNormal, 0, 512);
  controlHandle = osThreadCreate(osThread(control), (void *)&Save_Flash);

  /* definition and creation of Transimt */
  osThreadDef(Transimt, Transimt_Task, osPriorityHigh, 0, 512);
  TransimtHandle = osThreadCreate(osThread(Transimt), (void *)&IRQ_Table);

  /* definition and creation of Treport */
  osThreadDef(Treport, Report_Task, osPriorityIdle, 0, 512);
  TreportHandle = osThreadCreate(osThread(Treport), (void *)&Save_Flash);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
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
  pDwinHandle pDewin = (pDwinHandle)argument;
  // osDelay(5);
  // /*Parameters are stored in the holding register*/
  // Param_WriteBack(&Save_Flash);
  /*Solve the problem of screen background parameter delay*/
  osDelay(3000);
  Report_Backparam(Dwin_Object, &Save_Flash.Param);
  /* Infinite loop */
  for (;;)
  {
    /*https://www.cnblogs.com/w-smile/p/11333950.html*/
    if ((osOK == osSemaphoreWait(Recive_Rs232Handle, osWaitForever)) && pDewin)
    {
      /*Close the reporting timer*/
      // osTimerStop(ReportHandle);
      /*Suspend reporting task*/
      // osThreadSuspend(TreportHandle);
      Dwin_Handler(pDewin);
      /*Resume reporting task*/
      // osThreadResume(TreportHandle);
      // xTimerReset(ReportHandle, 1U);
      // osTimerStart(ReportHandle, REPORRT_TIMERS);
#if defined(USING_DEBUG)
      // shellPrint(Shell_Object, "Screen received a packet of data .\r\n");
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
  struct ModbusRTUSlave *pSlave = (struct ModbusRTUSlave *)argument;
  /* Infinite loop */
  for (;;)
  {
    /*https://www.cnblogs.com/w-smile/p/11333950.html*/
    if ((osOK == osSemaphoreWait(Recive_Rs485Handle, osWaitForever)) && pSlave)
    {
      pSlave->uartId = 0x03;
      mdRTU_Handler(pSlave);
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
  struct ModbusRTUSlave *pSlave = (struct ModbusRTUSlave *)argument;
  /* Infinite loop */
  for (;;)
  {
    /*https://www.cnblogs.com/w-smile/p/11333950.html*/
    if ((osOK == osSemaphoreWait(Recive_WifiHandle, osWaitForever)) && pSlave)
    {
      pSlave->uartId = 0x05;
      // mdRTU_Handler(Slave1_Object);
      OTA_Update(pSlave);
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
  struct ModbusRTUSlave *pSlave = (struct ModbusRTUSlave *)argument;
  /* Infinite loop */
  for (;;)
  {
    /*https://www.cnblogs.com/w-smile/p/11333950.html*/
    if ((osOK == osSemaphoreWait(Recive_LteHandle, osWaitForever)) && pSlave)
    {
      pSlave->uartId = 0x02;
      // mdRTU_Handler(Slave1_Object);
      OTA_Update(pSlave);
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
#define MASTER_OBJECT Slave1_Object
#define ULONG_MAX 0xFFFFFFFF
  uint32_t target_slave_id = 0;
  struct ModbusRTUSlave *pMaster = (struct ModbusRTUSlave *)argument;

  /* Infinite loop */
  for (;;)
  {
    /*https://www.cnblogs.com/w-smile/p/11333950.html*/
    if ((osOK == osSemaphoreWait(Recive_CpuHandle, osWaitForever)) && pMaster)
    {
      pMaster->uartId = 0x01;
      pMaster->portRTUMasterHandle(pMaster, (mdU8)target_slave_id);
      xTaskNotifyWait(0x00,             /* Don't clear any notification bits on entry. */
                      ULONG_MAX,        /* Reset the notification value to 0 on exit. */
                      &target_slave_id, /* Notified value pass out in ulNotifiedValue. */
                      osWaitForever);   /* Block indefinitely. */
    }
  }
  /* USER CODE END Master_Task */
}

/* USER CODE BEGIN Header_IRQ_Task */
/**
 * @brief Function implementing the Interrupt thread.
 * @param argument: Not used
 * @retval None
 */
__inline uint8_t Get_InterruptSite(uint16_t id)
{
#define ERROR_INTERRUPT 0xFF
  uint8_t site = 0x00;

  if ((id == 0U) || ((id % 2U) && (id != 0x01)))
    return ERROR_INTERRUPT;

  while (id != 0x01)
  {
    id >>= 1U, site++;
  }
  return site;
}
/**
 * @brief Function implementing the Interrupt thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_IRQ_Task */
void IRQ_Task(void const *argument)
{
  /* USER CODE BEGIN IRQ_Task */
  uint16_t interrupt_id;
  Slave_IRQTableTypeDef *sp_irq = (Slave_IRQTableTypeDef *)argument;
  IRQ_Request *p_current = NULL;
  /* Infinite loop */
  for (;;)
  {
    /*It is detected that the slave has issued an interrupt request*/
    if (xQueueReceive(CodeQueueHandle, &interrupt_id, osWaitForever) == pdPASS)
    {
      p_current = &sp_irq->pReIRQ[sp_irq->SiteCount];
#if defined(USING_DEBUG)
      // shellPrint(Shell_Object, "Note: Generate an interrupt: 0x%x.\r\n", interrupt_id);
#endif
      /*Separate the interrupt number and count the interrupt source*/
      while ((interrupt_id) && (sp_irq->SiteCount++ < CARD_NUM_MAX))
      /*Ensure that the previous round of interrupt has been handled correctly*/
      {
        p_current->site = interrupt_id;
        interrupt_id &= interrupt_id - 1U;
        p_current->site -= interrupt_id;
        p_current->flag = true;
        /*Assign priority to the current interrupt source*/
        if (sp_irq->TableCount)
        {
          for (IRQ_Code *p = sp_irq->pIRQ; p < sp_irq->pIRQ + sp_irq->TableCount; p++)
          {
            if ((uint8_t)(pow(2.0, p->SlaveId)) == p_current->site)
            {
              p_current->Priority = p->Priority;
              break;
            }
          }
        }
// sp_irq->SiteCount++;
#if defined(USING_DEBUG)
        // shellPrint(Shell_Object, "Note: re_irq[%d]_id = 0x%x.\r\n", sp_irq->SiteCount, p_current->site);
#endif
        p_current++;
      }
      /*Record in the interrupt list. When the board sends an
      interrupt request for the second time, it will be prioritized*/
      if (sp_irq->TableCount && (sp_irq->SiteCount > 1U))
      {
        Quick_Sort(sp_irq->pReIRQ, sp_irq->SiteCount);
      }
      for (uint16_t i = 0; i < sp_irq->SiteCount; i++)
      {
        xQueueSend(SureQueueHandle, &sp_irq->pReIRQ[i], 0);
      }
      sp_irq->SiteCount = 0;
    }
  }
  /* USER CODE END IRQ_Task */
}

/* USER CODE BEGIN Header_Control_Task */
/**
 * @brief  Check whether the status of each switch changes
 * @param  p_current current state
 * @param  p_last Last status
 * @param  size  Detection length
 * @retval true/false
 */
bool Check_Qx_State(mdBit *p_current, mdBit *p_last, uint8_t size)
{
  bool ret = false;
  if (p_current && p_last && size)
  {
    for (uint8_t i = 0; i < size; i++)
    {
      if (p_current[i] != p_last[i])
      {
        p_last[i] = p_current[i];
        ret = true;
      }
    }
  }
  return ret;
}

/**
 * @brief Function implementing the control thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_Control_Task */
void Control_Task(void const *argument)
{
  /* USER CODE BEGIN Control_Task */
  mdBit sbit = mdLow, mode = mdLow, bitx = mdLow;
  mdSTATUS ret = mdFALSE;
  mdBit wbit[VX_SIZE];
  float Ptank = 0, Pcarburetor = 0;
#if (USING_USERTIMER0 || USING_USERTIMER1)
  static Soft_Timer_HandleTypeDef timer[] = {
      {.counts = 0, .flag = false},
      {.counts = 0, .flag = false},
  };
#endif
  // bool flag_group[] = {false, false};
  Save_HandleTypeDef *ps = (Save_HandleTypeDef *)argument;
  Save_User usinfo;
  static bool mutex_flag[] = {false, false, false};
  /*Solve the problem of screen background parameter delay*/
  // osDelay(2000);
  // Report_Backparam(Dwin_Object, &ps->Param);
  /* Infinite loop */
  for (;;)
  {
    uint16_t error_code = 0;
    /*Initialize WBIT*/
    memset(wbit, false, VX_SIZE);
    /*The default state of the initialization electric valve is off*/
    // Close_Qx(5U), Open_Qx(6U), Close_Qx(7U), Open_Qx(8U);
#define __USER_CONTRL_PARAMTER_CALCULATION
    {
      ret = mdRTU_ReadInputRegisters(Slave1_Object, INPUT_ANALOG_START_ADDR, BX_SIZE * 2U, (mdU16 *)&ps->User);
      if (ret == mdFALSE)
      {
#if defined(USING_DEBUG)
        shellPrint(Shell_Object, "Failed to read input register!\r\n");
#endif
        goto __exit;
      }
      mdBit enable_sate[] = {false, false};
      /*Read whether the remote enables the secondary sensor to participate in the control*/
      ret = mdRTU_ReadCoils(Slave1_Object, ENABLE_S_PTANK_ADDR, sizeof(enable_sate) / sizeof(mdBit), enable_sate);
      if (ret == mdFALSE)
      {
#if defined(USING_DEBUG)
        shellPrint(Shell_Object, "enable_sate[0] = 0x%d,enable_sate[1] = 0x%d\r\n", enable_sate[0], enable_sate[1]);
#endif
        goto __no_action;
      }
      // taskENTER_CRITICAL();
      for (float *p = &ps->User.Ptank_M, *pu = &ps->Param.Ptank_max, *pinfo = (float *)&usinfo;
           p < &ps->User.Ptank_M + BX_SIZE; p++, pinfo++)
      {
#define ERROR_BASE_CODE 0x02
        uint8_t site = p - &ps->User.Ptank_M;
#if defined(USING_DEBUG)
        // shellPrint(Shell_Object, "R_Current[0x%X] = %.3f\r\n", p, *p);
#endif
        /*User sensor access error check*/
#define ERROR_CHECK
        {
          /*Filter sub sensor error detection*/
          if (!error_code && (!site || (site % 2)))
          {
            error_code = *p <= 0.0F ? (3U * site + ERROR_BASE_CODE) : (*p < CURRENT_LOWER ? (3U * site + ERROR_BASE_CODE + 1U) : (*p > (CURRENT_LOWER + CURRENT_UPPER + 1.0F) ? (3U * site + ERROR_BASE_CODE + 2U) : 0));
          }
        }
        if (p < &ps->User.Ltank)
        {
          *p = Get_Target(*p, *pu, *(pu + 1U));
          pu += 2U;
        }
        else
        {
          *p = (p == &ps->User.Ptank_S) ? Get_Target(*p, ps->Param.Ptank_max, ps->Param.Ptank_min)
               : (p == &ps->User.Ltank) ? Get_Ptank_Level(Get_Target(*p, ps->Param.Ltank_max, ps->Param.Ltank_min),
                                                          ps->Param.Htank, ps->Param.Rtank, ps->Param.LEtank)
                                        : Get_Target(*p, ps->Param.Ptoler_upper, ps->Param.Ptoler_lower);
        }
        /*Convert analog signal into physical quantity*/
        // *p = Get_Target(*p, *pu, *(pu + 1U));
        *p = *p <= 0.0F ? 0 : *p;
        *pinfo = *p;
#if defined(USING_DEBUG)
        // shellPrint(Shell_Object, "max = %.3f,min = %.3f, C_Value[0x%p] = %.3fMpa/M3\r\n", *pu, *(pu + 1U), p, *p);
#endif
      }
      ps->Param.Error_Code = error_code;
#define __Enable_Secondary_Sensor
      {
        Ptank = enable_sate[0] ? ps->Param.Error_Code = 0, ps->User.Ptank_S : ps->User.Ptank_M;
        Pcarburetor = enable_sate[1] ? ps->Param.Error_Code = 0, ps->User.Pgas_soutlet : ps->User.Pvap_outlet;
      }
      // taskEXIT_CRITICAL();
      if (xQueueSend(UserQueueHandle, &usinfo, 0) != pdPASS)
      {
#if defined(USING_DEBUG)
        shellPrint(Shell_Object, "Error: Failed to send user parameters!\r\n");
        shellPrint(Shell_Object, "OS remaining heap = %dByte, Mini heap = %dByte\r\n\r\n",
                   xPortGetFreeHeapSize(), xPortGetMinimumEverFreeHeapSize());
#endif
      }
    }
    // goto __no_action;
#if defined(USING_DEBUG)
    // shellPrint(Shell_Object, "ps->User.Ptank = %.3f\r\n", ps->User.Ptank);
#endif
    sbit = mdLow;
    /*Read start signal:0-N,As long as one start signal is valid, it is valid*/
    for (uint8_t i = 0; i < START_SIGNAL_MAX; i++)
    {
      ret = mdRTU_ReadInputCoil(Slave1_Object, INPUT_DIGITAL_START_ADDR + i, bitx);
      sbit += bitx;
      // rbit |= (uint8_t)(bitx << i);
#if defined(USING_DEBUG)
      // shellPrint(Shell_Object, "bitx[%d] = 0x%d\r\n", i, bitx);
#endif
      /*When the start signal is valid, the corresponding user valve is also valid.*/
#define __OPEN_USER_VALVE
      {
        wbit[USER_COIL_OFFSET + i] = bitx;
      }
      if (ret == mdFALSE)
      {
#if defined(USING_DEBUG)
        shellPrint(Shell_Object, "IN_D[%d] = 0x%d\r\n", INPUT_DIGITAL_START_ADDR, sbit);
#endif
        goto __no_action;
      }
    }
    /*Manual mode management highest authority*/
    ret = mdRTU_ReadCoil(Slave1_Object, M_MODE_ADDR, mode);
    if (ret == mdFALSE)
    {
#if defined(USING_DEBUG)
      shellPrint(Shell_Object, "Mode[%d] = 0x%d\r\n", INPUT_DIGITAL_START_ADDR, mode);
#endif
      goto __no_action;
    }
    if (mode == mdHigh)
    {
#if defined(USING_DEBUG)
      shellPrint(Shell_Object, "@Note: Manual mode startup, automatic management failure!\r\n");
#endif
      /*Clear error codes*/
      ps->Param.Error_Code = 0;
      goto __exit;
    }
    /*Safe operation guarantee*/
#define SAFETY____________________________________________________________________Start
    if ((Ptank <= ps->Param.Ptank_limit) || (ps->User.Ltank <= ps->Param.Ltank_limit) ||
        (ps->Param.Error_Code))
    {
      /*close Q0、Q1、Q3、Q4*/
      // Close_Qx(0U), Close_Qx(1U), Close_Qx(3U), Close_Qx(4U);
      memset(wbit, false, VX_SIZE);
/*reset timer*/
#if (USING_USERTIMER0)
      Reset_SoftTimer(&timer[0U], T_5S);
#endif
#if (USING_USERTIMER1)
      Reset_SoftTimer(&timer[1U], T_5S); //######
#endif
#if defined(USING_DEBUG_APPLICATION)
      shellPrint(Shell_Object, "@SAF: close Q0 Q1 Q2 Q3.\r\n");
#endif
      goto __no_action;
    }
#define SAFETY____________________________________________________________________End
#if defined(USING_DEBUG_APPLICATION)
    shellPrint(Shell_Object, "sbit = 0x%d\r\n", sbit);
#endif
    /*Start mode: The sbit value is valid if it is not 0.*/
    if (sbit)
    {
/*Action condition set of pressure relief solenoid valve and gas phase valve*/
#define A1____________________________________________________________________Start
#if defined(USING_DEBUG_APPLICATION)
      // shellPrint(Shell_Object, "ps->User.Ptank = %.3f, ps->Param.PSspf_start = %.3f\r\n", ps->User.Ptank, ps->Param.PSspf_start);
#endif
      /*The pressure of the storage tank is protected in the safe mode,
      as long as the outlet pressure conditions of the vaporizer are met here*/
      if (Pcarburetor > ps->Param.PSvap_outlet_Start)
      {
#if (USING_USERTIMER0)
        SoftTimer_IsTrue(&timer[0U]); //#######
#endif
#if (USING_USERTIMER1)
        SoftTimer_IsTrue(&timer[1U]); //#######
#endif
        Open_Qx(3U);
#if defined(USING_DEBUG_APPLICATION)
        shellPrint(Shell_Object, "@A1-1: open Q3.\r\n");
#endif
      }
      (Ptank >= ps->Param.PSspf_start)  ? mutex_flag[0] = true
      : (Ptank <= ps->Param.PSspf_stop) ? mutex_flag[0] = false
                                        : false;
      /*Pressure relief*/
      if (mutex_flag[0])
      {
        Open_Qx(0U);
#if (USING_USERTIMER0)
        /*reset timer*/
        // Reset_SoftTimer(&timer[0U], T_5S);
#endif
#if defined(USING_DEBUG_APPLICATION)
        shellPrint(Shell_Object, "@A1-2: open Q0 Q3.\r\n");
#endif
        goto __no_action;
      }
      else
      {
#if defined(USING_DEBUG_APPLICATION)
        shellPrint(Shell_Object, "@A1-3: close Q0 Q2.\r\n");
#endif
      }
#define A1____________________________________________________________________End
/*Logic during normal operation of gas station*/
#define B1C1____________________________________________________________________Start
      if (Pcarburetor >= ps->Param.PSvap_outlet_Start)
      {
        // SoftTimer_IsTrue(&timer[1U]); //#######
        /*open V1 、close V2*/
        Open_Qx(1U);
#if defined(USING_DEBUG_APPLICATION)
        shellPrint(Shell_Object, "@B1C1-1: open Q1 close Q0\r\n");
#endif
#if (USING_USERTIMER0)
        Set_SoftTimer_Count(&timer[0U], T_5S); //#####
#endif
#if (USING_USERTIMER1)
        Set_SoftTimer_Flag(&timer[1U], false); //######
#endif
            // SoftTimer_IsTrue(&timer[0U]);
        /*open outlet valve*/
        Open_Qx(3U);
#if defined(USING_DEBUG_APPLICATION)
        shellPrint(Shell_Object, "@B1C1-2: open Q3.\r\n");
#endif
      }
      else if (Pcarburetor <= ps->Param.PSvap_outlet_stop &&
               Ptank > ps->Param.PStank_supplement)
      {
#if (USING_USERTIMER0)
        /*reset timer*/
        Reset_SoftTimer(&timer[0U], T_5S);
#endif
#if (USING_USERTIMER1)
        Set_SoftTimer_Count(&timer[1U], T_5S); //#######
#endif
        /*close Q0 and outlet valve*/
        Open_Qx(0U);
#if defined(USING_DEBUG_APPLICATION)
        shellPrint(Shell_Object, "@B1C1-3: open  Q0 Q3;close Q1.\r\n");
#endif
      }
#define B1C1____________________________________________________________________End
      /*Ensure that after the startup mode is switched to the shutdown mode*/
      mutex_flag[1] ? mutex_flag[1] = false : false;
      mutex_flag[2] ? mutex_flag[2] = false : false;
    }
    /*stop mode*/
    else
    {
#if (USING_USERTIMER0)
      /*clear flag*/
      Reset_SoftTimer(&timer[0U], T_5S);
#endif
#if (USING_USERTIMER1)
      Reset_SoftTimer(&timer[1U], T_5S); //######
#endif
      /*Check whether the pressure relief operation is on*/
      mutex_flag[0] ? mutex_flag[0] = false : false;
      /*Carburetor back pressure to storage tank*/
#define A2____________________________________________________________________Start
      if (Pcarburetor > ps->Param.Pback_difference && (Ptank < ps->Param.Ptank_difference))
      {
        /*open Q0*/
        Open_Qx(0U);
#if defined(USING_DEBUG_APPLICATION)
        shellPrint(Shell_Object, "@A2-1: open Q0.\r\n");
#endif
      }
      else
      {
        /*Close Q0*/
        // Close_Qx(1U);
#if defined(USING_DEBUG_APPLICATION)
        shellPrint(Shell_Object, "@A2-2: Close Q0.\r\n");
#endif
      }
#define A2____________________________________________________________________End
      /*Pressure relief of vaporizer to designated greenhouse*/
#define B2____________________________________________________________________Start
      (Pcarburetor >= ps->Param.PPvap_outlet_Start)  ? mutex_flag[1] = true
      : (Pcarburetor <= ps->Param.PPvap_outlet_stop) ? mutex_flag[1] = false
                                                     : false;
      if (mutex_flag[1])
      {
        // Open_Qx(5U), Close_Qx(6U);
        Open_Qx(3U);
        Open_Qx(4U);
#if defined(USING_DEBUG_APPLICATION)
        shellPrint(Shell_Object, "@B2-1: open Q3 Q4.\r\n");
#endif
      }
      else
      {
#if defined(USING_DEBUG_APPLICATION)
        shellPrint(Shell_Object, "@B2-2: close Q3 Q4.\r\n");
#endif
      }
#define B2____________________________________________________________________End
/*The storage tank pressure is leaked through the vent valve*/
#define C2____________________________________________________________________Start
      (Ptank >= ps->Param.PPspf_start)  ? mutex_flag[2] = true
      : (Ptank <= ps->Param.PPspf_stop) ? mutex_flag[2] = false
                                        : false;
      if (mutex_flag[2])
      {
        Open_Qx(2U);
#if defined(USING_DEBUG_APPLICATION)
        shellPrint(Shell_Object, "@C2-1: open Q2.\r\n");
#endif
      }
      else
      {
#if defined(USING_DEBUG_APPLICATION)
        shellPrint(Shell_Object, "@C2-2: close Q2.\r\n");
#endif
      }
#define C2____________________________________________________________________End
    }
  __no_action:
#if defined(USING_DEBUG_APPLICATION)
    // for (uint16_t i = 0; i < VX_SIZE; i++)
    // {
    //   // ret = mdRTU_WriteCoil(Slave1_Object, i, wbit[i]);
    //   shellPrint(Shell_Object, "wbit[%d] = 0x%x\r\n", i, wbit[i]);
    // }
#endif
    ret = mdRTU_WriteCoils(Slave1_Object, OUT_DIGITAL_START_ADDR, VX_SIZE, wbit);
    if (ret == mdFALSE)
    {
#if defined(USING_DEBUG)
      shellPrint(Shell_Object, "write failed!\r\n");
#endif
    }
  __exit:
    osDelay(ACTION_TIMES);
  }
  /* USER CODE END Control_Task */
}

/* USER CODE BEGIN Header_Transimt_Task */
/**
 * @brief Transmission processing.
 * @param tp_irq: Slave Interrupt Table Pointer
 * @param rp_irq: Slave Interrupt Request Table Pointer
 * @param ps:Store parameter handle
 * @retval None
 */
static void DataTransmit_Handle(Slave_IRQTableTypeDef *sp_irq, IRQ_Request *rp_irq, Save_HandleTypeDef *ps)
{
  static uint16_t interrupt_record = 0xFFFF;
  uint16_t first_interrupt = 0;
  uint8_t slave_id = 0x00;
  IRQ_Code *pt_irq = NULL;

  if (rp_irq && rp_irq->site)
  {
    /*Get the slave ID according to the interrupt number*/
    slave_id = (uint8_t)(log(rp_irq->site) / log(2.0));
    /*Set data transmission target channel*/
    MASTER_OBJECT->uartId = 0x01;
    /*Detect whether it is interrupted for the first time*/
    first_interrupt = interrupt_record & rp_irq->site;
    /*Check the first access backplane sign*/
    if (first_interrupt)
    {
      /*Read card type*/
      mdRTU_Master_Codex(MASTER_OBJECT, MODBUS_CODE_17, slave_id, NULL, 0);
    }
    /*The board is not connected to the backplane for the first time*/
    else
    { /*Interrupt device table is not empty*/
      if (sp_irq->TableCount)
      { /*Look for known slaves in the interrupt table*/
        pt_irq = Find_TargetSlave_AdoptId(sp_irq, slave_id);

        if (pt_irq)
        {
          /*Interrupt priority sorting is performed when the board is read back*/
          switch (pt_irq->TypeCoding)
          {
          case Card_AnalogInput:
          {
            mdRTU_Master_Codex(MASTER_OBJECT, MODBUS_CODE_4, pt_irq->SlaveId, NULL, CARD_SIGNAL_MAX * 2U);
          }
          break;
          case Card_DigitalInput:
          {
            mdRTU_Master_Codex(MASTER_OBJECT, MODBUS_CODE_2, pt_irq->SlaveId, NULL, CARD_SIGNAL_MAX);
          }
          break;
          case Card_Lora1:
          case Card_Lora2:
          {
            mdRTU_Master_Codex(MASTER_OBJECT, MODBUS_CODE_2, pt_irq->SlaveId, NULL, CARD_COMM_OFFSET_MAX);
          }
          break;
          /*Invalid board, remove interrupt record table*/
          default:
          {
            /*Read card type*/
            mdRTU_Master_Codex(MASTER_OBJECT, MODBUS_CODE_17, slave_id, NULL, 0);
          }
          break;
          }
        }
      }
    }
    /*Clear the flag after the slave responds correctly*/
    if (xTaskNotify(MasterHandle, slave_id, eSetValueWithoutOverwrite) == pdPASS)
    {
      if (first_interrupt)
      {
        /*Clear first access flag*/
        interrupt_record &= ~rp_irq->site;
      }
      /*Process the next interrupt in the interrupt table*/
    }
    /*no response from slave*/
    else
    {
      if (ps && ps->Param.Slave_IRQ_Table.IRQ_Table_SetFlag == SAVE_SURE_CODE)
      {
#define CARD_ERROR_CODE 23U
        /*Record error code*/
        ps->Param.Error_Code = CARD_ERROR_CODE;
      }
      /*Read card type*/
#if defined(USING_DEBUG)
      // shellPrint(Shell_Object, "Error: Interrupt Slave 0x%02x No Answer!"__INFORMATION(),
      //            slave_id);
      shellPrint(Shell_Object, "Error: Interrupt Slave 0x%02x No Answer!\r\n",
                 slave_id);
#endif
    }
    /*Task notification delay leads to decrease in recognition success rate*/
    osDelay(30);
  }
  else
  {
    /*Slave error handling*/
#if defined(USING_DEBUG)
    // shellPrint(Shell_Object, "Error: Invalid interrupt source: 0x%x."__INFORMATION(),
    //            sp_irq->pReIRQ[sp_irq->SiteCount].site);
    shellPrint(Shell_Object, "Error: Invalid interrupt source: 0x%x, table_site :0x%p\r\n.",
               rp_irq->site, rp_irq);
#endif
  }
}

/**
 * @brief Digital output scheduler.
 * @param sp: Slave Interrupt Table Pointer
 * @param pr: Reporting object pointer
 * @param ctype: Target board type with lookup
 * @retval None
 */
static void Digital_Output(Report_TypeDef *pdr)
{
  uint16_t num, count, bit;
  // uint8_t *pdest = NULL;
  if (pdr && pdr->pr && pdr->sp && pdr->pwbit && (pdr->ctype != Card_None))
  {
    pdr->pr->count = 0;
    if (Save_TargetSlave_Id(pdr->sp, pdr->ctype, pdr->pr, pdr->nums))
    {
      uint8_t target_value[pdr->pr->id_size], len = 0, counts = 0, base_offset = 0;
      for (num = 0; num < pdr->nums; ++num)
      {
        /*Integrated signal source*/
        memset(target_value, 0x00, pdr->pr->id_size);

        switch (pdr->ctype)
        {
        case Card_DigitalOutput:
        {
          counts = 0x01, base_offset = num * CARD_SIGNAL_MAX, len = 0x01;
          // pdest = target_value;
        }
        break;
        case Card_Lora1:
        {
          counts = 0x04, base_offset = num * (CARD_SIGNAL_MAX * 4U), len = 0x04;
          // pdest = &target_value[4U];
          // memcpy(target_value, &target_value[4U], pdr->pr->id_size - 4U);
          /*前移4bit:去掉气站部分有线阀数据,最后8bit数据重复*/
          memcpy(pdr->pwbit, &pdr->pwbit[4U], VX_SIZE - 4U);
        }
        break;
        default:
          break;
        }
        if (counts && num < pdr->pr->count)
        {
          for (count = 0; count < counts; ++count)
          {
            for (bit = 0; bit < CARD_SIGNAL_MAX; ++bit)
            {
              target_value[count] |= (uint8_t)pdr->pwbit[bit + count * CARD_SIGNAL_MAX + base_offset] << bit;
            }
          }
          mdRTU_Master_Codex(MASTER_OBJECT, MODBUS_CODE_15, pdr->pr->p_id[num], (mdU8 *)target_value, len);
          xTaskNotify(MasterHandle, pdr->pr->p_id[num], eSetValueWithOverwrite);
          osDelay(50);
        }
        else
          break;
      }
    }
  }
}

/**
 * @brief Function implementing the Transimt thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_Transimt_Task */
void Transimt_Task(void const *argument)
{
  /* USER CODE BEGIN Transimt_Task */
  Slave_IRQTableTypeDef *sp_irq = (Slave_IRQTableTypeDef *)argument;
  Save_HandleTypeDef *ps = &Save_Flash;
  IRQ_Request requset_irq = {0};
  mdSTATUS ret = mdFALSE;
  mdBit wbit[VX_SIZE], copy_wbit[VX_SIZE];
  uint8_t target_type[__Get_TargetBoardNum(CARD_SIGNAL_MAX)];
  R_TargetTypeDef record_type = {
      .count = 0,
      .p_id = target_type,
      .id_size = sizeof(target_type),
  };
  Report_TypeDef di_report[2] = {
      {
          .ctype = Card_DigitalOutput,
          .pr = &record_type,
          .sp = sp_irq,
          .nums = __Get_TargetBoardNum(CARD_SIGNAL_MAX),
          .pwbit = wbit,
      },
      {
          .ctype = Card_Lora1,
          .pr = &record_type,
          .sp = sp_irq,
          .nums = __Get_TargetBoardNum((CARD_SIGNAL_MAX * 4U)),
          .pwbit = wbit,
      },
  };
  /* Infinite loop */
  for (;;)
  {
    /*Interrupt request count is not empty*/
    if (xQueueReceive(SureQueueHandle, &requset_irq, 2) == pdPASS)
    {
      /*Suspend interrupt processing task*/
      // osThreadSuspend(InterruptHandle);
      /*Check if the interrupt source is valid*/
      DataTransmit_Handle(sp_irq, &requset_irq, ps);
      /*Resume interrupt processing task*/
      // osThreadResume(InterruptHandle);
      // osDelay(100);
    }
    /*One cycle processing completed*/
    else
    {
      ret = mdRTU_ReadCoils(Slave1_Object, OUT_DIGITAL_START_ADDR, VX_SIZE, wbit);
      if (ret == mdFALSE)
      {
#if defined(USING_DEBUG)
        shellPrint(Shell_Object, "read failed!\r\n");
#endif
      }
      else
      {
#define Board_DigitalOutput_______________________________________________________________________Start
        if (Check_Qx_State(wbit, copy_wbit, VX_SIZE) && sp_irq->TableCount && record_type.p_id)
        {
          /*Set data transmission target channel*/
          MASTER_OBJECT->uartId = 0x01;

          for (uint8_t i = 0; i < sizeof(di_report) / sizeof(Report_TypeDef); ++i)
            Digital_Output(&di_report[i]);
        }
#define Board_DigitalOutput_______________________________________________________________________End
      }
    }
    // /*Resume interrupt processing task*/
    // osThreadResume(InterruptHandle);

    // osDelay(10);
    // osDelay(1);
  }
  /* USER CODE END Transimt_Task */
}

/* USER CODE BEGIN Header_Report_Task */
/**
 * @brief Function implementing the Treport thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_Report_Task */
void Report_Task(void const *argument)
{
  /* USER CODE BEGIN Report_Task */
  mdSTATUS ret = mdFALSE;
  mdBit bit = mdLow;
  static bool first_flag = false;
  Save_HandleTypeDef *ps = (Save_HandleTypeDef *)argument;
  Save_User urinfo;
  mdU16 buf[3U] = {0, 0, 0};
  /* Infinite loop */
  for (;;)
  {
    /*Clear last status*/
    memset(buf, 0x00, sizeof(buf));
    for (uint16_t i = 0; i < DWIN_ADDR_SIZE; i++)
    {
      ret = mdRTU_ReadInputCoil(Slave1_Object, INPUT_DIGITAL_START_ADDR + i, bit);
      buf[0] |= i > (CARD_SIGNAL_MAX - 1U) ? (mdU8)(bit << (i - CARD_SIGNAL_MAX))
                                           : (mdU16)(bit << ((i % CARD_SIGNAL_MAX) + CARD_SIGNAL_MAX));
      ret = mdRTU_ReadCoil(Slave1_Object, OUT_DIGITAL_START_ADDR + i, bit);
      buf[2] |= i > (CARD_SIGNAL_MAX - 1U) ? (mdU8)(bit << (i - CARD_SIGNAL_MAX))
                                           : (mdU16)(bit << ((i % CARD_SIGNAL_MAX) + CARD_SIGNAL_MAX));
      if (ret == mdFALSE)
      {
#if defined(USING_DEBUG)
        shellPrint(Shell_Object, "Error: Modbus register read failed!" __INFORMATION());
#endif
      }
    }
#if defined(USING_DEBUG)
    // shellPrint(Shell_Object, "rbit = 0x%02x.\r\n", rbit);
#endif
    /*Start signal*/
    buf[1] = Read_ATx_State() << 8U;
    Dwin_Object->Dw_Write(Dwin_Object, SS_SIGNAL_ADDR, (uint8_t *)buf, sizeof(buf));
    osDelay(DELAY_TIMES);
    if (xQueueReceive(UserQueueHandle, &urinfo, 0) != pdPASS)
    {
#if defined(USING_DEBUG)
      shellPrint(Shell_Object, "Error: Failed to receive user parameters!\r\n");
#endif
    }
    else
    {
      for (float *puser = &urinfo.Ptank_M; puser < &urinfo.Ptank_M + BX_SIZE; puser++)
      {
#if defined(USING_DEBUG)
        // shellPrint(Shell_Object, "Value[%d] = %.3fMpa/M3\r\n", i, temp_data[i]);
#endif
        Endian_Swap((uint8_t *)puser, 0U, sizeof(float));
      }
      Dwin_Object->Dw_Write(Dwin_Object, PRESSURE_OUT_ADDR, (uint8_t *)&urinfo, BX_SIZE * sizeof(float));
      osDelay(DELAY_TIMES);
    }
    /*Report error code*/
    if (ps->Param.Error_Code && (ps->Param.Slave_IRQ_Table.IRQ_Table_SetFlag == SAVE_SURE_CODE))
    {
      buf[0] = (uint16_t)ps->Param.Error_Code;
      buf[0] = (buf[0] >> 8U) | (buf[0] << 8U);
      buf[1] = 0x0100;
      Dwin_Object->Dw_Write(Dwin_Object, ERROR_CODE_ADDR, (uint8_t *)buf, sizeof(buf[0]) + sizeof(buf[1]));
      osDelay(DELAY_TIMES);
      Dwin_Object->Dw_Page(Dwin_Object, ERROR_PAGE);
      first_flag = false;
    }
    else
    {
      if ((!first_flag) && (ps->Param.Slave_IRQ_Table.IRQ_Table_SetFlag == SAVE_SURE_CODE))
      {
        first_flag = true;
        Dwin_Object->Dw_Page(Dwin_Object, MAIN_PAGE);
      }
    }
    /*Parameters are periodically written back to the holding registers*/
    Param_WriteBack(ps);
    osDelay(1000);
  }
  /* USER CODE END Report_Task */
}

/* Report_Callback function */
void Report_Callback(void const *argument)
{
  /* USER CODE BEGIN Report_Callback */

#define Get_Board_Icon(__type)                                 \
  ((__type) == Card_None ? 0x00 : (__type) >= Card_Wifi ? 0x05 \
                                                        : ((__type) / 0x10 + 1U))
  /*Special attention should be paid here:
  the parameter transfer in the timer is only the timer identifier
  and cannot be used for other parameter pointer transfer*/
#if defined(USING_DEBUG)
  // shellPrint(Shell_Object, "Report_Callback !\r\n");
#endif
  static bool first_flag = false;
  Slave_IRQTableTypeDef *p_irq = &IRQ_Table;
  Save_HandleTypeDef *ps = &Save_Flash;

  if (p_irq && p_irq->TableCount && (!first_flag))
  {
#define BOARD_REPORT_SIZE (sizeof(uint16_t) * CARD_NUM_MAX)
#if defined(USING_FREERTOS)
    uint16_t board_buf[BOARD_REPORT_SIZE], *pBoard = board_buf;
    // uint16_t *pBoard = (uint16_t *)CUSTOM_MALLOC(BOARD_REPORT_SIZE);
    if (pBoard)
    {
#endif
      memset(pBoard, 0x00, BOARD_REPORT_SIZE);
      for (IRQ_Code *p = p_irq->pIRQ; p < p_irq->pIRQ + p_irq->TableCount; p++)
      {
        if (p->SlaveId < BOARD_REPORT_SIZE)
        {
          pBoard[p->SlaveId] = (uint16_t)Get_Board_Icon((uint8_t)p->TypeCoding) << 8U;
#if defined(USING_DEBUG)
          // shellPrint(Shell_Object, "pBoard[%d] = %d\r\n", p->SlaveId, pBoard[p->SlaveId]);
#endif
        }
      }
      /*Report board type*/
      Dwin_Object->Dw_Write(Dwin_Object, BOARD_TYPE_ADDR, (uint8_t *)pBoard, BOARD_REPORT_SIZE);
#if defined(USING_FREERTOS)
    }
    // CUSTOM_FREE(pBoard);
#endif
  }
  /*When not configured, update the board information at any time, otherwise it will
  only be updated for the first time*/
  first_flag = (ps->Param.Slave_IRQ_Table.IRQ_Table_SetFlag == SAVE_SURE_CODE) ? true : false;
  /* USER CODE END Report_Callback */
}

/* Modbus_Callback function */
void Modbus_Callback(void const *argument)
{
  /* USER CODE BEGIN Modbus_Callback */
  HAL_GPIO_TogglePin(GPIOB, LED_Pin);
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
  /*Ensure that the message queue has been initialized when the interrupt arrives*/
  if (CodeQueueHandle)
  {
    xQueueSendToBackFromISR(CodeQueueHandle, &irq_id, &xHigherPriorityTaskWoken);
    // xQueueSendFromISR(CodeQueueHandle, &GPIO_Pin, &xHigherPriorityTaskWoken);
    /* Now the buffer is empty we can switch context if necessary. */
    if (xHigherPriorityTaskWoken)
    {
      UBaseType_t uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
      /* Actual macro used here is port specific. */
      osThreadYield();
      taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
    }
  }
  /* NOTE: This function Should not be modified, when the callback is needed,
           the HAL_GPIO_EXTI_Callback could be implemented in the user file
   */
}

/* USER CODE END Application */
