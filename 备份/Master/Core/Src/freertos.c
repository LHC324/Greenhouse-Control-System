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
    .crc16 = 0xFF17,
    .Ptank_max = 4.0F,
    .Ptank_min = 0.0F,
    .Pvap_outlet_max = 4.0F,
    .Pvap_outlet_min = 0.0F,
    .Pgas_soutlet_max = 4.0F,
    .Pgas_soutlet_min = 0.0F,
    .Ltank_max = 50.0F,
    .Ltank_min = 0.0F,
    .Ptoler_upper = 0.5F,
    .Ptoler_lower = 0.1F,
    .Ltoler_upper = 2.0F,
    .Ltoler_lower = 0.5F,
    .PSspf_start = 2.0F,
    .PSspf_stop = 1.8F,
    .PSvap_outlet_Start = 1.2F,
    .PSvap_outlet_stop = 1.1F,
    .Pback_difference = 0.2F,
    .Ptank_difference = 2.0F,
    .PPvap_outlet_Start = 2.2F,
    .PPvap_outlet_stop = 1.8F,
    .PPspf_start = 2.3F,
    .PPspf_stop = 2.1F,
    .Ptank_limit = 1.2F,
    .Ltank_limit = 2.0F,

    .User_Name = 0x07E6,
    .User_Code = 0x0522,
    .Error_Code = 0x0000,
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

/**
 * @brief	Background setting parameters will be written to the Modbus holding register
 * @details
 * @param	ps Point to the address of the first background parameter
 * @retval	None
 */
void Param_WriteBack(Save_HandleTypeDef *ps)
{
  uint16_t len = sizeof(Save_Param) - sizeof(uint32_t) - sizeof(ps->Param.crc16);
  /*Parameters are written to the mdbus hold register*/
  float *pdata = (float *)CUSTOM_MALLOC(len);
  if (pdata && ps)
  {
    memset(pdata, 0x00, len);
    memcpy(pdata, &ps->Param, len);
    for (uint8_t i = 0; i < len / sizeof(float); i++)
    {
      Endian_Swap((uint8_t *)&pdata[i], 0U, sizeof(float));
    }
    mdSTATUS ret = mdRTU_WriteHoldRegs(Slave1_Object, PARAM_MD_ADDR, len, (mdU16 *)pdata);
    /*Write user name and password*/
    ret = mdRTU_WriteHoldRegs(Slave1_Object, MDUSER_NAME_ADDR, 2U, (mdU16 *)&ps->Param.User_Name);
    mdU16 temp_data[2U] = {(mdU16)CURRENT_SOFT_VERSION, (mdU16)((uint32_t)((*(__IO uint32_t *)UPDATE_SAVE_ADDRESS) >> 16U))};
#if defined(USING_DEBUG)
    shellPrint(Shell_Object, "version:%d, flag:%d.\r\n", temp_data[0], temp_data[1]);
#endif
    /*Write software version number and status*/
    ret = mdRTU_WriteHoldRegs(Slave1_Object, SOFT_VERSION_ADDR, 2U, temp_data);
    if (ret == mdFALSE)
    {
#if defined(USING_DEBUG)
      shellPrint(Shell_Object, "Parameter write to hold register failed!\r\n");
#endif
    }
  }
  CUSTOM_FREE(pdata);
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
#if defined(USING_FREERTOS)
  float *pdata = (float *)CUSTOM_MALLOC(sizeof(Save_Param) - 1U);
  if (!pdata)
    goto __exit;
#else
  float pdata[sizeof(Save_Param) - 1U];
#endif

  memcpy(pdata, sp, sizeof(Save_Param) - 1U);
  for (float *p = pdata; p < pdata + sizeof(Save_Param) / sizeof(float) - 1U; p++)
  {
#if defined(USING_DEBUG)
    shellPrint(Shell_Object, "sp = %p,p = %p, *p = %.3f\r\n", sp, p, *p);
#endif
    Endian_Swap((uint8_t *)p, 0U, sizeof(float));
  }
  pd->Dw_Write(pd, PARAM_SETTING_ADDR, (uint8_t *)pdata, sizeof(Save_Param) - (sizeof(sp->User_Name) + sizeof(sp->User_Code) + sizeof(sp->Error_Code) + sizeof(sp->crc16)));

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
#define Update_Page 0x0F
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
  uint16_t status = 0U;
  uint8_t bit = 0;

  for (uint8_t i = 0; i < PINX_NUM; i++)
  {
    pGPIOx = i > 2U ? WIFI_LINK_GPIO_Port : LTE_LINK_GPIO_Port;
    GPIO_Pinx = i > 1U ? (i < 3U ? LTE_LINK_Pin : (i < 4U ? WIFI_READY_Pin : WIFI_LINK_Pin)) : LTE_NET_Pin;
    bit = (uint32_t)(pGPIOx->IDR & GPIO_Pinx) ? 0U : 1U;
    /*The 1.8V level of link and net of 4G module matches
    with the 3.3 level of MCU, resulting in effective level reversal*/
    bit = i > 2U ? bit : !bit;
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
  /*Turn off the global interrupt in bootloader, and turn it on here*/
  // __set_FAULTMASK(0);
  /*Solve the problem that the background data cannot be received due to the unstable power supply when the Devon screen is turned on*/
  // HAL_Delay(2000);
  Report_Backparam(Dwin_Object, &ps->Param);
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

  /* definition and creation of UserQueue */
  osMessageQDef(UserQueue, 4, Save_User);
  UserQueueHandle = osMessageCreate(osMessageQ(UserQueue), NULL);

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
  InterruptHandle = osThreadCreate(osThread(Interrupt), NULL);

  /* definition and creation of control */
  osThreadDef(control, Control_Task, osPriorityNormal, 0, 512);
  controlHandle = osThreadCreate(osThread(control), NULL);

  /* definition and creation of Transimt */
  osThreadDef(Transimt, Transimt_Task, osPriorityHigh, 0, 512);
  TransimtHandle = osThreadCreate(osThread(Transimt), NULL);

  /* definition and creation of Treport */
  osThreadDef(Treport, Report_Task, osPriorityIdle, 0, 512);
  TreportHandle = osThreadCreate(osThread(Treport), NULL);

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
  /* Infinite loop */
  for (;;)
  {
    /*https://www.cnblogs.com/w-smile/p/11333950.html*/
    if ((osOK == osSemaphoreWait(Recive_Rs232Handle, osWaitForever)) && pDewin)
    {
      /*Close the reporting timer*/
      osTimerStop(ReportHandle);
      /*Suspend reporting task*/
      osThreadSuspend(TreportHandle);
      Dwin_Handler(pDewin);
      /*Resume reporting task*/
      osThreadResume(TreportHandle);
      // xTimerReset(ReportHandle, 1U);
      osTimerStart(ReportHandle, 1000U);
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
      osThreadSuspend(InterruptHandle);
      // osThreadSuspend(TransimtHandle);
      pMaster->uartId = 0x01;
      pMaster->portRTUMasterHandle(pMaster, (mdU8)target_slave_id);
      osThreadResume(InterruptHandle);
      // osThreadResume(TransimtHandle);
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
  static Slave_IRQTableTypeDef *sp_irq = &IRQ_Table;
  static IRQ_Request *p_current = NULL;
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
      while ((interrupt_id) && (sp_irq->SiteCount < CARD_NUM_MAX) &&
             /*Ensure that the previous round of interrupt has been handled correctly*/
             (!p_current->flag))
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
        p_current = &sp_irq->pReIRQ[++sp_irq->SiteCount];
      }
      /*Record in the interrupt list. When the board sends an
      interrupt request for the second time, it will be prioritized*/
      if (sp_irq->TableCount && (sp_irq->SiteCount > 1U))
      {
        Quick_Sort(sp_irq->pReIRQ, sp_irq->SiteCount);
      }
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
__inline bool Check_Vx_State(mdBit *p_current, mdBit *p_last, uint8_t size)
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
#define DELAY_TIMES 5U
/* USER CODE END Header_Control_Task */
void Control_Task(void const *argument)
{
  /* USER CODE BEGIN Control_Task */
  mdBit sbit = mdLow, mode = mdLow, bitx = mdLow;
  // mdU32 addr;
  mdSTATUS ret = mdFALSE;
  // static mdBit wbit[] = {false, false, false, false, false};
  // static mdBit wbit[VX_SIZE], copy_wbit[VX_SIZE];
  static mdBit wbit[VX_SIZE];
  static Soft_Timer_HandleTypeDef timer[] = {
      {.counts = 0, .flag = false},
      {.counts = 0, .flag = false},
  };
  Save_HandleTypeDef *ps = &Save_Flash;
  Save_User usinfo;
  static bool mutex_flag = false;
  static uint8_t state = 1;
  // Slave_IRQTableTypeDef *sp_irq = &IRQ_Table;
  // uint8_t target_type[TARGET_BOARD_NUM];
  // R_TargetTypeDef record_type = {
  //     .count = 0,
  //     .p_id = target_type,
  //     // .pTIRQ = target_type,
  //     // .size = sizeof(target_type) / sizeof(IRQ_Code),
  // };
  /* Infinite loop */
  for (;;)
  {
    // MASTER_OBJECT->uartId = 0x01;
    // MASTER_OBJECT->mdRTU_MasterCodex(MASTER_OBJECT, MODBUS_CODE_17, 0x02);

    /*Initialize WBIT*/
    memset(wbit, false, VX_SIZE);
    // memset(copy_wbit, false, VX_SIZE);

    uint16_t error_code = 0;

    memset((void *)&ps->User, 0x00, BX_SIZE * 2U);
    ret = mdRTU_ReadInputRegisters(Slave1_Object, INPUT_ANALOG_START_ADDR, BX_SIZE * 2U, (mdU16 *)&ps->User);
    if (ret == mdFALSE)
    {
#if defined(USING_DEBUG)
      shellPrint(Shell_Object, "Failed to read input register!\r\n");
#endif
      goto __no_action;
    }
    // taskENTER_CRITICAL();
    for (float *p = &ps->User.Ptank, *pu = &ps->Param.Ptank_max, *pinfo = (float *)&usinfo;
         p < &ps->User.Ptank + BX_SIZE; p++, pu += 2U, pinfo++)
    {
#define ERROR_BASE_CODE 0x02
      uint8_t site = p - &ps->User.Ptank;
#if defined(USING_DEBUG)
      // shellPrint(Shell_Object, "R_Current[0x%X] = %.3f\r\n", p, *p);
#endif
      Endian_Swap((uint8_t *)p, 0U, sizeof(float));
      /*User sensor access error check*/
#define ERROR_CHECK
      {
        if (!error_code)
          error_code = *p <= 0.0F ? (3U * site + ERROR_BASE_CODE) : (*p < CURRENT_LOWER ? (3U * site + ERROR_BASE_CODE + 1U) : (*p > (CURRENT_LOWER + CURRENT_UPPER + 1.0F) ? (3U * site + ERROR_BASE_CODE + 2U) : 0));
      }
      /*Convert analog signal into physical quantity*/
      *p = Get_Target(*p, *pu, *(pu + 1U));
      *p = *p <= 0.0F ? 0 : *p;
      *pinfo = *p;
#if defined(USING_DEBUG)
      // shellPrint(Shell_Object, "max = %.3f,min = %.3f, C_Value[0x%p] = %.3fMpa/M3\r\n", *pu, *(pu + 1U), p, *p);
#endif
    }
    ps->Param.Error_Code = error_code;
    // taskEXIT_CRITICAL();
    if (xQueueSend(UserQueueHandle, &usinfo, 10) != pdPASS)
    // if (osMailPut(UserQueueHandle, &usinfo) != osOK)
    {
#if defined(USING_DEBUG)
      shellPrint(Shell_Object, "Error: Failed to send user parameters!\r\n");
#endif
    }

#if defined(USING_DEBUG)
    // shellPrint(Shell_Object, "ps->User.Ptank = %.3f\r\n", ps->User.Ptank);
#endif

    /*Read start signal:0-N,As long as one start signal is valid, it is valid*/
    for (uint8_t i = 0; i < START_SIGNAL_MAX; i++)
    {
      ret = mdRTU_ReadInputCoil(Slave1_Object, INPUT_DIGITAL_START_ADDR, bitx);
      sbit += bitx;
      wbit[OFFSET] = bitx;
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
      shellPrint(Shell_Object, "Note: Manual mode startup, automatic management failure!\r\n");
#endif
      /*Clear error codes*/
      ps->Param.Error_Code = 0;
      goto __exit;
    }
    /*Safe operation guarantee*/
#define SAFETY
    {
      if ((ps->User.Ptank <= ps->Param.Ptank_limit) || (ps->User.Ltank <= ps->Param.Ltank_limit) || (ps->Param.Error_Code))
      {
        /*close V1、V2、V3*/
        Close_Vx(1U), Close_Vx(2U), Close_Vx(3U), Close_Vx(4U), Close_Vx(5U);
#if defined(USING_DEBUG_APPLICATION)
        shellPrint(Shell_Object, "SAF: close V1 V2 V3\r\n");
#endif
        goto __no_action;
      }
    }
#if defined(USING_DEBUG_APPLICATION)
    shellPrint(Shell_Object, "sbit = 0x%d\r\n", sbit);
#endif
    /*Start mode: The sbit value is valid if it is not 0.*/
    if (sbit)
    {
#define D1
      {
        /*close V4*/
        Close_Vx(4U);
#if defined(USING_DEBUG_APPLICATION)
        shellPrint(Shell_Object, "D1: close V4\r\n");
#endif
      }
#define A1
      {
#if defined(USING_DEBUG_APPLICATION)
        // shellPrint(Shell_Object, "ps->User.Ptank = %.3f, ps->Param.PSspf_start = %.3f\r\n", ps->User.Ptank, ps->Param.PSspf_start);
#endif
        /*Pressure relief*/
        if (ps->User.Ptank >= ps->Param.PSspf_start)
        { /*open counter*/
          // Set_SoftTimer_Flag(&timer[0U], true);
          /*openV1、openV2、V3*/
          Close_Vx(1U), Open_Vx(2U), Open_Vx(3U);
          /*set flag*/
          mutex_flag = true;
#if defined(USING_DEBUG_APPLICATION)
          shellPrint(Shell_Object, "A1: open V1 open V2 V3\r\n");
#endif
          goto __no_action;
        }
        else
        {
          if (ps->User.Ptank <= ps->Param.PSspf_stop)
          {
            mutex_flag = false;
            /*close V2 、close V4*/
            Close_Vx(2U), Close_Vx(4U);
#if defined(USING_DEBUG_APPLICATION)
            shellPrint(Shell_Object, "A1: close V2 V4\r\n");
#endif
          }
          else
          {
            if (mutex_flag)
            {
              goto __no_action;
            }
            else
            {
              /*openV3*/
              Open_Vx(3U);
#if defined(USING_DEBUG_APPLICATION)
              shellPrint(Shell_Object, "A1: open V3\r\n");
#endif
            }
          }
        }
      }
#define B1C1
      {
        if (ps->User.Pvap_outlet >= ps->Param.PSvap_outlet_Start)
        {
          Set_SoftTimer_Flag(&timer[1U], true);
          if (Sure_Overtimes(&timer[1U], STIMES))
          {
            /*open V3*/
            Open_Vx(3U);
#if defined(USING_DEBUG_APPLICATION)
            shellPrint(Shell_Object, "B1C1: open V3\r\n");
#endif
          }
          /*open V1 、close V2*/
          Open_Vx(1U), Close_Vx(2U);
#if defined(USING_DEBUG_APPLICATION)
          shellPrint(Shell_Object, "B1C1: open V1 close V2\r\n");
#endif
        }
        else if (ps->User.Pvap_outlet <= ps->Param.PSvap_outlet_stop)
        {
          /*Clear counter*/
          Clear_Counter(&timer[1U]);
          /*open counter*/
          // Set_SoftTimer_Flag(&timer[1U], true);
          /*close V3*/
          Close_Vx(3U), Close_Vx(1U), Open_Vx(2U);
#if defined(USING_DEBUG_APPLICATION)
          shellPrint(Shell_Object, "B1C1: close V3 close V1 open V2\r\n");
#endif
        }
      }
    }
    /*stop mode*/
    else
    {
      /*Check whether the pressure relief operation is on*/
      if (mutex_flag)
      {
        mutex_flag = false;
      }
#define A2
      {
        if (((ps->User.Pvap_outlet - ps->User.Ptank) >= ps->Param.Pback_difference) &&
            (ps->User.Ptank < ps->Param.Ptank_difference))
        {
          /*open V2*/
          Open_Vx(2U);
#if defined(USING_DEBUG_APPLICATION)
          shellPrint(Shell_Object, "A2: open V2\r\n");
#endif
        }
        else
        {
          /*Close V2*/
          Close_Vx(2U);
#if defined(USING_DEBUG_APPLICATION)
          shellPrint(Shell_Object, "A2: Close V2\r\n");
#endif
        }
      }
#define B2
      {
        if (ps->User.Pvap_outlet >= ps->Param.PPvap_outlet_Start)
        {
          /*open V3、open V5、close V2*/
          Open_Vx(3U), Open_Vx(5U), Close_Vx(2U);
#if defined(USING_DEBUG_APPLICATION)
          shellPrint(Shell_Object, "B2: open V3 open V5 close V2\r\n");
#endif
        }
        else if (ps->User.Pvap_outlet <= ps->Param.PPvap_outlet_stop)
        {
          /*close V3*/
          Close_Vx(3U), Close_Vx(5U);
#if defined(USING_DEBUG_APPLICATION)
          shellPrint(Shell_Object, "B2: close V3 close V5\r\n");
#endif
        }
      }
#define C2
      {
        if (ps->User.Ptank >= ps->Param.PPspf_start)
        {
          /*open V4*/
          Open_Vx(4U);
#if defined(USING_DEBUG_APPLICATION)
          shellPrint(Shell_Object, "C2: open V4\r\n");
#endif
        }
        else if (ps->User.Ptank <= ps->Param.PPspf_stop)
        {
          /*close V4*/
          Close_Vx(4U);
#if defined(USING_DEBUG_APPLICATION)
          shellPrint(Shell_Object, "C2: close V4\r\n");
#endif
        }
      }
#define D2
      {
        /*close V1*/
        Close_Vx(1U);
#if defined(USING_DEBUG_APPLICATION)
        shellPrint(Shell_Object, "D2: close V1\r\n");
#endif
      }
    }
  __no_action:
#if defined(USING_DEBUG_APPLICATION)
    for (uint16_t i = 0; i < VX_SIZE; i++)
    {
      // ret = mdRTU_WriteCoil(Slave1_Object, i, wbit[i]);
      shellPrint(Shell_Object, "wbit[%d] = 0x%x\r\n", i, wbit[i]);
    }
#endif
    // memset(wbit, 0x01, VX_SIZE);
    state ^= 1;
    for (uint8_t i = 0; i < VX_SIZE; i++)
    {
      wbit[i] = state;
    }
    ret = mdRTU_WriteCoils(Slave1_Object, OUT_DIGITAL_START_ADDR, VX_SIZE, wbit);
    if (ret == mdFALSE)
    {
#if defined(USING_DEBUG)
      shellPrint(Shell_Object, "write failed!\r\n");
#endif
    }
    /*Check whether the status of each switch changes*/
    // memcpy(copy_wbit, wbit, VX_SIZE);
    // if (Check_Vx_State(wbit, copy_wbit, VX_SIZE))
    // if (sp_irq->TableCount && record_type.p_id)
    // {
    //   record_type.count = 0;
    //   // uint8_t *q = record_type.p_id;
    //   // // /*Find target board*/
    //   // for (IRQ_Code *p = sp_irq->pIRQ; p < sp_irq->pIRQ + sp_irq->TableCount; p++)
    //   // {
    //   //   if (p->TypeCoding == Card_DigitalOutput)
    //   //   {
    //   //     if (record_type.count++ < TARGET_BOARD_NUM)
    //   //     {
    //   //       *q++ = p->SlaveId;
    //   //     }
    //   //   }
    //   // }
    //   if (Save_TargetSlave_Id(sp_irq, Card_DigitalOutput, &record_type))
    //   {
    //     /*Set data transmission target channel*/
    //     MASTER_OBJECT->uartId = 0x01;
    //     for (uint8_t num = 0; num < TARGET_BOARD_NUM; num++)
    //     {
    //       /*Integrated signal source*/
    //       uint8_t target_value = 0;
    //       for (uint8_t bit = 0; bit < CARD_SIGNAL_MAX; bit++)
    //       {
    //         target_value |= wbit[bit + num * CARD_SIGNAL_MAX] << bit;
    //         // target_value = 0xFF;
    //       }
    //       if (num < record_type.count)
    //       {
    //         mdRTU_Master_Codex(MASTER_OBJECT, MODBUS_CODE_15, record_type.p_id[num], &target_value, 0);
    //         if (xTaskNotify(MasterHandle, record_type.p_id[num], eSetValueWithoutOverwrite) == pdPASS)
    //         {
    //           osDelay(DELAY_TIMES);
    //         }
    //         // osDelay(1000);
    //       }
    //     }
    //   }
    // }
  __exit:
    osDelay(1000);
  }
  /* USER CODE END Control_Task */
}

/* USER CODE BEGIN Header_Transimt_Task */
/**
 * @brief Function implementing the Transimt thread.
 * @param argument: Not used
 * @retval None
 */
#define INCREMENT_COUNT()                                                           \
  do                                                                                \
  {                                                                                 \
    rp_irq->flag = false;                                                           \
    sp_irq->SiteCount = sp_irq->SiteCount ? rp_irq++, (sp_irq->SiteCount - 1U) : 0; \
  } while (0)

#define RESET_POINTER()      \
  do                         \
  {                          \
    rp_irq = sp_irq->pReIRQ; \
  } while (0)
// sp_irq->SiteCount = 0;
// tp_irq = sp_irq->pIRQ;
/* USER CODE END Header_Transimt_Task */
void Transimt_Task(void const *argument)
{
  /* USER CODE BEGIN Transimt_Task */
  // static uint16_t interrupt_record = 0xFFFF, first_interrupt;
  // static Slave_IRQTableTypeDef *sp_irq = &IRQ_Table;
  // static IRQ_Code *tp_irq = NULL;
  // tp_irq = sp_irq->pIRQ;
  // static IRQ_Request *rp_irq = NULL;
  // rp_irq = sp_irq->pReIRQ;
  uint16_t interrupt_record = 0xFFFF, first_interrupt;
  Slave_IRQTableTypeDef *sp_irq = &IRQ_Table;
  IRQ_Code *tp_irq = sp_irq->pIRQ;
  IRQ_Request *rp_irq = sp_irq->pReIRQ;
  uint8_t slave_id = 0x00;
  mdSTATUS ret = mdFALSE;
  // static mdBit wbit[] = {false, false, false, false, false};
  static mdBit wbit[VX_SIZE], copy_wbit[VX_SIZE];
  uint8_t target_type[TARGET_BOARD_NUM];
  R_TargetTypeDef record_type = {
      .count = 0,
      .p_id = target_type,
  };
  /* Infinite loop */
  for (;;)
  {
    /*Interrupt request count is not empty*/
    // if ((sp_irq->SiteCount) && (rp_irq < rp_irq + sp_irq->SiteCount))
    if ((sp_irq->SiteCount) && (rp_irq < sp_irq->pReIRQ + sp_irq->SiteCount))
    {
      /*Suspend interrupt processing task*/
      osThreadSuspend(InterruptHandle);
      /*Check if the interrupt source is valid*/
      if ((rp_irq->site) && (rp_irq->flag))
      {
        /*Get the slave ID according to the interrupt number*/
        // slave_id = rp_irq->site - 1U;
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
            tp_irq = Find_TargetSlave_AdoptId(sp_irq, slave_id);

            /*如果是sp_irq->SiteCount >1,则需要按优先级排序（�???????????????????个周期仅排序�???????????????????次），否则直接根据板卡类型处�???????????????????*/
            /*Interrupt priority sorting is performed when the board is read back*/
            // switch (sp_irq->pIRQ->TypeCoding)
            switch (tp_irq->TypeCoding)
            {
            case Card_AnalogInput:
            {
              mdRTU_Master_Codex(MASTER_OBJECT, MODBUS_CODE_4, tp_irq->SlaveId, NULL, 0);
            }
            break;
            case Card_DigitalInput:
            {
              mdRTU_Master_Codex(MASTER_OBJECT, MODBUS_CODE_2, tp_irq->SlaveId, NULL, 0);
            }
            break;
            /*Invalid board, remove interrupt record table*/
            default:
            {
              /*Read card type*/
              // MASTER_OBJECT->mdRTU_MasterCodex(MASTER_OBJECT, MODBUS_CODE_17, slave_id, NULL, 0);
              mdRTU_Master_Codex(MASTER_OBJECT, MODBUS_CODE_17, slave_id, NULL, 0);
              INCREMENT_COUNT();
            }
            break;
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
          // else
          // {
          //   tp_irq++;
          // }
          /*Clear the current interrupt request bit and reduce the number of remaining
           unprocessed interrupts*/
          // rp_irq->flag = false;
          // sp_irq->SiteCount = sp_irq->SiteCount ? (sp_irq->SiteCount - 1U) : 0;
          // rp_irq++;
          INCREMENT_COUNT();
        }
        /*no response from slave*/
        else
        {
          INCREMENT_COUNT();
          /*Read card type*/
          // MASTER_OBJECT->mdRTU_MasterCodex(MASTER_OBJECT, MODBUS_CODE_17, slave_id);
          // INCREMENT_COUNT();
#if defined(USING_DEBUG)
          shellPrint(Shell_Object, "Error: Interrupt Slave 0x%02x No Answer!"__INFORMATION(),
                     slave_id);
#endif
        }
        /*Task notification delay leads to decrease in recognition success rate*/
        osDelay(100);
      }
      else
      {
        /*Slave error handling*/
#if defined(USING_DEBUG)
        // shellPrint(Shell_Object, "Error: Invalid interrupt source: 0x%x."__INFORMATION(),
        //            sp_irq->pReIRQ[sp_irq->SiteCount].site);
        shellPrint(Shell_Object, "Error: Invalid interrupt source: 0x%x, table_site :0x%p\r\n.",
                   sp_irq->pReIRQ[sp_irq->SiteCount].site, rp_irq);
#endif
        // RESET_POINTER();
        INCREMENT_COUNT();
        // rp_irq->flag = true;
      }
      /*Resume interrupt processing task*/
      osThreadResume(InterruptHandle);
      osDelay(100);
    }
    /*One cycle processing completed*/
    else
    {
      // rp_irq = sp_irq->pReIRQ;
      // // sp_irq->pIRQ = sp_irq->pIRQ;
      // tp_irq = sp_irq->pIRQ;
      RESET_POINTER();

      ret = mdRTU_ReadCoils(Slave1_Object, OUT_DIGITAL_START_ADDR, VX_SIZE, wbit);
      if (ret == mdFALSE)
      {
#if defined(USING_DEBUG)
        shellPrint(Shell_Object, "read failed!\r\n");
#endif
      }
      else
      {
        // if (Check_Vx_State(wbit, copy_wbit, VX_SIZE) && sp_irq->TableCount && record_type.p_id)
        if (sp_irq->TableCount && record_type.p_id)
        {
          record_type.count = 0;
          if (Save_TargetSlave_Id(sp_irq, Card_DigitalOutput, &record_type))
          {
            /*Set data transmission target channel*/
            MASTER_OBJECT->uartId = 0x01;
            for (uint8_t num = 0; num < TARGET_BOARD_NUM; num++)
            {
              /*Integrated signal source*/
              uint8_t target_value = 0;
              for (uint8_t bit = 0; bit < CARD_SIGNAL_MAX; bit++)
              {
                target_value |= wbit[bit + num * CARD_SIGNAL_MAX] << bit;
                // target_value = 0xFF;
              }
              if (num < record_type.count)
              {
                mdRTU_Master_Codex(MASTER_OBJECT, MODBUS_CODE_15, record_type.p_id[num], &target_value, 0);
                if (xTaskNotify(MasterHandle, record_type.p_id[num], eSetValueWithoutOverwrite) == pdPASS)
                // if (xTaskNotify(MasterHandle, record_type.p_id[num], eSetValueWithOverwrite) == pdPASS)
                {
                  osDelay(100);
                }
              }
            }
          }
        }
        osDelay(500);
      }
    }
    // /*Resume interrupt processing task*/
    // osThreadResume(InterruptHandle);

    // osDelay(100);
  }
  /* USER CODE END Transimt_Task */
}

/* USER CODE BEGIN Header_Report_Task */
/**
 * @brief Function implementing the Treport thread.
 * @param argument: Not used
 * @retval None
 */
#define Get_Board_Icon(__type)                                 \
  ((__type) == Card_None ? 0x00 : (__type) >= Card_Wifi ? 0x05 \
                                                        : ((__type) / 0x10 + 1U))
/* USER CODE END Header_Report_Task */
void Report_Task(void const *argument)
{
  /* USER CODE BEGIN Report_Task */

  // mdU16 rbit = mdLow, wbit = mdLow;
  mdU16 at_state = mdLow;
  uint8_t bit = 0;
  // mdSTATUS ret = mdFALSE;
  uint16_t value = 0x0000;
  static bool first_flag = false;
  Save_HandleTypeDef *ps = &Save_Flash;
  Save_User urinfo;
  Slave_IRQTableTypeDef *p_irq = &IRQ_Table;
  // osEvent user_event = {
  //     .status = osOK,
  //     .value.p = &urinfo,
  // };
  /* Infinite loop */
  for (;;)
  {
    // #if defined(USING_FREERTOS)
    //   float *pdata = (float *)CUSTOM_MALLOC(sizeof(float) * ADC_DMA_CHANNEL);
    //   if (!pdata)
    //     goto __exit;
    // #else
    //   float pdata[ADC_DMA_CHANNEL];
    // #endif
    //   memset(pdata, 0x00, sizeof(float) * ADC_DMA_CHANNEL);

    // for (uint16_t i = 0; i < EXTERN_DIGITALIN_MAX; i++)
    // {
    //   mdRTU_ReadInputCoil(Slave1_Object, INPUT_DIGITAL_START_ADDR + i, bit);
    //   rbit |= bit << (i + 8U);
    //   mdRTU_ReadCoil(Slave1_Object, OUT_DIGITAL_START_ADDR + i, bit);
    //   wbit |= bit << (i + 8U);
    // }
    /*Read 4G module status*/
    // rbit |= Read_LTE_State();
#if defined(USING_DEBUG)
    // shellPrint(Shell_Object, "rbit = 0x%02x.\r\n", rbit);
#endif
    /*Digital input*/
    // Dwin_Object->Dw_Write(Dwin_Object, DIGITAL_INPUT_ADDR, (uint8_t *)&rbit, sizeof(rbit));
    // osDelay(DELAY_TIMES);
    /*Digital output*/
    // Dwin_Object->Dw_Write(Dwin_Object, DIGITAL_OUTPUT_ADDR, (uint8_t *)&wbit, sizeof(wbit));
    // osDelay(DELAY_TIMES);
    /*Report the status of at module*/
    at_state = Read_ATx_State() << 8U;
    Dwin_Object->Dw_Write(Dwin_Object, ATX_STATE_ADDR, (uint8_t *)&at_state, sizeof(at_state));
    osDelay(DELAY_TIMES);
    if (xQueueReceive(UserQueueHandle, (void *)&urinfo, osWaitForever) != pdPASS)
    // user_event = osMailGet(UserQueueHandle, osWaitForever);
    // if (user_event.status == osOK)
    {
#if defined(USING_DEBUG)
      shellPrint(Shell_Object, "Error: Failed to receive user parameters!\r\n");
#endif
    }
    for (float *puser = &urinfo.Ptank; puser < &urinfo.Ptank + BX_SIZE; puser++)
    {
#if defined(USING_DEBUG)
      // shellPrint(Shell_Object, "Value[%d] = %.3fMpa/M3\r\n", i, temp_data[i]);
#endif
      Endian_Swap((uint8_t *)puser, 0U, sizeof(float));
    }
    Dwin_Object->Dw_Write(Dwin_Object, PRESSURE_OUT_ADDR, (uint8_t *)&urinfo, BX_SIZE * sizeof(float));
    osDelay(DELAY_TIMES);

    if (p_irq->TableCount)
    {
#define BOARD_REPORT_SIZE (sizeof(uint16_t) * CARD_NUM_MAX)
#if defined(USING_FREERTOS)
      uint16_t *pBoard = (uint16_t *)CUSTOM_MALLOC(BOARD_REPORT_SIZE);
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
        osDelay(DELAY_TIMES);
#if defined(USING_FREERTOS)
      }
      CUSTOM_FREE(pBoard);
#endif
    }

    /*Analog input*/
    // mdRTU_ReadInputRegisters(Slave1_Object, INPUT_ANALOG_START_ADDR, ADC_DMA_CHANNEL * 2U, (mdU16 *)pdata);
    // Dwin_Object->Dw_Write(Dwin_Object, ANALOG_INPUT_ADDR, (uint8_t *)pdata, ADC_DMA_CHANNEL * sizeof(float));
    // osDelay(DELAY_TIMES);
    /*Analog output*/
    // mdRTU_ReadHoldRegisters(Slave1_Object, OUT_ANALOG_START_ADDR, EXTERN_ANALOGOUT_MAX * 2U, (mdU16 *)pdata);
    // Dwin_Object->Dw_Write(Dwin_Object, ANALOG_OUTPUT_ADDR, (uint8_t *)pdata, EXTERN_ANALOGOUT_MAX * sizeof(float));
    // osDelay(DELAY_TIMES);
    /*Report error code*/
//     if (ps->Param.Error_Code)
//     {
// #define ERROR_PAGE 0x10
// #define MAIN_PAGE 0x03
//       value = (uint16_t)ps->Param.Error_Code;
//       value = (value >> 8U) | (value << 8U);
//       Dwin_Object->Dw_Write(Dwin_Object, ERROR_CODE_ADDR, (uint8_t *)&value, sizeof(value));
//       value = 0x0100;
//       osDelay(5);
//       Dwin_Object->Dw_Page(Dwin_Object, ERROR_PAGE);
//       osDelay(5);
//       /*Open error animation*/
//       Dwin_Object->Dw_Write(Dwin_Object, ERROR_ANMATION, (uint8_t *)&value, sizeof(value));
//       first_flag = false;
//     }
//     else
//     {
//       if (!first_flag)
//       {
//         first_flag = true;
//         Dwin_Object->Dw_Page(Dwin_Object, MAIN_PAGE);
//       }
//     }
#if defined(USING_FREERTOS)
    // __exit:
    // CUSTOM_FREE(pdata);
#endif
    osDelay(500);
  }
  /* USER CODE END Report_Task */
}

/* Report_Callback function */
void Report_Callback(void const *argument)
{
  /* USER CODE BEGIN Report_Callback */
#if defined(USING_DEBUG)
  // shellPrint(Shell_Object, "Report_Callback !\r\n");
#endif
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

  /* NOTE: This function Should not be modified, when the callback is needed,
           the HAL_GPIO_EXTI_Callback could be implemented in the user file
   */
}

/* USER CODE END Application */
