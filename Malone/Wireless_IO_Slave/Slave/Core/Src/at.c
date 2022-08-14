#include "at.h"
//#include "comdef.h"
#include "main.h"
#include "mdrtuslave.h"
#if defined(USING_RTTHREAD)
#include "rtthread.h"
#else
#include "cmsis_os.h"
#endif
#include "shell_port.h"

bool g_LoraMutex = false;
extern UART_HandleTypeDef huart1;

extern osThreadId shellHandle;
extern osThreadId mdbusHandle;
extern osTimerId Timer1Handle;
extern osSemaphoreId ReciveHandle;

/*定义一个标志，在配置模式下，操作系统相关任务不执行*/
// bool g_Modbus_ExeFlag = false;

const AtHandle At_Table[] = {
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
#define AT_TABLE_SIZE (sizeof(At_Table) / sizeof(AtHandle))

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
 * @brief	缓冲区解析回调函数
 * @details
 * @param	None
 * @retval	None
 */
static void At_ParserCallback(char *pbuf)
{
    char *pRecv = "+CH:", id = 0x01;
    ModbusRTUSlaveHandler pH = Slave_Object;
    if (pbuf)
    {
        // char *pstr = strstr(pre, pRecv);
        // if (pstr)
        {
            if (strstr(pbuf, pRecv))
            {
                if ((pbuf[7] != '\r') || (pbuf[8] != '\n'))
                    id = (pbuf[6] - '0') * 10U + (pbuf[7] - '0');
                else
                    id = pbuf[6] - '0';
                pH->slaveId = (uint8_t)id;
                shellPrint(Shell_Object, "@Success:slave_id:[%d],pbuf[6]:%c.\r\n",
                           id, pbuf[6]);
            }
        }
    }
}

/**
 * @brief       等待接收到指定串
 * @param[in]   resp    - 期待待接收串(如"OK",">")
 * @param[in]   timeout - 等待超时时间
 */
At_InfoList Wait_Recv(Shell *const shell, const ReceiveBufferHandle pB, const char *resp, uint16_t timeout)
{
    At_InfoList ret = CONF_TOMEOUT;
    uint32_t timer = HAL_GetTick();

    do
    {
        if (GET_TIMEOUT_FLAG(timer, HAL_GetTick(), timeout, HAL_MAX_DELAY))
            break;
        // osDelay(1);
    } while (!pB->count);
    if (pB->count)
    {
        pB->buf[pB->count] = '\0';
        ret = strstr((const char *)&(pB->buf), resp)
                  ? CONF_SUCCESS
                  : (strstr((const char *)&(pB->buf), AT_CMD_ERROR) ? CONF_ERROR : CONF_TOMEOUT);
        /*特殊指令解析回调*/
        At_ParserCallback((char *)pB->buf);
        // #if defined(USING_DEBUG)
        // shellPrint(shell, ">[MCU<-L101]:%s, timer = %d\r\n", pB->buf, timer);
        shellPrint(shell, ">[MCU<-L102]:%s\r\n", pB->buf);
        // #endif
    }
    mdClearReceiveBuffer(pB);

    return ret;
}

/**
 * @brief  获取目标at指令
 * @param  shell 终端对象
 * @param  data  接收的数据
 * @retval None
 */
const AtHandle *Get_AtCmd(const AtHandle *pAt, At_InfoList list, uint16_t size)
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
 * @brief	AT模块接收上层自由指令
 * @details
 * @param	None
 * @retval	SlaveId
 */
// static void At_ExeFree_Cmd(Shell *shell, ModbusRTUSlaveHandler ph,
//                            AtHandle const *pat, char *const psource, char *const ptarget)
static void At_ExeFree_Cmd(Shell *shell, ModbusRTUSlaveHandler ph,
                           AtHandle const *pat, char *const psource)
{
    AtHandle const *ps = NULL;
    At_InfoList result = CONF_SUCCESS;
    char *pre = NULL, *pdest = NULL;

    if (!shell && !ph && !pat && !psource)
    {
        shellWriteString(shell, atText[EXIT_CONF]);
        return;
    }

    /*进入命令模式，并关闭回显*/
    for (At_InfoList at_cmd = CMD_MODE; at_cmd < POWER_MODE; at_cmd++)
    { /*执行完毕后退出指令模式*/
        at_cmd = (at_cmd == POWER_MODE - 1U) ? RESTART : at_cmd;
        ps = Get_AtCmd(pat, at_cmd, AT_TABLE_SIZE);
        pre = ((at_cmd == CMD_MODE) || (at_cmd == SET_ECHO)) ? ps->pRecv
                                                             : AT_CMD_OK;
        if (ps)
        {
#if defined(USING_FREERTOS)
            char *pStr = (char *)CUSTOM_MALLOC(sizeof(char) * strlen(ps->pSend) + strlen(AT_CMD_END_MARK_CRLF));
            if (pStr)
            { /*拷贝时带上'\0'*/
                memcpy(pStr, ps->pSend, strlen(ps->pSend) + 1U);
                if (at_cmd > CMD_SURE)
                {
                    strcat(pStr, AT_CMD_END_MARK_CRLF);
                }
                pdest = (at_cmd != SET_UART) ? pStr : (char *)psource;
                // #if defined(USING_DEBUG)
                shellPrint(shell, "\r\n%s", atText[at_cmd]);
                shellPrint(shell, ">[MCU->L102]:%s\r\n", pdest);
                // #endif
                // mdClearReceiveBuffer(ph->receiveBuffer);
                ph->mdRTUSendString(ph, (mdU8 *)pdest, strlen(pdest));
            }
            CUSTOM_FREE(pStr);
#else
            pH->mdRTUSendString(pH, (mdU8 *)pDest, strlen(pDest));
            /*加上结尾*/
            if (at_cmd > CMD_SURE)
            {
                pH->mdRTUSendString(pH, (mdU8 *)AT_CMD_END_MARK_CRLF, strlen(AT_CMD_END_MARK_CRLF));
            }
#endif
        }
        result = Wait_Recv(shell, ph->receiveBuffer, pre, MAX_URC_RECV_TIMEOUT);
        shellWriteString(shell, atText[result]);
        if ((result != CONF_SUCCESS) || (at_cmd == RESTART))
        {
            shellWriteString(shell, atText[EXIT_CONF]);
        }
    }
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
    char *pBuf = (char *)CUSTOM_MALLOC(sizeof(char) * SIZE);
    if (!pBuf)
    {
        return;
    }
    memset(pBuf, 0x00, SIZE);
#else
    char pBuf[64U] = {0};
#endif
    ModbusRTUSlaveHandler pH = Slave_Object;
    AtHandle const *pAt = At_Table;
    uint8_t len = 0;

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
                pBuf[len] = '\0';
                // #if defined(USING_DEBUG)
                shellPrint(shell, "\r\nInput:%s\r\n", pBuf);
                // shellWriteEndLine(shell, pBuf, len);
                // #endif
                At_ExeFree_Cmd(shell, pH, pAt, strcat(pBuf, AT_CMD_END_MARK_CRLF));
                len = 0U;
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
                pBuf[len++] = *pData;
                // shellPrint(shell, "\r\nval = %c\r\n", pBuf[len]);
                //                len = (len < SIZE) ? len : 0U;
                len %= SIZE;
                shell->write(pData, 0x01);
            }
            break;
            }
        }
    }

__exit:
#if defined(USING_FREERTOS)
    CUSTOM_FREE(pBuf);
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
    ModbusRTUSlaveHandler pH = Slave_Object;
    AtHandle const *pAt = At_Table, *pS = NULL;
    char *pRe = NULL;
    At_InfoList result = CONF_SUCCESS;
    bool at_mutex = false;

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
                // #if defined(USING_DEBUG)
                shellPrint(shell, "\r\n%s", atText[at_cmd]);
                shellPrint(shell, ">[MCU->L102]:%s\r\n", pS->pSend);
                // #endif
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
                        pH->mdRTUSendString(pH, (mdU8 *)pStr, strlen(pStr));
                    }
                    CUSTOM_FREE(pStr);
#else
                    pH->mdRTUSendString(pH, (mdU8 *)pS->pSend, strlen(pS->pSend));
                    /*加上结尾*/
                    if (at_cmd > CMD_SURE)
                    {
                        pH->mdRTUSendString(pH, (mdU8 *)AT_CMD_END_MARK_CRLF, strlen(AT_CMD_END_MARK_CRLF));
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
    ModbusRTUSlaveHandler pH = Slave_Object;

    if (cmd > FREE_MODE)
    {
        shellWriteString(sh, atText[UNKOWN_MODE]);
        return;
    }
    shellWriteString(sh, atText[cmd]);

    // osThreadSuspendAll();
    g_LoraMutex = true;
    cmd ? Free_Mode(sh, &recive_data) : Config_Mode(sh, &recive_data);
    mdClearReceiveBuffer(pH->receiveBuffer);
    g_LoraMutex = false;
    // osThreadResumeAll();

#if defined(USING_DEBUG)
    // shellPrint(sh, "portNVIC_INT_CTRL_REG = 0x%x\r\n", portNVIC_INT_CTRL_REG);
#endif
    return;
}
// #if defined(USING_DEBUG)
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), at, At_Handle, config);
// #endif

/**
 * @brief	通过AT模块获取从机ID
 * @details
 * @param	None
 * @retval	SlaveId
 */
void At_GetSlaveId(void)
{
    Shell *sh = Shell_Object;
    ModbusRTUSlaveHandler pH = Slave_Object;
    AtHandle const *pAt = At_Table;
    char *pSend = "AT+CH?\r\n";

    // osThreadSuspendAll();
    g_LoraMutex = true;
    At_ExeFree_Cmd(sh, pH, pAt, pSend);
    g_LoraMutex = false;
    mdClearReceiveBuffer(pH->receiveBuffer);
    return;

    // osThreadResumeAll();

    // #define RETRY_COUNTS 3U
    // #define AT_CMD_ERROR "ERR"
    //     ModbusRTUSlaveHandler ph = Slave_Object;
    //     ReceiveBufferHandle pb = ph->receiveBuffer;
    //     char *pSend = "AT+CH?\r\n", *pRecv = "+CH:";

    //     uint8_t counts = 0, id = 0;
    //     bool result = true;

    //     Shell *shell = Shell_Object;
    //     AtHandle const *pAt = At_Table, *pS = NULL;
    //     At_InfoList result = CONF_SUCCESS;
    //     char *pRe = NULL, *pDest = NULL;

    //     /*进入命令模式，并关闭回显*/
    //     for (At_InfoList at_cmd = CMD_MODE; at_cmd < POWER_MODE; at_cmd++)
    //     { /*执行完毕后退出指令模式*/
    //         at_cmd = (at_cmd == POWER_MODE - 1U) ? RESTART : at_cmd;
    //         pS = Get_AtCmd(pAt, at_cmd, AT_TABLE_SIZE);
    //         pRe = ((at_cmd == CMD_MODE) || (at_cmd == SET_ECHO)) ? pS->pRecv : AT_CMD_OK;
    //         if (pS)
    //         {
    // #if defined(USING_FREERTOS)
    //             char *pStr = (char *)CUSTOM_MALLOC(sizeof(char) * strlen(pS->pSend) + strlen(AT_CMD_END_MARK_CRLF));
    //             if (pStr)
    //             { /*拷贝时带上'\0'*/
    //                 memcpy(pStr, pS->pSend, strlen(pS->pSend) + 1U);
    //                 if (at_cmd > CMD_SURE)
    //                 {
    //                     strcat(pStr, AT_CMD_END_MARK_CRLF);
    //                 }
    //                 pDest = (at_cmd != SET_UART) ? pStr : pSend;
    // #if defined(USING_DEBUG)
    //                 shellPrint(shell, "\r\n%s", atText[at_cmd]);
    //                 shellPrint(shell, ">[MCU->L101]:%s\r\n", pDest);
    // #endif
    //                 ph->mdRTUSendString(ph, (mdU8 *)pDest, strlen(pDest));
    //             }
    //             CUSTOM_FREE(pStr);
    // #else
    //             pH->mdRTUSendString(pH, (mdU8 *)pDest, strlen(pDest));
    //             /*加上结尾*/
    //             if (at_cmd > CMD_SURE)
    //             {
    //                 pH->mdRTUSendString(pH, (mdU8 *)AT_CMD_END_MARK_CRLF, strlen(AT_CMD_END_MARK_CRLF));
    //             }
    // #endif
    //         }
    //         result = Wait_Recv(shell, ph->receiveBuffer, pRe, MAX_URC_RECV_TIMEOUT);
    //         shellWriteString(shell, atText[result]);
    //         if ((result != CONF_SUCCESS) || (at_cmd == RESTART))
    //         {
    //             shellWriteString(shell, atText[EXIT_CONF]);
    //             // break;
    //         }
    //     }

    // if (ph)
    // {
    //     while (result)
    //     {
    //         mdClearReceiveBuffer(pb);
    //         ph->mdRTUSendString(ph, (uint8_t *)pSend, strlen(pRecv));
    //         // HAL_UART_Transmit(pa->huart, (uint8_t *)pat->pSend, strlen(pat->pSend), pat->WaitTimes);
    //         uint32_t timer = HAL_GetTick();

    //         while (!pb->count)
    //         {
    //             if (GET_TIMEOUT_FLAG(timer, HAL_GetTick(), MAX_URC_RECV_TIMEOUT, HAL_MAX_DELAY))
    //                 break;
    //         }
    //         if (pb->count)
    //         {
    //             pb->buf[pb->count] = '\0';
    //             char *pdest = strstr((const char *)&(pb->buf), (const char *)pRecv);
    //             if (pdest == NULL)
    //             {
    //                 counts++;
    //                 shellPrint(Shell_Object, "Send:%sResponse instruction:%s and %s mismatch.\r\n",
    //                            pSend, pb->buf, pRecv);
    //             }
    //             else
    //             {
    //                 if ((pSend[8] != '\r') || (pSend[8] != '\n'))
    //                     id = (pSend[7] - '0') * 10U + (pSend[8] - '0');
    //                 else
    //                     id = pdest[7] - '0';
    //                 shellPrint(Shell_Object, "@Success:Command:%ssent successfully,Recive:%s, id:%d.\r\n",
    //                            pSend, pb->buf, id);
    //                 result = false;
    //             }
    //         }
    //         else
    //         {
    //             counts++;
    //             shellPrint(Shell_Object, "At module does not respond!data:%s.\r\n", pb->buf);
    //         }
    //         if (counts >= RETRY_COUNTS)
    //         {
    //             shellPrint(Shell_Object, "@Error:Retransmission exceeds the maximum number of times!\r\n");
    //             id = 0x01;
    //             result = false;
    //         }
    //     }
    // }
    // return id;
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), get_slaveid, At_GetSlaveId, get);
