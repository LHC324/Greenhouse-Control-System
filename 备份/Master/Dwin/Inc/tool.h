#ifndef __TOOL_H__
#define __TOOL_H__

#ifdef __cplusplus
extern "C"
{
#endif
#include "main.h"
#if defined(USING_FREERTOS)
#include "cmsis_os.h"
#endif

#define __INFORMATION()                            \
    "Line: %d,  Date: %s, Time: %s, Name: %s\r\n", \
        __LINE__, __DATE__, __TIME__, __FILE__

/*使用就绪列表*/
#define USING_FREERTOS_LIST 0
/*交换任意类型数据*/
#define USING_SWAP_ANY 1

    /*使用卡尔曼滤波*/
    // #define KALMAN

#if defined(KALMAN)
/*以下为卡尔曼滤波参数*/
#define LASTP 0.500F   //上次估算协方差
#define COVAR_Q 0.005F //过程噪声协方差
#define COVAR_R 0.067F //测噪声协方差

    typedef struct
    {
        float Last_Covariance; //上次估算协方差 初始化值为0.02
        float Now_Covariance;  //当前估算协方差 初始化值为0
        float Output;          //卡尔曼滤波器输出 初始化值为0
        float Kg;              //卡尔曼增益 初始化值为0
        float Q;               //过程噪声协方差 初始化值为0.001
        float R;               //观测噪声协方差 初始化值为0.543
    } KFP;

    extern float kalmanFilter(KFP *kfp, float input);
#else
/*左移次数*/
#define FILTER_SHIFT 4U

typedef struct
{
    bool First_Flag;
    float SideBuff[1 << FILTER_SHIFT];
    float *Head;
    float Sum;
} SideParm;
extern float sidefilter(SideParm *side, float input);
#endif

    typedef struct _Range
    {
        int start, end;
    } Range __attribute__((aligned(4)));

    typedef struct DMA_HandleTypeDef *pDmaHandle;
    typedef struct DMA_HandleTypeDef DmaHandle;
    struct DMA_HandleTypeDef
    {
        UART_HandleTypeDef *huart;
        DMA_HandleTypeDef *phdma;
        uint8_t *pRbuf;
        uint8_t RxSize;
        uint32_t *pRxCount;
#if !defined(USING_FREERTOS)
        bool *pRecive_FinishFlag;
#else
    osSemaphoreId Semaphore;
#endif
    } __attribute__((aligned(4)));

/*硬件支持的板卡数*/
#define CARD_NUM_MAX 0x10
/*板卡类型数*/
#define CARD_TYPE_MAX 0x08
/*每块板卡的信号数*/
#define CARD_SIGNAL_MAX 0x08
/*板卡类型的优先级上限*/
#define PRIORITY_MIN 0x00
/*板卡类型的优先级上限*/
#define PRIORITY_MAX 0x50
/*无效中断源位置*/
#define INACTIVE_SITE 0x00

#define START_SIGNAL_MAX 10U
#define OFFSET 4U
#define BX_SIZE 4U
#define VX_SIZE 14U // 5U
#define STIMES 10U
#define CURRENT_UPPER 16.0F
#define CURRENT_LOWER 4.0F
#define Get_Target(__current, __upper, __lower) \
    (((__current)-CURRENT_LOWER) / CURRENT_UPPER * ((__upper) - (__lower)) + (__lower))
#define Set_SoftTimer_Flag(__obj, __val) \
    ((__obj)->flag = (__val))
#define Sure_Overtimes(__obj, __times) \
    ((__obj)->flag ? (++(__obj)->counts >= (__times) ? (__obj)->counts = 0U, (__obj)->flag = false, true : false) : false)
#define Clear_Counter(__obj) ((__obj)->counts = 0U)
#define Open_Vx(__x) ((__x) <= VX_SIZE ? wbit[__x - 1U] = true : false)
#define Close_Vx(__x) ((__x) <= VX_SIZE ? wbit[__x - 1U] = false : false)
#define TARGET_BOARD_NUM ((VX_SIZE + (CARD_SIGNAL_MAX - 1U)) / CARD_SIGNAL_MAX)

    typedef enum
    {
        Card_AnalogInput = 0x00,
        Card_AnalogOutput = 0x10,
        Card_DigitalInput = 0x20,
        Card_DigitalOutput = 0x30,
        Card_Wifi = 0xC0,
        Card_4G = 0xD0,
        Card_Lora1 = 0xE0,
        Card_Lora2 = 0xF0,
        Card_None = 0x55,
    } Card_Tyte;
    typedef struct
    {
        uint8_t SlaveId;
        uint16_t Priority;
        Card_Tyte TypeCoding;
        uint16_t Number;
        // bool flag;
    } IRQ_Code;

    typedef struct
    {
        uint16_t site;
        uint16_t Priority;
        bool flag;
    } IRQ_Request;

    typedef struct
    {
        IRQ_Request *pReIRQ;
        uint16_t SiteCount;
        IRQ_Code *pIRQ;
        uint16_t TableCount;
        // uint16_t Amount;

#if (USING_FREERTOS_LIST)
        List_t *LReady, *LBlock;
#endif
    } Slave_IRQTableTypeDef __attribute__((aligned(4)));

    typedef struct
    {
        // IRQ_Code *pTIRQ;
        // uint8_t size;
        uint8_t *p_id;
        uint8_t count;
    } R_TargetTypeDef;

    extern IRQ_Code IRQ[CARD_NUM_MAX];
    extern Slave_IRQTableTypeDef IRQ_Table;
#define QUICK_SORT_TYPE IRQ_Request
    extern void Quick_Sort(QUICK_SORT_TYPE *pData, int len);
    extern void Add_ListItem(List_t *list, TickType_t data);
    extern ListItem_t *Find_ListItem(List_t *list, TickType_t data);
    extern UBaseType_t Remove_ListItem(List_t *list, TickType_t data);
    extern ListItem_t *Get_OneListItem(List_t *list, ListItem_t **p);
    extern IRQ_Code *Find_TargetSlave_AdoptId(Slave_IRQTableTypeDef *irq, uint8_t target_id);
    extern IRQ_Code *Find_TargetSlave_AdoptType(Slave_IRQTableTypeDef *irq, IRQ_Code *p_current);
    extern bool Save_TargetSlave_Id(Slave_IRQTableTypeDef *irq, Card_Tyte type, R_TargetTypeDef *psave);
    extern void IRQ_Coding(Slave_IRQTableTypeDef *irq, uint8_t code);
    extern uint16_t Get_Crc16(uint8_t *ptr, uint16_t length, uint16_t init_dat);
    extern void Endian_Swap(uint8_t *pData, uint8_t start, uint8_t length);
    extern void DMA_Recive(pDmaHandle pd);
#define DMA_ReciveHandle(__obj) (DMA_Recive(&(__obj)->Uart))
#define DMA_ReciveHandl1(__obj) (DMA_Recive(&(__obj)))
#ifdef __cplusplus
}
#endif

#endif /* __TOOL_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
