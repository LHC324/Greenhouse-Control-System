#ifndef __IO_UART_H__
#define __IO_UART_H__

#ifdef __cplusplus
extern "C"
{
#endif
#include "main.h"

/*用户波特率*/
#define User_BaudRate 9600U
/*软件串口支持的最大波特率*/
#define MAX_IOUART_BAUDRATE 57600U
/*系数个数*/
#define MAX_PARAM 5U
/*接收时的滤波次数*/
#define MAX_SAMPING 3U
/*1S*/
#define TOTAL_TIMES 1000000.0F
/*对应波特率时发送一位所用的时间*/
#define BIT_TIMES(b) (TOTAL_TIMES / (float)b)
/*获取定时时间规则*/
#define GET_RULE1(b) (BIT_TIMES(b) * 5.0F / 13.0F)
#define GET_RULE2(b) (BIT_TIMES(b) * 5.0F / 52.0F)
#define GET_RULE3(b) (BIT_TIMES(b) * 21.0F / 26.0F)

    typedef enum
    {
        B_1200 = 0,
        B_2400,
        B_4800,
        B_9600,
        B_115200
    } Baud_Rate;

    typedef enum
    {
        Start_Recv = 0,
        Sampling2,
        Sampling3,
        Sampling1,
        Start_Send
    } Option_Type;

    typedef enum
    {
        COM_NONE_BIT,
        COM_START_BIT,
        COM_DATA_BIT,
        COM_CHECK_BIT,
        COM_STOP_BIT
    } Uart_State;

    typedef enum
    {
        NONE = 0,
        ODD,
        EVEN
    } Check;

    typedef struct
    {
        struct
        { /*目标定时器*/
            TIM_HandleTypeDef *Timer_Handle;
            bool En; /*半双工时使用*/
            Uart_State Status;
            uint8_t Bits;         /*发送位计数*/
            uint16_t Len;         /*发送数据长度统计*/
            uint8_t *pBuf;        /*发送缓冲区指针*/
            uint16_t Total_Times; /*发送一个字节花费的时间*/
            bool Finsh_Flag;      /*发送完成标志*/
        } Tx;
        struct
        {
            /*目标定时器*/
            TIM_HandleTypeDef *Timer_Handle;
            bool En;
            IRQn_Type IRQn; /*接收引脚的外部中断*/
            Uart_State Status;
            uint8_t Bits;                       /*接收位计数*/
            uint8_t Filters;                    /*滤波次数*/
            uint8_t Data;                       /*接收到的一字节数据*/
            uint16_t Len;                       /*接收到数据长度统计*/
            uint8_t *pBuf;                      /*接收缓冲区指针*/
            uint16_t Sam_Times[MAX_PARAM - 1U]; /*三次采样时间*/
            bool Finsh_Flag;                    /*接收完成标志*/
        } Rx;
        Check Check_Type;
    } IoUart_HandleTypeDef __attribute__((aligned(4)));

    extern IoUart_HandleTypeDef S_Uart1;
    extern void MX_Suart_Init(void);
    extern bool Get_ValidBits(uint8_t n);
    extern HAL_StatusTypeDef HAL_SUART_Transmit(IoUart_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout);
    extern HAL_StatusTypeDef HAL_SUART_Receive(IoUart_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout);
#ifdef __cplusplus
}
#endif

#endif /* _io_uart_*/
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
