#include "io_signal.h"
#include "main.h"
#include "mdrtuslave.h"
#include "adc.h"
#include "shell_port.h"

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
void Io_Digital_Handle(void)
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
        /*读取外部数字引脚状态:翻转光耦的输入信号*/
        bit = (mdBit)HAL_GPIO_ReadPin(pGPIOx, GPIO_Pinx) ? 0 : 1;
        /*计算出出当前写入地址*/
        addr = DIGITAL_START_ADDR + i;
        /*写入输入线圈*/
        // ret = g_Mdmaster->registerPool->mdWriteCoil(g_Mdmaster->registerPool, addr, bit);
        // ret = mdRTU_WriteCoil(Master_Object, addr, bit);
        ret = mdRTU_WriteInputCoil(Master_Object, addr, bit);
        /*写入失败*/
        if (ret == mdFALSE)
        {
#if defined(USING_DEBUG)
            // mdhandler->registerPool->mdReadInputCoil(mdhandler->registerPool, addr, &bit);
            shellPrint(&shell, "DD[%d] = 0x%d\r\n", i, bit);
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
    // ret = g_Mdmaster->registerPool->mdWriteHoldRegisters(g_Mdmaster->registerPool, addr, sizeof(temp_data), (mdU16 *)&temp_data);
    ret = mdRTU_WriteHoldRegs(Master_Object, addr, sizeof(temp_data), temp_data);

    /*写入失败*/
    if (ret == mdFALSE)
    {
    }
#if defined(USING_DEBUG)
    // shellPrint(&shell,"AD[%d] = 0x%d\r\n", 0, Get_AdcValue(ADC_CHANNEL_1));
#endif
}
