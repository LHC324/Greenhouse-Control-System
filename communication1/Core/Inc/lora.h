#ifndef _LORA_H_
#define _LORA_H_

#ifdef __cplusplus
extern "C"
{
#endif
#include "main.h"
#if defined(USING_RTTHREAD)
#include "rtthread.h"
#else
#include "cmsis_os.h"
#endif

/*支持的最大从机*/
#define SLAVE_MAX_NUMBER 15U
#define LORA_TX_BUF_SIZE 64U
#define LORA_RX_BUF_SIZE 64U
/*A型从机地址*/
#define A_TYPE_SLAVE_MIN_ADDR 1U
#define A_TYPE_SLAVE_MAX_ADDR 4U
#define B_TYPE_SLAVE_START_ADDR 5U
/*A型从机数字输入路数*/
#define DIGITAL_INPUT_NUMBERS 8U
#define DIGITAL_INPUT_OFFSET 1U
#define DIGITAL_OUTPUT_OFFSET 5U

/*定义Master发送缓冲区字节数*/
#define PF_TX_SIZE 64U
#define LEVENTS (sizeof(L101_Map) / sizeof(L101_HandleTypeDef))
/*累计三次超时或者错误后，改变上报的时间*/
#define SUSPEND_TIMES 3U
/*30s增加一个离线设备扫描*/
#define TOTAL_SUM 5U
    /*Enter键值*/
    // #define ENTER_CODE 0x2F
    typedef enum
    {
        L_None,
        L_Wait,
        L_OK,
        L_Error,
        L_TimeOut,
    } Lora_State;

    typedef struct
    {
        struct
        {
            union
            {
                uint16_t Val;
                struct
                {
                    uint8_t L;
                    uint8_t H;
                } V;
            } Device_Addr;
            uint8_t Channel;
        } Frame_Head;
        uint8_t Slave_Id;
        // bool (*func)(void *);
    } Lora_Map;

    typedef struct Lora_HandleTypeDef *pLoraHandle;
    typedef struct Lora_HandleTypeDef Lora_Handle;
    struct Lora_HandleTypeDef
    {
        uint8_t MapSize;
        Lora_Map *pMap;
        /*建立lora模块各节点间调度数据结构*/
        struct
        {
            List_t *Ready;
            List_t *Block;
            uint8_t Event_Id;
            uint8_t Period; /*阻塞设备调度周期*/
            bool First_Flag;
        } Schedule;
        struct
        {
            uint8_t Id;
            uint8_t *pTbuf;
            uint8_t TxSize;
            uint8_t TxCount;
        } Master;
        struct
        {
            uint8_t *pRbuf;
            uint8_t RxSize;
            uint8_t RxCount;
#if defined(USING_FREERTOS)
            /*使用操作系统时二值信号量*/
            void *bSemaphore;
#endif
        } Slave;
        struct
        { /*当前状态*/
            Lora_State State;
            /*超时时间*/
            uint32_t OverTimes;
            /*错误计数器*/
            uint32_t Counter;
        } Check;
        void (*Set_Lora_Factory)(void);
        bool (*Get_Lora_Status)(pLoraHandle);
        // bool (*Lora_Handle)(pLoraHandle);
        void (*Lora_Recive_Poll)(pLoraHandle);
        void (*Lora_Transmit_Poll)(pLoraHandle);
        void (*Lora_TI_Recive)(pLoraHandle, DMA_HandleTypeDef *);
        bool (*Lora_MakeFrame)(pLoraHandle, Lora_Map *);
        UART_HandleTypeDef *huart;
        /*预留外部接口*/
        void *pHandle;
    } __attribute__((aligned(4)));

    extern pLoraHandle Lora_Object;
    extern void MX_Lora_Init(void);

#define Clear_LoraBuffer(__ptr, name, b)                                     \
    do                                                                       \
    {                                                                        \
        (__ptr)                         ? memset((__ptr)->name.p##b##buf, 0, \
                                                 (__ptr)->name.b##xCount),   \
            (__ptr)->name.b##xCount = 0 : false;                             \
    } while (0)

#define Check_FirstFlag(__ptr)                                                                          \
    do                                                                                                  \
    {                                                                                                   \
        if (!(__ptr)->Schedule.First_Flag)                                                              \
        {                                                                                               \
            (__ptr)->Schedule.First_Flag =                                                              \
                (++(__ptr)->Schedule.Event_Id >= (__ptr)->MapSize) ? (__ptr)->Schedule.Event_Id = 0xFF, \
            true                                                                                        \
                : false;                                                                                \
        }                                                                                               \
    } while (0)

/*获取主机号*/
#define Get_LoraId(__obj) ((__obj)->Slave.pRbuf[0U])
#define Lora_ReciveHandle(__obj, __dma) ((__obj)->Lora_TI_Recive((__obj), (__dma)))

#ifdef __cplusplus
}
#endif

#endif /* _L101_H_*/
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
