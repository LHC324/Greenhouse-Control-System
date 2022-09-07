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
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
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
#include "stdbool.h"
#include "string.h"
#include "math.h"
#include "stdio.h"
#include <stdarg.h>
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
// #define USING_DEBUG 1
#define USING_RTOS
#define RELAY_CLOSE_TIMES 1000U
// #define USING_L101
#define USING_IO_UART
/*as a repeater*/
//#define AS_REPEATER
/*Use relay mode*/
#define USING_REPEATER_MODE
#if defined(USING_FREERTOS)
/*Custom memory management*/
#define CUSTOM_MALLOC pvPortMalloc
#define CUSTOM_FREE vPortFree
#else
#define CUSTOM_MALLOC malloc
#define CUSTOM_FREE free
#endif
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
#define AIIN_Pin GPIO_PIN_0
#define AIIN_GPIO_Port GPIOA
#define WAKEUP_Pin GPIO_PIN_1
#define WAKEUP_GPIO_Port GPIOA
#define AVIN_Pin GPIO_PIN_2
#define AVIN_GPIO_Port GPIOA
#define RELOAD_Pin GPIO_PIN_1
#define RELOAD_GPIO_Port GPIOB
#define IO_UART_RX_Pin GPIO_PIN_8
#define IO_UART_RX_GPIO_Port GPIOA
#define IO_UART_RX_EXTI_IRQn EXTI9_5_IRQn
#define IO_UART_TX_Pin GPIO_PIN_9
#define IO_UART_TX_GPIO_Port GPIOA
#define STATUS_Pin GPIO_PIN_10
#define STATUS_GPIO_Port GPIOA
#define DDI0_Pin GPIO_PIN_11
#define DDI0_GPIO_Port GPIOA
#define WDI_Pin GPIO_PIN_12
#define WDI_GPIO_Port GPIOA
#define RELAY_Pin GPIO_PIN_9
#define RELAY_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
