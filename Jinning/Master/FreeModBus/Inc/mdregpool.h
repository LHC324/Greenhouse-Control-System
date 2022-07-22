#ifndef __MDREGPOOL_H__
#define __MDREGPOOL_H__


#include "mdtype.h"
#include "mdconfig.h"


typedef struct Register* RegisterHandle;
struct Register
{
    mdU32 addr;
    mdU32 data;
    RegisterHandle next;
};

typedef struct RegisterPool* RegisterPoolHandle;
struct RegisterPool
{
    RegisterHandle pool;
    //线圈、输入状态、输入寄存器、保持寄存器
    RegisterHandle quickMap[4][REGISTER_POOL_MAX_BUFFER];
    mdU32 curRegisterNumber,maxRegisterNumber;

    mdSTATUS (*mdReadBit)(RegisterPoolHandle handler,mdU32 addr,mdBit *bit);
    mdSTATUS (*mdWriteBit)(RegisterPoolHandle handler,mdU32 addr,mdBit bit);
    mdSTATUS (*mdReadBits)(RegisterPoolHandle handler,mdU32 addr,mdU32 len,mdBit *bits);
    mdSTATUS (*mdWriteBits)(RegisterPoolHandle handler,mdU32 addr,mdU32 len,mdBit *bits);
    mdSTATUS (*mdReadU16)(RegisterPoolHandle handler,mdU32 addr,mdU16 *data);
    mdSTATUS (*mdWriteU16)(RegisterPoolHandle handler,mdU32 addr,mdU16 data);
    mdSTATUS (*mdReadU16s)(RegisterPoolHandle handler,mdU32 addr,mdU32 len,mdU16 *data);
    mdSTATUS (*mdWriteU16s)(RegisterPoolHandle handler,mdU32 addr,mdU32 len,mdU16 *data);

    mdSTATUS (*mdReadCoil)(RegisterPoolHandle handler, mdU32 addr, mdBit* bit);
    mdSTATUS (*mdReadCoils)(RegisterPoolHandle handler, mdU32 addr, mdU32 len, mdBit* bits);
    mdSTATUS (*mdWriteCoil)(RegisterPoolHandle handler, mdU32 addr, mdBit bit);
    mdSTATUS (*mdWriteCoils)(RegisterPoolHandle handler, mdU32 addr, mdU32 len, mdBit* bits);
    mdSTATUS (*mdReadInputCoil)(RegisterPoolHandle handler, mdU32 addr, mdBit* bit);
    mdSTATUS (*mdReadInputCoils)(RegisterPoolHandle handler, mdU32 addr, mdU32 len, mdBit* bits);
    mdSTATUS (*mdWriteInputCoil)(RegisterPoolHandle handler, mdU32 addr, mdBit bit);
    mdSTATUS (*mdWriteInputCoils)(RegisterPoolHandle handler, mdU32 addr, mdU32 len, mdBit* bits);
    mdSTATUS (*mdReadInputRegister)(RegisterPoolHandle handler, mdU32 addr, mdU16* data);
    mdSTATUS (*mdReadInputRegisters)(RegisterPoolHandle handler, mdU32 addr, mdU32 len, mdU16* data);
    mdSTATUS (*mdWriteInputRegister)(RegisterPoolHandle handler, mdU32 addr, mdU16 data);
    mdSTATUS (*mdWriteInputRegisters)(RegisterPoolHandle handler, mdU32 addr, mdU32 len, mdU16* data);
    mdSTATUS (*mdReadHoldRegister)(RegisterPoolHandle handler, mdU32 addr, mdU16* data);
    mdSTATUS (*mdReadHoldRegisters)(RegisterPoolHandle handler, mdU32 addr, mdU32 len, mdU16* data);
    mdSTATUS (*mdWriteHoldRegister)(RegisterPoolHandle handler, mdU32 addr, mdU16 data);
    mdSTATUS (*mdWriteHoldRegisters)(RegisterPoolHandle handler, mdU32 addr, mdU32 len, mdU16* data);
};


mdExport mdSTATUS mdCreateRegisterPool(RegisterPoolHandle* regpoolhandle);
mdExport mdVOID mdDestoryRegisterPool(RegisterPoolHandle* regpoolhandle);

#define mdGetBit(reg,offset) ((reg>>offset)&1)
#define mdSetBit(handler,offset,bit) do{(handler->data) |= (bit << offset);}while(0)

#endif

