/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
//#define USING_DEBUG 1
#define USING_RTOS
#define MDTASK_SENDTIMES 50U
// #define USING_L101
//#define USING_ASHINING
//#define USING_TRANSPARENT_MODE
#define USING_REPEATER_MODE
#define USING_IO_UART
#if defined(USING_FREERTOS)
/*Custom memory management*/
#define CUSTOM_MALLOC pvPortMalloc
#define CUSTOM_FREE vPortFree
#else
#define CUSTOM_MALLOC malloc
#define CUSTOM_FREE free
#endif
// #define USING_AT
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
/*Terminal key value*/
#define ESC_CODE 0x1B
#define BACKSPACE_CODE 0x08
#define ENTER_CODE 0x0D
#define SPOT_CODE 0x2E
#define COMMA_CODE 0x2C
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
#define GET_TIMEOUT_FLAG(Stime, Ctime, timeout, MAX) \
  ((Ctime) < (Stime) ? !!((((MAX) - (Stime)) + (Ctime)) > (timeout)) : !!((Ctime) - (Stime) > (timeout)))
/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define AIIN_Pin GPIO_PIN_1
#define AIIN_GPIO_Port GPIOA
#define AIVN_Pin GPIO_PIN_2
#define AIVN_GPIO_Port GPIOA
#define DDI0_Pin GPIO_PIN_3
#define DDI0_GPIO_Port GPIOA
#define DDI0_EXTI_IRQn EXTI3_IRQn
#define DDI1_Pin GPIO_PIN_4
#define DDI1_GPIO_Port GPIOA
#define DDI1_EXTI_IRQn EXTI4_IRQn
#define DDI2_Pin GPIO_PIN_5
#define DDI2_GPIO_Port GPIOA
#define DDI2_EXTI_IRQn EXTI9_5_IRQn
#define DDI3_Pin GPIO_PIN_6
#define DDI3_GPIO_Port GPIOA
#define DDI3_EXTI_IRQn EXTI9_5_IRQn
#define DDI4_Pin GPIO_PIN_7
#define DDI4_GPIO_Port GPIOA
#define DDI4_EXTI_IRQn EXTI9_5_IRQn
#define DDI5_Pin GPIO_PIN_0
#define DDI5_GPIO_Port GPIOB
#define DDI5_EXTI_IRQn EXTI0_IRQn
#define DDI6_Pin GPIO_PIN_1
#define DDI6_GPIO_Port GPIOB
#define DDI6_EXTI_IRQn EXTI1_IRQn
#define DDI7_Pin GPIO_PIN_10
#define DDI7_GPIO_Port GPIOB
#define DDI7_EXTI_IRQn EXTI15_10_IRQn
#define WDT_Pin GPIO_PIN_11
#define WDT_GPIO_Port GPIOB
#define IO_UART_RX_Pin GPIO_PIN_8
#define IO_UART_RX_GPIO_Port GPIOA
#define IO_UART_RX_EXTI_IRQn EXTI9_5_IRQn
#define IO_UART_TX_Pin GPIO_PIN_9
#define IO_UART_TX_GPIO_Port GPIOA
#define RELOAD_Pin GPIO_PIN_10
#define RELOAD_GPIO_Port GPIOA
#define WEAKUP_Pin GPIO_PIN_11
#define WEAKUP_GPIO_Port GPIOA
#define STATUS_Pin GPIO_PIN_12
#define STATUS_GPIO_Port GPIOA
#define LORA_RX_Pin GPIO_PIN_6
#define LORA_RX_GPIO_Port GPIOB
#define LORA_TX_Pin GPIO_PIN_7
#define LORA_TX_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
