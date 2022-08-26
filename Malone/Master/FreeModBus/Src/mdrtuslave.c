#include <stdlib.h>
#include "mdrtuslave.h"
#include "mdcrc16.h"
#include "usart.h"
#include "shell_port.h"
#include "tool.h"
#include "dwin.h"

#if (USER_MODBUS_LIB)
/*Modbus作为主站时，缓冲区数据结构*/
#define BUFFER_TYPE DmaHandle

/*modebus对象选用的目标串口*/
// #define MDSLAVE1_UART huart7
// #define MDSLAVE2_UART huart3

#if defined(USING_FREERTOS)
extern void *pvPortMalloc(size_t xWantedSize);
extern void vPortFree(void *pv);
#endif

/*定义Modbus句柄*/
ModbusRTUSlaveHandler mdSlave1;
ModbusRTUSlaveHandler mdSlave2;

extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart3_rx;
extern DMA_HandleTypeDef hdma_uart5_rx;
extern DMA_HandleTypeDef hdma_usart2_rx;

#if defined(USING_FREERTOS)
extern osSemaphoreId Recive_CpuHandle;
extern osSemaphoreId Recive_Rs485Handle;
extern osSemaphoreId Recive_Rs232Handle;
extern osSemaphoreId Recive_WifiHandle;
extern osSemaphoreId Recive_LteHandle;
#endif
/*此处必须加static*/
uint8_t cpu_rx_buffer[MODBUS_PDU_SIZE_MAX];
uint32_t cpu_rx_count;
DmaHandle Modbus_Cpu = {
    .huart = &huart1,
    .phdma = &hdma_usart1_rx,
    .pRbuf = cpu_rx_buffer,
    .RxSize = MODBUS_PDU_SIZE_MAX,
    .pRxCount = &cpu_rx_count,
#if defined(USING_FREERTOS)
    .Semaphore = NULL,
#else
    .pRecive_FinishFlag = NULL,
#endif
};

DmaHandle Modbus_4G = {
    .huart = &huart2,
    .phdma = &hdma_usart2_rx,
    .pRbuf = NULL,
    .RxSize = MODBUS_PDU_SIZE_MAX,
    .pRxCount = NULL,
#if defined(USING_FREERTOS)
    .Semaphore = NULL,
#else
    .pRecive_FinishFlag = NULL,
#endif
};

DmaHandle Modbus_Wifi = {
    .huart = &huart5,
    .phdma = &hdma_uart5_rx,
    .pRbuf = NULL,
    .RxSize = MODBUS_PDU_SIZE_MAX,
    .pRxCount = NULL,
#if defined(USING_FREERTOS)
    .Semaphore = NULL,
#else
    .pRecive_FinishFlag = NULL,
#endif
};

DmaHandle Modbus_Rs485 = {
    .huart = &huart3,
    .phdma = &hdma_usart3_rx,
    .pRbuf = NULL,
    .RxSize = MODBUS_PDU_SIZE_MAX,
    .pRxCount = NULL,
#if defined(USING_FREERTOS)
    .Semaphore = NULL,
#else
    .pRecive_FinishFlag = NULL,
#endif
};

/**
 * @brief	modbus钩子函数
 * @details
 * @param   @handler 句柄
 * @param	@addr 当前寄存器地址
 * @retval	None
 */
static mdVOID mdRTUHook(ModbusRTUSlaveHandler handler, mdU16 addr)
{
    pDwinHandle pd = Dwin_Object;
    // Save_HandleTypeDef *ps = &Save_Flash;
    Save_HandleTypeDef *ps = (Save_HandleTypeDef *)pd->Slave.pHandle;
    extern uint32_t FLASH_Write(uint32_t Address, const uint16_t *pBuf, uint32_t Size);
    bool save_flag = false;

    /*处于后台参数区*/
    if ((addr >= PARAM_BASE_ADDR) && (addr < PARAM_END_ADDR))
    {
        uint8_t site = (addr - PARAM_BASE_ADDR) / 2U;
        float data = 0, temp_data = 0;

        mdSTATUS ret = mdRTU_ReadHoldRegisters(handler, addr, 2U, (mdU16 *)&data);
        if (ret == mdFALSE)
        {
#if defined(USING_DEBUG)
            shellPrint(Shell_Object, "Holding register addr[0x%x], Read: %.3f failed!\r\n", addr, data);
#endif
        }
        else
        {
            // float *pdata = (float *)pd->Slave.pHandle;
            float *pdata = (float *)&ps->Param;
            // uint8_t *ptarget = NULL;
            /*保留迪文屏幕值*/
            // temp_data = data;
            // Endian_Swap((uint8_t *)&temp_data, 0U, sizeof(float));
            if ((data >= pd->Slave.pMap[site].lower) && (data <= pd->Slave.pMap[site].upper))
            {
                temp_data = data;
                /*确认数据回传到屏幕*/
                // pd->Dw_Write(pd, pd->Slave.pMap[site].addr, (uint8_t *)&temp_data, sizeof(float));
                if (site < pd->Slave.Events_Size)
                {
                    pdata[site] = data;
                    /*存储标志*/
                    save_flag = true;
                }
            }
            else
            {
                temp_data = data < pd->Slave.pMap[site].lower ? pd->Slave.pMap[site].lower : pd->Slave.pMap[site].upper;
                /*确认数据回传到屏幕*/
                // pd->Dw_Write(pd, pd->Slave.pMap[site].addr, (uint8_t *)&pd->Slave.pMap[site].upper, sizeof(float));
                /*设置目标寄存器值为上限/下限*/
                mdRTU_WriteHoldRegs(handler, addr, 2U, (mdU16 *)&temp_data);
            }
            Endian_Swap((uint8_t *)&temp_data, 0U, sizeof(float));
            /*确认数据回传到屏幕*/
            pd->Dw_Write(pd, pd->Slave.pMap[site].addr, (uint8_t *)&temp_data, sizeof(float));
        }
#if defined(USING_DEBUG)
        shellPrint(Shell_Object, "Modbus[0x%x] received a data: %.3f.\r\n", addr, data);
#endif
    }
    /*用户名和密码处理*/
    else if (addr >= PARAM_END_ADDR)
    {
        uint16_t data = 0;
        mdSTATUS ret = mdRTU_ReadHoldRegisters(handler, addr, 2U, (mdU16 *)&data);
        if (ret == mdFALSE)
        {
#if defined(USING_DEBUG)
            shellPrint(Shell_Object, "Holding register addr[0x%x], Read: 0x%x failed!\r\n", addr, data);
#endif
        }
        else
        {
#define NUMBER_USER_MAX 9999U
            uint8_t site = addr - PARAM_BASE_ADDR;
            uint16_t *puser = (typeof(ps->Param.User_Name) *)&ps->Param;
            if (data <= NUMBER_USER_MAX)
            {
                puser[site] = data;
                /*存储标志*/
                save_flag = true;
            }
        }
#if defined(USING_DEBUG)
        shellPrint(Shell_Object, "Modbus[0x%x] received a data: %d.\r\n", addr, data);
#endif
    }
    if (save_flag)
    {
        save_flag = false;
#if defined(USING_FREERTOS)
        taskENTER_CRITICAL();
#endif
        /*计算crc校验码*/
        ps->Param.crc16 = Get_Crc16((uint8_t *)&ps->Param, sizeof(Save_Param) - sizeof(ps->Param.crc16), 0xFFFF);
        /*参数保存到Flash*/
        FLASH_Write(PARAM_SAVE_ADDRESS, (uint16_t *)&ps->Param, sizeof(Save_Param));
#if defined(USING_FREERTOS)
        taskEXIT_CRITICAL();
#endif
#if defined(USING_DEBUG)
        shellPrint(Shell_Object, "Save parameters...\r\n");
#endif
    }
}

/**
 * @brief	获取目标uart
 * @details
 * @param	None
 * @retval	None
 */
#define Get_ModbusUart(__id) ((__id == 0x02) ? &huart2 : ((__id == 0x03) ? &huart3 : &huart5))

/**
 * @brief	接口：Modbus协议栈发送底层接口
 * @details
 * @param	@c 待发送字符
 * @param   @handler 句柄
 * @retval	None
 */
static mdVOID popchar(ModbusRTUSlaveHandler handler, mdU8 *data, mdU32 length)
{
    // UART_HandleTypeDef *pHuart = (handler->slaveId == SLAVE1_ID) ? &MDSLAVE1_UART : &MDSLAVE2_UART;
    UART_HandleTypeDef *pHuart; //= Get_ModbusUart(handler->uartId);
    switch (handler->uartId)
    {
    case 0x01:
    {
        pHuart = &huart1;
    }
    break;
    case 0x02:
    {
        pHuart = &huart2;
    }
    break;
    case 0x03:
    {
        pHuart = &huart3;
    }
    break;
    case 0x05:
    {
        pHuart = &huart5;
    }
    break;
    default:
        pHuart = NULL;
        break;
    }
/*启用DMA发送一包数据*/
#if (USING_DMA_TRANSPORT)
    if (pHuart)
    {
        HAL_UART_Transmit_DMA(pHuart, data, length);
        while (__HAL_UART_GET_FLAG(pHuart, UART_FLAG_TC) == RESET)
        {
        }
#else
    HAL_UART_Transmit(pHuart, data, length, 0xFFFF);
#endif
    }
}

/*
    ModbusInit
        @void
    接口：初始化Modbus协议栈
*/
mdAPI mdVOID MX_ModbusInit(mdVOID)
{
#define USER_TYPE Slave_IRQTableTypeDef
    struct ModbusRTUSlaveRegisterInfo info = {
        .slaveId = SLAVE1_ID,
        .usartBaudRate = SLAVE1_BUAD_RATE,
        .mdRTUPopChar = popchar,
        .puser = &IRQ_Table,
    };
    /*Creation failed*/
    if (mdCreateModbusRTUSlave(&Slave1_Object, info) != mdTRUE)
    {
#if defined(USING_DEBUG)
        // shellPrint(Shell_Object,"AD[%d] = 0x%d\r\n", 0, Get_AdcValue(ADC_CHANNEL_1));
#endif
    }
    /*初始化接收缓冲区*/
    // Modbus_Cpu.pRbuf = mdRTU_Recive_Buf(Slave1_Object);
    // Modbus_Cpu.pRxCount = (uint32_t *)&mdRTU_Recive_Len(Slave1_Object);
    Modbus_Cpu.Semaphore = Recive_CpuHandle;
    Modbus_4G.pRbuf = mdRTU_Recive_Buf(Slave1_Object);
    Modbus_4G.pRxCount = (uint32_t *)&mdRTU_Recive_Len(Slave1_Object);
    Modbus_4G.Semaphore = Recive_LteHandle;
    Modbus_Wifi.pRbuf = mdRTU_Recive_Buf(Slave1_Object);
    Modbus_Wifi.pRxCount = (uint32_t *)&mdRTU_Recive_Len(Slave1_Object);
    Modbus_Wifi.Semaphore = Recive_WifiHandle;
    info.slaveId = SLAVE2_ID;
    info.usartBaudRate = SLAVE2_BUAD_RATE;
    if (mdCreateModbusRTUSlave(&Slave2_Object, info) != mdTRUE)
    {
#if defined(USING_DEBUG)
        // shellPrint(Shell_Object,"AD[%d] = 0x%d\r\n", 0, Get_AdcValue(ADC_CHANNEL_1));
#endif
    }
    Modbus_Rs485.pRbuf = mdRTU_Recive_Buf(Slave2_Object);
    Modbus_Rs485.pRxCount = (uint32_t *)&mdRTU_Recive_Len(Slave2_Object);
    Modbus_Rs485.Semaphore = Recive_Rs485Handle;
}

#if (!USING_DMA_TRANSPORT)
/*
    portRtuPushChar
        @handler 句柄
        @c 待接收字符
        @return
    接口：接收一个字符
*/
static mdVOID
portRtuPushChar(ModbusRTUSlaveHandler handler, mdU8 c)
{
    ReceiveBufferHandle recbuf = handler->receiveBuffer;
    recbuf->buf[recbuf->count++] = c;
}

/*
    portRtuPushString
        @handler 句柄
        @*data   数据
        @length  数据长度
    接口：接收一个字符串
*/
static mdVOID portRtuPushString(ModbusRTUSlaveHandler handler, mdU8 *data, mdU32 length)
{
    ReceiveBufferHandle recbuf = handler->receiveBuffer;
    memcpy(&recbuf->buf[0], data, length);
    recbuf->count = length;
}
#endif

/*
    mdRTUSendString
        @handler 句柄
        @*data   数据缓冲区
        @length  数据长度
        @return
    接口：发送一帧数据
*/
static mdVOID mdRTUSendString(ModbusRTUSlaveHandler handler, mdU8 *data, mdU32 length)
{
    if (handler)
    {
        handler->mdRTUPopChar(handler, data, length);
    }
}

/*
    mdRTUError
        @handler 句柄
        @error   错误码
        @return
    接口：Modbus协议栈出错处理
*/
static mdVOID mdRTUError(ModbusRTUSlaveHandler handler, mdU8 error)
{
}

#if defined(USING_MASTER)

// typedef struct
// {
//     mdU8 Conter;
//     mdU8 *pBuf;
// } mdRTUMaster;
// mdRTUMaster g_Master;

/**
 * @brief  带CRC的发送从站数据
 * @param  _pBuf 数据缓冲区指针
 * @param  _ucLen 数据长度
 * @retval None
 */
// mdVOID mdRTU_SendWithCRC(mdU8 *_pBuf, mdU8 _ucLen)
// {
// #define MOD_TX_BUF_SIZE 64U

//     uint16_t crc;
//     uint8_t buf[MOD_TX_BUF_SIZE];

//     memcpy(buf, _pBuf, _ucLen);
//     crc = mdCrc16(_pBuf, _ucLen);
//     buf[_ucLen++] = crc;
//     buf[_ucLen++] = crc >> 8;

//     HAL_UART_Transmit(&MDSLAVE1_UART, buf, _ucLen, 0xffff);
// }

/**
 * @brief  Modbus作为主站请求一些特定信息
 * @param  handler modbus句柄
 * @param  slave_id 目标从站id
 * @retval None
 */
mdVOID mdRTUHandleCode11(ModbusRTUSlaveHandler handler, mdU8 slave_id)
{
    mdU8 data[4U] = {slave_id, 0x11};
    mdU16 crc16 = 0U;

    crc16 = mdCrc16(data, 2U);
    memcpy(&data[2U], &crc16, sizeof(crc16));
    handler->mdRTUPopChar(handler, data, sizeof(data));
}

mdVOID mdRTU_MasterCodex(ModbusRTUSlaveHandler handler, mdU8 fun_code, mdU8 slave_id, mdU8 *pdata, mdU8 len)
{
#define DATA_MAX_SIZE ((CARD_SIGNAL_MAX * 4U) + 9U) //始终以最大所需数据定义内存空间
    mdU8 data_size = 0, *pdest = NULL, *pBuf = NULL;
    // mdU8 read_data[] = {slave_id, fun_code, 0x00, 0x00, 0x00, 0x08};
    mdU8 read_data[] = {slave_id, fun_code, 0x00, 0x00, len / 0xFF, (len < CARD_SIGNAL_MAX ? 8U : len % 0xFF)};
    mdU8 write_data[DATA_MAX_SIZE] = {slave_id, fun_code, 0x00, 0x00, 0x00, 0x08, 0x01, 0x00, 0x00, 0x00, 0x00};
    // mdU8 write_analog[(CARD_SIGNAL_MAX * 4U) + 9U] = {slave_id, fun_code, 0x00, 0x00, (len / 2U) / 0xFF,
    //                                                   (len / 2U) % 0xFF, len};
    mdU16 crc16;

    switch (fun_code)
    {
    /*读离散输入*/
    case MODBUS_CODE_2:
    {
        data_size = sizeof(read_data);
        pdest = read_data;
    }
    break;
    /*读输入寄存器*/
    case MODBUS_CODE_4:
    {
        data_size = sizeof(read_data);
        // read_data[5U] *= 2U;
        pdest = read_data;
    }
    break;
    /*读板卡*/
    case MODBUS_CODE_17:
    {
        data_size = 2U;
        pdest = read_data;
    }
    break;
    /*写多个线圈*/
    case MODBUS_CODE_15:
    {
        data_size = (len == 0x01) ? 8U : 11U;

        if (pdata && (len < data_size))
        {
            memcpy(&write_data[7U], pdata, len);
        }
        pdest = write_data;
    }
    break;
    /*写多个保持寄存器*/
    case MODBUS_CODE_16:
    {
        data_size = sizeof(write_data);
        if (pdata && (len < data_size))
        {
            write_data[4U] = (len / 2U) / 0xFF, write_data[5U] = (len / 2U) % 0xFF, write_data[6U] = len;
            memcpy(&write_data[7U], pdata, len);
        }
        pdest = write_data;
    }
    break;
    default:
        data_size = 0;
        break;
    }
    if (data_size)
    {
#if defined(USING_FREERTOS)
        pBuf = (mdU8 *)CUSTOM_MALLOC(data_size + sizeof(crc16));
        if (!pBuf)
            goto __exit;
#endif
        if (pdest)
        {
            memcpy(pBuf, pdest, data_size);
            crc16 = mdCrc16(pBuf, data_size);
            memcpy(&pBuf[data_size], &crc16, sizeof(crc16));
            handler->mdRTUPopChar(handler, pBuf, data_size + sizeof(crc16));
        }
    }

__exit:
#if defined(USING_FREERTOS)
    CUSTOM_FREE(pBuf);
#endif
}

/**
 * @brief  有人云自定义46指令
 * @param  slaveaddr 从站地址
 * @param  regaddr 寄存器开始地址
 * @param  reglength 寄存器长度
 * @param  dat 数据
 * @retval None
 */
mdVOID mdRTUHandleCode46(ModbusRTUSlaveHandler handler, mdU16 regaddr, mdU16 reglen, mdU8 datalen, mdU8 *dat)
{
    mdU8 frame_head[] = {handler->slaveId, 0x46, regaddr >> 8U, regaddr, reglen >> 8U, reglen,
                         datalen};
    mdU16 counter = sizeof(frame_head);
    mdU16 crc16 = 0U;
#if defined(USING_FREERTOS)
    mdU8 *pBuf = (mdU8 *)CUSTOM_MALLOC(counter + datalen + sizeof(crc16));
    if (!pBuf)
        goto __exit;
#else
    mdU8 pBuf[MODBUS_PDU_SIZE_MAX];
    memset(pBuf, 0x00, MODBUS_PDU_SIZE_MAX);
#endif
    memcpy(pBuf, frame_head, counter);
    /*发送数据拷贝到发送缓冲区*/
    memcpy(&pBuf[counter], dat, datalen);
    counter += datalen;
    crc16 = mdCrc16(pBuf, counter);
    memcpy(&pBuf[counter], &crc16, sizeof(crc16));
    counter += sizeof(crc16);

    handler->mdRTUPopChar(handler, pBuf, counter);
__exit:
#if defined(USING_FREERTOS)
    CUSTOM_FREE(pBuf);
#endif
}

#endif

/*
    mdRTUHandleCode1
        @handler 句柄
        @return
    接口：解析01功能码
*/
static mdVOID mdRTUHandleCode1(ModbusRTUSlaveHandler handler)
{
    //    mdU32 reclen = handler->receiveBuffer->count;
    mdU8 *recbuf = handler->receiveBuffer->buf;
    RegisterPoolHandle regPool = handler->registerPool;
    mdU16 startAddress = ToU16(recbuf[2], recbuf[3]);
    mdU16 length = ToU16(recbuf[4], recbuf[5]);
    mdBit *data;
    mdU8 *data2;
    mdU8 length2 = 0;
    mdU16 crc;

    mdmalloc(data, mdBit, length);
    regPool->mdReadCoils(regPool, startAddress, length, data);
    length2 = length % 8 > 0 ? length / 8 + 1 : length / 8;
    mdmalloc(data2, mdU8, 5 + length2);
    data2[0] = recbuf[0];
    data2[1] = recbuf[1];
    data2[2] = length2;
    for (mdU32 i = 0; i < length2; i++)
    {
        for (mdU32 j = 0; j < 8 && (i * 8 + j) < length; j++)
        {
            data2[3 + i] |= ((data[i * 8 + j] & 0x01) << j);
        }
    }
    crc = mdCrc16(data2, 3 + length2);
    /*注意CRC顺序*/
    data2[3 + length2] = LOW(crc);
    data2[4 + length2] = HIGH(crc);
    handler->mdRTUSendString(handler, data2, 5 + length2);
    mdfree(data);
    mdfree(data2);
}

static mdVOID mdRTUHandleCode2(ModbusRTUSlaveHandler handler)
{
    mdU8 *recbuf = handler->receiveBuffer->buf;
    RegisterPoolHandle regPool = handler->registerPool;
    mdU16 startAddress = ToU16(recbuf[2], recbuf[3]);
    mdBit *data;
    mdU16 length = ToU16(recbuf[4], recbuf[5]);
    mdU8 *data2;
    mdU8 length2 = 0;
    mdU16 crc;

    mdmalloc(data, mdBit, length);
    regPool->mdReadInputCoils(regPool, startAddress, length, data);
    length2 = length % 8 > 0 ? length / 8 + 1 : length / 8;
    mdmalloc(data2, mdU8, 5 + length2);
    data2[0] = recbuf[0];
    data2[1] = recbuf[1];
    data2[2] = length2;
    for (mdU32 i = 0; i < length2; i++)
    {
        for (mdU32 j = 0; j < 8 && (i * 8 + j) < length; j++)
        {
            data2[3 + i] |= ((data[i * 8 + j] & 0x01) << j);
        }
    }
    crc = mdCrc16(data2, 3 + length2);
    data2[3 + length2] = LOW(crc);
    data2[4 + length2] = HIGH(crc);
    handler->mdRTUSendString(handler, data2, 5 + length2);
    mdfree(data);
    mdfree(data2);
}

static mdVOID mdRTUHandleCode3(ModbusRTUSlaveHandler handler)
{
    mdU8 *recbuf = handler->receiveBuffer->buf;
    RegisterPoolHandle regPool = handler->registerPool;
    mdU16 startAddress = ToU16(recbuf[2], recbuf[3]);
    mdU16 *data;
    mdU16 length = ToU16(recbuf[4], recbuf[5]);
    mdU8 *data2;
    mdU16 crc;

    mdmalloc(data, mdU16, length);
    regPool->mdReadHoldRegisters(regPool, startAddress, length, data);
    /*此处为了解决由于ARM小端存储造成的半字顺序混乱问题*/
    if (length > sizeof(mdU8))
    { /*只针对2个及以上寄存器而言*/
        mdU16Swap(data, length);
    }

    mdmalloc(data2, mdU8, 5 + length * 2);
    data2[0] = recbuf[0];
    data2[1] = recbuf[1];
    data2[2] = (mdU8)(length * 2);
    for (mdU32 i = 0; i < length; i++)
    {
        data2[3 + 2 * i] = HIGH(data[i]);
        data2[3 + 2 * i + 1] = LOW(data[i]);
    }
    crc = mdCrc16(data2, 3 + length * 2);
    data2[3 + length * 2] = LOW(crc);
    data2[4 + length * 2] = HIGH(crc);
    handler->mdRTUSendString(handler, data2, 5 + length * 2);
    mdfree(data);
    mdfree(data2);
}

static mdVOID mdRTUHandleCode4(ModbusRTUSlaveHandler handler)
{
    mdU8 *recbuf = handler->receiveBuffer->buf;
    RegisterPoolHandle regPool = handler->registerPool;
    mdU16 startAddress = ToU16(recbuf[2], recbuf[3]);
    mdU16 *data;
    mdU16 length = ToU16(recbuf[4], recbuf[5]);
    mdU8 *data2;
    mdU16 crc;

    mdmalloc(data, mdU16, length);
    regPool->mdReadInputRegisters(regPool, startAddress, length, data);
    /*此处为了解决由于ARM小端存储造成的半字顺序混乱问题*/
    if (length > sizeof(mdU8))
    { /*只针对2个及以上寄存器而言*/
        mdU16Swap(data, length);
    }
    mdmalloc(data2, mdU8, 5 + length * 2);
    data2[0] = recbuf[0];
    data2[1] = recbuf[1];
    data2[2] = (mdU8)(length * 2);
    for (mdU32 i = 0; i < length; i++)
    {
        data2[3 + 2 * i] = HIGH(data[i]);
        data2[3 + 2 * i + 1] = LOW(data[i]);
    }
    crc = mdCrc16(data2, 3 + length * 2);
    data2[3 + length * 2] = LOW(crc);
    data2[4 + length * 2] = HIGH(crc);
    handler->mdRTUSendString(handler, data2, 5 + length * 2);
    mdfree(data);
    mdfree(data2);
}

static mdVOID mdRTUHandleCode5(ModbusRTUSlaveHandler handler)
{
    mdU32 reclen = handler->receiveBuffer->count;
    mdU8 *recbuf = handler->receiveBuffer->buf;
    RegisterPoolHandle regPool = handler->registerPool;
    mdU16 startAddress = ToU16(recbuf[2], recbuf[3]);
    mdBit data = ToU16(recbuf[4], recbuf[5]) > 0 ? mdHigh : mdLow;
    regPool->mdWriteCoil(regPool, startAddress, data);
    handler->mdRTUSendString(handler, recbuf, reclen);
}

static mdVOID mdRTUHandleCode6(ModbusRTUSlaveHandler handler)
{
    mdU32 reclen = handler->receiveBuffer->count;
    mdU8 *recbuf = handler->receiveBuffer->buf;
    RegisterPoolHandle regPool = handler->registerPool;
    mdU16 startAddress = ToU16(recbuf[2], recbuf[3]);
    mdU16 data = ToU16(recbuf[4], recbuf[5]);
    regPool->mdWriteHoldRegister(regPool, startAddress, data);
    handler->mdRTUSendString(handler, recbuf, reclen);
}

static mdVOID mdRTUHandleCode15(ModbusRTUSlaveHandler handler)
{
    mdU8 *recbuf = handler->receiveBuffer->buf;
    RegisterPoolHandle regPool = handler->registerPool;
    mdU16 startAddress = ToU16(recbuf[2], recbuf[3]);
    mdU16 length = ToU16(recbuf[4], recbuf[5]);
    mdU8 *data;
    mdU16 crc;
    for (mdU32 i = 0; i < length; i++)
    {
        regPool->mdWriteCoil(regPool, startAddress + i, ((recbuf[7 + i / 8] >> (i % 8)) & 0x01));
    }
    mdmalloc(data, mdU8, 8);
    memcpy(data, recbuf, 6);
    crc = mdCrc16(data, 6);
    data[6] = LOW(crc);
    data[7] = HIGH(crc);
    handler->mdRTUSendString(handler, data, 8);
    mdfree(data);
}

static mdVOID mdRTUHandleCode16(ModbusRTUSlaveHandler handler)
{
    mdU8 *recbuf = handler->receiveBuffer->buf;
    RegisterPoolHandle regPool = handler->registerPool;
    mdU16 startAddress = ToU16(recbuf[2], recbuf[3]);
    mdU16 length = ToU16(recbuf[4], recbuf[5]);
    mdU8 *data;
    mdU16 crc;
    for (mdU32 i = 0; i < length; i++)
    {
        regPool->mdWriteHoldRegister(regPool, startAddress + i,
                                     ToU16(recbuf[7 + (length - 2 * i)], recbuf[7 + (length - 2 * i) + 1]));
    }
    mdmalloc(data, mdU8, 8);
    memcpy(data, recbuf, 6);
    crc = mdCrc16(data, 6);
    data[6] = LOW(crc);
    data[7] = HIGH(crc);
    handler->mdRTUSendString(handler, data, 8);
    mdfree(data);
}

#define MOD_WORD 1U
#define MOD_DWORD 2U
/*获取主机号*/
#define Get_ModId(__obj) ((__obj)->pRbuf[0U])
/*获取Modbus功能号*/
#define Get_ModFunCode(__obj) ((__obj)->pRbuf[1U])
/*获取Modbus协议数据*/
#define Get_Data(__ptr, __s, __size)                                           \
    ((__size) < 2U                                                             \
         ? (((__ptr)->pRbuf[__s] << 8U) | ((__ptr)->pRbuf[__s + 1U]))          \
         : (((__ptr)->pRbuf[__s] << 24U) | ((__ptr)->pRbuf[__s + 1U] << 16U) | \
            ((__ptr)->pRbuf[__s + 2U] << 8U) | ((__ptr)->pRbuf[__s + 3U])))

#define Get_MasterCrc(__buf, __count) \
    (ToU16((__buf)[(__count)-1U], (__buf)[(__count)-2U]))
/**
 * @brief  Modbus作主机时事件处理
 * @param  handler modbus句柄
 * @retval None
 */
static mdVOID portRtuMasterHandle(ModbusRTUSlaveHandler handler, uint8_t target_slaveid)
{
    pDmaHandle p = &Modbus_Cpu;
    Save_HandleTypeDef *ps = &Save_Flash;
    uint32_t rx_count = *p->pRxCount;
    USER_TYPE *ptable = (USER_TYPE *)handler->puser;
    mdU16 startAddress = 0;
    mdU8 length = 0;
    typeof(ptable->pIRQ) p_target = NULL;

    if (rx_count < 3U)
    {
        handler->mdRTUError(handler, ERROR2);
        return;
    }
    if ((mdCrc16(p->pRbuf, rx_count - 2) != Get_MasterCrc(p->pRbuf, rx_count)) && CRC_CHECK != 0)
    {
        handler->mdRTUError(handler, ERROR3);
        return;
    }
    /*首次识别板卡时核对id号，后面再次上电后不需要核对*/
    if ((ps->Param.Slave_IRQ_Table.IRQ_Table_SetFlag != SAVE_SURE_CODE) && (Get_ModId(p) != target_slaveid))
    {
        handler->mdRTUError(handler, ERROR4);
        return;
    }

    switch (Get_ModFunCode(p))
    {
        /*根据板卡信息编码中断表*/
    case MODBUS_CODE_17:
    { /*如果中断信息表并未记录到flash，则进行板卡检测*/
        if (ps->Param.Slave_IRQ_Table.IRQ_Table_SetFlag != SAVE_SURE_CODE)
        {
            IRQ_Coding(ptable, p->pRbuf[3U]);
        }
        /*重新上电后检测每个卡槽位板卡信息是否与记录信息匹配*/
        // else
        // {
        // }

        // #if defined(USING_DEBUG)
        //         shellPrint(Shell_Object, "@Note: The board is coded as 0x%02x.\r\n",
        //                    p->pRbuf[3U]);
        // #endif
    }
    break;
    /*读回的线圈状态写会主CPU寄存器组*/
    case MODBUS_CODE_1:
    { /*寄存器地址与从机地址偏移*/
        // startAddress = CARD_SIGNAL_MAX * ptable->pIRQ->SlaveId;
        // length = p->pRbuf[2];
        // for (mdU8 i = 0; i < length; i++)
        // {
        //     handler->registerPool->mdWriteCoil(handler->registerPool, startAddress + i, ((p->pRbuf[3 + i / 8U] >> (i % 8U)) & 0x01));
        // }
        // handler->registerPool->mdWriteCoils(handler->registerPool, startAddress, length, p->pRbuf[7]);
    }
    break;
        /*读回的输入线圈状态写回主CPU寄存器组*/
    case MODBUS_CODE_2:
    {
        p_target = Find_TargetSlave_AdoptId(ptable, Get_ModId(p));
        if (!p_target)
        {
#if defined(USING_DEBUG)
            shellPrint(Shell_Object, "Error: The target board is not in the interrupt table." __INFORMATION());
#endif
            return;
        }
        length = p->pRbuf[2];
        startAddress = length * CARD_SIGNAL_MAX * p_target->Number;
        mdU8 i = 0, j = 0;
        for (i = 0; i < length; ++i)
        {
            for (j = 0; j < CARD_SIGNAL_MAX; ++j)
                mdRTU_WriteInputCoil(handler, startAddress + i * CARD_SIGNAL_MAX + j,
                                     ((p->pRbuf[3U + i] >> (j % 8U)) & 0x01));
        }
    }
    break;
    /*读回的保持寄存器写回主CPU寄存器组*/
    case MODBUS_CODE_3:
    {
    }
    break;
    /*读回的输入寄存器状态写回主CPU寄存器组*/
    case MODBUS_CODE_4:
    {
        p_target = Find_TargetSlave_AdoptId(ptable, Get_ModId(p));
        if (!p_target)
        {
#if defined(USING_DEBUG)
            shellPrint(Shell_Object, "Error: The target board is not in the interrupt table." __INFORMATION());
#endif
            return;
        }
        startAddress = CARD_SIGNAL_MAX * 2U * p_target->Number;
        length = p->pRbuf[2] / 2U;
        mdRTU_WriteInputRegisters(handler, startAddress, length, (mdU16 *)&p->pRbuf[3]);
    }
    break;
    default:
        break;
    }
    mdClearReceiveBuffer(handler->receiveBuffer);
}

/*
    mdRtuBaseTimerTick
        @handler 句柄
        @time   时长跨度，单位 us
        @return
    接口：帧间隙监控
*/
static mdVOID portRtuTimerTick(ModbusRTUSlaveHandler handler, mdU32 ustime)
{
    // static mdU32 lastCount;
    // static mdU64 timeSum;
    // static mdFMStatus error;
    // if (handler->receiveBuffer->count > 0)
    // {
    //     if (handler->receiveBuffer->count != lastCount)
    //     {
    //         if (timeSum > handler->invalidTime)
    //         {
    //             error++;
    //         }
    //         lastCount = handler->receiveBuffer->count;
    //         timeSum = 0;
    //     }
    //     if (timeSum > handler->stopTime)
    //     {
    //         if (error == 0 || IGNORE_LOSS_FRAME != 0)
    //         { /*发生主机对从机寄存器写数据操作*/
    //             handler->updateFlag = true;
    //             handler->mdRTUCenterProcessor(handler);
    //         }
    //         else
    //         {
    //             handler->mdRTUError(handler, ERROR1);
    //         }
    //         mdClearReceiveBuffer(handler->receiveBuffer);
    //         TIMER_CLEAN();
    //     }
    //     timeSum += ustime;
    // }
    // else
    // {
    //     TIMER_CLEAN();
    // }
    /*发生主机对从机寄存器写数据操作*/
    // handler->updateFlag = true;
    handler->mdRTUCenterProcessor(handler);
    mdClearReceiveBuffer(handler->receiveBuffer);
}

/*
    mdModbusRTUCenterProcessor
        @handler 句柄
        @receFrame 待处理的帧（已校验通过）
    处理一帧，并且通过接口发送处理结果
*/
static mdVOID mdRTUCenterProcessor(ModbusRTUSlaveHandler handler)
{
    mdU32 reclen = handler->receiveBuffer->count;
    mdU8 *recbuf = handler->receiveBuffer->buf;
    if (reclen < 3)
    {
        handler->mdRTUError(handler, ERROR2);
        return;
    }
    if (mdCrc16(recbuf, reclen - 2) != mdGetCrc16() && CRC_CHECK != 0)
    {
        handler->mdRTUError(handler, ERROR3);
        return;
    }
    if (mdGetSlaveId() != handler->slaveId)
    {
        handler->mdRTUError(handler, ERROR4);
        return;
    }
    switch (mdGetCode())
    {
    case MODBUS_CODE_1:
        handler->mdRTUHandleCode1(handler);
        break;
    case MODBUS_CODE_2:
        handler->mdRTUHandleCode2(handler);
        break;
    case MODBUS_CODE_3:
        handler->mdRTUHandleCode3(handler);
        break;
    case MODBUS_CODE_4:
        handler->mdRTUHandleCode4(handler);
        break;
    case MODBUS_CODE_5:
        handler->mdRTUHandleCode5(handler);
        break;
    case MODBUS_CODE_6:
        handler->mdRTUHandleCode6(handler);
        /*更改用户名和密码*/
        handler->mdRTUHook(handler, ToU16(recbuf[2], recbuf[3]));
        break;
    case MODBUS_CODE_15:
        handler->mdRTUHandleCode15(handler);
        break;
    case MODBUS_CODE_16:
        handler->mdRTUHandleCode16(handler);
        /*更新屏幕后台参数*/
        handler->mdRTUHook(handler, ToU16(recbuf[2], recbuf[3]));
        break;
    default:
        handler->mdRTUError(handler, ERROR5);
        break;
    }
}

/* ================================================================== */
/*                        API                                         */
/* ================================================================== */
/*
    mdCreateModbusRTUSlave
        @handler 句柄
        @mdRtuPopChar 字符发送函数
    创建一个modbus从机
*/
mdSTATUS mdCreateModbusRTUSlave(ModbusRTUSlaveHandler *handler, struct ModbusRTUSlaveRegisterInfo info)
{
#if defined(USING_FREERTOS)
    (*handler) = (ModbusRTUSlaveHandler)CUSTOM_MALLOC(sizeof(struct ModbusRTUSlave));
#else
    (*handler) = (ModbusRTUSlaveHandler)malloc(sizeof(struct ModbusRTUSlave));
#endif
#if defined(USING_DEBUG)
    shellPrint(Shell_Object, "Slave[%d]_handler = 0x%p\r\n", info.slaveId, *handler);
#endif
    if ((*handler) != NULL)
    {
        (*handler)->mdRTUHook = mdRTUHook;
        (*handler)->mdRTUPopChar = info.mdRTUPopChar;
        (*handler)->mdRTUCenterProcessor = mdRTUCenterProcessor;
        (*handler)->mdRTUError = mdRTUError;
        (*handler)->slaveId = info.slaveId;
        (*handler)->puser = info.puser;
        (*handler)->invalidTime = (int)(1.5 * DATA_BITS * 1000 * 1000 / info.usartBaudRate);
        (*handler)->stopTime = (int)(3.5 * DATA_BITS * 1000 * 1000 / info.usartBaudRate);
        (*handler)->uartId = 0x00;
#if (!USING_DMA_TRANSPORT)
        (*handler)->portRTUPushChar = portRtuPushChar;
        (*handler)->portRTUPushString = portRtuPushString;
#else
        (*handler)->portRTUPushChar = NULL;
        (*handler)->portRTUPushString = NULL;
#endif
        (*handler)->portRTUTimerTick = portRtuTimerTick;
        (*handler)->portRTUMasterHandle = portRtuMasterHandle;
        (*handler)->mdRTU_MasterCodex = mdRTU_MasterCodex;
        (*handler)->mdRTUSendString = mdRTUSendString;
        (*handler)->mdRTUHandleCode1 = mdRTUHandleCode1;
        (*handler)->mdRTUHandleCode2 = mdRTUHandleCode2;
        (*handler)->mdRTUHandleCode3 = mdRTUHandleCode3;
        (*handler)->mdRTUHandleCode4 = mdRTUHandleCode4;
        (*handler)->mdRTUHandleCode5 = mdRTUHandleCode5;
        (*handler)->mdRTUHandleCode6 = mdRTUHandleCode6;
        (*handler)->mdRTUHandleCode15 = mdRTUHandleCode15;
        (*handler)->mdRTUHandleCode16 = mdRTUHandleCode16;
        (*handler)->mdRTUHandleCode46 = mdRTUHandleCode46;
        // (*handler)->mdRTUHandleCode11 = mdRTUHandleCode11;

        mdSTATUS ret1 = mdCreateRegisterPool(&((*handler)->registerPool));
        mdSTATUS ret2 = mdCreateReceiveBuffer(&((*handler)->receiveBuffer));
        if (ret1 && ret2)
        {
            return mdTRUE;
        }
        else
        {
#if defined(USING_DEBUG)
            shellPrint(Shell_Object, "Cpool = %d, Crec = %d\r\n", ret1, ret2);
#endif
#if defined(USING_FREERTOS)
            CUSTOM_FREE((*handler));
#else
            free((*handler));
#endif
        }
    }
    return mdFALSE;
}

/*
    mdDestoryModbusRTUSlave
        @handler 句柄
    销毁一个modbus从机
*/
mdVOID mdDestoryModbusRTUSlave(ModbusRTUSlaveHandler *handler)
{
    mdDestoryRegisterPool(&((*handler)->registerPool));
    mdDestoryReceiveBuffer(&((*handler)->receiveBuffer));
    mdfree(*handler);
    (*handler) = NULL;
}

/*
    mdU16Swap
        @*data   交换的缓冲区
        @*length 长度
    交换一个mdU16缓冲区中相邻两个元素
*/
mdVOID mdU16Swap(mdU16 *data, mdU32 length)
{
    mdU16 temp = 0;

    for (mdU16 *p = &data[0]; p < &data[0] + length; p += 2)
    {
        temp = *p;
        *p = *(p + 1);
        *(p + 1) = temp;
    }
}

#endif
