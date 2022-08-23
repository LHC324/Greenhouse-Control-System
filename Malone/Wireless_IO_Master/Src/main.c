/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "adc.h"
#include "dma.h"
#include "iwdg.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "shell_port.h"
#include "mdrtuslave.h"
#include "L101.h"
#include "io_uart.h"
#include "io_signal.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE BEGIN PTD */
extern void MX_RT_Thread_Init(void);
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_SPI2_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_IWDG_Init();
  /* USER CODE BEGIN 2 */
#if defined(USING_IO_UART)
  // HAL_Delay(500);
  MX_Suart_Init();
#endif
  User_Shell_Init(Shell_Object);
  ModbusInit(&Master_Object);
  /*DMA buffer must point to an entity address!!!*/
  HAL_UART_Receive_DMA(&huart1, mdRTU_Recive_Buf(Master_Object), MODBUS_PDU_SIZE_MAX);
  // Slist_Init();
#if defined(USING_RTTHREAD)
  MX_RT_Thread_Init();
#endif
  /*Only ADC1 channel 0 uses DMA*/
  /*Start hardware watchdog*/
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
  HAL_ADC_Start_DMA(&hadc1, (uint32_t *)Adc_buffer, ADC_DMA_SIZE);
  /* USER CODE END 2 */

  /* Call init function for freertos objects (in freertos.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /*After the main thread runs, it is automatically deleted by the operating system*/
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  Period elapsed callback in non blocking mode
 * @note   This function is called  when TIM1 interrupt took place, inside
 * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
 * a global variable "uwTick" used as application time base.
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */
  IoUart_HandleTypeDef *huart = &S_Uart1;
  static uint8_t Samp_Bits = 0;
  // uint8_t *p = &huart->Tx.pBuf[0];
  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1)
  {
    HAL_IncTick();
    DDIx_TimerOut_Check(&DDIx);
  }
  /* USER CODE BEGIN Callback 1 */
  else if (htim->Instance == TIM3)
  { /*Data transmission, transmission priority, no transmission before entering the receiving state*/
    if (huart->Tx.En)
    { /*Serial port sending bit status judgment*/
      switch (huart->Tx.Status)
      {
      case COM_START_BIT:
      {
        HAL_GPIO_WritePin(IO_UART_TX_GPIO_Port, IO_UART_TX_Pin, GPIO_PIN_RESET);
        huart->Tx.Status = COM_DATA_BIT;
        huart->Tx.Bits = 0U;
      }
      break;
      case COM_DATA_BIT:
      {
        HAL_GPIO_WritePin(IO_UART_TX_GPIO_Port, IO_UART_TX_Pin, (GPIO_PinState)((*(huart->Tx.pBuf) >> huart->Tx.Bits) & 0x01));

        if (++huart->Tx.Bits >= 8U)
        {
          huart->Tx.Status = huart->Check_Type ? COM_CHECK_BIT : COM_STOP_BIT;
        }
      }
      break;
      case COM_CHECK_BIT:
      {
      }
      break;
      case COM_STOP_BIT:
      {
        HAL_GPIO_WritePin(IO_UART_TX_GPIO_Port, IO_UART_TX_Pin, GPIO_PIN_SET);
        huart->Tx.Bits = 0U;
        if (--huart->Tx.Len)
        {
          huart->Tx.Status = COM_START_BIT;
          huart->Tx.pBuf++;
        }
        else
        {
          huart->Tx.Status = COM_NONE_BIT;
          huart->Tx.En = false;
          huart->Rx.En = true;
          HAL_TIM_Base_Stop_IT(huart->Tx.Timer_Handle);
          __HAL_TIM_SET_COUNTER(huart->Tx.Timer_Handle, 0U);
          /*清除外部中断线挂起寄存器*/
          __HAL_GPIO_EXTI_CLEAR_IT(IO_UART_RX_Pin);
          HAL_NVIC_EnableIRQ(huart->Rx.IRQn);
          huart->Tx.Finsh_Flag = true;
        }
      }
      break;
      default:
        break;
      }
    }
  }
  else if (htim->Instance == TIM4)
  {
    if (huart->Rx.En)
    {
      switch (huart->Rx.Status)
      {
      case COM_START_BIT:
      {
        Samp_Bits = (Samp_Bits << 1U) | (HAL_GPIO_ReadPin(IO_UART_RX_GPIO_Port, IO_UART_RX_Pin) & 0x01);

        if (++huart->Rx.Filters >= MAX_SAMPING)
        {
          if (!Get_ValidBits(Samp_Bits))
          {
            huart->Rx.Status = COM_DATA_BIT;
            __HAL_TIM_SET_AUTORELOAD(huart->Rx.Timer_Handle, huart->Rx.Sam_Times[huart->Rx.Filters]);
          }
          else
          {
            huart->Rx.Status = COM_STOP_BIT;
          }
          Samp_Bits = 0U;
          huart->Rx.Filters = 0U;
        }
        else
        {
          __HAL_TIM_SET_AUTORELOAD(huart->Rx.Timer_Handle, huart->Rx.Sam_Times[huart->Rx.Filters]);
        }
      }
      break;
      case COM_DATA_BIT: /*Data bit*/
      {
        Samp_Bits = (Samp_Bits << 1U) | (HAL_GPIO_ReadPin(IO_UART_RX_GPIO_Port, IO_UART_RX_Pin) & 0x01);

        if (++huart->Rx.Filters >= MAX_SAMPING)
        {
          huart->Rx.Data |= (Get_ValidBits(Samp_Bits) & 0x01) << huart->Rx.Bits;
          if (huart->Rx.Bits >= 7U)
          {
            huart->Rx.Bits = 0;
            huart->Rx.Status = huart->Check_Type ? COM_CHECK_BIT : COM_STOP_BIT;
          }
          else
          {
            huart->Rx.Bits++;
          }
          __HAL_TIM_SET_AUTORELOAD(huart->Rx.Timer_Handle, huart->Rx.Sam_Times[huart->Rx.Filters]);
          huart->Rx.Filters = 0U;
          Samp_Bits = 0U;
        }
        else
        {
          __HAL_TIM_SET_AUTORELOAD(huart->Rx.Timer_Handle, huart->Rx.Sam_Times[huart->Rx.Filters]);
        }
      }
      break;
      case COM_CHECK_BIT: /*Check bit*/
      {
        huart->Rx.Status = COM_NONE_BIT;
      }
      break;
      case COM_STOP_BIT: /*stop bit*/
      {
        Samp_Bits = (Samp_Bits << 1U) | (HAL_GPIO_ReadPin(IO_UART_RX_GPIO_Port, IO_UART_RX_Pin) & 0x01);

        if (++huart->Rx.Filters >= MAX_SAMPING)
        { /*Stop bit received correctly*/
          if (Get_ValidBits(Samp_Bits))
          {
            huart->Rx.Len++;
            huart->Rx.Status = COM_NONE_BIT;
            // huart->Rx.En = true;
            // huart->Tx.En = false; ///
            HAL_TIM_Base_Stop_IT(huart->Rx.Timer_Handle);
            __HAL_TIM_SET_COUNTER(huart->Rx.Timer_Handle, 0U);
            // HAL_NVIC_EnableIRQ(huart->Rx.IRQn);
            /*Two interrupts are generated when one data is received?*/
            // huart->Rx.Finsh_Flag ^= true;
            huart->Rx.Finsh_Flag = true;
          }
          huart->Rx.Filters = 0U;
          Samp_Bits = 0U;
        }
        else
        {
          __HAL_TIM_SET_AUTORELOAD(huart->Rx.Timer_Handle, huart->Rx.Sam_Times[huart->Rx.Filters]);
        }
      }
      break;
      default:
        break;
      }
    }
  }
  /* USER CODE END Callback 1 */
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
