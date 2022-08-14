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
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

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
// #define USING_DEBUG 1
// #define USING_DEBUG_APPLICATION
#define USING_RTOS
#define USING_DMA
/*Custom memory management*/
#define CUSTOM_MALLOC pvPortMalloc
#define CUSTOM_FREE vPortFree
#define CURRENT_HARDWARE_VERSION 140
#define CURRENT_SOFT_VERSION 150
#define SYSTEM_VERSION() ((uint32_t)((CURRENT_HARDWARE_VERSION << 16U) | CURRENT_SOFT_VERSION))
#define PARAM_BASE_ADDR 0x0010
// #define MDUSER_NAME_ADDR  0x0038
// #define SOFT_VERSION_ADDR 0x003A
#define SAVE_SIZE 26U
#define SAVE_SURE_CODE 0x5AA58734
// #define UPDATE_FLAG_FLASH_PAGE 254
#define UPDATE_SAVE_ADDRESS ((uint32_t)0x081C0000) /* Base @ of Sector 6, Bank2, 128 Kbyte */
// #define USER_SAVE_FLASH_PAGE 255U
#define PARAM_SAVE_ADDRESS ((uint32_t)0x081E0000) /* Base @ of Sector 7, Bank2, 128 Kbyte */
#define UPDATE_CMD 0x1234
#define UPDATE_APP1 0x5AA5
#define UPDATE_APP2 0xA55A
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
#define REPORT_TIMES 1200U
#define MD_TIMES 1000U
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
#define ESC_CODE 0x1B
#define BACKSPACE_CODE 0x08
#define ENTER_CODE 0x0D
#define SPOT_CODE 0x2E
#define COMMA_CODE 0x2C

/*Use ready list*/
#define USING_FREERTOS_LIST 0

#define CARD_NUM_MAX 0x10

  typedef enum
  {
    Card_AnalogInput = 0x00,
    Card_AnalogOutput = 0x10,
    Card_DigitalInput = 0x20,
    Card_DigitalOutput = 0x30,
    Card_Wifi = 0xC0,
    Card_4G = 0xD0,
    Card_Lora1 = 0xE0,
    Card_Lora2 = 0xF0,
    Card_None = 0x55,
  } Card_Tyte;
  typedef struct
  {
    uint8_t SlaveId;
    uint16_t Priority;
    Card_Tyte TypeCoding;
    uint16_t Number;
    // bool flag;
  } IRQ_Code;

  typedef struct
  {
    uint16_t site;
    uint16_t Priority;
    bool flag;
  } IRQ_Request;

  typedef struct
  {
    IRQ_Request *pReIRQ;
    uint16_t SiteCount;
    IRQ_Code *pIRQ;
    uint16_t TableCount;
    // uint16_t Amount;

#if (USING_FREERTOS_LIST)
    List_t *LReady, *LBlock;
#endif
  } Slave_IRQTableTypeDef __attribute__((aligned(4)));

  typedef struct
  {
    float Ptank_M;
    float Pvap_outlet;
    float Pgas_soutlet;
    float Ltank;
    float Ptank_S;
    float Flowmeter_A;
    float Flowmeter_B;
  } Save_User;

  typedef struct
  {
    float Ptank_max;        //储槽压力上限
    float Ptank_min;        //储槽压力下限
    float Pvap_outlet_max;  //汽化器出口压力上限
    float Pvap_outlet_min;  //汽化器出口压力下限
    float Pgas_soutlet_max; //气站出口压力上限
    float Pgas_soutlet_min; //气站出口压力下限
    float Ltank_max;        //储槽液位高度上限
    float Ltank_min;        //储槽液位高度下限
    float Ptoler_upper;     //流量计量程上限
    float Ptoler_lower;     //流量计量程下限
    float Ltoler_upper;     //液位容差上限
    // float Ltoler_lower; //液位容差下限
    float PStank_supplement; //启动模式：储槽补压启动值
    /*Threshold 2*/
    float PSspf_start;        //启动模式：储槽泄压启动值
    float PSspf_stop;         //启动模式：储槽泄压停止值
    float PSvap_outlet_Start; //启动模式：汽化器补压停止值
    float PSvap_outlet_stop;  //启动模式：汽化器补压启动值
    float Pback_difference;   //汽化器回压阈值
    float Ptank_difference;   //储槽回压阈值
    float PPvap_outlet_Start; //停机模式：汽化器泄压启动值
    float PPvap_outlet_stop;  //停机模式：汽化器泄压停止值
    float PPspf_start;        //停机模式储槽泄压启动值
    float PPspf_stop;         //停机模式储槽泄压停止值
    float Ptank_limit;        //安全策略：储槽压力阈值
    float Ltank_limit;        //安全策略：储槽液位阈值
    float Htank;              //储罐参数：储槽封头内高度
    float Rtank;              //储罐参数：储槽圆柱半径
    float LEtank;             //储罐参数：储槽圆体长度
    float Dtank;              //储槽参数：储槽液体密度

    uint32_t Error_Code;
    uint16_t User_Name;
    uint16_t User_Code;
    uint32_t System_Version;
    uint32_t System_Flag;
    struct
    {
      // IRQ_Request ReIRQ[16];
      // uint16_t SiteCount;
      IRQ_Code IRQ[16];
      uint16_t TableCount;
      uint32_t IRQ_Table_SetFlag;
    } Slave_IRQ_Table;
    uint32_t crc16;
  } Save_Param;
  typedef struct
  {
    Save_User User;
    Save_Param Param;
  } Save_HandleTypeDef __attribute__((aligned(4)));

  typedef struct
  {
    uint16_t counts;
    bool flag;
  } Soft_Timer_HandleTypeDef;

  extern Save_HandleTypeDef Save_Flash;
  extern Save_Param Save_InitPara;
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
#define M_IRQ2_Pin GPIO_PIN_2
#define M_IRQ2_GPIO_Port GPIOH
#define M_IRQ2_EXTI_IRQn EXTI2_IRQn
#define M_IRQ3_Pin GPIO_PIN_3
#define M_IRQ3_GPIO_Port GPIOH
#define M_IRQ3_EXTI_IRQn EXTI3_IRQn
#define M_IRQ4_Pin GPIO_PIN_4
#define M_IRQ4_GPIO_Port GPIOH
#define M_IRQ4_EXTI_IRQn EXTI4_IRQn
#define M_IRQ5_Pin GPIO_PIN_5
#define M_IRQ5_GPIO_Port GPIOH
#define M_IRQ5_EXTI_IRQn EXTI9_5_IRQn
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
#define M_IRQ6_Pin GPIO_PIN_6
#define M_IRQ6_GPIO_Port GPIOH
#define M_IRQ6_EXTI_IRQn EXTI9_5_IRQn
#define M_IRQ7_Pin GPIO_PIN_7
#define M_IRQ7_GPIO_Port GPIOH
#define M_IRQ7_EXTI_IRQn EXTI9_5_IRQn
#define M_IRQ8_Pin GPIO_PIN_8
#define M_IRQ8_GPIO_Port GPIOH
#define M_IRQ8_EXTI_IRQn EXTI9_5_IRQn
#define M_IRQ9_Pin GPIO_PIN_9
#define M_IRQ9_GPIO_Port GPIOH
#define M_IRQ9_EXTI_IRQn EXTI9_5_IRQn
#define M_IRQ10_Pin GPIO_PIN_10
#define M_IRQ10_GPIO_Port GPIOH
#define M_IRQ10_EXTI_IRQn EXTI15_10_IRQn
#define M_IRQ11_Pin GPIO_PIN_11
#define M_IRQ11_GPIO_Port GPIOH
#define M_IRQ11_EXTI_IRQn EXTI15_10_IRQn
#define M_IRQ12_Pin GPIO_PIN_12
#define M_IRQ12_GPIO_Port GPIOH
#define M_IRQ12_EXTI_IRQn EXTI15_10_IRQn
#define WIFI_TX_Pin GPIO_PIN_12
#define WIFI_TX_GPIO_Port GPIOB
#define WIFI_RX_Pin GPIO_PIN_13
#define WIFI_RX_GPIO_Port GPIOB
#define CPU_TX_Pin GPIO_PIN_14
#define CPU_TX_GPIO_Port GPIOB
#define CPU_RX_Pin GPIO_PIN_15
#define CPU_RX_GPIO_Port GPIOB
#define WIFI_LINK_Pin GPIO_PIN_7
#define WIFI_LINK_GPIO_Port GPIOC
#define WIFI_READY_Pin GPIO_PIN_8
#define WIFI_READY_GPIO_Port GPIOC
#define WIFI_RESET_Pin GPIO_PIN_9
#define WIFI_RESET_GPIO_Port GPIOC
#define M_IRQ13_Pin GPIO_PIN_13
#define M_IRQ13_GPIO_Port GPIOH
#define M_IRQ13_EXTI_IRQn EXTI15_10_IRQn
#define M_IRQ14_Pin GPIO_PIN_14
#define M_IRQ14_GPIO_Port GPIOH
#define M_IRQ14_EXTI_IRQn EXTI15_10_IRQn
#define M_IRQ15_Pin GPIO_PIN_15
#define M_IRQ15_GPIO_Port GPIOH
#define M_IRQ15_EXTI_IRQn EXTI15_10_IRQn
#define M_IRQ0_Pin GPIO_PIN_0
#define M_IRQ0_GPIO_Port GPIOI
#define M_IRQ0_EXTI_IRQn EXTI0_IRQn
#define M_IRQ1_Pin GPIO_PIN_1
#define M_IRQ1_GPIO_Port GPIOI
#define M_IRQ1_EXTI_IRQn EXTI1_IRQn
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
/*Target address: target member +1*/
#define GET_PARAM_SITE(TYPE, MEMBER, SIZE) (offsetof(TYPE, MEMBER) / sizeof(SIZE))
#define PARAM_END_ADDR (PARAM_BASE_ADDR + GET_PARAM_SITE(Save_Param, Error_Code, uint16_t))
#define MDUSER_NAME_ADDR (PARAM_BASE_ADDR + GET_PARAM_SITE(Save_Param, User_Code, uint16_t))
#define SOFT_VERSION_ADDR (PARAM_BASE_ADDR + GET_PARAM_SITE(Save_Param, Slave_IRQ_Table, uint16_t))
  /* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
