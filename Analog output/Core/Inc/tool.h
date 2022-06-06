#ifndef __TOOL_H__
#define __TOOL_H__

#ifdef __cplusplus
extern "C"
{
#endif
#include "main.h"

    typedef enum
    {
        Enable = 0x00,
        DisEnable,
    } IRQ_Type;

    typedef struct
    {
        GPIO_TypeDef *pGPIOx;
        uint16_t Gpio_Pin;
        GPIO_PinState PinState;
    } Gpiox_info;
    typedef struct
    {
        Gpiox_info *pGpio;
        uint8_t PinNumbers;
    } Gpio_InfoTypeDef;

    typedef struct
    {
        Gpio_InfoTypeDef *p74HC138;
        // Gpiox_info CLK;
        // Gpiox_info CE;
        // Gpiox_info PL;
        Gpiox_info DO;
        // IRQ_Type IRQ;
    } Read_InfoTypeDef;

    extern void Write_74HC138(Gpio_InfoTypeDef *pg);
    extern uint8_t Read_74HC165(Read_InfoTypeDef *pr);
    extern uint8_t Get_CardId(void);
    extern void IRQ_Handle(IRQ_Type irq);

/*使用卡尔曼滤波*/
#define KALMAN

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

    extern uint16_t Get_Crc16(uint8_t *ptr, uint16_t length, uint16_t init_dat);
    extern void Endian_Swap(uint8_t *pData, uint8_t start, uint8_t length);
#if defined(USING_DEBUG)
    extern void Debug(const char *format, ...);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __TOOL_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
