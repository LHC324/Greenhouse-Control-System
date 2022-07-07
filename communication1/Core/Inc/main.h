/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
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
#include "stm32f0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
#define CUSTOM_MALLOC malloc
#define CUSTOM_FREE free
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define CODE2_Pin GPIO_PIN_13
#define CODE2_GPIO_Port GPIOC
#define CODE3_Pin GPIO_PIN_14
#define CODE3_GPIO_Port GPIOC
#define M_IRQ_Pin GPIO_PIN_15
#define M_IRQ_GPIO_Port GPIOC
#define TYPE0_Pin GPIO_PIN_0
#define TYPE0_GPIO_Port GPIOB
#define TYPE1_Pin GPIO_PIN_1
#define TYPE1_GPIO_Port GPIOB
#define TYPE2_Pin GPIO_PIN_2
#define TYPE2_GPIO_Port GPIOB
#define TYPE3_Pin GPIO_PIN_12
#define TYPE3_GPIO_Port GPIOB
#define CPU_TX_Pin GPIO_PIN_6
#define CPU_TX_GPIO_Port GPIOB
#define CPU_RX_Pin GPIO_PIN_7
#define CPU_RX_GPIO_Port GPIOB
#define CODE0_Pin GPIO_PIN_8
#define CODE0_GPIO_Port GPIOB
#define CODE1_Pin GPIO_PIN_9
#define CODE1_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
