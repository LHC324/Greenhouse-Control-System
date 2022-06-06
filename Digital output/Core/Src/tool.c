#include "tool.h"

/*138芯片引脚配置表*/
Gpiox_info info_pin[] = {
    {.pGPIOx = A_GPIO_Port, .Gpio_Pin = A_Pin, .PinState = GPIO_PIN_RESET},
    {.pGPIOx = B_GPIO_Port, .Gpio_Pin = B_Pin, .PinState = GPIO_PIN_RESET},
};
Gpio_InfoTypeDef gpio_74HC138 = {
    .pGpio = info_pin,
    .PinNumbers = sizeof(info_pin) / sizeof(Gpiox_info),
};

/**
 * @brief  写74HC138
 * @param  pg 译码表指针
 * @note   此函数要求ABC输出信号要同步
 * @retval None
 */
void Write_74HC138(Gpio_InfoTypeDef *pg)
{
    uint16_t pin = (uint16_t)pg->pGpio->pGPIOx->ODR;
    // uint16_t pin = 0;

#if defined(USING_DEBUG)
    Debug("current pin = 0x%0X\r\n", pin);
#endif
    for (Gpiox_info *pi = pg->pGpio; pi < pg->pGpio + pg->PinNumbers; pi++)
    {
        if (pi->PinState != GPIO_PIN_RESET)
        {
            pin |= pi->Gpio_Pin;
        }
        else
        {
            pin &= ~pi->Gpio_Pin;
        }
        // HAL_GPIO_WritePin(pi->pGPIOx, pi->Gpio_Pin, pi->PinState);
    }
#if defined(USING_DEBUG)
    // Debug("after pin = 0x%0X\r\n", pin);
#endif
    if (pg && pg->pGpio)
        /*ABC三个译码信号要接在GPIO同一端口，保证三个端口硬件信号同时输出*/
        // pg->pGpio->pGPIOx->BSRR = (uint32_t)(pin & 0xffff) | ((~pin) << 16U);
        pg->pGpio->pGPIOx->BSRR = (uint32_t)((pin & 0xffff) | ((uint32_t)(~pin) << 16U));
}

/**
 * @brief  串行方式读取74HC165并行数据
 * @param  pr 硬件相关引脚配置指针
 * @retval 卡槽和板卡编号
 */
uint8_t Read_74HC165(Read_InfoTypeDef *pr)
{
    uint8_t data = 0x00;
    // Gpiox_info info_pin[] = {
    //     {.pGPIOx = A_GPIO_Port, .Gpio_Pin = A_Pin, .PinState = GPIO_PIN_RESET},
    //     {.pGPIOx = B_GPIO_Port, .Gpio_Pin = B_Pin, .PinState = GPIO_PIN_RESET},
    // };
    // Gpio_InfoTypeDef gpio_74HC138 = {
    //     .pGpio = info_pin,
    //     .PinNumbers = sizeof(info_pin) / sizeof(Gpiox_info),
    // };
    // Gpio_InfoTypeDef *p74HC138 = &gpio_74HC138;

    if (pr && (pr->p74HC138))
    {
        pr->p74HC138->pGpio[0].PinState = GPIO_PIN_RESET;
        pr->p74HC138->pGpio[1].PinState = GPIO_PIN_RESET;
        /*通过138译码获得PL信号*/
        Write_74HC138(pr->p74HC138);
        // __NOP();
        // __NOP();
        // __NOP();
        pr->p74HC138->pGpio[0].PinState = GPIO_PIN_SET;
        pr->p74HC138->pGpio[1].PinState = GPIO_PIN_SET;
        /*PL来一个下降沿锁住数据*/
        Write_74HC138(pr->p74HC138);
        // HAL_Delay(1);
        for (uint8_t i = 0; i < 8U; i++)
        {
            if (pr->DO.pGPIOx)
            {
                /*拉低CLK*/
                pr->p74HC138->pGpio[1].PinState = GPIO_PIN_RESET;
                Write_74HC138(pr->p74HC138);
                pr->DO.PinState = HAL_GPIO_ReadPin(pr->DO.pGPIOx, pr->DO.Gpio_Pin);
                data |= (pr->DO.PinState & 0x01) << (7U - i);
#if defined(USING_DEBUG)
                // Debug("bit[%d] = 0x%02x\r\n", i, pr->DO.PinState);
#endif
                /*拉高CLK*/
                pr->p74HC138->pGpio[1].PinState = GPIO_PIN_SET;
                Write_74HC138(pr->p74HC138);
            }
        }
    }
    return data;
}

/**
 * @brief  获取卡槽和板卡编码
 * @param  None
 * @retval 卡槽和板卡编号
 */
uint8_t Get_CardId(void)
{
    Read_InfoTypeDef read_74HC165 = {
        .p74HC138 = &gpio_74HC138,
        .DO = {SO_GPIO_Port, SO_Pin, GPIO_PIN_RESET},
        // .IRQ = DisEnable,
    };

    return (Read_74HC165(&read_74HC165));
}

/**
 * @brief  从机中断信号处理
 * @param  irq 中断操作类型，产生/关闭
 * @retval None
 */
void IRQ_Handle(IRQ_Type irq)
{
    Gpio_InfoTypeDef *p138 = &gpio_74HC138;

    p138->pGpio[0].PinState = (irq == DisEnable) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    p138->pGpio[1].PinState = GPIO_PIN_SET;

    Write_74HC138(p138);
}

/**
 * @brief  取得16bitCRC校验码
 * @param  ptr   当前数据串指针
 * @param  length  数据长度
 * @param  init_dat 校验所用的初始数据
 * @retval 16bit校验码
 */
uint16_t Get_Crc16(uint8_t *ptr, uint16_t length, uint16_t init_dat)
{
    uint16_t crc16 = init_dat;

    for (uint16_t i = 0; i < length; i++)
    {
        crc16 ^= *ptr++;

        for (uint16_t j = 0; j < 8; j++)
        {
            crc16 = (crc16 & 0x0001) ? ((crc16 >> 1) ^ 0xa001) : (crc16 >> 1U);
        }
    }
    return (crc16);
}

/**
 * @brief  大小端数据类型交换
 * @note   对于一个单精度浮点数的交换仅仅需要2次
 * @param  pData 数据
 * @param  start 开始位置
 * @param  length  数据长度
 * @retval None
 */
void Endian_Swap(uint8_t *pData, uint8_t start, uint8_t length)
{
    uint8_t i = 0;
    uint8_t tmp = 0;
    uint8_t count = length / 2U;
    uint8_t end = start + length - 1U;

    for (; i < count; i++)
    {
        tmp = pData[start + i];
        pData[start + i] = pData[end - i];
        pData[end - i] = tmp;
    }
}

#if defined(KALMAN)
#define EPS 1e-8
/**
 *卡尔曼滤波器
 *@param KFP *kfp 卡尔曼结构体参数
 *   float input 需要滤波的参数的测量值（即传感器的采集值）
 *@return 滤波后的参数（最优值）
 */
float kalmanFilter(KFP *kfp, float input)
{
    /*预测协方差方程：k时刻系统估算协方差 = k-1时刻的系统协方差 + 过程噪声协方差*/
    kfp->Now_Covariance = kfp->Last_Covariance + kfp->Q;
    /*卡尔曼增益方程：卡尔曼增益 = k时刻系统估算协方差 / （k时刻系统估算协方差 + 观测噪声协方差）*/
    kfp->Kg = kfp->Now_Covariance / (kfp->Now_Covariance + kfp->R);
    /*更新最优值方程：k时刻状态变量的最优值 = 状态变量的预测值 + 卡尔曼增益 * （测量值 - 状态变量的预测值）*/
    kfp->Output = kfp->Output + kfp->Kg * (input - kfp->Output); //因为这一次的预测值就是上一次的输出值
    /*更新协方差方程: 本次的系统协方差付给 kfp->Last_Covariance 为下一次运算准备。*/
    kfp->Last_Covariance = (1 - kfp->Kg) * kfp->Now_Covariance;
    /*当kfp->Output不等于0时，负方向迭代导致发散到无穷小*/
    if (kfp->Output - 0.0 < EPS)
    {
        kfp->Kg = 0;
        kfp->Output = 0;
    }
    return kfp->Output;
}
#else
/**
 *滑动滤波器
 *@param SideParm *side 滑动结构体参数
 *   float input 需要滤波的参数的测量值（即传感器的采集值）
 *@return 滤波后的参数（最优值）
 */
float sidefilter(SideParm *side, float input)
{
    //第一次滤波
    if (side->First_Flag)
    {

        for (int i = 0; i < sizeof(side->SideBuff) / sizeof(float); i++)
        {
            side->SideBuff[i] = input;
        }

        side->First_Flag = false;
        side->Head = &side->SideBuff[0];
        side->Sum = input * (sizeof(side->SideBuff) / sizeof(float));
    }
    else
    {
        side->Sum = side->Sum - *side->Head + input;
        *side->Head = input;

        if (++side->Head > &side->SideBuff[sizeof(side->SideBuff) / sizeof(float) - 1])
        {
            side->Head = &side->SideBuff[0];
        }

        input = side->Sum / (1 << FILTER_SHIFT);
    }

    return input;
}
#endif

#if defined(USING_DEBUG)
void Debug(const char *format, ...)
{
    extern UART_HandleTypeDef huart1;
#define DEBUG_UART huart1
    uint16_t len;
    uint8_t temp_buffer[256U];
    va_list args;

    va_start(args, format);
    len = vsnprintf((char *)temp_buffer, sizeof(temp_buffer) + 1U, format, args);
    va_end(args);

    HAL_UART_Transmit(&DEBUG_UART, temp_buffer, len, 0xFFFF);
}
#endif
