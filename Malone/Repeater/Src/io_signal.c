#include "io_signal.h"
#include "io_uart.h"
#include "cmsis_os.h"
#include "mdrtuslave.h"
#include "adc.h"
#include "shell_port.h"

#define Get_Digital_Port(GPIO_Port) \
    (GPIO_Port ? GPIOA : GPIOB)

#define Get_Digital_Pin(GPIO_Pin) \
    (GPIO_Pin < 5U ? (DDI0_Pin << GPIO_Pin) : (GPIO_Pin < 7U ? (DDI5_Pin << (GPIO_Pin - 5U)) : DDI7_Pin))

DDIx_Group_HandleTypeDef DDIx_Group[EXTERN_DIGITAL_MAX] = {
    {
        {DDI0_GPIO_Port, DDI0_Pin},
        0,
    },
    {
        {DDI1_GPIO_Port, DDI1_Pin},
        0,
    },
    {
        {DDI2_GPIO_Port, DDI2_Pin},
        0,
    },
    {
        {DDI3_GPIO_Port, DDI3_Pin},
        0,
    },
    {
        {DDI4_GPIO_Port, DDI4_Pin},
        0,
    },
    {
        {DDI5_GPIO_Port, DDI5_Pin},
        0,
    },
    {
        {DDI6_GPIO_Port, DDI6_Pin},
        0,
    },
    {
        {DDI7_GPIO_Port, DDI7_Pin},
        0,
    },
};

// extern osMessageQId DDIx_QueueHandle;
DDIx_HandleTypeDef DDIx = {
    .pGroup = DDIx_Group,
    .Group_Size = sizeof(DDIx_Group) / sizeof(DDIx_Group_HandleTypeDef),
    .pQueue = NULL,
};

/**
 * @brief	外部数字量输入处理
 * @details	STM32F103C8T6共在io口扩展了8路数字输入
 * @param	None
 * @retval	None
 */
// void Io_Digital_Handle(void)
// {
//     GPIO_TypeDef *pGPIOx;
//     uint16_t GPIO_Pinx;
//     mdBit bit = mdLow;
//     mdU32 addr;
//     mdSTATUS ret;

//     for (uint16_t i = 0; i < EXTERN_DIGITAL_MAX; i++)
//     {
//         pGPIOx = Get_Digital_Port((i < 5U ? 1 : 0));
//         GPIO_Pinx = Get_Digital_Pin(i);
//         /*读取外部数字引脚状态:翻转光耦的输入信号*/
//         bit = (mdBit)HAL_GPIO_ReadPin(pGPIOx, GPIO_Pinx) ? 0 : 1;
//         /*计算出出当前写入地址*/
//         addr = DIGITAL_START_ADDR + i;
//         /*写入输入线圈*/
//         // ret = g_Mdmaster->registerPool->mdWriteCoil(g_Mdmaster->registerPool, addr, bit);
//         // ret = mdRTU_WriteCoil(Master_Object, addr, bit);
//         ret = mdRTU_WriteInputCoil(Master_Object, addr, bit);
//         /*写入失败*/
//         if (ret == mdFALSE)
//         {
// #if defined(USING_DEBUG)
//             // mdhandler->registerPool->mdReadInputCoil(mdhandler->registerPool, addr, &bit);
//             shellPrint(&shell, "DD[%d] = 0x%d\r\n", i, bit);
// #endif
//         }
//     }
// }

void Io_Digital_Handle(uint8_t *pdata)
{
    mdSTATUS ret;
    mdBit bit = mdLow;

    for (uint8_t i = 0; pdata && i < EXTERN_DIGITAL_MAX; i++)
    {
        bit = (*pdata >> i) & 0x01;
        ret = mdRTU_WriteInputCoil(Master_Object, DIGITAL_START_ADDR + i, bit);
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

// void Io_Digital_Handle(uint8_t site, uint8_t *pdata)
// {
//     mdSTATUS ret;
//     mdBit bit = *pdata;

//     if (site < EXTERN_DIGITAL_MAX)
//     {
//         ret = mdRTU_WriteInputCoil(Master_Object, DIGITAL_START_ADDR + site, bit);
//         /*写入失败*/
//         if (ret == mdFALSE)
//         {
// #if defined(USING_DEBUG)
//             // mdhandler->registerPool->mdReadInputCoil(mdhandler->registerPool, addr, &bit);
//             shellPrint(&shell, "DD[%d] = 0x%d\r\n", i, bit);
// #endif
//         }
//     }
// }

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

/**
 * @brief  Timer timeout detection.
 * @param  None
 * @retval None
 */
void DDIx_TimerOut_Check(DDIx_HandleTypeDef *pDDIx)
{
    // mdBit bits = mdLow, offset = mdLow;
    mdBit offset = mdLow;

    // UBaseType_t uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
    for (DDIx_Group_HandleTypeDef *p = pDDIx->pGroup;
         pDDIx && pDDIx->pGroup && p < pDDIx->pGroup + pDDIx->Group_Size; ++p, ++offset)
    {
        if (p->Timer_Count)
        {
            p->Timer_Count--;
        }
        else
        {
            p->Timer_Count = CHECK_DELAY_5MS;
            /*读取外部数字引脚状态:翻转光耦的输入信号*/
            // mdBit bit = (mdBit)HAL_GPIO_ReadPin(p->Gpio.pGPIOx, p->Gpio.GPIO_Pinx) ? 0 : 1;
            if (!(mdBit)HAL_GPIO_ReadPin(p->Gpio.pGPIOx, p->Gpio.GPIO_Pinx))
            {
                pDDIx->bits |= 1U << offset;
            }

            // if (pDDIx->pQueue)
            // { /* We have not woken a task at the start of the ISR. */
            //     BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            //     pDDIx->site = offset;
            //     bits = (mdBit)HAL_GPIO_ReadPin(p->Gpio.pGPIOx, p->Gpio.GPIO_Pinx) ? 0 : 1;
            //     /* Post the byte. */
            //     xQueueSendFromISR(pDDIx->pQueue, &bits, &xHigherPriorityTaskWoken);
            //     /* Now the buffer is empty we can switch context if necessary. */
            //     if (xHigherPriorityTaskWoken)
            //     {
            //         /* Actual macro used here is port specific. */
            //         taskYIELD();
            //     }
            // }

            // bits = (mdBit)HAL_GPIO_ReadPin(p->Gpio.pGPIOx, p->Gpio.GPIO_Pinx) ? 0 : 1;
            // if (Master_Object && pDDIx->pQueue)
            //     mdRTU_WriteInputCoil(Master_Object, DIGITAL_START_ADDR + offset, bits);
        }
    }
    // taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
}

/**
 * @brief  EXTI line detection callbacks.
 * @param  GPIO_Pin: Specifies the pins connected EXTI line
 * @retval None
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    DDIx_HandleTypeDef *pDDIx = &DDIx;

    /*模拟串口部分*/
    extern void HAL_Suart_EXTI_Callback(IoUart_HandleTypeDef * huart, uint16_t GPIO_Pin);
    HAL_Suart_EXTI_Callback(&S_Uart1, GPIO_Pin);

    for (DDIx_Group_HandleTypeDef *p = pDDIx->pGroup;
         pDDIx->pGroup && p < pDDIx->pGroup + pDDIx->Group_Size; ++p)
    {
        if (p->Gpio.GPIO_Pinx == GPIO_Pin)
        {
            p->Timer_Count = CHECK_DELAY_5MS;
        }
    }
}
