#include "io_signal.h"
#include "main.h"
#include "mdrtuslave.h"
#include "adc.h"
#include "shell_port.h"

#if defined(USING_SLAVE)
#define DDI5_Pin NULL
#define DDI7_Pin NULL
#endif

#define Get_Digital_Port(GPIO_Port) \
    (GPIO_Port ? GPIOA : GPIOB)

#define Get_Digital_Pin(GPIO_Pin) \
    (GPIO_Pin < 5U ? (DDI0_Pin << GPIO_Pin) : (GPIO_Pin < 7U ? (DDI5_Pin << (GPIO_Pin - 5U)) : DDI7_Pin))
/**
 * @brief	外部数字量输入处理
 * @details	STM32F103C8T6共在io口扩展了8路数字输入
 * @param	None
 * @retval	None
 */
void Io_Digital_Input(void)
{
    GPIO_TypeDef *pGPIOx;
    uint16_t GPIO_Pinx;
    mdBit bit = mdLow;
    mdU32 addr;
    mdSTATUS ret;

    for (uint16_t i = 0; i < EXTERN_DIGITAL_MAX; i++)
    {
        pGPIOx = Get_Digital_Port((i < 5U ? 1 : 0));
        GPIO_Pinx = Get_Digital_Pin(i);
        /*指针合法*/
        if (pGPIOx)
        {
            /*读取外部数字引脚状态*/
            bit = (mdBit)HAL_GPIO_ReadPin(pGPIOx, GPIO_Pinx);
        }
        /*计算出出当前写入地址*/
        addr = DIGITAL_INPUT_START_ADDR + i;
        /*写入线圈*/
        ret = mdhandler->registerPool->mdWriteCoil(mdhandler->registerPool, addr, bit);
        /*写入失败*/
        if (ret == mdFALSE)
        {
#if defined(USING_DEBUG)
            // mdhandler->registerPool->mdReadInputCoil(mdhandler->registerPool, addr, &bit);
            shellPrint(&shell, "DDI[%d] = 0x%d\r\n", i, bit);
#endif
        }
    }
}

/**
 * @brief	外部模拟量输入处理
 * @details	STM32F103C8T6共在io口扩展了2路模拟输入，通道0为电流，通道1为电压
 * @param	None
 * @retval	None
 */
void Io_Analog_Handle(void)
{
    mdU32 addr = ANALOG_START_ADDR;
    mdSTATUS ret;
    float temp_data[ADC_DMA_CHANNEL] = {0};

    // Get_AdcValue(ADC_CHANNEL_0);
    /*写入保持寄存器*/
    ret = mdhandler->registerPool->mdWriteHoldRegisters(mdhandler->registerPool, addr, sizeof(temp_data), (mdU16 *)&temp_data);
    /*写入失败*/
    if (ret == mdFALSE)
    {
    }
#if defined(USING_DEBUG)
    // shellPrint(&shell,"AD[%d] = 0x%d\r\n", 0, Get_AdcValue(ADC_CHANNEL_1));
#endif
}

#if defined(USING_SLAVE)
/**
 * @brief	数字量对应继电器输出
 * @details	从站仅有一路数字量继电器输出
 * @param	None
 * @retval	None
 */
void Io_Digital_Output(bool signal)
{
    mdBit bit = mdLow;
    mdU32 addr = DIGITAL_OUTPUT_START_ADDR;
    mdSTATUS ret = mdTRUE;
    /*产生超时信号，复位输出*/
    if (signal)
    {
        bit = mdLow;
        /*数据写回寄存器池*/
        mdhandler->registerPool->mdWriteCoil(mdhandler->registerPool, addr, bit);
    }
    else
    {
        /*读取远程信号*/
        ret = mdhandler->registerPool->mdReadCoil(mdhandler->registerPool, addr, &bit);
    }

    /*读取正确且没有超时信号产生*/
    if (ret == mdTRUE)
    {
        HAL_GPIO_WritePin(RELAY_GPIO_Port, RELAY_Pin, (GPIO_PinState)bit);
    }
#if defined(USING_DEBUG)
    // shellPrint(&shell,"DDOx = 0x%d\r\n", bit);
#endif
}
#endif
