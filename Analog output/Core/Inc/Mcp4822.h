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
#include "tool.h"

#define USING_SOFTE_NSS

#define INPUT_A 0x3000
#define INPUT_B 0xB000
#define SPI_TIMEOUT 0xFFFF

    typedef enum
    {
        DAC_OUT1 = 0x00,
        DAC_OUT2,
        DAC_OUT3,
        DAC_OUT4,
        DAC_OUT5,
        DAC_OUT6,
        DAC_OUT7,
        DAC_OUT8,
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
        Gpiox_info CS;
    } Dac_HandleTypeDef;

    extern Dac_HandleTypeDef Dac_HandleTypeDefect;
    extern void Mcp48xx_Write(Dac_HandleTypeDef *p_ch, unsigned short data);
    extern void Output_Current(Dac_HandleTypeDef *p_ch, float data);

#ifdef __cplusplus
}
#endif

#endif /* INC_MCP4822_H_ */
