#ifndef __MDRTUSLAVE_H__
#define __MDRTUSLAVE_H__

#include "mdtype.h"
#include "mdconfig.h"
#include "mdregpool.h"
#include "mdrecbuffer.h"

#if (USER_MODBUS_LIB)
#define UNREFERENCED_VALUE(P) (P)
#define USING_DMA_TRANSPORT 1
/*从机地址*/
#define SLAVE_ID 0x01
/*从机通讯波特率*/
#define BUAD_RATE 115200U
/*定时器周期为100us*/
#define TIMER_UTIME 100U

/*接收帧时错误*/
#define ERROR1 1
/*帧长度错误*/
#define ERROR2 2
/*CRC校验错误*/
#define ERROR3 3
/*站号错误*/
#define ERROR4 4
/*未知的功能码*/
#define ERROR5 5

#define LOW(n) ((mdU16)n % 256)
#define HIGH(n) ((mdU16)n / 256)
#define ToU16(high, low) ((((mdU16)high & 0x00ff) << 8) | \
                          ((mdU16)low & 0x00ff))
#define TIMER_CLEAN()  \
    do                 \
    {                  \
        lastCount = 0; \
        ustime = 0;    \
        timeSum = 0;   \
        error = 0;     \
    } while (0)

/* ================================================================== */
/*                        core                                        */
/* ================================================================== */
#define MODBUS_CODE_1 1
#define MODBUS_CODE_2 2
#define MODBUS_CODE_3 3
#define MODBUS_CODE_4 4
#define MODBUS_CODE_5 5
#define MODBUS_CODE_6 6
#define MODBUS_CODE_15 15
#define MODBUS_CODE_16 16

#define mdGetSlaveId() (recbuf[0])
#define mdGetCrc16() (ToU16(recbuf[reclen - 1], recbuf[reclen - 2]))
#define mdGetCode() (recbuf[1])
#if defined(USING_FREERTOS)
extern void *pvPortMalloc(size_t xWantedSize);
#define mdmalloc(pointer, type, length)                    \
    pointer = (type *)pvPortMalloc(sizeof(type) * length); \
    memset(pointer, 0, sizeof(type) * length)
#define mdfree(pointer) vPortFree(pointer)
#else
#define mdmalloc(pointer, type, length)              \
    pointer = (type *)malloc(sizeof(type) * length); \
    memset(pointer, 0, sizeof(type) * length)
#define mdfree(pointer) free(pointer)
#endif

typedef struct ModbusRTUSlave *ModbusRTUSlaveHandler;

struct ModbusRTUSlave
{
    mdU8 slaveId;
    mdU32 usartBaudRate;
    mdU32 stopTime, invalidTime;
    mdBOOL updateFlag;
    ReceiveBufferHandle receiveBuffer;
    RegisterPoolHandle registerPool;
    mdVOID (*mdRTUPopChar)(ModbusRTUSlaveHandler handler, mdU8 *data, mdU32 length);
    mdVOID (*mdRTUCenterProcessor)(ModbusRTUSlaveHandler handler);
    mdVOID (*mdRTUError)(ModbusRTUSlaveHandler handler, mdU8 error);

    mdVOID (*portRTUPushChar)(ModbusRTUSlaveHandler handler, mdU8 c);
    mdVOID (*portRTUTimerTick)(ModbusRTUSlaveHandler handler, mdU32 ustime);

    mdVOID (*portRTUPushString)(ModbusRTUSlaveHandler handler, mdU8 *data, mdU32 length);
    mdVOID (*mdRTUSendString)(ModbusRTUSlaveHandler handler, mdU8 *data, mdU32 length);
    mdVOID (*mdRTUHandleCode1)(ModbusRTUSlaveHandler handler);
    mdVOID (*mdRTUHandleCode2)(ModbusRTUSlaveHandler handler);
    mdVOID (*mdRTUHandleCode3)(ModbusRTUSlaveHandler handler);
    mdVOID (*mdRTUHandleCode4)(ModbusRTUSlaveHandler handler);
    mdVOID (*mdRTUHandleCode5)(ModbusRTUSlaveHandler handler);
    mdVOID (*mdRTUHandleCode6)(ModbusRTUSlaveHandler handler);
    mdVOID (*mdRTUHandleCode15)(ModbusRTUSlaveHandler handler);
    mdVOID (*mdRTUHandleCode16)(ModbusRTUSlaveHandler handler);
};

struct ModbusRTUSlaveRegisterInfo
{
    mdU8 slaveId;
    mdU32 usartBaudRate;
    mdVOID (*mdRTUPopChar)(ModbusRTUSlaveHandler handler, mdU8 *data, mdU32 length);
};

/*定义当前从机对象:不对外开放接口*/
mdAPI ModbusRTUSlaveHandler mdMaster;
#define Master_Object mdMaster
// mdAPI ModbusRTUSlaveHandler Master_Object;
mdAPI mdSTATUS mdCreateModbusRTUSlave(ModbusRTUSlaveHandler **handler, struct ModbusRTUSlaveRegisterInfo info);
mdAPI mdVOID mdDestoryModbusRTUSlave(ModbusRTUSlaveHandler **handler);
mdAPI mdVOID mdU16Swap(mdU16 *data, mdU32 length);
mdAPI mdVOID ModbusInit(ModbusRTUSlaveHandler *handler);
/*接口：100us定时器回调函数*/
#define mdRTU_Handler(obj) (obj->portRTUTimerTick(obj, TIMER_UTIME))
#define mdRTU_Recive_Buf(obj) (obj->receiveBuffer->buf)
#define mdRTU_Recive_Len(obj) (obj->receiveBuffer->count)
#define mdRTU_SendString(obj, buf, len) (obj->mdRTUSendString(obj, buf, len))
#define mdRTU_WriteInputCoil(obj, addr, bit) (obj->registerPool->mdWriteInputCoil(obj->registerPool, addr, bit))
#define mdRTU_WriteCoil(obj, addr, bit) (obj->registerPool->mdWriteCoil(obj->registerPool, addr, bit))
#define mdRTU_ReadCoil(obj, addr, bit) (obj->registerPool->mdReadCoil(obj->registerPool, addr, &bit))
#define mdRTU_WriteHoldRegs(obj, start_addr, len, data) (obj->registerPool->mdWriteHoldRegisters(obj->registerPool, start_addr, len, (mdU16 *)&data))
#endif

#endif
