/*
 * Dwin.c
 *
 *  Created on: 2022年3月25日
 *      Author: LHC
 */

#include "dwin.h"
// #include "tool.h"
#include "usart.h"
#include "shell_port.h"
#include "Flash.h"
#include "mdrtuslave.h"
// #if defined(USING_FREERTOS)
// #include "cmsis_os.h"
// #endif

/*定义迪文屏幕对象*/
pDwinHandle Dwin_Object;

extern void Report_Backparam(pDwinHandle pd, Save_Param *sp);

static void Dwin_Send(pDwinHandle pd);
static void Dwin_Write(pDwinHandle pd, uint16_t start_addr, uint8_t *dat, uint16_t len);
static void Dwin_Read(pDwinHandle pd, uint16_t start_addr, uint8_t words);
static void Dwin_PageChange(pDwinHandle pd, uint16_t page);
static void Dwin_Poll(pDwinHandle pd);
// void Dwin_Poll(pDwinHandle pd);
static void Dwin_ErrorHandle(pDwinHandle pd, uint8_t error_code, uint8_t site);
static void Dwin_EventHandle(pDwinHandle pd, uint8_t *pSite);
static void Restore_Factory(pDwinHandle pd, uint8_t *pSite);
static void Password_Handle(pDwinHandle pd, uint8_t *pSite);
static void Card_Handle(pDwinHandle pd, uint8_t *pSite);
static void Reset_Card_Info(pDwinHandle pd, uint8_t *pSite);
static void Sure_Card_Info(pDwinHandle pd, uint8_t *pSite);

/*迪文响应线程*/
DwinMap Dwin_ObjMap[] = {
	/*系统传感器上下限设置区*/
	{.addr = PTANK_MAX_ADDR, .upper = 4.0F, .lower = 0, .event = Dwin_EventHandle},
	{.addr = PTANK_MIN_ADDR, .upper = 4.0F, .lower = 0, .event = Dwin_EventHandle},
	{.addr = PVAOUT_MAX_ADDR, .upper = 4.0F, .lower = 0, .event = Dwin_EventHandle},
	{.addr = PVAOUT_MIN_ADDR, .upper = 4.0F, .lower = 0, .event = Dwin_EventHandle},
	{.addr = PGAS_MAX_ADDR, .upper = 4.0F, .lower = 0, .event = Dwin_EventHandle},
	{.addr = PGAS_MIN_ADDR, .upper = 4.0F, .lower = 0, .event = Dwin_EventHandle},
	{.addr = LTANK_MAX_ADDR, .upper = 20.0F, .lower = 0, .event = Dwin_EventHandle},
	{.addr = LTANK_MIN_ADDR, .upper = 5.0F, .lower = 0, .event = Dwin_EventHandle},
	{.addr = PTOLE_MAX_ADDR, .upper = 5000.0F, .lower = 0, .event = Dwin_EventHandle},
	{.addr = PTOLE_MIN_ADDR, .upper = 2000.0F, .lower = 0, .event = Dwin_EventHandle},
	{.addr = LEVEL_MAX_ADDR, .upper = 2.0F, .lower = 0, .event = Dwin_EventHandle},
	{.addr = LEVEL_MIN_ADDR, .upper = 2.0F, .lower = 0, .event = Dwin_EventHandle},
	/*系统控制参数区*/
	{.addr = SPSFS_MAX_ADDR, .upper = 4.0F, .lower = 0, .event = Dwin_EventHandle},
	{.addr = SPSFS_MIN_ADDR, .upper = 4.0F, .lower = 0, .event = Dwin_EventHandle},
	{.addr = PSVA_START_ADDR, .upper = 4.0F, .lower = 0, .event = Dwin_EventHandle},
	{.addr = PSVAP_STOP_ADDR, .upper = 4.0F, .lower = 0, .event = Dwin_EventHandle},
	{.addr = PBACK_MAX_ADDR, .upper = 4.0F, .lower = 0, .event = Dwin_EventHandle},
	{.addr = PBACK_MIN_ADDR, .upper = 4.0F, .lower = 0, .event = Dwin_EventHandle},
	{.addr = PPVAP_START_ADDR, .upper = 4.0F, .lower = 0, .event = Dwin_EventHandle},
	{.addr = PPVAP_STOP_ADDR, .upper = 4.0F, .lower = 0, .event = Dwin_EventHandle},
	{.addr = SPSFE_MAX_ADDR, .upper = 4.0F, .lower = 0, .event = Dwin_EventHandle},
	{.addr = SPSFE_MIN_ADDR, .upper = 4.0F, .lower = 0, .event = Dwin_EventHandle},
	{.addr = PTANK_LIMIT_ADDR, .upper = 2.0F, .lower = 0, .event = Dwin_EventHandle},
	{.addr = LTANK_LIMIT_ADDR, .upper = 200.0F, .lower = 0, .event = Dwin_EventHandle},
	{.addr = TANK_HEAD_HEIGHT, .upper = 5.0F, .lower = 0, .event = Dwin_EventHandle},
	{.addr = TANK_CY_RADIUS, .upper = 5.0F, .lower = 0.1, .event = Dwin_EventHandle},
	{.addr = TANK_CY_LENGTH, .upper = 100.0F, .lower = 0, .event = Dwin_EventHandle},
	{.addr = TANK_FL_DENSITY, .upper = 50.0F, .lower = 0, .event = Dwin_EventHandle},
	/*系统按钮设置地址*/
	{.addr = RESTORE_ADDR, .upper = 0xFFFF, .lower = 0, .event = Restore_Factory},
	{.addr = USER_NAME_ADDR, .upper = 9999, .lower = 0, .event = Password_Handle},
	{.addr = USER_PASSWORD_ADDR, .upper = 9999, .lower = 0, .event = Password_Handle},
	{.addr = LOGIN_SURE_ADDR, .upper = 0xFFFF, .lower = 0, .event = Password_Handle},
	{.addr = CANCEL_ADDR, .upper = 0xFFFF, .lower = 0, .event = Password_Handle},
	/*卡槽事件处理*/
	{.addr = CARD_SLOT1_ADDR, .upper = 0xFFFF, .lower = 0, .event = Card_Handle},
	{.addr = CARD_SLOT2_ADDR, .upper = 0xFFFF, .lower = 0, .event = Card_Handle},
	{.addr = CARD_SLOT3_ADDR, .upper = 0xFFFF, .lower = 0, .event = Card_Handle},
	{.addr = CARD_SLOT4_ADDR, .upper = 0xFFFF, .lower = 0, .event = Card_Handle},
	{.addr = CARD_SLOT5_ADDR, .upper = 0xFFFF, .lower = 0, .event = Card_Handle},
	{.addr = CARD_SLOT6_ADDR, .upper = 0xFFFF, .lower = 0, .event = Card_Handle},
	{.addr = CARD_SLOT7_ADDR, .upper = 0xFFFF, .lower = 0, .event = Card_Handle},
	{.addr = CARD_SLOT8_ADDR, .upper = 0xFFFF, .lower = 0, .event = Card_Handle},
	{.addr = CARD_SLOT9_ADDR, .upper = 0xFFFF, .lower = 0, .event = Card_Handle},
	{.addr = CARD_SLOT10_ADDR, .upper = 0xFFFF, .lower = 0, .event = Card_Handle},
	{.addr = CARD_SLOT11_ADDR, .upper = 0xFFFF, .lower = 0, .event = Card_Handle},
	{.addr = CARD_SLOT12_ADDR, .upper = 0xFFFF, .lower = 0, .event = Card_Handle},
	{.addr = CARD_SLOT13_ADDR, .upper = 0xFFFF, .lower = 0, .event = Card_Handle},
	{.addr = CARD_SLOT14_ADDR, .upper = 0xFFFF, .lower = 0, .event = Card_Handle},
	{.addr = CARD_SLOT15_ADDR, .upper = 0xFFFF, .lower = 0, .event = Card_Handle},
	{.addr = CARD_SLOT16_ADDR, .upper = 0xFFFF, .lower = 0, .event = Card_Handle},
	/*气站工作时间段和阀门工作时间设置*/
	/*复位板卡组态信息*/
	{.addr = RESET_CARD_INFO_ADDR, .upper = 0xFFFF, .lower = 0, .event = Reset_Card_Info},
	{.addr = SURE_CARD_INFO_ADDR, .upper = 0xFFFF, .lower = 0, .event = Sure_Card_Info},
};

#define Dwin_EventSize (sizeof(Dwin_ObjMap) / sizeof(DwinMap))

/**
 * @brief  创建迪文屏幕对象
 * @param  pd 需要初始化对象指针
 * @param  ps 初始化数据指针
 * @retval None
 */
static void Create_DwinObject(pDwinHandle *pd, pDwinHandle ps)
{
	if (!ps)
		return;
#if defined(USING_FREERTOS)
	(*pd) = (pDwinHandle)CUSTOM_MALLOC(sizeof(DwinHandle));
	if (!(*pd))
	{
		CUSTOM_FREE(*pd);
		return;
	}
	uint8_t *pTxbuf = (uint8_t *)CUSTOM_MALLOC(ps->Master.TxSize);
	if (!pTxbuf)
	{
		CUSTOM_FREE(pTxbuf);
		return;
	}
	// uint8_t *pRxbuf = (uint8_t *)CUSTOM_MALLOC(ps->Slave.RxSize);
	uint8_t *pRxbuf = (uint8_t *)CUSTOM_MALLOC(ps->Uart.RxSize);
	if (!pRxbuf)
	{
		CUSTOM_FREE(pRxbuf);
		return;
	}
#else
	uint8_t pTxbuf[ps->Master.TxSize];
	uint8_t pRxbuf[ps->Slave.RxSize];
#endif
	memset(pTxbuf, 0x00, ps->Master.TxSize);
	// memset(pRxbuf, 0x00, ps->Slave.RxSize);
	memset(pRxbuf, 0x00, ps->Uart.RxSize);
#if defined(USING_DEBUG)
	shellPrint(Shell_Object, "Dwin[%d]_handler = 0x%p\r\n", ps->Id, *pd);
#endif

	(*pd)->Id = ps->Id;
	(*pd)->Dw_Transmit = Dwin_Send;
	(*pd)->Dw_Write = Dwin_Write;
	(*pd)->Dw_Read = Dwin_Read;
	(*pd)->Dw_Page = Dwin_PageChange;
	(*pd)->Dw_Poll = Dwin_Poll;
	(*pd)->Dw_Error = Dwin_ErrorHandle;
	(*pd)->Dw_Delay = (void (*)(uint32_t))osDelay;
	(*pd)->Master.pTbuf = pTxbuf;
	(*pd)->Master.TxCount = 0U;
	(*pd)->Master.TxSize = ps->Master.TxSize;
	// (*pd)->Slave.pRbuf = pRxbuf;
	// (*pd)->Slave.RxSize = ps->Slave.RxSize;
	// (*pd)->Slave.RxCount = 0U;
	// (*pd)->pUart = ps->pUart;
	// (*pd)->pUart->huart = ps->pUart->huart;
	// (*pd)->pUart->phdma = ps->pUart->phdma;
	// (*pd)->pUart->Semaphore = ps->pUart->phdma;
	// (*pd)->pUart->pRbuf = pRxbuf;
	// (*pd)->pUart->RxSize = ps->pUart->RxSize;
	// (*pd)->pUart->RxCount = 0U;

	if (!ps->Uart.pRbuf)
	{
		ps->Uart.pRbuf = pRxbuf;
	}
	memcpy(&(*pd)->Uart, &ps->Uart, sizeof(DmaHandle));
	(*pd)->Slave.pMap = ps->Slave.pMap;
	(*pd)->Slave.Events_Size = ps->Slave.Events_Size;
	// (*pd)->Slave.pMap->pHandle = ps->Slave.pMap->pHandle;
	(*pd)->Slave.pHandle = ps->Slave.pHandle;
	(*pd)->Slave.pHandle1 = ps->Slave.pHandle1;
	(*pd)->Slave.pHandle2 = ps->Slave.pHandle2;
}

/**
 * @brief  销毁迪文屏幕对象
 * @param  pd 需要初始化对象指针
 * @retval None
 */
void Free_DwinObject(pDwinHandle *pd)
{
	if (*pd)
	{
		CUSTOM_FREE((*pd)->Master.pTbuf);
		// CUSTOM_FREE((*pd)->Slave.pRbuf);
		CUSTOM_FREE((*pd)->Uart.pRbuf);
		CUSTOM_FREE((*pd));
	}
}

/**
 * @brief  带CRC的发送数据帧
 * @param  _pBuf 数据缓冲区指针
 * @param  _ucLen 数据长度
 * @retval None
 */
void MX_DwinInit()
{
	// extern Save_HandleTypeDef Save_Flash;
	// sizeof(DwinHandle);
	DwinHandle Dwin;
	static uint32_t dwin_rx_count = 0U;
	extern DMA_HandleTypeDef hdma_uart4_rx;
	extern osSemaphoreId Recive_Rs232Handle;
	DmaHandle Dma_Param = {
		.huart = &huart4,
		.phdma = &hdma_uart4_rx,
		.pRbuf = NULL,
		.RxSize = RX_BUF_SIZE,
		.pRxCount = &dwin_rx_count,
#if defined(USING_FREERTOS)
		.Semaphore = Recive_Rs232Handle,
#else
		.pRecive_FinishFlag = NULL,
#endif
	};

	Dwin.Id = 0x00;
	Dwin.Master.TxSize = TX_BUF_SIZE;
	// Dwin.pUart->RxSize = RX_BUF_SIZE;
	Dwin.Slave.pMap = Dwin_ObjMap;
	Dwin.Slave.Events_Size = Dwin_EventSize;
	Dwin.Uart = Dma_Param;
	Dwin.Slave.pHandle = &Save_Flash;
	Dwin.Slave.pHandle1 = &IRQ_Table;
	Dwin.Slave.pHandle2 = Slave1_Object;
	/*定义迪文屏幕使用目标串口*/
	// Dwin.huart = &huart1;
	// Dwin.pUart->huart = &huart1;
/*使用屏幕接收处理时*/
#define TYPEDEF_STRUCT float
#define TYPEDEF_STRUCT1 Slave_IRQTableTypeDef
#define TYPEDEF_STRUCT2 ModbusRTUSlaveHandler
#define TYPEDEF_STRUCT3 Save_HandleTypeDef
	Create_DwinObject(&Dwin_Object, &Dwin);
}

/**
 * @brief  带CRC的发送数据帧
 * @param  _pBuf 数据缓冲区指针
 * @param  _ucLen 数据长度
 * @retval None
 */
static void Dwin_Send(pDwinHandle pd)
{
#if (USING_CRC == 1U)
	uint16_t crc16 = Get_Crc16(&pd->Master.pTbuf[3U], pd->Master.TxCount - 3U, 0xffff);

	memcpy(&pd->Master.pTbuf[pd->Master.TxCount], (uint8_t *)&crc16, sizeof(crc16));
	pd->Master.TxCount += sizeof(crc16);
#endif

#if defined(USING_DMA)
#if defined(USING_FREERTOS)
	// uint8_t *pbuf = (uint8_t *)CUSTOM_MALLOC(pd->Master.TxCount);
	// if (!pbuf)
	// 	CUSTOM_FREE(pbuf);
#endif
	// memset(pbuf, 0x00, pd->Master.TxCount);
	// memcpy(pbuf, pd->Master.pTbuf, pd->Master.TxCount);
	HAL_UART_Transmit_DMA(pd->Uart.huart, pd->Master.pTbuf, pd->Master.TxCount);
	/*https://blog.csdn.net/mickey35/article/details/80186124*/
	/*https://blog.csdn.net/qq_40452910/article/details/80022619*/
	while (__HAL_UART_GET_FLAG(pd->Uart.huart, UART_FLAG_TC) == RESET)
	{
		// osDelay(1);
	}
#else
	HAL_UART_Transmit(pd->huart, pd->Master.pTbuf, pd->Master.TxCount, 0xffff);
#endif
#if defined(USING_FREERTOS)
	// CUSTOM_FREE(pbuf);
#endif
}

/**
 * @brief  写数据变量到指定地址并显示
 * @param  pd 迪文屏幕对象句柄
 * @param  start_addr 开始地址
 * @param  dat 指向数据的指针
 * @param  length 数据长度
 * @retval None
 */
static void Dwin_Write(pDwinHandle pd, uint16_t start_addr, uint8_t *pdat, uint16_t len)
{
#if (USING_CRC)
	uint8_t buf[] = {
		0x5A, 0xA5, len + 3U + 2U, WRITE_CMD, start_addr >> 8U,
		start_addr};
#else
	uint8_t buf[] = {
		0x5A, 0xA5, len + 3U, WRITE_CMD, start_addr >> 8U,
		start_addr};
#endif
	pd->Master.TxCount = 0U;
	memcpy(pd->Master.pTbuf, buf, sizeof(buf));
	pd->Master.TxCount += sizeof(buf);
	memcpy(&pd->Master.pTbuf[pd->Master.TxCount], pdat, len);
	pd->Master.TxCount += len;
#if defined(USING_DEBUG)
	// shellPrint(Shell_Object, "pd = %p, pd->Master.TxCount = %d.\r\n", pd, pd->Master.TxCount);
	// shellPrint(Shell_Object, "pd->Master.pTbuf = %s.\r\n", pd->Master.pTbuf);
#endif

	pd->Dw_Transmit(pd);
}

/**
 * @brief  读出指定地址指定长度数据
 * @param  pd 迪文屏幕对象句柄
 * @param  start_addr 开始地址
 * @param  dat 指向数据的指针
 * @param  length 数据长度
 * @retval None
 */
static void Dwin_Read(pDwinHandle pd, uint16_t start_addr, uint8_t words)
{
#if (USING_CRC)
	uint8_t buf[] = {
		0x5A, 0xA5, 0x04 + 2U, READ_CMD, start_addr >> 8U,
		words};
#else
	uint8_t buf[] = {
		0x5A, 0xA5, 0x04, READ_CMD, start_addr >> 8U,
		words};
#endif
	pd->Master.TxCount = 0U;
	memcpy(pd->Master.pTbuf, buf, sizeof(buf));
	pd->Master.TxCount += sizeof(buf);

	// Dwin_Send(pd);
	pd->Dw_Transmit(pd);
}

/**
 * @brief  迪文屏幕指定页面切换
 * @param  pd 迪文屏幕对象句柄
 * @param  page 目标页面
 * @retval None
 */
static void Dwin_PageChange(pDwinHandle pd, uint16_t page)
{
#if (USING_CRC)
	uint8_t buf[] = {
		0x5A, 0xA5, 0x07 + 2U, WRITE_CMD, 0x00, 0x84, 0x5A, 0x01,
		page >> 8U, page};
#else
	uint8_t buf[] = {
		0x5A, 0xA5, 0x07, WRITE_CMD, 0x00, 0x84, 0x5A, 0x01,
		page >> 8U, page};
#endif
	pd->Master.TxCount = 0U;
	memcpy(pd->Master.pTbuf, buf, sizeof(buf));
	pd->Master.TxCount += sizeof(buf);

	pd->Dw_Transmit(pd);
}

/*83指令返回数据以一个字为基础*/
#define DW_WORD 1U
#define DW_DWORD 2U

/*获取迪文屏幕地址*/
// #define Get_Addr(__ptr, __s1, __s2, __size) \
// 	((__size) > 1U ? (((__ptr)->Slave.pRbuf[__s1] << 8U) | ((__ptr)->Slave.pRbuf[__s2])) : ((__ptr)->Slave.pRbuf[__s1]))
/*获取迪文屏幕数据*/
#define Get_Data(__ptr, __s, __size) \
	((__size) < 2U ? (((__ptr)->Uart.pRbuf[__s] << 8U) | ((__ptr)->Uart.pRbuf[__s + 1U])) : (((__ptr)->Uart.pRbuf[__s] << 24U) | ((__ptr)->Uart.pRbuf[__s + 1U] << 16U) | ((__ptr)->Uart.pRbuf[__s + 2U] << 8U) | ((__ptr)->Uart.pRbuf[__s + 3U])))

/**
 * @brief  迪文屏幕接收数据解析
 * @param  pd 迪文屏幕对象句柄
 * @retval None
 */
static void Dwin_Poll(pDwinHandle pd)
// void Dwin_Poll(pDwinHandle pd)
{ 
#define DWIN_MIN_FRAME_LEN 5U // 3个前导码+2个crc16
	/*检查帧头是否符合要求*/
	if ((pd->Uart.pRbuf[0] == 0x5A) && (pd->Uart.pRbuf[1] == 0xA5))
	{
		uint16_t addr = Get_Data(pd, 4U, DW_WORD);
		if (*pd->Uart.pRxCount < DWIN_MIN_FRAME_LEN)
			return;
		/*检查CRC是否正确*/
		uint16_t crc16 = Get_Crc16(&pd->Uart.pRbuf[3U], *pd->Uart.pRxCount - 5U, 0xFFFF);
		crc16 = (crc16 >> 8U) | (crc16 << 8U);
#if defined(USING_DEBUG)
		// shellPrint(Shell_Object, "addr = 0x%x\r\n", addr);
#endif
		if (crc16 == Get_Data(pd, *pd->Uart.pRxCount - 2U, DW_WORD))
		{
			for (uint8_t i = 0; i < pd->Slave.Events_Size; i++)
			{
				if (pd->Slave.pMap[i].addr == addr)
				{
					if (pd->Slave.pMap[i].event)
						pd->Slave.pMap[i].event(pd, &i);
					break;
				}
			}
		}
	}
#if defined(USING_DEBUG)
	// for (uint16_t i = 0; i < *pd->Uart.pRxCount; i++)
	// 	shellPrint(Shell_Object, "pRbuf[%d] = 0x%x\r\n", i, pd->Uart.pRbuf[i]);
#endif
	memset(pd->Uart.pRbuf, 0x00, *pd->Uart.pRxCount);
	*pd->Uart.pRxCount = 0U;
	#undef DWIN_MIN_FRAME_LEN
}

/**
 * @brief  迪文屏幕接收数据处理
 * @param  pd 迪文屏幕对象句柄
 * @param  pSite 记录当前Map中位置
 * @retval None
 */
static void Dwin_EventHandle(pDwinHandle pd, uint8_t *pSite)
{
	TYPEDEF_STRUCT data = (TYPEDEF_STRUCT)Get_Data(pd, 7U, pd->Uart.pRbuf[6U]) / 10.0F;
	// TYPEDEF_STRUCT *pdata = (TYPEDEF_STRUCT *)pd->Slave.pMap[*pSite].pHandle;
	// TYPEDEF_STRUCT *pdata = (TYPEDEF_STRUCT *)pd->Slave.pHandle;
	// Save_HandleTypeDef *ps = &Save_Flash;
	TYPEDEF_STRUCT3 *ps = (TYPEDEF_STRUCT3 *)pd->Slave.pHandle;
	TYPEDEF_STRUCT *pdata = (TYPEDEF_STRUCT *)&ps->Param;

#if defined(USING_DEBUG)
	shellPrint(Shell_Object, "data = %.3f, *psite = %d.\r\n", data, *pSite);
#endif
	if ((data >= pd->Slave.pMap[*pSite].lower) && (data <= pd->Slave.pMap[*pSite].upper))
	{
		if ((pdata) && (*pSite < pd->Slave.Events_Size))
			pdata[*pSite] = data;
		// #if defined(USING_FREERTOS)
		// 		taskENTER_CRITICAL();
		// #endif
		// 		/*参数保存到Flash*/
		// 		FLASH_Write(PARAM_SAVE_ADDRESS, (uint32_t *)&Save_Flash.Param, sizeof(Save_Param));
		// #if defined(USING_FREERTOS)
		// 		taskEXIT_CRITICAL();
		// #endif
		Endian_Swap((uint8_t *)&data, 0U, sizeof(TYPEDEF_STRUCT));
		/*确认数据回传到屏幕*/
		pd->Dw_Write(pd, pd->Slave.pMap[*pSite].addr, (uint8_t *)&data, sizeof(TYPEDEF_STRUCT));
#if defined(USING_DEBUG)
//		shellPrint(Shell_Object, "pdata[%d] = %.3f,Ptank_max = %.3f.\r\n", *pSite, pdata[*pSite], Save_Flash.Param.Ptank_max);
#endif
	}
	else
	{
#define BELOW_LOWER_LIMIT 1U
#define ABOVE_UPPER_LIMIT 2U

		uint8_t error = data < pd->Slave.pMap[*pSite].lower ? BELOW_LOWER_LIMIT : ABOVE_UPPER_LIMIT;
		/*上下限保存到控制参数区*/
		pdata[*pSite] = (error == BELOW_LOWER_LIMIT) ? pd->Slave.pMap[*pSite].lower : pd->Slave.pMap[*pSite].upper;
		/*屏幕传回参数越界处理：维持原值不变、或者切换报错页面*/
		pd->Dw_Error(pd, error, *pSite);
	}
	/*数据同时更新到本地保持寄存器*/
	mdSTATUS ret = mdRTU_WriteHoldRegs(Slave1_Object, PARAM_BASE_ADDR, GET_PARAM_SITE(Save_Param, User_Name, uint16_t),
									   (mdU16 *)&ps->Param);
	if (ret == mdFALSE)
	{
#if defined(USING_DEBUG)
		shellPrint(Shell_Object, "Parameter write to hold register failed!\r\n");
#endif
	}
/*设置错误或者正确都将保存有效参数*/
#if defined(USING_FREERTOS)
	taskENTER_CRITICAL();
#endif
	/*计算crc校验码*/
	ps->Param.crc16 = Get_Crc16((uint8_t *)&ps->Param, sizeof(Save_Param) - sizeof(ps->Param.crc16), 0xFFFF);
	/*参数保存到Flash*/
	FLASH_Write(PARAM_SAVE_ADDRESS, (uint32_t *)&ps->Param, sizeof(Save_Param));
#if defined(USING_FREERTOS)
	taskEXIT_CRITICAL();
#endif
}

/**
 * @brief  迪文屏幕错误处理
 * @param  pd 迪文屏幕对象句柄
 * @param  error_code 错误代码
 * @param  site 当前对象出错位置
 * @retval None
 */
static void Dwin_ErrorHandle(pDwinHandle pd, uint8_t error_code, uint8_t site)
{
#define ERROR_NOTE_PAGE 30U
	TYPEDEF_STRUCT tdata = (error_code == BELOW_LOWER_LIMIT) ? pd->Slave.pMap[site].lower : pd->Slave.pMap[site].upper;
	uint16_t tarry[] = {0, 0, 0};
#if defined(USING_DEBUG)
	if (error_code == BELOW_LOWER_LIMIT)
	{
		shellPrint(Shell_Object, "Error: Below lower limit %.3f.\r\n", tdata);
	}
	else
	{
		tarry[0] = 0x0100;
		shellPrint(Shell_Object, "Error: Above upper limit %.3f.\r\n", tdata);
	}
#endif
	Endian_Swap((uint8_t *)&tdata, 0U, sizeof(TYPEDEF_STRUCT));
	/*设置错误时将显示上下限*/
	pd->Dw_Write(pd, pd->Slave.pMap[site].addr, (uint8_t *)&tdata, sizeof(TYPEDEF_STRUCT));
	pd->Dw_Delay(NEXT_DELAT_TIMES);
	/*切换到提示页面*/
	pd->Dw_Page(pd, ERROR_NOTE_PAGE);
	pd->Dw_Delay(NEXT_DELAT_TIMES);
	memcpy(&tarry[1], (void *)&tdata, sizeof(tdata));
	pd->Dw_Write(pd, NOTE_PAGE_ADDR, (uint8_t *)&tarry, sizeof(tarry));
}

/**
 * @brief  系统参数恢复出厂设置
 * @param  pd 迪文屏幕对象句柄
 * @param  pSite 记录当前Map中位置
 * @retval None
 */
static void Restore_Factory(pDwinHandle pd, uint8_t *pSite)
{
	uint16_t rcode = Get_Data(pd, 7U, pd->Uart.pRbuf[6U]);
	// Save_HandleTypeDef *ps = &Save_Flash;
	TYPEDEF_STRUCT3 *ps = (TYPEDEF_STRUCT3 *)pd->Slave.pHandle;
	if (rcode == RSURE_CODE)
	{
		memcpy(&ps->Param, &Save_InitPara, sizeof(Save_Param));
#if defined(USING_FREERTOS)
		taskENTER_CRITICAL();
#endif
		//		FLASH_Write(PARAM_SAVE_ADDRESS, (uint32_t *)&Save_InitPara, sizeof(Save_Param));
		/*计算crc校验码*/
		ps->Param.crc16 = Get_Crc16((uint8_t *)&ps->Param, sizeof(Save_Param) - sizeof(ps->Param.crc16), 0xFFFF);
		FLASH_Write(PARAM_SAVE_ADDRESS, (uint32_t *)&ps->Param, sizeof(Save_Param));
#if defined(USING_FREERTOS)
		taskEXIT_CRITICAL();
#endif
		// extern void Param_WriteBack(Save_HandleTypeDef * ps);
		// /*更新屏幕同时，更新远程参数*/
		// Param_WriteBack(ps);
		Report_Backparam(Dwin_Object, &ps->Param);
#if defined(USING_DEBUG)
		shellPrint(Shell_Object, "success: Factory settings restored successfully!\r\n");
#endif
	}
}

/**
 * @brief  密码处理
 * @param  pd 迪文屏幕对象句柄
 * @param  pSite 记录当前Map中位置
 * @retval None
 */
static void Password_Handle(pDwinHandle pd, uint8_t *pSite)
{
// #define USER_NAMES 0x07E6
// #define USER_PASSWORD 0x0522
#define PAGE_NUMBER 0x016
	TYPEDEF_STRUCT3 *ps = (TYPEDEF_STRUCT3 *)pd->Slave.pHandle;
	uint16_t data = Get_Data(pd, 7U, pd->Uart.pRbuf[6U]);
	uint16_t addr = pd->Slave.pMap[*pSite].addr;
	static uint16_t user_name = 0x0000, user_code = 0x0000, error = 0x0000;

	if ((data >= pd->Slave.pMap[*pSite].lower) && (data <= pd->Slave.pMap[*pSite].upper))
	{
		addr == USER_NAME_ADDR ? user_name = data : (addr == USER_PASSWORD_ADDR ? user_code = data : 0U);
		if ((addr == LOGIN_SURE_ADDR) && (data == RSURE_CODE))
		{ /*密码用户名正确*/
			if ((user_name == ps->Param.User_Name) && (user_code == ps->Param.User_Code))
			{ /*清除错误信息*/
				error = 0x0000;
				pd->Dw_Page(pd, PAGE_NUMBER);
#if defined(USING_DEBUG)
				shellPrint(Shell_Object, "success: The password is correct!\r\n");
#endif
			}
			else
			{
				/*用户名、密码错误*/
				if ((user_name != ps->Param.User_Name) && (user_code != ps->Param.User_Code))
				{
					error = 0x0300;
#if defined(USING_DEBUG)
					shellPrint(Shell_Object, "error: Wrong user name and password!\r\n");
#endif
				}
				/*用户名错误*/
				else if (user_name != ps->Param.User_Name)
				{
					error = 0x0100;
#if defined(USING_DEBUG)
					shellPrint(Shell_Object, "error: User name error!\r\n");
#endif
				}
				/*密码错误*/
				else
				{
					error = 0x0200;

#if defined(USING_DEBUG)
					shellPrint(Shell_Object, "error: User password error!\r\n");
#endif
				}
			}
		}
		if ((addr == CANCEL_ADDR) && (data == RCANCEL_CODE))
		{
			error = 0x0000;
			user_name = user_code = 0x0000;
			uint32_t temp_value = 0x0000;
			pd->Dw_Write(pd, USER_NAME_ADDR, (uint8_t *)&temp_value, sizeof(temp_value));
#if defined(USING_DEBUG)
			shellPrint(Shell_Object, "success: Clear Error Icon!\r\n");
#endif
		}
		pd->Dw_Write(pd, ERROR_NOTE_ADDR, (uint8_t *)&error, sizeof(error));
	}

#if defined(USING_DEBUG)
	shellPrint(Shell_Object, "data = %d,user_name = %d, user_code = %d\r\n", data, user_name, user_code);
#endif
}

/**
 * @brief  板卡事件处理
 * @param  pd 迪文屏幕对象句柄
 * @param  pSite 记录当前Map中位置
 * @retval None
 */
static void Card_Handle(pDwinHandle pd, uint8_t *pSite)
{
#define Get_Change_Page(__type)                             \
	((__type) == Card_AnalogInput	  ? ANALOG_INPUT_PAGE   \
	 : (__type) == Card_AnalogOutput  ? ANALOG_OUTPUT_PAGE  \
	 : (__type) == Card_DigitalInput  ? DIGITAL_INPUT_PAGE  \
	 : (__type) == Card_DigitalOutput ? DIFITAL_OUTPUT_PAGE \
	 : (__type) > Card_Lora2		  ? NONE_PAGE           \
									  : COMMUNICATION_PAGE)
#define Get_Board_Type(__pd) \
	(!(__pd)->Uart.pRbuf[8U] \
		 ? Card_None         \
		 : ((uint16_t)(__pd)->Uart.pRbuf[7U] << 8U | (__pd)->Uart.pRbuf[8U] - 1U) * 0x10)

	TYPEDEF_STRUCT1 *sp_irq = (TYPEDEF_STRUCT1 *)pd->Slave.pHandle1;
	TYPEDEF_STRUCT2 pSlave = (TYPEDEF_STRUCT2)pd->Slave.pHandle2;
	uint8_t card_site = (uint8_t)(pd->Slave.pMap[*pSite].addr - CARD_SLOT1_ADDR), *pDest = NULL, rSize = 0;
	// uint8_t card_used = 0x00;
	Card_Tyte card_type = (Card_Tyte)Get_Board_Type(pd);
	uint16_t report_addr, read_addr;
	mdSTATUS ret = mdFALSE;
	// float analog_data[CARD_SIGNAL_MAX];
	// mdBit digital_data[CARD_SIGNAL_MAX * 4U];
	mdBit temp_data[(CARD_COMM_OFFSET_MAX + 0x20) * 2U];
	// mdU16 digital = 0x0000;
	mdU16 digital[] = {0, 0, 0, 0, 0, 0, 0, 0};
	IRQ_Code *p_target = NULL;

	memset(temp_data, 0x00, sizeof(temp_data));
#if defined(USING_DEBUG)
	shellPrint(Shell_Object, "card_site = %d, card_addr = 0x%x.\r\n", card_site, pd->Slave.pMap[*pSite].addr);
#endif
	/*查找匹配从机:确认当前是同类型板卡中第几张*/
	p_target = Find_TargetSlave_AdoptId(sp_irq, card_site);
	if (p_target)
	{
		if (p_target->Number)
		{
#if defined(USING_DEBUG)
			shellPrint(Shell_Object, "The number of boards of current type is:%d.\r\n", p_target->Number);
#endif
		}
		/*交换高低字节*/
		uint16_t count = (uint16_t)(p_target->Number >> 8U) | (p_target->Number << 8U);
		pd->Dw_Write(pd, CARD_SAMETYPE_DIFF_ADDR, (uint8_t *)&count, sizeof(p_target->Number));
		// HAL_Delay(50);
		// osDelay(NEXT_DELAT_TIMES);
		pd->Dw_Delay(NEXT_DELAT_TIMES);
	}

	if (sp_irq && (sp_irq->TableCount) && (card_site < CARD_NUM_MAX))
	{
		/*目标板卡在中断表中非空*/
		if (!p_target)
		{
#if defined(USING_DEBUG)
			shellPrint(Shell_Object, "Error: The target board is not in the interrupt table." __INFORMATION());
#endif
			return;
		}
		/*通信板卡转为lora1*/
		if (card_type == (Card_Tyte)64U)
			card_type = Card_Lora1;
		switch (card_type)
		{
		case Card_AnalogInput:
		{
			read_addr = CARD_SIGNAL_MAX * p_target->Number * 2U;
			// ret = mdRTU_ReadInputRegisters(pSlave, read_addr, CARD_SIGNAL_MAX * sizeof(float) / 2U,
			// 							   (mdU16 *)&analog_data);
			ret = mdRTU_ReadInputRegisters(pSlave, read_addr, CARD_SIGNAL_MAX * sizeof(float) / 2U,
										   (mdU16 *)&temp_data);
			report_addr = ANALOG_INPUT_ADDR;
		}
		break;
		case Card_AnalogOutput:
		{
			read_addr = CARD_SIGNAL_MAX * p_target->Number * 2U;
			// ret = mdRTU_ReadHoldRegisters(pSlave, read_addr, CARD_SIGNAL_MAX * sizeof(float) / 2U,
			// 							  (mdU16 *)&analog_data);
			ret = mdRTU_ReadHoldRegisters(pSlave, read_addr, CARD_SIGNAL_MAX * sizeof(float) / 2U,
										  (mdU16 *)&temp_data);
			report_addr = ANALOG_OUTPUT_ADDR;
		}
		break;
		case Card_DigitalInput:
		{
			read_addr = CARD_SIGNAL_MAX * p_target->Number;
			// ret = mdRTU_ReadInputCoils(pSlave, read_addr, CARD_SIGNAL_MAX, digital_data);
			ret = mdRTU_ReadInputCoils(pSlave, read_addr, CARD_SIGNAL_MAX, temp_data);
			report_addr = DIGITAL_INPUT_ADDR;
		}
		break;
		case Card_DigitalOutput:
		{
			read_addr = CARD_SIGNAL_MAX * p_target->Number;
			// ret = mdRTU_ReadCoils(pSlave, read_addr, CARD_SIGNAL_MAX, digital_data);
			ret = mdRTU_ReadCoils(pSlave, read_addr, CARD_SIGNAL_MAX, temp_data);
			report_addr = DIGITAL_OUTPUT_ADDR;
		}
		break;
		case Card_Lora1:
		case Card_Lora2:
		{
			/*需要同时组合32路数字输入和数字输出，以及32bit在线码*/
			read_addr = CARD_COMM_OFFSET_MAX * p_target->Number; //数字输入和输出的读取地址
			ret = mdRTU_ReadInputCoils(pSlave, read_addr, CARD_COMM_OFFSET_MAX, temp_data);
			/*去掉气站部分的4个有线阀门*/
			ret = mdRTU_ReadCoils(pSlave, DIGITAL_OUTPUTOFFSET + (read_addr / 2U), (CARD_COMM_OFFSET_MAX / 2U), &temp_data[CARD_COMM_OFFSET_MAX]);
			/*上报地址：以第一张数字输入板卡基地址开始，后续扩展需要重新分配更多迪文屏幕地址*/
			report_addr = CARD_COMM_REPORT_ADDR;
		}
		break;
		default:
			break;
		}

		if (card_type != Card_None)
		{ /*切换至指定板卡页面*/
			pd->Dw_Page(pd, Get_Change_Page(card_type));
			// osDelay(NEXT_DELAT_TIMES);
			pd->Dw_Delay(NEXT_DELAT_TIMES);
#if defined(USING_DEBUG)
			shellPrint(Shell_Object, "page = %d, card_code = 0x%x.\r\n", Get_Change_Page(card_type), card_type);
#endif
		}
		if (ret == mdTRUE)
		{
			switch (card_type)
			{
			case Card_AnalogInput:
			case Card_AnalogOutput:
			{
				for (uint8_t i = 0; i < CARD_SIGNAL_MAX; i++)
				{
					// Endian_Swap((uint8_t *)&analog_data[i], 0U, sizeof(float));
					Endian_Swap((uint8_t *)&temp_data[i * sizeof(float)], 0U, sizeof(float));
				}
				// pDest = (uint8_t *)&analog_data[0];
				pDest = &temp_data[0];
				rSize = CARD_SIGNAL_MAX * sizeof(float);
			}
			break;
			case Card_DigitalInput:
			case Card_DigitalOutput:
			{
				for (uint8_t i = 0; i < CARD_SIGNAL_MAX; i++)
				{
					// digital |= (((mdU8)digital_data[i] & 0x01) << i);
					digital[0] |= ((temp_data[i] & 0x01) << i);
				}
				digital[0] <<= 8U;
				pDest = (uint8_t *)&digital[0];
				rSize = sizeof(digital[0]);
			}
			break;
			case Card_Lora1:
			case Card_Lora2:
			{ /*组合32bit的DI、DO, 在线状态*/
				for (uint8_t count = 0; count < sizeof(digital) / sizeof(digital[0]); count++)
				{
					for (uint8_t i = 0; i < CARD_SIGNAL_MAX * 2U; i++)
					{
						digital[count] |= ((temp_data[count * 16U + i] & 0x01) << i);
					}
					digital[count] = (digital[count] >> 8U) | (digital[count] << 8U);
				}
				pDest = (uint8_t *)&digital[0];
				/*32bitDI+32bitDO*/
				rSize = sizeof(digital);
			}
			break;
			default:
				// pDest = NULL, rSize = 0;
				break;
			}
		}
		else
		{
#if defined(USING_DEBUG)
			shellPrint(Shell_Object, "Error: Modbus register read failed!\r\n");
#endif
		}
		if (pDest && rSize)
		{
			pd->Dw_Write(pd, report_addr, pDest, rSize);
		}
	}
}

/**
 * @brief  复位板卡组态信息
 * @param  pd 迪文屏幕对象句柄
 * @param  pSite 记录当前Map中位置
 * @retval None
 */
static void Reset_Card_Info(pDwinHandle pd, uint8_t *pSite)
{
	uint16_t rcode = Get_Data(pd, 7U, pd->Uart.pRbuf[6U]);
	// Save_HandleTypeDef *ps = &Save_Flash;
	TYPEDEF_STRUCT3 *ps = (TYPEDEF_STRUCT3 *)pd->Slave.pHandle;
	// Slave_IRQTableTypeDef *pt = (Slave_IRQTableTypeDef *)pd->Slave.pHandle1;
	if (rcode == RSURE_CODE)
	{
		/*切换到重启电源提示界面*/
		pd->Dw_Page(pd, RESET_POEWR_NOTE_PAGE);
		/*清空中断请求列表和中断表*/
		if (ps)
		{
			// ps->Param.Slave_IRQ_Table.SiteCount = ps->Param.Slave_IRQ_Table.TableCount = 0;
			// ps->Param.Slave_IRQ_Table.IRQ_Table_SetFlag = false;
			// for (uint8_t i = 0; i < CARD_NUM_MAX; i++)
			// {
			// 	ps->Param.Slave_IRQ_Table.ReIRQ[i].site = INACTIVE_SITE;
			// 	ps->Param.Slave_IRQ_Table.ReIRQ[i].Priority = PRIORITY_MAX;
			// 	ps->Param.Slave_IRQ_Table.ReIRQ[i].flag = false;
			// 	ps->Param.Slave_IRQ_Table.IRQ[i].SlaveId = CARD_NUM_MAX;
			// 	ps->Param.Slave_IRQ_Table.IRQ[i].Priority = PRIORITY_MAX;
			// 	ps->Param.Slave_IRQ_Table.IRQ[i].TypeCoding = Card_None;
			// 	ps->Param.Slave_IRQ_Table.IRQ[i].Priority = 0;
			// }
			memset(&ps->Param.Slave_IRQ_Table, 0x00, sizeof(ps->Param.Slave_IRQ_Table));
			ps->Param.Slave_IRQ_Table.IRQ_Table_SetFlag = 0xFFFFFFFF;
		}
		/*重新计算CRC校验码*/
		/*Must be 4 byte aligned!!!*/
		ps->Param.crc16 = Get_Crc16((uint8_t *)&ps->Param, sizeof(Save_Param) - sizeof(ps->Param.crc16), 0xFFFF);
#if defined(USING_DEBUG)
		shellPrint(Shell_Object, "ps->Param.crc16 = 0x%x.\r\n", ps->Param.crc16);
#endif

#if defined(USING_FREERTOS)
		taskENTER_CRITICAL();
#endif
		FLASH_Write(PARAM_SAVE_ADDRESS, (uint32_t *)&ps->Param, sizeof(Save_Param));
#if defined(USING_FREERTOS)
		taskEXIT_CRITICAL();
#endif
		// Report_Backparam(Dwin_Object, &ps->Param);
#if defined(USING_DEBUG)
		shellPrint(Shell_Object, "success: Board information cleared successfully!\r\n");
#endif
	}
}

/**
 * @brief  确认板卡组态信息
 * @param  pd 迪文屏幕对象句柄
 * @param  pSite 记录当前Map中位置
 * @retval None
 */
static void Sure_Card_Info(pDwinHandle pd, uint8_t *pSite)
{
	uint16_t rcode = Get_Data(pd, 7U, pd->Uart.pRbuf[6U]);
	// Save_HandleTypeDef *ps = &Save_Flash;
	TYPEDEF_STRUCT3 *ps = (TYPEDEF_STRUCT3 *)pd->Slave.pHandle;
	if (rcode == RSURE_CODE)
	{
		/*切换到主界面*/
		pd->Dw_Page(pd, MAIN_PAGE);
		/*设置板卡信息记录标志有效*/
		ps->Param.Slave_IRQ_Table.IRQ_Table_SetFlag = SAVE_SURE_CODE;
		/*拷贝中断表到falsh存储结构*/
		memcpy(ps->Param.Slave_IRQ_Table.IRQ, IRQ_Table.pIRQ, sizeof(ps->Param.Slave_IRQ_Table.IRQ));
		ps->Param.Slave_IRQ_Table.TableCount = IRQ_Table.TableCount;
		/*重新计算CRC校验码*/
		/*Must be 4 byte aligned!!!*/
		ps->Param.crc16 = Get_Crc16((uint8_t *)&ps->Param, sizeof(Save_Param) - sizeof(ps->Param.crc16), 0xFFFF);
#if defined(USING_DEBUG)
		shellPrint(Shell_Object, "ps->Param.crc16 = 0x%x.\r\n", ps->Param.crc16);
#endif

#if defined(USING_FREERTOS)
		taskENTER_CRITICAL();
#endif
		FLASH_Write(PARAM_SAVE_ADDRESS, (uint32_t *)&ps->Param, sizeof(Save_Param));
#if defined(USING_FREERTOS)
		taskEXIT_CRITICAL();
#endif
#if defined(USING_DEBUG)
		shellPrint(Shell_Object, "success: Board information recording succeeded!\r\n");
#endif
	}
}
