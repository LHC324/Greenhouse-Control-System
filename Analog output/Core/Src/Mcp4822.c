/*
 * MCP48xx.c
 *
 *  Created on: Jan 11, 2021
 *      Author: LHC
 */
#include "Mcp4822.h"
#include "spi.h"
#include "tool.h"

/*定义DAC输出对象*/
// Dac_Obj dac_object;

float gDac_OutPrarm[8U][2U] = {
	{200.61237F, -1.50459F},
	{199.14890F, 10.45532F},
	{204.49438F, -19.42697F},
	{202.72280F, -20.27230F},
	{200.61237F, -1.50459F},
	{199.14890F, 10.45532F},
	{204.49438F, -19.42697F},
	{202.72280F, -20.27230F},
};

/**
 * @brief	对MCP48xx写数据
 * @details
 * @param	data:写入的数据
 * @param   p_ch :当前充电通道指针
 * @retval	None
 */
void Mcp48xx_Write(Dac_HandleTypeDef *p_ch, uint16_t data)
{
	SPI_HandleTypeDef *pSpi = &hspi1;

	if (pSpi && (p_ch->CS.pGPIOx))
	{
		data &= 0x0FFF;
		/*选择mcp4822通道*/
		data |= (p_ch->Mcpxx_Id ? INPUT_B : INPUT_A);
		/*软件拉低CS信号*/
		HAL_GPIO_WritePin(p_ch->CS.pGPIOx, p_ch->CS.Gpio_Pin | CS_Pin, GPIO_PIN_RESET);
		/*调用硬件spi发送函数*/
		HAL_SPI_Transmit(pSpi, (uint8_t *)&data, sizeof(data), SPI_TIMEOUT);
		/*软件拉高CS信号*/
		HAL_GPIO_WritePin(p_ch->CS.pGPIOx, p_ch->CS.Gpio_Pin | CS_Pin, GPIO_PIN_SET);
	}
#if defined(USING_DEBUG)
	// shellPrint(&shell,"AD[%d] = 0x%d\r\n", 0, Get_AdcValue(ADC_CHANNEL_1));
	// Usart1_Printf("cs1 = %d, pGPIOx = %d\r\n", CS1_GPIO_Port, pGPIOx);
#endif
}

/**
 * @brief	对目标通道输出电流
 * @details
 * @param   p_ch :目标通道
 * @param	data:写入的数据
 * @retval	None
 */
void Output_Current(Dac_HandleTypeDef *p_ch, float data)
{
#define CURRENT_MAX 21.0F
	/*浮点数电流值转换为uint16_t*/
	uint16_t value = (uint16_t)(gDac_OutPrarm[p_ch->Channel][0] * data + gDac_OutPrarm[p_ch->Channel][1]);

	value = data ? ((data >= CURRENT_MAX) ? 0xFFFF : value) : 0U;
	Mcp48xx_Write(p_ch, value);
}
