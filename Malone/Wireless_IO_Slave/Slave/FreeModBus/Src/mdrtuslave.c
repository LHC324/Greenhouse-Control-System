#include <stdlib.h>
#include <string.h>
#include "mdrtuslave.h"
#include "mdcrc16.h"
#include "usart.h"
#include "shell_port.h"
#include "cmsis_os.h"

#if defined(USING_FREERTOS)
extern void *pvPortMalloc(size_t xWantedSize);
extern void vPortFree(void *pv);
#endif

#if (USER_MODBUS_LIB)
/* ================================================================== */
/*                        接口区                                       */
/* ================================================================== */
ModbusRTUSlaveHandler mdhandler;

/*
    popchar
        @handler 句柄
        @c 待发送字符
    接口：Modbus协议栈发送底层接口
*/
static mdVOID popchar(ModbusRTUSlaveHandler handler, mdU8 *data, mdU32 length)
{
/*启用DMA发送一包数据*/
#if (USING_DMA_TRANSPORT)
    HAL_UART_Transmit_DMA(&huart3, data, length);
    while (__HAL_UART_GET_FLAG(&huart3, UART_FLAG_TC) == RESET)
    {
    }
#else
    for (mdU32 i = 0; i < length; i++)
    {
        handler->mdRTUPopChar(handler, data[i]);
    }
#endif
}

/*
    portRtuPushChar
        @handler 句柄
        @c 待接收字符
        @return
    接口：接收一个字符
*/
static mdVOID portRtuPushChar(ModbusRTUSlaveHandler handler, mdU8 c)
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
    popchar(handler, data, length);
}

/*
    ModbusInit
        @void
    接口：初始化Modbus协议栈
*/
void ModbusInit(void)
{
    struct ModbusRTUSlaveRegisterInfo info;
    info.slaveId = SLAVE_ID;
    info.usartBaudRate = BUAD_RATE;
    info.mdRTUPopChar = popchar;
    mdCreateModbusRTUSlave(&mdhandler, info);
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

/*
    modbusRTU_Handler
        @void
    接口：100us定时器回调函数
*/
// void inline mdRTU_Handler(void)
// {
// // #if defined(USING_DEBUG)
// //     shellPrint(&shell,"tick = 0x%p\r\n", portRtuTimerTick);
// // #endif
//     mdhandler->portRTUTimerTick(mdhandler, TIMER_UTIME);

// }

#if !defined(AS_REPEATER)
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
    mdU8 temp_buf[16U] = {MASTER_ID, MASTER_ID, MASTER_ID};
    memcpy(&temp_buf[3U], recbuf, reclen);
    RegisterPoolHandle regPool = handler->registerPool;
    mdU16 startAddress = ToU16(recbuf[2], recbuf[3]);
    mdBit data = ToU16(recbuf[4], recbuf[5]) > 0 ? mdHigh : mdLow;
    regPool->mdWriteCoil(regPool, startAddress, data);
    handler->mdRTUSendString(handler, temp_buf, reclen + 3U);
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
                                     ToU16(recbuf[7 + 2 * i], recbuf[7 + 2 * i + 1]));
    }
    mdmalloc(data, mdU8, 8);
    memcpy(data, recbuf, 6);
    crc = mdCrc16(data, 6);
    data[6] = LOW(crc);
    data[7] = HIGH(crc);
    handler->mdRTUSendString(handler, data, 8);
    mdfree(data);
}
#endif

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
    // if (handler->receiveBuffer->count != lastCount)
    // {
    //     if (timeSum > handler->invalidTime)
    //     {
    //         error++;
    //     }
    //     lastCount = handler->receiveBuffer->count;
    //     timeSum = 0;
    // }
    // if(timeSum > handler->stopTime)
    // {
    // if(error == 0 || IGNORE_LOSS_FRAME != 0)
    // {   /*发生主机对从机寄存器写数据操作*/
    // handler->updateFlag = true;
    // handler->mdRTUCenterProcessor(handler);
    // }
    // else
    // {
    //     handler->mdRTUError(handler, ERROR1);
    // }
    // mdClearReceiveBuffer(handler->receiveBuffer);
    // TIMER_CLEAN();
    // }
    // timeSum += ustime;
    // }
    // else
    // {
    // TIMER_CLEAN();
    // }

    ReceiveBufferHandle pBuf = handler->receiveBuffer;

    if (pBuf->count > 0)
    {
        handler->mdRTUCenterProcessor(handler);
    }
    mdClearReceiveBuffer(handler->receiveBuffer);
}

#if defined(USING_REPEATER_MODE)
/**
 * @brief	获取Lora模块当前是否空闲
 * @details	无线发送数据时拉低，用于指示发送繁忙状态
 * @param	None
 * @retval	ture/fale
 */
extern bool inline Get_L101_Status(void);
#endif

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
#if defined(USING_DEBUG)
    shellPrint(&shell, "gcrc = 0x%04x, crc = 0x%04x\r\n", mdCrc16(recbuf, reclen - 2), mdGetCrc16());
#endif
#if defined(AS_REPEATER)
    uint8_t direction = mdGetSlaveId() & 0x80;

    uint8_t tx_size = reclen + 3U, *pdata = CUSTOM_MALLOC(tx_size);

    if (!pdata)
        goto __exit;
    memset(pdata, 0x00, tx_size);
    /*A->B*/
    if (direction)
    { /*去掉方向*/
        recbuf[0] &= 0x7F;
        /*地址和信道相同:与从机id号不同，偏移1位，中继器占用*/
        pdata[1] = pdata[2] = recbuf[0] + 1U;
    }
    memcpy(&pdata[3U], recbuf, reclen);
    /*B->A:全部为0*/
    while (!Get_L101_Status())
    {
        osDelay(1);
    }
    handler->mdRTUSendString(handler, pdata, tx_size);
	HAL_GPIO_TogglePin(RELAY_GPIO_Port, RELAY_Pin);
__exit:
    CUSTOM_FREE(pdata);
#else
    if (mdCrc16(recbuf, reclen - 2U) != mdGetCrc16() && (CRC_CHECK != 0))
    {
        handler->mdRTUError(handler, ERROR3);
        return;
    }
#if defined(USING_REPEATER_MODE)
    if (mdGetSlaveId() != (handler->slaveId - 1U))
#else
    if (mdGetSlaveId() != handler->slaveId)
#endif
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
        break;
    case MODBUS_CODE_15:
        handler->mdRTUHandleCode15(handler);
        break;
    case MODBUS_CODE_16:
        handler->mdRTUHandleCode16(handler);
        break;
    default:
        handler->mdRTUError(handler, ERROR5);
        break;
    }
#endif
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
    (*handler) = (ModbusRTUSlaveHandler)pvPortMalloc(sizeof(struct ModbusRTUSlave));
#else
    (*handler) = (ModbusRTUSlaveHandler)malloc(sizeof(struct ModbusRTUSlave));
#endif
#if defined(USING_DEBUG)
    shellPrint(&shell, "handler = 0x%p\r\n", *handler);
#endif
    if ((*handler) != NULL)
    {
        (*handler)->mdRTUPopChar = info.mdRTUPopChar;
        (*handler)->mdRTUCenterProcessor = mdRTUCenterProcessor;
        (*handler)->mdRTUError = mdRTUError;
        (*handler)->slaveId = info.slaveId;
        (*handler)->invalidTime = (int)(1.5 * DATA_BITS * 1000 * 1000 / info.usartBaudRate);
        (*handler)->stopTime = (int)(3.5 * DATA_BITS * 1000 * 1000 / info.usartBaudRate);
        (*handler)->updateFlag = false;
        (*handler)->portRTUPushChar = portRtuPushChar;
        (*handler)->portRTUTimerTick = portRtuTimerTick;
        (*handler)->portRTUPushString = portRtuPushString;
        (*handler)->mdRTUSendString = mdRTUSendString;
#if !defined(AS_REPEATER)
        (*handler)->mdRTUHandleCode1 = mdRTUHandleCode1;
        (*handler)->mdRTUHandleCode2 = mdRTUHandleCode2;
        (*handler)->mdRTUHandleCode3 = mdRTUHandleCode3;
        (*handler)->mdRTUHandleCode4 = mdRTUHandleCode4;
        (*handler)->mdRTUHandleCode5 = mdRTUHandleCode5;
        (*handler)->mdRTUHandleCode6 = mdRTUHandleCode6;
        (*handler)->mdRTUHandleCode15 = mdRTUHandleCode15;
        (*handler)->mdRTUHandleCode16 = mdRTUHandleCode16;
#endif

        if (mdCreateRegisterPool(&((*handler)->registerPool)) &&
            mdCreateReceiveBuffer(&((*handler)->receiveBuffer)))
        {
            return mdTRUE;
        }
        else
        {
#if defined(USING_DEBUG)
            shellPrint(&shell, "Cpool = %d, Crec = %d\r\n", mdCreateRegisterPool(&((*handler)->registerPool)), mdCreateReceiveBuffer(&((*handler)->receiveBuffer)));
#endif
#if defined(USING_FREERTOS)
            vPortFree((*handler));
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
