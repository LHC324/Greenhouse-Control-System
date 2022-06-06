/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    tim.h
  * @brief   This file contains all the function prototypes for
  *          the tim.c file
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
#ifndef __TIM_H__
#define __TIM_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
  typedef void (*ptime)(void);
  typedef struct
  {
    uint16_t counter;        /*current count value*/
    uint16_t target_counter; /*target count*/
    bool execute_flag;       /*execute flag*/
    bool enable;             /*enable flag*/
    ptime event;             /*corresponding event*/
  } SoftTimer;

  typedef struct
  {
    SoftTimer *pmap;
    uint8_t numbers;
  } Timer_HandleTypeDef __attribute__((aligned(4)));

  extern Timer_HandleTypeDef *ptimer;
  extern void Timer_Handle(void);
/* USER CODE END Includes */

extern TIM_HandleTypeDef htim1;

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_TIM1_Init(void);

/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __TIM_H__ */

