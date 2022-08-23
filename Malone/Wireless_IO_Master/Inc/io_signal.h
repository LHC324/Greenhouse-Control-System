#ifndef _IO_SIGNAL_H_
#define _IO_SIGNAL_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "main.h"

/*定义外部数字量输入路数*/
#define EXTERN_DIGITAL_MAX 8U
/*定义外部模拟量输入路数*/
#define EXTERN_ANALOG_MAX 2U
/*数字信号量在内存中初始地址*/
#define DIGITAL_START_ADDR 0x00
/*模拟信号量在内存中初始地址*/
#define ANALOG_START_ADDR 0x00
/*检测输入引脚延时*/
#define CHECK_DELAY_5MS 5U

    typedef struct
    {
        GPIO_TypeDef *pGPIOx;
        uint16_t GPIO_Pinx;
    } GPIOx_HandleTypeDef;

    typedef struct
    {
        GPIOx_HandleTypeDef Gpio;
        uint32_t Timer_Count;
    } DDIx_Group_HandleTypeDef;

    typedef struct
    {
        DDIx_Group_HandleTypeDef *pGroup;
        uint16_t Group_Size;
        uint16_t site, bits;
        void *pQueue;
    } DDIx_HandleTypeDef __attribute__((aligned(4)));

    extern DDIx_HandleTypeDef DDIx;

    // extern void Io_Digital_Handle(void);
    extern void Io_Digital_Handle(uint8_t *pdata);
    // extern void Io_Digital_Handle(uint8_t site, uint8_t *pdata);
    extern void Io_Analog_Handle(void);
    extern void DDIx_TimerOut_Check(DDIx_HandleTypeDef *pDDIx);

#ifdef __cplusplus
}
#endif

#endif /* __IO_SIGNAL_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
