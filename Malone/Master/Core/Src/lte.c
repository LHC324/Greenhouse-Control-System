#include "lte.h"
#include "usart.h"
#include "mdrtuslave.h"
#include "shell_port.h"
#if defined(USING_FREERTOS)
#include "cmsis_os.h"
#endif

/*定义AT模块对象*/
pAtHandle Lte_Object;
pAtHandle Wifi_Object;

/*局部函数申明*/
static void Free_AtObject(pAtHandle *pa);
static void At_SetPin(pAtHandle pa, Gpiox_info *pgpio, GPIO_PinState PinState);
static void At_SetDefault(pAtHandle pa);
static bool At_ExeAppointCmd(pAtHandle pa, AT_Command *pat);

/*4G模块AT指令列表*/
AT_Command Lte_list[] = {

	{"+++", "a", Get_Ms(2U)},
	{"a", "+ok", Get_Ms(2U)},
	{"AT+E=OFF\r\n", "OK", Get_Ms(0.2F)},

	{"AT+HEARTDT=7777772E796E7061782E636F6D\r\n", "OK", Get_Ms(0.2F)},

	{"AT+WKMOD=NET\r\n", "OK", Get_Ms(0.2F)},
	{"AT+SOCKAEN=ON\r\n", "OK", Get_Ms(0.2F)},
	{"AT+SOCKASL=LONG\r\n", "OK", Get_Ms(0.2F)},
	{"AT+SOCKA=TCP,clouddata.usr.cn,15000\r\n", "OK", Get_Ms(0.2F)},

	{"AT+REGEN=ON\r\n", "OK", Get_Ms(0.2F)},
	{"AT+REGTP=CLOUD\r\n", "OK", Get_Ms(0.2F)},
	{LTE_CLOUD_ID, "OK", Get_Ms(0.2F)},
	{"AT+UART=" UART_PARAM "\r\n", "OK", Get_Ms(0.2F)},
	{"AT+UARTFL=" PACKAGE_SIZE "\r\n", "OK", Get_Ms(0.2F)},
	{"AT+Z\r\n", "OK", Get_Ms(0.2F)},
};
#define LTE_CMD_SIZE (sizeof(Lte_list) / sizeof(AT_Command))

/*USR-C210模块AT指令列表*/
AT_Command Wifi_list[] = {
	/*WIFI模块推出透传模式进入AT指令模式*/
	{"+++", "a", Get_Ms(2U)},
	/*WIFI模块响应后，主动发送”a“*/
	{"a", "+Ok", Get_Ms(2U)},
	/*关闭回显*/
	{"AT+E=OFF\r\n", "+OK", Get_Ms(0.02F)},
	/*显示SSID*/
	//{"AT+HSSID = OFF\r\n", "+OK", 500},
	/*WIFI工作模式：AP + STA*/
	{AP_STA_MODE, "+OK", Get_Ms(0.02F)},
	/*设置路由器名称*/
	{AP_NAME, "+OK", Get_Ms(0.02F)},
	/*设置心跳数据:www.ynpax.com*/
	{"AT+HEARTDT=7777772E796E7061782E636F6D\r\n", "+OK", Get_Ms(0.02F)},
	/*SSID和密码不能程序输入，需要在现场根据用户方的WIFI设置通过WEB方式修改*/
	/*设置WIFI登录SSID，密码*/
	{"AT+WSTA=union,*!ynzfkj20091215!*\r\n", "+OK", Get_Ms(0.02F)},
	/*透传云设置*/
	{"AT+REGENA=CLOUD,FIRST\r\n", "+OK", Get_Ms(0.02F)},
	/*设置STOCKA参数*/
	{"AT+SOCKA=TCPC,cloud.usr.cn,15000\r\n", "+OK", Get_Ms(0.02F)},
	/*设置搜索服务器和端口*/
	{"AT+SEARCH=15000,cloud.usr.cn\r\n", "+OK", Get_Ms(0.02F)},
	/*透传云ID，透传云密码*/
	{WIFI_CLOUD_ID, "+OK", Get_Ms(0.02F)},
	/*设置DHCP*/
	{"AT+WANN=DHCP\r\n", "+OK", Get_Ms(0.02F)},
	/*软件重启USR-C210*/
	{"AT+Z\r\n", "+OK", Get_Ms(0.02F)},
	/*设置透传模式*/
	// {"AT+ENTM\r\n", "+OK", 50U}
};

#define WIFI_CMD_SIZE sizeof(Wifi_list) / sizeof(AT_Command)

/**
 * @brief	创建AT模块对象
 * @details
 * @param	pa 需要初始化对象指针
 * @param   ps 初始化数据指针
 * @retval	None
 */
static void Creat_AtObject(pAtHandle *pa, pAtHandle ps)
{
	if (!ps)
		return;
#if defined(USING_FREERTOS)
	(*pa) = (pAtHandle)CUSTOM_MALLOC(sizeof(AtHandle));
	if (!(*pa))
		CUSTOM_FREE(*pa);
#else

#endif

#if defined(USING_DEBUG)
	shellPrint(Shell_Object, "At[%d]_handler = 0x%p\r\n", ps->Id, *pa);
#endif
	(*pa)->Id = ps->Id;
	(*pa)->huart = ps->huart;
	(*pa)->pHandle = ps->pHandle;
	(*pa)->Table = ps->Table;
	(*pa)->Gpio = ps->Gpio;
	(*pa)->Free_AtObject = Free_AtObject;
	(*pa)->AT_SetPin = At_SetPin;
	(*pa)->AT_SetDefault = At_SetDefault;
	(*pa)->AT_ExeAppointCmd = At_ExeAppointCmd;
}

/**
 * @brief	销毁AT模块对象
 * @details
 * @param	pa 对象指针
 * @retval	None
 */
static void Free_AtObject(pAtHandle *pa)
{
	if (*pa)
	{
		CUSTOM_FREE((*pa));
	}
}

/**
 * @brief	销毁AT模块对象
 * @details
 * @param	None
 * @retval	None
 */
void MX_AtInit(void)
{
	At_Gpio lte_gpio = {
		.Reload = {.pGPIOx = LTE_RELOAD_GPIO_Port, .Gpio_Pin = LTE_RELOAD_Pin},
		.Reset = {.pGPIOx = LTE_RESET_GPIO_Port, .Gpio_Pin = LTE_RESET_Pin},
	};
	AT_Table lte_table = {
		.pList = Lte_list,
		.Comm_Num = LTE_CMD_SIZE,
	};
	extern UART_HandleTypeDef huart2;
	AtHandle lte = {
		.Id = 0x00,
		.Gpio = lte_gpio,
		.Table = lte_table,
		.huart = &huart2,
		.pHandle = Slave1_Object->receiveBuffer,
	};
	Creat_AtObject(&Lte_Object, &lte);

	At_Gpio wifi_gpio = {
		.Reload = {.pGPIOx = WIFI_RELOAD_GPIO_Port, .Gpio_Pin = WIFI_RELOAD_Pin},
		.Reset = {.pGPIOx = WIFI_RESET_GPIO_Port, .Gpio_Pin = WIFI_RESET_Pin},
	};
	AT_Table wifi_table = {
		.pList = Wifi_list,
		.Comm_Num = WIFI_CMD_SIZE,
	};
	extern UART_HandleTypeDef huart5;
	AtHandle wifi = {
		.Id = 0x01,
		.Gpio = wifi_gpio,
		.Table = wifi_table,
		.huart = &huart5,
		.pHandle = Slave1_Object->receiveBuffer,
	};
	Creat_AtObject(&Wifi_Object, &wifi);
}

/**
 * @brief	AT模块的reset、reload
 * @details
 * @param	None
 * @retval	None
 */
static void At_SetPin(pAtHandle pa, Gpiox_info *pgpio, GPIO_PinState PinState)
{
	if (pgpio && (pgpio->pGPIOx))
		HAL_GPIO_WritePin(pgpio->pGPIOx, pgpio->Gpio_Pin, PinState);
}

/**
 * @brief	AT模块初始化默认参数
 * @details
 * @param	None
 * @retval	None
 */
static void At_SetDefault(pAtHandle pa)
{
	if (pa && (pa->Table.pList) && (pa->huart))
	{
		for (AT_Command *pat = pa->Table.pList; pat < pa->Table.pList + pa->Table.Comm_Num; pat++)
		{
			if (!pa->AT_ExeAppointCmd(pa, pat))
			{
				pat = pa->Table.pList + pa->Table.Comm_Num - 1U;
				shellPrint(Shell_Object, "@Error:Module configuration failed.Module is restarting...\r\n");
				/*执行一次重启指令：防止卡死在AT模式下*/
				pa->AT_ExeAppointCmd(pa, pat);
				break;
			}
		}
	}
}

/**
 * @brief	AT模块执行指定指令
 * @details
 * @param	None
 * @retval	None
 */
static bool At_ExeAppointCmd(pAtHandle pa, AT_Command *pat)
{
#define RETRY_COUNTS 3U
#define AT_CMD_ERROR "ERR"
	ReceiveBufferHandle pb = (ReceiveBufferHandle)pa->pHandle;
	uint8_t counts = 0;
	bool result = true;
	if (pa && pat && pb)
	{
		while (result)
		{
			mdClearReceiveBuffer(pb);
			HAL_UART_Transmit(pa->huart, (uint8_t *)pat->pSend, strlen(pat->pSend), pat->WaitTimes);
			uint32_t timer = HAL_GetTick();

			while (!pb->count)
			{
				if (GET_TIMEOUT_FLAG(timer, HAL_GetTick(), pa->Table.pList->WaitTimes, HAL_MAX_DELAY))
					break;
				// osDelay(1);
			}
			if (pb->count)
			{
				pb->buf[pb->count] = '\0';
				if (strstr((const char *)&(pb->buf), (const char *)pat->pRecv) == NULL)
				{
					counts++;
					shellPrint(Shell_Object, "Send:%sResponse instruction:%s and %s mismatch.\r\n",
							   pat->pSend, pb->buf, pat->pRecv);
				}
				else
				{
					shellPrint(Shell_Object, "Command:%ssent successfully,Recive:%s",
							   pat->pSend, pb->buf);
					break;
				}
			}
			else
			{
				counts++;
				shellPrint(Shell_Object, "At module does not respond!data:%s.\r\n", pb->buf);
			}
			if (counts >= RETRY_COUNTS)
			{
				result = false;
				shellPrint(Shell_Object, "@Error:Retransmission exceeds the maximum number of times!\r\n");
			}
		}
	}
	return result;
}

/**
 * @brief	配置AT模块参数
 * @details
 * @param	None
 * @retval	None
 */
void At_Config(void)
{
	osThreadSuspendAll();
	MX_AtInit();

	/*关闭4G模块空闲中断*/
	if (Lte_Object)
	{
		shellPrint(Shell_Object, "@Success:At object allocation succeeded.\r\n");
		Lte_Object->AT_SetDefault(Lte_Object);
		Lte_Object->Free_AtObject(&Lte_Object);
	}
	else
		shellPrint(Shell_Object, "@Error:At object allocation failed!\r\n");
	osThreadResumeAll();
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), at_config, At_Config, configure);
