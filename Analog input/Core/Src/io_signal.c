#include "io_signal.h"
#include "main.h"
#include "tool.h"
#include "Modbus.h"
#if defined(USING_INPUT_REGISTER) || defined(USING_HOLD_REGISTER)
#include "adc.h"
#include "Mcp4822.h"
#endif

/**
 * @brief	外部数字量输入处理
 * @details	STM32F030F4共在io口扩展了8路数字输入
 * @param	None
 * @retval	None
 */
#if defined(USING_INPUT_COIL)
void Read_Digital_Io(void)
{ /*引脚默认指向输入第一路*/
    GPIO_TypeDef *pGPIOx = NULL;
    uint16_t GPIO_Pinx = 0x00;
    pModbusHandle pd = Modbus_Object;
    uint8_t wbits[EXTERN_DIGITALIN_MAX];
    /*备份记录区*/
    static uint8_t history_wbits = 0x00;
    memset(wbits, 0x00, sizeof(wbits));
    for (uint8_t *pdata = wbits; pdata < wbits + EXTERN_DIGITALIN_MAX; GPIO_Pinx <<= 1U, pdata++)
    { /*读取外部数字引脚状态:翻转光耦的输入信号*/
        *pdata = (uint8_t)HAL_GPIO_ReadPin(pGPIOx, GPIO_Pinx) ? 0U : 1U;
        /*只要有一个状态和上一次不同，则发出中断请求*/
        if ((uint8_t)(history_wbits >> (pdata - wbits)) ^ (*pdata))
        {
            *(bool *)pd->Slave.pHandle = false;
#if defined(USING_DEBUG)
            Debug("Input coil[0x%p] status change!\r\n", pdata, *pdata);
#endif
        }
        /*当前状态为下一次历史记录*/
        if (*pdata)
            history_wbits |= (uint8_t)(history_wbits << (pdata - wbits));
        else
            history_wbits &= ~(uint8_t)(history_wbits << (pdata - wbits));
    }

    if (pd && pGPIOx)
    {
        pd->Slave.Reg_Type = InputCoil;
        pd->Slave.Operate = Write;
        /*写入对应寄存器*/
        if (!pd->Mod_Operatex(pd, INPUT_DIGITAL_START_ADDR, wbits, EXTERN_DIGITALIN_MAX))
        {
#if defined(USING_DEBUG)
            Debug("Input coil write failed!\r\n");
#endif
        }
    }
}
#endif

/**
 * @brief	数字量输出
 * @details	STM32F030F4共在io口扩展了8路数字输出
 * @param	None
 * @retval	None
 */
#if defined(USING_COIL)
void Write_Digital_IO(void)
{
    /*引脚默认指向输入第一路*/
    GPIO_TypeDef *pGPIOx = NULL;
    uint16_t GPIO_Pinx = 0x00;
    pModbusHandle pd = Modbus_Object;
    uint8_t rbits[EXTERN_DIGITALOUT_MAX];

    if (pd && pGPIOx)
    {
        pd->Slave.Reg_Type = Coil;
        pd->Slave.Operate = Read;
        /*读取对应寄存器*/
        if (!pd->Mod_Operatex(pd, OUT_DIGITAL_START_ADDR, rbits, EXTERN_DIGITALOUT_MAX))
        {
#if defined(USING_DEBUG)
            Debug("Coil reading failed!\r\n");
            return;
#endif
        }

        for (uint8_t i = 0; i < EXTERN_DIGITALOUT_MAX; i++)
            HAL_GPIO_WritePin(pGPIOx, GPIO_Pinx, (GPIO_PinState)rbits[i]);
    }
}
#endif

float Adc_Param[ADC_DMA_CHANNEL][2] = {
    {0.008941086F, 0.54000809F},
    {0.009064281F, 0.549737849F},
    {0.00907234F, 0.550323292F},
    {0.009069635F, 0.562189562F},
    {0.009066967F, 0.532366555F},
    {0.009035415F, 0.550182F},
    {0.009066897F, 0.552336103F},
    {0.009069586F, 0.554958248F},
};
/**
 * @brief	外部模拟量输入处理
 * @details	STM32F030F4共在io口扩展了8路模拟输入
 * @param	None
 * @retval	None
 */
#if defined(USING_INPUT_REGISTER)
void Read_Analog_Io(void)
{
#define MAX_RANGE 0.025F
#define CP 0.00911038F
#define CQ 0.641778298F
    static bool first_flag = false;
    pModbusHandle pd = Modbus_Object;
    uint16_t tdata = 0;
/*滤波结构需要不断迭代，否则滤波器无法正常工作*/
#if defined(KALMAN)
    KFP hkfp = {
        .Last_Covariance = LASTP,
        .Kg = 0,
        .Now_Covariance = 0,
        .Output = 0,
        .Q = COVAR_Q,
        .R = COVAR_R,
    };
    static KFP pkfp[ADC_DMA_CHANNEL];
#else
    SideParm side = {
        .First_Flag = true,
        .Head = &side.SideBuff[0],
        .SideBuff = {0},
        .Sum = 0,
    };
    static SideParm pside[ADC_DMA_CHANNEL];
#endif
    /*保证仅首次copy*/
    if (!first_flag)
    {
        first_flag = true;
        for (uint16_t ch = 0; ch < ADC_DMA_CHANNEL; ch++)
        {
#if defined(KALMAN)
            memcpy(&pkfp[ch], &hkfp, sizeof(hkfp));
#else
            memcpy(&pside[ch], &side, sizeof(pside));
#endif
        }
    }
    float *pdata = (float *)CUSTOM_MALLOC(ADC_DMA_CHANNEL * sizeof(float));
    if (!pdata)
        goto __exit;
    /*备份区，记录上一次数据*/
    static float back_data[ADC_DMA_CHANNEL] = {0};
    memset(pdata, 0x00, ADC_DMA_CHANNEL * sizeof(float));

    for (uint16_t ch = 0; ch < ADC_DMA_CHANNEL; ch++)
    { /*获取DAC值*/
        tdata = Get_AdcValue(ch);
        pdata[ch] = Adc_Param[ch][0] * tdata + Adc_Param[ch][1];
        // pdata[ch] = CP * tdata + CQ;
#if defined(USING_DEBUG)
        Debug("ch[%d]= %d.\r\n", ch, tdata);
#endif
        pdata[ch] = (pdata[ch] <= CQ) ? 0 : pdata[ch];
        /*滤波处理*/
#if defined(KALMAN)
        pdata[ch] = kalmanFilter(&pkfp[ch], pdata[ch]);
#else
        pdata[ch] = sidefilter(&pside[ch], pdata[ch]);
#endif
        /*模拟输入波动较大：发出中断信号*/
        if (fabs(back_data[ch] - pdata[ch]) >= MAX_RANGE)
        {
            *(bool *)pd->Slave.pHandle = false;
#if defined(USING_DEBUG)
            Debug("ch[%d]= %.3f.\r\n", ch, pdata[ch]);
#endif
            back_data[ch] = pdata[ch];
        }
        // back_data[ch] = pdata[ch];
        // pdata[0] = 12.5F;
        // /*大小端转换*/
        // Endian_Swap((uint8_t *)&pdata[ch], 0U, sizeof(float));

#if defined(USING_DEBUG)
        // shellPrint(Shell_Object, "R_AD[%d] = %.3f\r\n", ch, pdata[ch]);
#endif
    }
    if (pd)
    {
        pd->Slave.Reg_Type = InputRegister;
        pd->Slave.Operate = Write;
        /*写入对应寄存器*/
        if (!pd->Mod_Operatex(pd, INPUT_ANALOG_START_ADDR, (uint8_t *)pdata, EXTERN_ANALOGIN_MAX * sizeof(float)))
        {
#if defined(USING_DEBUG)
            Debug("Input register write failed!\r\n");
#endif
        }
    }

__exit:
    CUSTOM_FREE(pdata);
}
#endif

/**
 * @brief	模拟量输出
 * @details	STM32F030F4共在io口扩展了8路数字输出
 * @param	None
 * @retval	None
 */
#if defined(USING_HOLD_REGISTER)
// void Write_Analog_IO(void)
// {
//     Dac_Obj dac_object[EXTERN_ANALOGOUT_MAX] = {
//         {.Channel = DAC_OUT1, .Mcpxx_Id = Input_A},
//         {.Channel = DAC_OUT2, .Mcpxx_Id = Input_B},
//         {.Channel = DAC_OUT3, .Mcpxx_Id = Input_Other},
//         {.Channel = DAC_OUT4, .Mcpxx_Id = Input_Other},
//     };
//     pModbusHandle pd = Modbus_Object;

//     float *pdata = (float *)CUSTOM_MALLOC(EXTERN_ANALOGOUT_MAX * sizeof(float));
//     if (!pdata)
//         goto __exit;
//     memset(pdata, 0x00, EXTERN_ANALOGOUT_MAX * sizeof(float));
//     /*读出保持寄存器*/
//     if (pd)
//     {
//         pd->Slave.Reg_Type = HoldRegister;
//         pd->Slave.Operate = Read;
//         /*写入对应寄存器*/
//         if (!pd->Mod_Operatex(pd, OUT_ANALOG_START_ADDR, (uint8_t *)pdata, EXTERN_ANALOGOUT_MAX * sizeof(float)))
//         {
// #if defined(USING_DEBUG)
//             Debug("Hold register read failed!\r\n");
// #endif
//         }
//     }

//     for (uint16_t ch = 0; ch < EXTERN_ANALOGOUT_MAX; ch++)
//     {
//         /*大小端转换*/
//         Endian_Swap((uint8_t *)&pdata[ch], 0U, sizeof(float));
// #if defined(USING_DEBUG)
//         // shellPrint(Shell_Object, "W_AD[%d] = %.3f\r\n", ch, pdata[ch]);
// #endif
//         Output_Current(&dac_object[ch], pdata[ch]);
//     }

// __exit:
//     CUSTOM_FREE(pdata);
// }
#endif
