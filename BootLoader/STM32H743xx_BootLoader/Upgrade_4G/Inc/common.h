/**
 ******************************************************************************
 * @file    IAP_Main/Inc/common.h
 * @author  MCD Application Team
 * @brief   This file provides all the headers of the common functions.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __COMMON_H
#define __COMMON_H

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
//#include "stm3210e_eval.h"
//#include "main.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Constants used by Serial Command Line Mode */
#define Get_Ms(__min) ((uint32_t)((__min)*60U * 1000U))
#define TX_TIMEOUT ((uint32_t)10000)
#define RX_TIMEOUT 2U
#define EXIT_TIMEOUT 0.5F

/* Exported macro ------------------------------------------------------------*/
#define IS_CAP_LETTER(c) (((c) >= 'A') && ((c) <= 'F'))
#define IS_LC_LETTER(c) (((c) >= 'a') && ((c) <= 'f'))
#define IS_09(c) (((c) >= '0') && ((c) <= '9'))
#define ISVALIDHEX(c) (IS_CAP_LETTER(c) || IS_LC_LETTER(c) || IS_09(c))
#define ISVALIDDEC(c) IS_09(c)
#define CONVERTDEC(c) (c - '0')

#define CONVERTHEX_ALPHA(c) (IS_CAP_LETTER(c) ? ((c) - 'A' + 10) : ((c) - 'a' + 10))
#define CONVERTHEX(c) (IS_09(c) ? ((c) - '0') : CONVERTHEX_ALPHA(c))

/* Exported functions ------------------------------------------------------- */
void Int2Str(uint8_t *p_str, uint32_t intnum);
uint32_t Str2Int(uint8_t *inputstr, uint32_t *intnum);
void Serial_PutString(uint8_t *p_string);
HAL_StatusTypeDef Serial_PutByte(uint8_t param);

#endif /* __COMMON_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
