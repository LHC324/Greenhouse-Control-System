#ifndef __MDRTUSLAVE_H__
#define __MDRTUSLAVE_H__

#include "mdtype.h"
#include "mdconfig.h"
#include "mdregpool.h"
#include "mdrecbuffer.h"

#if (USER_MODBUS_LIB)
#define UNREFERENCED_VALUE(P) (P)
#define USING_DMA_TRANSPORT 1
#define USING_MASTER
/*从机地址*/
#define SLAVE1_ID 0x02
#define SLAVE2_ID 0x03
/*从机通讯波特率*/
#define SLAVE1_BUAD_RATE 115200U
#define SLAVE2_BUAD_RATE 9600U
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
#define MODBUS_CODE_17 17
#define MODBUS_CODE_46 46

#define mdGetSlaveId() (recbuf[0])
#define mdGetCrc16() (ToU16(recbuf[reclen - 1], recbuf[reclen - 2]))
#define mdGetCode() (recbuf[1])
#if defined(USING_FREERTOS)
// extern void *pvPortMalloc(size_t xWantedSize);
#define mdmalloc(pointer, type, length)                     \
    pointer = (type *)CUSTOM_MALLOC(sizeof(type) * length); \
    memset(pointer, 0, sizeof(type) * length)
#define mdfree(pointer) CUSTOM_FREE(pointer)
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
    mdU8 uartId;
    ReceiveBufferHandle receiveBuffer;
    RegisterPoolHandle registerPool;
    mdVOID (*mdRTUHook)(ModbusRTUSlaveHandler, mdU16);
    mdVOID (*mdRTUPopChar)(ModbusRTUSlaveHandler handler, mdU8 *data, mdU32 length);
    mdVOID (*mdRTUCenterProcessor)(ModbusRTUSlaveHandler handler);
    mdVOID (*mdRTUError)(ModbusRTUSlaveHandler handler, mdU8 error);

    mdVOID (*portRTUPushChar)(ModbusRTUSlaveHandler handler, mdU8 c);
    mdVOID (*portRTUTimerTick)(ModbusRTUSlaveHandler handler, mdU32 ustime);
    mdVOID (*portRTUMasterHandle)(ModbusRTUSlaveHandler, mdU8);

    mdVOID (*portRTUPushString)(ModbusRTUSlaveHandler handler, mdU8 *data, mdU32 length);
    mdVOID (*mdRTUSendString)(ModbusRTUSlaveHandler handler, mdU8 *data, mdU32 length);
    mdVOID (*mdRTU_MasterCodex)(ModbusRTUSlaveHandler handler, mdU8 fun_code, mdU8 slave_id, mdU8 *pdata, mdU8 len);
    mdVOID (*mdRTUHandleCode1)(ModbusRTUSlaveHandler handler);
    mdVOID (*mdRTUHandleCode2)(ModbusRTUSlaveHandler handler);
    mdVOID (*mdRTUHandleCode3)(ModbusRTUSlaveHandler handler);
    mdVOID (*mdRTUHandleCode4)(ModbusRTUSlaveHandler handler);
    mdVOID (*mdRTUHandleCode5)(ModbusRTUSlaveHandler handler);
    mdVOID (*mdRTUHandleCode6)(ModbusRTUSlaveHandler handler);
    mdVOID (*mdRTUHandleCode15)(ModbusRTUSlaveHandler handler);
    mdVOID (*mdRTUHandleCode16)(ModbusRTUSlaveHandler handler);
    mdVOID (*mdRTUHandleCode46)(ModbusRTUSlaveHandler handler, mdU16 regaddr, mdU16 reglen, mdU8 datalen, mdU8 *dat);
    // mdVOID (*mdRTUHandleCode11)(ModbusRTUSlaveHandler, mdU8);
    mdVOID *puser;
};

struct ModbusRTUSlaveRegisterInfo
{
    mdU8 slaveId;
    mdU32 usartBaudRate;
    mdVOID (*mdRTUPopChar)(ModbusRTUSlaveHandler handler, mdU8 *data, mdU32 length);
    mdVOID *puser;
};

/*定义当前从机对象:不对外开放接口*/
mdAPI ModbusRTUSlaveHandler mdSlave1;
mdAPI ModbusRTUSlaveHandler mdSlave2;
#define Slave1_Object mdSlave1
#define Slave2_Object mdSlave2
// mdAPI ModbusRTUSlaveHandler Master_Object;
mdAPI mdSTATUS mdCreateModbusRTUSlave(ModbusRTUSlaveHandler *handler, struct ModbusRTUSlaveRegisterInfo info);
mdAPI mdVOID mdDestoryModbusRTUSlave(ModbusRTUSlaveHandler *handler);
mdAPI mdVOID mdU16Swap(mdU16 *data, mdU32 length);
mdAPI mdVOID mdRTUHandleCode46(ModbusRTUSlaveHandler handler, mdU16 regaddr, mdU16 reglen, mdU8 datalen, mdU8 *dat);
#define mdRTU_46H mdRTUHandleCode46
// mdAPI mdVOID ModbusInit(ModbusRTUSlaveHandler *handler);
mdAPI mdVOID MX_ModbusInit(void);
/*接口：100us定时器回调函数*/
#define mdRTU_Handler(obj) (obj->portRTUTimerTick(obj, TIMER_UTIME))
#define mdRTU_Recive_Buf(obj) (obj->receiveBuffer->buf)
#define mdRTU_Recive_Len(obj) (obj->receiveBuffer->count)
#define mdRTU_SendString(obj, buf, len) (obj->mdRTUSendString(obj, buf, len))
#define mdRTU_ReadCoil(obj, addr, pbit) (obj->registerPool->mdReadCoil(obj->registerPool, addr, (mdBit *)&pbit))
#define mdRTU_ReadCoils(obj, addr, len, pbits) (obj->registerPool->mdReadCoils(obj->registerPool, addr, len, pbits))
#define mdRTU_WriteCoil(obj, addr, bit) (obj->registerPool->mdWriteCoil(obj->registerPool, addr, bit))
#define mdRTU_WriteCoils(obj, addr, len, pbits) (obj->registerPool->mdWriteCoils(obj->registerPool, addr, len, pbits))
#define mdRTU_ReadInputCoil(obj, addr, pbit) (obj->registerPool->mdReadInputCoil(obj->registerPool, addr, (mdBit *)&pbit))
#define mdRTU_ReadInputCoils(obj, addr, len, pbits) (obj->registerPool->mdReadInputCoils(obj->registerPool, addr, len, pbits))
#define mdRTU_WriteInputCoil(obj, addr, bit) (obj->registerPool->mdWriteInputCoil(obj->registerPool, addr, bit))
#define mdRTU_ReadInputRegister(obj, addr, pdata) (obj->registerPool->mdReadInputRegister(obj->registerPool, addr, pdata))
#define mdRTU_ReadInputRegisters(obj, addr, len, pdata) (obj->registerPool->mdReadInputRegisters(obj->registerPool, addr, len, pdata))
#define mdRTU_WriteInputRegister(obj, addr, data) (obj->registerPool->mdWriteInputRegister(obj->registerPool, addr, data))
#define mdRTU_WriteInputRegisters(obj, addr, len, pdata) (obj->registerPool->mdWriteInputRegisters(obj->registerPool, addr, len, pdata))
#define mdRTU_ReadHoldRegister(obj, addr, pdata) (obj->registerPool->mdReadHoldRegister(obj->registerPool, addr, pdata))
#define mdRTU_ReadHoldRegisters(obj, addr, len, pdata) (obj->registerPool->mdReadHoldRegisters(obj->registerPool, addr, len, pdata))
#define mdRTU_WriteHoldReg(obj, addr, data) (obj->registerPool->mdWriteHoldRegister(obj->registerPool, addr, data))
#define mdRTU_WriteHoldRegs(obj, addr, len, pdata) (obj->registerPool->mdWriteHoldRegisters(obj->registerPool, addr, len, pdata))
#define mdRTU_Master_Codex(obj, code, id, pdata, len) (obj->mdRTU_MasterCodex(obj, code, id, pdata, len))

/*输入数字信号量在内存中初始地址*/
#define INPUT_DIGITAL_START_ADDR 0x00
/*输出数字信号量在内存中初始地址*/
#define OUT_DIGITAL_START_ADDR 0x00
/*输入模拟信号量在内存中初始地址*/
#define INPUT_ANALOG_START_ADDR 0x00
/*输出模拟信号量在内存中初始地址*/
#define OUT_ANALOG_START_ADDR 0x00
/*手动模式有效信号地址*/
#define M_MODE_ADDR 0x0020
/*启用副储槽压力传感器地址*/
#define ENABLE_S_PTANK_ADDR (M_MODE_ADDR + 1U)
/*启用副汽化器压力传感器地址*/
#define ENABLE_S_PCARBURETOR_ADDR (ENABLE_S_PTANK_ADDR + 1U)
#endif

#endif
