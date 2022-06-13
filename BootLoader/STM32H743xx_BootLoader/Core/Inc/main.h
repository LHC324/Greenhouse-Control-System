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
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
#define UPDATE_SAVE_ADDRESS ((uint32_t)0x081C0000)
#define UPDATE_CMD 0x1234
#define UPDATE_APP1 0x5AA5
#define UPDATE_APP2 0xA55A
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
#define CARD6_Pin GPIO_PIN_8
#define CARD6_GPIO_Port GPIOI
#define CARD7_Pin GPIO_PIN_9
#define CARD7_GPIO_Port GPIOI
#define CARD8_Pin GPIO_PIN_10
#define CARD8_GPIO_Port GPIOI
#define CARD9_Pin GPIO_PIN_11
#define CARD9_GPIO_Port GPIOI
#define LTE_LINK_Pin GPIO_PIN_0
#define LTE_LINK_GPIO_Port GPIOA
#define LTE_NET_Pin GPIO_PIN_1
#define LTE_NET_GPIO_Port GPIOA
#define LTE_RX_Pin GPIO_PIN_2
#define LTE_RX_GPIO_Port GPIOA
#define LTE_TX_Pin GPIO_PIN_3
#define LTE_TX_GPIO_Port GPIOA
#define LTE_RESET_Pin GPIO_PIN_4
#define LTE_RESET_GPIO_Port GPIOA
#define LTE_RELOAD_Pin GPIO_PIN_5
#define LTE_RELOAD_GPIO_Port GPIOA
#define USB_EN_Pin GPIO_PIN_7
#define USB_EN_GPIO_Port GPIOA
#define LED_Pin GPIO_PIN_1
#define LED_GPIO_Port GPIOB
#define WIFI_RELOAD_Pin GPIO_PIN_11
#define WIFI_RELOAD_GPIO_Port GPIOB
#define WIFI_TX_Pin GPIO_PIN_12
#define WIFI_TX_GPIO_Port GPIOB
#define WIFI_RX_Pin GPIO_PIN_13
#define WIFI_RX_GPIO_Port GPIOB
#define WIFI_LINK_Pin GPIO_PIN_7
#define WIFI_LINK_GPIO_Port GPIOC
#define WIFI_READY_Pin GPIO_PIN_8
#define WIFI_READY_GPIO_Port GPIOC
#define WIFI_RESET_Pin GPIO_PIN_9
#define WIFI_RESET_GPIO_Port GPIOC
#define CARD0_Pin GPIO_PIN_2
#define CARD0_GPIO_Port GPIOI
#define CARD1_Pin GPIO_PIN_3
#define CARD1_GPIO_Port GPIOI
#define RS485_RX_Pin GPIO_PIN_10
#define RS485_RX_GPIO_Port GPIOC
#define RS485_TX_Pin GPIO_PIN_11
#define RS485_TX_GPIO_Port GPIOC
#define CARD10_Pin GPIO_PIN_9
#define CARD10_GPIO_Port GPIOG
#define CARD11_Pin GPIO_PIN_10
#define CARD11_GPIO_Port GPIOG
#define CARD12_Pin GPIO_PIN_11
#define CARD12_GPIO_Port GPIOG
#define CARD13_Pin GPIO_PIN_12
#define CARD13_GPIO_Port GPIOG
#define CARD14_Pin GPIO_PIN_13
#define CARD14_GPIO_Port GPIOG
#define CARD15_Pin GPIO_PIN_14
#define CARD15_GPIO_Port GPIOG
#define RS232_TX_Pin GPIO_PIN_8
#define RS232_TX_GPIO_Port GPIOB
#define RS232_RX_Pin GPIO_PIN_9
#define RS232_RX_GPIO_Port GPIOB
#define CARD2_Pin GPIO_PIN_4
#define CARD2_GPIO_Port GPIOI
#define CARD3_Pin GPIO_PIN_5
#define CARD3_GPIO_Port GPIOI
#define CARD4_Pin GPIO_PIN_6
#define CARD4_GPIO_Port GPIOI
#define CARD5_Pin GPIO_PIN_7
#define CARD5_GPIO_Port GPIOI
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
