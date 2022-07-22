/**
 ******************************************************************************
 * @brief        AT命令通信管理(OS版本)
 *
 * Copyright (c) 2022, <1606820017@qq.com>
 *
 * SPDX-License-Identifier: GNU-3.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-01-02     Morro        Initial version.
 * 2021-02-01     Morro        支持URC回调中接收数据.
 * 2021-02-05     Morro        1.修改struct at_obj,去除链表管理机制
 *                             2.删除 at_obj_destroy接口
 * 2021-03-21     Morro        删除at_obj中的at_work_ctx_t域,减少内存使用
 * 2021-04-08     Morro        解决重复释放信号量导致命令出现等待超时的问题
 * 2021-05-15     Morro        优化URC匹配程序
 * 2021-07-20     Morro        增加锁,解决执行at_do_work时urc部分数据丢失问题
 * 2021-11-20     Morro        按单字符处理接收,避免出现在urc事件中读取数据时
 *                             导致后面数据丢失问题
 ******************************************************************************
 */
#include "at.h"
#include "comdef.h"
#include "main.h"
#include "Modbus.h"
#if defined(USING_RTTHREAD)
#include "rtthread.h"
#else
#include "cmsis_os.h"
#endif
#include "shell_port.h"

/*AT等待时间*/
#define AT_WAITTIMES 300U
/*设置/查询模块 AT 命令回显设置*/
#define SECHO "OFF"
/*查询/设置串口参数*/
#define BAUD_PARA "115200,8,1,NONE,NFC"
/*设置查询工作模式*/
#define SWORK_MODE "FP"
/*查询设置休眠模式*/
#define SPOWER_MODE "RUN"
/*空闲时间:3~240 单位秒（默认 20）*/
#define SIMT "20"
/*time： 500,1000,1500,2000,2500,3000,3500,4000ms（默认 2000）*/
#define SWTM "2000"
/*class： 1~10（默认 10）*/
/*速率对应关系（速率为理论峰值，实际速度要较小一些）：
 1: 268bps
 2: 488bps
 3: 537bps
 4: 878bps
 5: 977bps
 6: 1758bps
 7: 3125bps
 8: 6250bps
 9: 10937bps
 10: 21875bps*/
#define SSPD "10"
/*addr： 0~65535（默认 0）65535 为广播地址，同信道同速率的模块都能接收*/
#define SADDR "0"
/*L102-L 工作频段：(398+ch)MHz*/
#define SCH "0"
/**/
#define SFEC "ON"
/*10~20（默认 20db）不推荐使用小功率发送，其电源利用效率不高*/
#define SPWR "20"
/*time： 0~15000ms（默认 500）仅在 LR/LSR 模式下有效，表示进入接收状态所持续的最长时间，当速率等级较慢的时候应适
当的增加该值以保证数据不会被截断。LSR 模式下如果该值设置为 0 则模块发送数据后不开启接收。*/
#define SRTO "500"
/*time： 相邻数据包间隔，范围：100~60000ms*/
#define SSQT "1000"
/*key：16 字节 HEX 字符串*/
#define SKEY "30313233343536373839414243444546"
/*设置/查询快速进入低功耗使能标志,sta： 1 为打开，0 为关闭。*/
#define SPFLAG "0"
/*设置/查询快速进入低功耗数据
Data：123456（默认 123456）。
Style：ascii、hex（默认 ascii）。*/
#define SPDATE "123456,hex"
/*设置/查询发送完成回复标志,sta：1 为打开，0 为关闭。*/
#define SSENDOK "0"

extern UART_HandleTypeDef huart1;

extern osThreadId shellHandle;
extern osThreadId mdbusHandle;
extern osTimerId Timer1Handle;
extern osSemaphoreId ReciveHandle;

typedef enum
{
    CONF_MODE = 0x00,
    FREE_MODE,
    UNKOWN_MODE,
    USER_ESC,
    CONF_ERROR,
    CONF_TOMEOUT,
    CONF_SUCCESS,
    INPUT_ERROR,
    CMD_MODE, //
    CMD_SURE,
    SET_ECHO,
    SET_UART,
    WORK_MODE,
    POWER_MODE,
    SET_TIDLE,
    SET_TWAKEUP,
    SPEED_GRADE,
    TARGET_ADDR,
    CHANNEL,
    CHECK_ERROR,
    TRANS_POWER,
    SET_OUTTIME,
    // SET_KEY,
    RESTART,
    SIG_STREN,
    EXIT_CMD,
    RECOVERY,
    SELECT_NID,
    SELECT_VER,
    LOW_PFLAG,
    LOW_PDATE,
    FINISH_FLAG,
    EXIT_CONF,
    NO_CMD
} At_InfoList;

typedef struct
{
    At_InfoList Name;
    char *pSend;
    char *pRecv;
    void (*event)(char *data);
} At_HandleTypeDef __attribute__((aligned(4)));

// typedef struct
// {
//     char *pRecv;
//     uint16_t WaitTimes;
// }AT_COMM_HandleTypeDef __attribute__((aligned(4)));

// static uint16_t g_ATWait_Times = AT_WAITTIMES;

At_HandleTypeDef At_Table[] = {
    {.Name = CMD_MODE, .pSend = "+++", "a", NULL},
    {.Name = CMD_SURE, .pSend = "a", AT_CMD_OK, NULL},
    {.Name = EXIT_CMD, .pSend = "AT+ENTM", NULL, NULL},                                  /*退出命令模式，恢复原工作模式*/
    {.Name = SET_ECHO, .pSend = "AT+E=" SECHO, "AT+E", NULL},                            /*设置/查询模块 AT 命令回显设置*/
    {.Name = RESTART, .pSend = "AT+Z", "LoRa Start!", NULL},                             /*重启模块*/
    {.Name = RECOVERY, .pSend = "AT+CFGTF", "+CFGTF:SAVED", NULL},                       /*复制当前配置参数为用户默认出厂配置*/
    {.Name = SELECT_NID, .pSend = "AT+NID", "+NID:", NULL},                              /*查询模块节点 ID*/
    {.Name = SELECT_VER, .pSend = "AT+VER", "+VER:", NULL},                              /*查询模块固件版本*/
    {.Name = SET_UART, .pSend = "AT+UART=" BAUD_PARA, "+UART:" BAUD_PARA, NULL},         /*设置串口参数*/
    {.Name = WORK_MODE, .pSend = "AT+WMODE=" SWORK_MODE, "+WMODE:" SWORK_MODE, NULL},    /*设置工作模式*/
    {.Name = POWER_MODE, .pSend = "AT+PMODE=" SPOWER_MODE, "+PMODE:" SPOWER_MODE, NULL}, /*设置功耗模式*/
    {.Name = SET_TIDLE, .pSend = "AT+ITM=" SIMT, "+ITM:" SIMT, NULL},                    /*设置空闲时间:LR/LSR模式有效*/
    {.Name = SET_TWAKEUP, .pSend = "AT+WTM=" SWTM, "+WTM:" SWTM, NULL},                  /*设置唤醒间隔：此参数对 RUN、LSR 模式无效*/
    {.Name = SPEED_GRADE, .pSend = "AT+SPD=" SSPD, "+SPD:" SSPD, NULL},                  /*设置速率等级*/
    {.Name = TARGET_ADDR, .pSend = "AT+ADDR=" SADDR, "+ADDR:" SADDR, NULL},              /*设置目的地址*/
    {.Name = CHANNEL, .pSend = "AT+CH=" SCH, "+CH:" SCH, NULL},                          /*设置信道*/
    {.Name = CHECK_ERROR, .pSend = "AT+FEC=" SFEC, "+FEC:" SFEC, NULL},                  /*设置前向纠错*/
    {.Name = TRANS_POWER, .pSend = "AT+PWR=" SPWR, "+PWR:" SPWR, NULL},                  /*设置发射功率*/
    {.Name = SET_OUTTIME, .pSend = "AT+RTO=" SRTO, "+RTO:" SRTO, NULL},                  /*设置接收超时时间*/
    {.Name = SIG_STREN, .pSend = "AT+SQT=" SSQT, "+SQT:" SSQT, NULL},                    /*查询信号强度/设置数据自动发送间隔*/
    // {.Name = SET_KEY, .pSend = "AT+KEY=" SKEY, "+KEY:" SKEY, NULL},                      /*设置数据加密字*/
    {.Name = LOW_PFLAG, .pSend = "AT+PFLAG=" SPFLAG, "+PFLAG:" SPFLAG, NULL},       /*设置/查询快速进入低功耗使能标志*/
    {.Name = LOW_PDATE, .pSend = "AT+PDATE=" SPDATE, "+PDATE:" SPDATE, NULL},       /*设置/查询快速进入低功耗数据*/
    {.Name = FINISH_FLAG, .pSend = "AT+SENDOK=" SSENDOK, "+SENDOK:" SSENDOK, NULL}, /*设置/查询发送完成回复标志*/
};
#define AT_TABLE_SIZE (sizeof(At_Table) / sizeof(At_HandleTypeDef))

static const char *atText[] = {
    [CONF_MODE] = "Note: Enter configuration!\r\n",
    [FREE_MODE] = "Note: Enter free mode!\r\n",
    [UNKOWN_MODE] = "Error: Unknown mode!\r\n",
    [USER_ESC] = "Warning: User cancel!\r\n",
    [CONF_ERROR] = "Error: Configuration failed!\r\n",
    [CONF_TOMEOUT] = "Error: Configuration timeout.\r\n",
    [CONF_SUCCESS] = "Success: Configuration succeeded!\r\n",
    [INPUT_ERROR] = "Error: Input error!\r\n",
    [CMD_MODE] = "Note: Enter transparent mode!\r\n",
    [CMD_SURE] = "Note: Confirm to exit the transparent transmission mode?\r\n",
    [SET_ECHO] = "Note: Set echo?\r\n",
    [SET_UART] = "Note: Set serial port parameters!\r\n",
    [WORK_MODE] = "Note: Please enter the working mode?(0:TRANS/1:FP)\r\n",
    [POWER_MODE] = "Note: Please enter the power consumption mode?(0:RUN/1:LR/2:WU/3:LSR)\r\n",
    [SET_TIDLE] = "Note: Set idle time.\r\n",
    [SET_TWAKEUP] = "Note: Set wake-up interval.\r\n",
    [SPEED_GRADE] = "Note: Please enter the rate level?(1~10)\r\n",
    [TARGET_ADDR] = "Note: Please enter the destination address?(0~65535)\r\n",
    [CHANNEL] = "Note: Please enter the channel?(0~127)\r\n",
    [CHECK_ERROR] = "Note: Enable forward error correction?(1:true/0:false)\r\n",
    [TRANS_POWER] = "Note: Please input the transmission power?(10~20db)\r\n",
    [SET_OUTTIME] = "Note: Please enter the receiving timeout?(LR/LSR mode is valid,0~15000ms)\r\n",
    // [SET_KEY] = "Note: Please enter the data encryption word?(16bit Hex)\r\n",
    [RESTART] = "Note: Device restart!\r\n",
    [SIG_STREN] = "Note: Query signal strength.\r\n",
    [EXIT_CMD] = "Note: Exit command mode!\r\n",
    [RECOVERY] = "Note: Restore default parameters!\r\n",
    [SELECT_NID] = "Note: Query node ID?\r\n",
    [SELECT_VER] = "Note: Query version number?\r\n",
    [LOW_PFLAG] = "Note: Set / query fast access low power enable flag.\r\n",
    [LOW_PDATE] = "Note: Set / query fast access to low-power data.\r\n",
    [FINISH_FLAG] = "Note: Set / query sending completion reply flag.\r\n",
    [EXIT_CONF] = "Note: Please press \"ESC\" to end the configuration!\r\n",
    [NO_CMD] = "Error: Command does not exist!\r\n",
};

/*清除HAL库计数器*/
#define SET_HAL_TICK(__value) (uwTick = __value)

/**
 * @brief       等待接收到指定串
 * @param[in]   resp    - 期待待接收串(如"OK",">")
 * @param[in]   timeout - 等待超时时间
 */
At_InfoList Wait_Recv(Shell *const shell, const pModbusHandle pb, const char *resp, uint16_t timeout)
{
    At_InfoList ret = CONF_TOMEOUT;
    //    SET_HAL_TICK(0U);
    uint32_t timer = HAL_GetTick();

    // ret = CONF_SUCCESS;
#if defined(USING_DEBUG)
    // shellPrint(shell, ">wait:%s\r\n", resp);
#endif

#if defined(USING_DEBUG)
    // shellPrint(shell, "error:%d\r\n", __HAL_UART_GET_FLAG(&huart1, UART_FLAG_ORE));
#endif

    /*DMA receive interrupt is managed by the operating system*/
    // portENABLE_INTERRUPTS();
    while (!pb->Slave.RxCount)
    {

        // if (HAL_GetTick() - timer > timeout)
        if (GET_TIMEOUT_FLAG(timer, HAL_GetTick(), timeout, HAL_MAX_DELAY))
            break;
        osDelay(1);
    }
    // portDISABLE_INTERRUPTS();
#if defined(USING_DEBUG)
    // shellPrint(shell, "pb->count:%d\r\n", pb->count);
    // shellPrint(shell, "pb:%p\r\n", pb);
    // shellPrint(shell, "tick = :%d\r\n", HAL_GetTick());
#endif
    if (pb->count)
    {
        pb->buf[pb->count] = '\0';
        ret = strstr((const char *)&(pb->buf), resp) ? CONF_SUCCESS : (strstr((const char *)&(pb->buf), AT_CMD_ERROR) ? CONF_ERROR : CONF_TOMEOUT);
#if defined(USING_DEBUG)
        // shellPrint(shell, ">[MCU<-L101]:%s, timer = %d\r\n", pb->buf, timer);
        shellPrint(shell, ">[MCU<-L101]:%s\r\n", pb->buf);
#endif
    }
    mdClearReceiveBuffer(pb);

    return ret;
}

/**
 * @brief  获取目标at指令
 * @param  shell 终端对象
 * @param  data  接收的数据
 * @retval None
 */
At_HandleTypeDef *Get_AtCmd(At_HandleTypeDef *pAt, At_InfoList list, uint16_t size)
{
    for (uint16_t i = 0; i < size; i++)
    {
        if (pAt[i].Name == list)
        {
            return &pAt[i];
        }
    }

    return NULL;
}

/**
 * @brief  自由模式
 * @note   关于多次运行后：接收错误、从中断中进入临界区：https://www.freertos.org/FreeRTOS_Support_Forum_Archive/August_2006/freertos_portENTER_critical_1560359.html
 * @param  shell 终端对象
 * @param  data  接收的数据
 * @retval None
 */
void Free_Mode(Shell *shell, char *pData)
{
#if defined(USING_FREERTOS)
#define SIZE 64U
    char *pbuf = (char *)CUSTOM_MALLOC(sizeof(char) * SIZE);
    if (!pbuf)
    {
        return;
    }
#else
    char pbuf[64U] = {0};
#endif
    // ModbusRTUSlaveHandler pH = Master_Object;
    pModbusHandle pH = &Modbus_Object;
    At_HandleTypeDef *pAt = At_Table, *pS = NULL;
    At_InfoList result = CONF_SUCCESS;
    char *pRe = NULL, *pDest = NULL;
    uint16_t len = 0;

#if defined(USING_DEBUG)
    // shellPrint(shell, "pH = 0x%02x\r\n", pH);
#endif
    while (1)
    {
        if (shell->read(pData, 0x01))
        {
            switch (*pData)
            {
            case ENTER_CODE:
            {
                pbuf[len] = '\0';
                len = 0U;
#if defined(USING_DEBUG)
                shellPrint(shell, "\r\nInput:%s\r\n", pbuf);
#endif
                /*进入命令模式，并关闭回显*/
                for (At_InfoList at_cmd = CMD_MODE; at_cmd < POWER_MODE; at_cmd++)
                { /*执行完毕后退出指令模式*/
                    at_cmd = (at_cmd == POWER_MODE - 1U) ? RESTART : at_cmd;
                    pS = Get_AtCmd(pAt, at_cmd, AT_TABLE_SIZE);
                    pRe = ((at_cmd == CMD_MODE) || (at_cmd == SET_ECHO)) ? pS->pRecv : AT_CMD_OK;
                    if (pS)
                    {
#if defined(USING_FREERTOS)
                        char *pStr = (char *)CUSTOM_MALLOC(sizeof(char) * strlen(pS->pSend) + strlen(AT_CMD_END_MARK_CRLF));
                        if (pStr)
                        { /*拷贝时带上'\0'*/
                            memcpy(pStr, pS->pSend, strlen(pS->pSend) + 1U);
                            if (at_cmd > CMD_SURE)
                            {
                                strcat(pStr, AT_CMD_END_MARK_CRLF);
                            }
                            pDest = (at_cmd != SET_UART) ? pStr : strcat(pbuf, AT_CMD_END_MARK_CRLF);
#if defined(USING_DEBUG)
                            // SHELL_LOCK(shell);
                            shellPrint(shell, "\r\n%s", atText[at_cmd]);
                            shellPrint(shell, ">[MCU->L101]:%s\r\n", pDest);
                            // SHELL_UNLOCK(shell);
#endif
                            pH->mdRTUSendString(pH, (uint8_t *)pDest, strlen(pDest));
                        }
                        CUSTOM_FREE(pStr);
#else
                        pH->mdRTUSendString(pH, (uint8_t *)pDest, strlen(pDest));
                        /*加上结尾*/
                        if (at_cmd > CMD_SURE)
                        {
                            pH->mdRTUSendString(pH, (uint8_t *)AT_CMD_END_MARK_CRLF, strlen(AT_CMD_END_MARK_CRLF));
                        }
#endif
                    }
                    result = Wait_Recv(shell, pH->receiveBuffer, pRe, MAX_URC_RECV_TIMEOUT);
                    shellWriteString(shell, atText[result]);
                    if ((result != CONF_SUCCESS) || (at_cmd == RESTART))
                    {
                        shellWriteString(shell, atText[EXIT_CONF]);
                        // break;
                    }
                }
            }
            break;
            case BACKSPACE_CODE:
            {
                len = len ? shellDeleteCommandLine(shell, 0x01), --len : 0;
            }
            break;
            case ESC_CODE:
            {
                goto __exit;
            }
            default:
            {
#if defined(USING_DEBUG)
                // shellPrint(shell, "\r\nlen = %d\r\n", len);
#endif
                pbuf[len++] = *pData;
                // shellPrint(shell, "\r\nval = %c\r\n", pbuf[len]);
                len = (len < SIZE) ? len : 0U;
                shell->write(pData, 0x01);
            }
            break;
            }
        }
    }

__exit:
#if defined(USING_FREERTOS)
    CUSTOM_FREE(pbuf);
#endif
}

/**
 * @brief  参数配置模式
 * @param  shell 终端对象
 * @param  data  接收的数据
 * @retval None
 */
static void Config_Mode(Shell *shell, char *pData)
{
    pModbusHandle pH = &Modbus_Object;
    At_HandleTypeDef *pAt = At_Table, *pS = NULL;
    char *pRe = NULL;
    At_InfoList result = CONF_SUCCESS;
    bool at_mutex = false;

    // mdClearReceiveBuffer(pH->receiveBuffer);
    // HAL_UART_Receive_DMA(&huart1, mdRTU_Recive_Buf(Master_Object), MODBUS_PDU_SIZE_MAX);
    while ((*pData) != ESC_CODE)
    { /*接收到的数据不为0*/
        if (shell->read(pData, 0x01))
        {
#if defined(USING_DEBUG)
            // shellPrint(shell, "*pdata = 0x%02x\r\n", *pData);
#endif
            for (At_InfoList at_cmd = CMD_MODE; (at_cmd < SIG_STREN) && (!at_mutex); at_cmd++)
            {
                pS = Get_AtCmd(pAt, at_cmd, AT_TABLE_SIZE);
#if defined(USING_DEBUG)
                // shellPrint(shell, "at_cmd = %d, %s", at_cmd, atText[at_cmd]);
                // shellPrint(shell, "huart->RxState = %d\r\n", huart1.RxState);
                shellPrint(shell, "\r\n%s", atText[at_cmd]);
                shellPrint(shell, ">[MCU->L101]:%s\r\n", pS->pSend);
#endif
                if (pS->pSend)
                {
#if defined(USING_FREERTOS)
                    char *pStr = (char *)CUSTOM_MALLOC(sizeof(char) * strlen(pS->pSend) + strlen(AT_CMD_END_MARK_CRLF));
                    if (pStr)
                    { /*拷贝时带上'\0'*/
                        memcpy(pStr, pS->pSend, strlen(pS->pSend) + 1U);
                        if (at_cmd > CMD_SURE)
                        {
                            strcat(pStr, AT_CMD_END_MARK_CRLF);
                        }
                        // pH->mdRTUSendString(pH, (uint8_t *)pStr, strlen(pStr));
                        memcpy(pH->Master.pTbuf, (uint8_t *)pStr, strlen(pStr));
                        pH->Mod_Transmit(pH, NotUsedCrc);
                    }
                    CUSTOM_FREE(pStr);
#else
                    pH->mdRTUSendString(pH, (uint8_t *)pS->pSend, strlen(pS->pSend));
                    /*加上结尾*/
                    if (at_cmd > CMD_SURE)
                    {
                        pH->mdRTUSendString(pH, (uint8_t *)AT_CMD_END_MARK_CRLF, strlen(AT_CMD_END_MARK_CRLF));
                    }
#endif
                }
                else
                {
                    at_mutex = true;
                    shellWriteString(shell, atText[NO_CMD]);
                    break;
                }
                pRe = ((at_cmd == CMD_MODE) || (at_cmd == SET_ECHO)) ? pS->pRecv : AT_CMD_OK;
                result = Wait_Recv(shell, pH->receiveBuffer, pRe, MAX_URC_RECV_TIMEOUT);
                // result = Wait_Recv(shell, pH->receiveBuffer, pS->pRecv, MAX_URC_RECV_TIMEOUT);
                shellWriteString(shell, atText[result]);
                if ((result != CONF_SUCCESS) || (at_cmd == RESTART))
                {
                    at_mutex = true;
                    shellWriteString(shell, atText[EXIT_CONF]);
                    break;
                }
            }
        }
    }
}

/**
 * @brief  通过AT指令配置L101模块参数
 * @param  cmd 命令模式 1参数配置 2自由指令
 * @retval None
 */
void At_Handle(uint8_t cmd)
{
    Shell *sh = Shell_Object;
    char recive_data = '\0';

    if (cmd > FREE_MODE)
    {
        shellWriteString(sh, atText[UNKOWN_MODE]);
        return;
    }
    //    __set_PRIMASK(1); /* 禁止全局中断*/
    /*挂起mdbusHandle任务*/
    // osThreadSuspend(mdbusHandle);
    osThreadSuspendAll();
    /*停止发送定时器*/
    osTimerStop(Timer1Handle);
    //    __set_PRIMASK(0); /*  使能全局中断 */
    // __HAL_UART_DISABLE_IT(&huart1, UART_IT_IDLE);
    // HAL_NVIC_DisableIRQ(USART1_IRQn);
    shellWriteString(sh, atText[cmd]);
    cmd ? Free_Mode(sh, &recive_data) : Config_Mode(sh, &recive_data);
    /*打开发送定时器*/
    osTimerStart(Timer1Handle, MDTASK_SENDTIMES);
    /*恢复mdbusHandle任务*/
    // osThreadResume(mdbusHandle);
#if defined(USING_DEBUG)
    // shellPrint(sh, "portNVIC_INT_CTRL_REG = 0x%x\r\n", portNVIC_INT_CTRL_REG);
#endif
    // __set_PRIMASK(1); /* 禁止全局中断*/
    osThreadResumeAll();
    // __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
    // HAL_NVIC_EnableIRQ(USART1_IRQn);
    // __set_PRIMASK(0); /*  使能全局中断 */
}
#if defined(USING_DEBUG)
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), at, At_Handle, config);
#endif
