#ifndef __MDRECBUFFER_H__
#define __MDRECBUFFER_H__

#include "mdtype.h"
#include "mdconfig.h"

typedef struct ReceiveBuffer* ReceiveBufferHandle;
struct ReceiveBuffer
{
    mdU8  buf[MODBUS_PDU_SIZE_MAX];
    mdU32 count;
};

mdAPI mdVOID mdClearReceiveBuffer(ReceiveBufferHandle handler);
mdAPI mdSTATUS mdCreateReceiveBuffer(ReceiveBufferHandle *handler);
mdAPI mdVOID mdDestoryReceiveBuffer(ReceiveBufferHandle *handler);

#endif
