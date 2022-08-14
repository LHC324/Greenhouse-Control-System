
#include "mdrecbuffer.h"
#include <stdlib.h>
#include <string.h>

#if defined(USING_FREERTOS)
extern void *pvPortMalloc( size_t xWantedSize );
extern void vPortFree( void *pv );
#endif

#if(USER_MODBUS_LIB)
/*
    mdClearReceiveBuffer
        @handler 句柄
        @return
    复位接收缓冲
*/
mdVOID mdClearReceiveBuffer(ReceiveBufferHandle handler)
{
    handler->count = 0;
    memset(handler->buf,0,MODBUS_PDU_SIZE_MAX);
}

/*
    mdCreateReceiveBuffer
        @handler 句柄
        @return
    创建并初始化接收缓冲
*/
mdSTATUS mdCreateReceiveBuffer(ReceiveBufferHandle *handler)
{
#if defined(USING_FREERTOS)    
    (*handler) = (ReceiveBufferHandle)pvPortMalloc(sizeof(struct ReceiveBuffer));
#else
    (*handler) = (ReceiveBufferHandle)malloc(sizeof(struct ReceiveBuffer));
#endif
    if(!handler){
#if defined(USING_FREERTOS)
        vPortFree(handler);
#else
        free(handler); 
#endif
        return mdFALSE;
    }
    mdClearReceiveBuffer(*handler);
    return mdTRUE;
}

/*
    mdDestoryReceiveBuffer
        @handler 句柄
        @return
    销毁接收缓冲，释放内存
*/
mdVOID mdDestoryReceiveBuffer(ReceiveBufferHandle *handler)
{
#if defined(USING_FREERTOS)
    vPortFree(handler);
#else
    free(handler); 
#endif
    (*handler) = NULL;
}
#endif

