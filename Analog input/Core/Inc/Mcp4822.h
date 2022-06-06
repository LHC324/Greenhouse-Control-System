/*
 * MCP48xx.h
 *
 *  Created on: Jan 11, 2021
 *      Author: play
 */

#ifndef INC_MCP4822_H_
#define INC_MCP4822_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define MULTI_SLAVE 1

#define INPUT_A 0x3000
#define INPUT_B 0xB000
#define SPI_TIMEOUT 0xFFFF

#define MCP_SCK_PORT SPI_SCK_GPIO_Port
#define MCP_SCK_PIN SPI_SCK_Pin
#define MCP_SDI_PORT SPI_MOSI_GPIO_Port
#define MCP_SDI_PIN SPI_MOSI_Pin

#if (MULTI_SLAVE)

#define MCP_CS_PORT SPI_CS1_GPIO_Port
#define MCP_CS_PIN SPI_CS1_Pin

#define MCP_CS2_PORT SPI_CS2_GPIO_Port
#define MCP_CS2_PIN SPI_CS2_Pin

#define CS_1 1
#define CS_2 2

#else

#define MCP_CS_PORT SPI_CS1_GPIO_Port
#define MCP_CS_PIN SPI_CS1_Pin

#define CS_1 1

#endif

    typedef enum
    {
        DAC_OUT1 = 0x00,
        DAC_OUT2,
        DAC_OUT3,
        DAC_OUT4
    } Target_Channel;

    typedef enum
    {
        Input_A = 0x00,
        Input_B,
        Input_Other
    } MCP48xx_Channel;

    typedef struct
    {
        MCP48xx_Channel Mcpxx_Id;
        Target_Channel Channel;
    } Dac_Obj;

    extern Dac_Obj dac_object;
    extern void Mcp48xx_Write(Dac_Obj *p_ch, unsigned short data);
    extern void Output_Current(Dac_Obj *p_ch, float data);

#ifdef __cplusplus
}
#endif

#endif /* INC_MCP4822_H_ */
