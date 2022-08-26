/********************************************************************
 **--------------文件信息---------------------------------------------
 **文   件   名：wifi.h
 **创 建  日 期：2021年4月9日
 **最后修改日期：
 **版 权 信  息: 云南兆富科技有限公司
 **程   序   员：LHC
 **版   本   号：V3.0
 **描        述：WIFI驱动程序（USR-C215）
 **修 改 日  志:
 *********************************************************************/
#ifndef _WIFI_H_
#define _WIFI_H_

#ifdef __cplusplus
extern "C"
{
#endif
#include "main.h"

/*定义WIFI模块相关引脚*/
#define WIFI_RESET
#define WIFI_RELOAD

/*定义设备ID*/
#define LTE_DRIVERS_ID "87"
#define WIFI_DRIVERS_ID "77"
/*定义串口参数*/
#define UART_PARAM "115200,8,1,NONE,NFC"
/*定义数据打包长度*/
#define PACKAGE_SIZE "2048"
/*定义热点ID*/
#define AP_ID "Greenhouse_Control_System"
/*转化为字符串*/
#define SWITCH_STR(S) #S
/*拼接一个宏定义和字符串*/
#define STR_MCRO() ("AT+WAP=" AP_ID ",NONE\r\n")
/*连接两个字符串S1、S2*/
#define STR_CONNECT(S1, S2) (S1##""##S2)
/*当前设备热点名称*/
#define AP_NAME STR_MCRO() //"AT+WAP=PLC7_AP,NONE\r\n"
/*当前设备云平台设备号*/
#define LTE_CLOUD_ID "AT+CLOUD=000196390000000000" LTE_DRIVERS_ID ",SkdGAzyl\r\n"
/*WIFI设备云平台设备号*/
#define WIFI_CLOUD_ID "AT+REGCLOUD=000196390000000000" WIFI_DRIVERS_ID ",SkdGAzyl\r\n"
/*当前WIFI模块工作方式*/
#define AP_STA_MODE "AT+WMODE=APSTA\r\n"
#define STA_MODE "AT+WMODE=APSTA\r\n"
/*进入透传模式命令*/
#define ENTM_CMD "AT+ENTM\r\n"
/*模块重启命令*/
#define RESTART_CMD "AT+Z\r\n"

#define Get_Ms(__sec) ((uint32_t)((__sec)*1000U))
#define GET_TIMEOUT_FLAG(Stime, Ctime, timeout, MAX) \
	((Ctime) < (Stime) ? !!((((MAX) - (Stime)) + (Ctime)) > (timeout)) : !!((Ctime) - (Stime) > (timeout)))
	typedef unsigned char (*event)(void);
	typedef struct
	{
		char *pSend;
		char *pRecv;
		uint16_t WaitTimes;
		// event pFunc;
	} AT_Command;

	typedef struct
	{
		uint8_t Comm_Num;
		AT_Command *pList;
	} AT_Table;

	typedef struct
	{
		GPIO_TypeDef *pGPIOx;
		uint16_t Gpio_Pin;
	} Gpiox_info;

	typedef struct
	{
		Gpiox_info Reset, Reload;
	} At_Gpio;

	typedef struct AT_HandleTypeDef *pAtHandle;
	typedef struct AT_HandleTypeDef AtHandle;
	struct AT_HandleTypeDef
	{
		uint8_t Id;
		AT_Table Table;
		At_Gpio Gpio;
		UART_HandleTypeDef *huart;
		void *pHandle;
		void (*AT_SetPin)(pAtHandle, Gpiox_info *, GPIO_PinState);
		void (*AT_SetDefault)(pAtHandle);
		bool (*AT_ExeAppointCmd)(pAtHandle, AT_Command *);
		void (*Free_AtObject)(pAtHandle *);
	} __attribute__((aligned(4)));

	extern pAtHandle Lte_Object;
	extern pAtHandle Wifi_Object;
	extern void MX_AtInit(void);
#ifdef __cplusplus
}
#endif

#endif /* _WIFI_H_ */
