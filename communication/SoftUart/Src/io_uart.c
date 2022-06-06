#include "io_uart.h"
#include "tim.h"
// #include "shell_port.h"
#if defined(USING_RTTHREAD)
#include "rtthread.h"
#else
// #include "cmsis_os.h"
#endif

/*定义串口*/
IoUart_HandleTypeDef S_Uart1 = {0};
// static uint16_t g_table[MAX_PARAM] = {0};

/**
 * @brief 定义模拟串口相关波特率表
 * @4800bps             __________________
 * |     Start_Bit     |
 * | 80   20   20   88 | 单位:us
 * |____|____|____|____|
 * @param rate 目标波特率
 * @param table 参数表
 * @return true/false
 */
// static bool Get_BaudTable(uint16_t rate, uint16_t *table)
// {
//     uint16_t temp_table[MAX_PARAM] = {
//         GET_RULE1(rate), GET_RULE2(rate), GET_RULE2(rate), GET_RULE3(rate), BIT_TIMES(rate)};
//     if (rate > MAX_IOUART_BAUDRATE)
//     {
//         return false;
//     }
//     return (memcpy(table, temp_table, sizeof(temp_table)) ? true : false);
// }

static bool Get_BaudTable(IoUart_HandleTypeDef *huart, uint32_t rate)
{
  if (rate > MAX_IOUART_BAUDRATE)
  {
    return false;
  }
  huart->Tx.Total_Times = BIT_TIMES(rate);
  huart->Rx.Sam_Times[Start_Recv] = GET_RULE1(rate);
  huart->Rx.Sam_Times[Sampling2] = GET_RULE2(rate);
  huart->Rx.Sam_Times[Sampling3] = GET_RULE2(rate);
  huart->Rx.Sam_Times[Sampling1] = GET_RULE3(rate);

  return true;
}

/**
 * @brief	从三次采样中取得有效数据
 * @details
 * @param	n 采集记录的位值
 * @retval	true/false
 */
bool Get_ValidBits(uint8_t n)
{
  uint8_t sum = 0;

  for (; n; n >>= 1U)
  {
    sum += n & 0x01;
  }

  return ((sum < 2) ? (false) : (true));
}

/**
 * @brief	模拟串口初始化
 * @details 目标定时器模块不启用自动装载模式！
 * @param	None
 * @retval	None
 */
void MX_Suart_Init(void)
{
  // Get_BaudTable(User_BaudRate, &g_table);
  // Get_BaudTable(&S_Uart1, User_BaudRate);
  // S_Uart1.Check_Type = NONE;
  // S_Uart1.Tx.Finsh_Flag = false;
  // S_Uart1.Tx.Status = COM_NONE_BIT;
  // S_Uart1.Tx.Timer_Handle = &htim3;
  // S_Uart1.Tx.pGPIOx =
  // S_Uart1.Rx.Finsh_Flag = false;
  // S_Uart1.Rx.Status = COM_NONE_BIT;
  // S_Uart1.Rx.Timer_Handle = &htim4;
  // S_Uart1.Rx.IRQn = EXTI9_5_IRQn;
}

/*获得接收超时标志*/
// #define GET_SUART_FLAG(time, timeout) \
//     ((HAL_GetTick() - time > timeout) ? true : false)

#define GET_SUART_FLAG(time, timeout) \
  (!!(HAL_GetTick() - time > timeout))

/**
 * @brief	模拟串口发送数据
 * @details
 * @param	huart 模拟串口句柄
 * @param   PData 数据指针
 * @param   Size  数据长度
 * @param   Timeout 超时时间
 * @retval	HAL_OK/HAL_BUSY/HAL_ERROR
 */
HAL_StatusTypeDef HAL_SUART_Transmit(IoUart_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
  uint32_t tickstart = 0U;

  if ((pData == NULL) || (Size == 0U))
  {
    return HAL_ERROR;
  }
  /* Init tickstart for timeout managment */
  tickstart = HAL_GetTick();
  huart->Tx.Len = Size;
  huart->Tx.pBuf = pData;
  if (huart->Tx.Status == COM_NONE_BIT)
  {
    huart->Tx.Status = COM_START_BIT;
    huart->Tx.En = true;
    // huart->Rx.En = false; ///
    __HAL_TIM_SET_COUNTER(huart->Tx.Timer_Handle, 0U);
    __HAL_TIM_SET_AUTORELOAD(huart->Tx.Timer_Handle, huart->Tx.Total_Times);
    HAL_TIM_Base_Start_IT(huart->Tx.Timer_Handle);

    while (!huart->Tx.Finsh_Flag)
    {
      // if (GET_SUART_FLAG(tickstart, Timeout) || (Timeout == 0U))
      if (GET_TIMEOUT_FLAG(tickstart, HAL_GetTick(), Timeout, HAL_MAX_DELAY) || (Timeout == 0U))
      {
        return HAL_TIMEOUT;
      }
    }
    huart->Tx.Finsh_Flag = false;

    return HAL_OK;
  }
  return HAL_BUSY;
}

/**
 * @brief	模拟串口接收数据
 * @details
 * @param	huart 模拟串口句柄
 * @param   PData 数据指针
 * @param   Size  数据长度
 * @param   Timeout 超时时间
 * @retval	HAL_OK/HAL_BUSY
 */
HAL_StatusTypeDef HAL_SUART_Receive(IoUart_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
  uint32_t tickstart = 0U;

  if (huart->Rx.Status == COM_NONE_BIT)
  {
    if ((pData == NULL) || (Size == 0U))
    {
      return HAL_ERROR;
    }
    /* Init tickstart for timeout managment */
    tickstart = HAL_GetTick();

    while (!huart->Rx.Finsh_Flag)
    { /*此处操作系统未初始化，将失败*/
#if defined(USING_RTOS)
      // osDelay(1);
#endif
      // if (GET_SUART_FLAG(tickstart, Timeout) || (Timeout == 0U))
      if (GET_TIMEOUT_FLAG(tickstart, HAL_GetTick(), Timeout, HAL_MAX_DELAY) || (Timeout == 0U))
      {
        *pData = huart->Rx.Data = huart->Rx.Len = 0U;
        return HAL_TIMEOUT;
      }
      else
      {
        return HAL_BUSY;
      }
    }
#if defined(USING_DEBUG)
    // shellPrint(&shell, "huart->Rx.Finsh_Flag = %d\r\n", huart->Rx.Finsh_Flag);
#endif
    /*https://blog.csdn.net/weixin_42529949/article/details/109202660*/
    /*清除外部中断线挂起寄存器*/
    __HAL_GPIO_EXTI_CLEAR_IT(huart->Rx.Rx_Pin);
    /*接收完成后再打开接收中断*/
    HAL_NVIC_EnableIRQ(huart->Rx.IRQn);
    huart->Rx.Finsh_Flag = false;
    (Size > 1U) ? (pData[huart->Rx.Len] = huart->Rx.Data) : (*pData = huart->Rx.Data);
    huart->Rx.Data = 0U;
    /*Size == 1时，huart->Rx.Len做清零处理，否则需要判断是否接收足够尺寸数据*/
    if (huart->Rx.Len == Size)
    {
      huart->Rx.Len = 0U;
    }

    return HAL_OK;
  }
  else
  {
    return HAL_BUSY;
  }
}

/**
 * @brief	模拟串口中断模式发送
 * @details
 * @param	huart 模拟串口句柄
 * @retval	None
 */
void HAL_SUART_TI_Transmit(IoUart_HandleTypeDef *huart)
{
  /*Data transmission, transmission priority, no transmission before entering the receiving state*/
  if (huart->Tx.En)
  { /*Serial port sending bit status judgment*/
    switch (huart->Tx.Status)
    {
    case COM_START_BIT:
    {
      HAL_GPIO_WritePin(huart->Tx.pGPIOx, huart->Tx.Tx_Pin, GPIO_PIN_RESET);
      huart->Tx.Status = COM_DATA_BIT;
      huart->Tx.Bits = 0U;
    }
    break;
    case COM_DATA_BIT:
    {
      HAL_GPIO_WritePin(huart->Tx.pGPIOx, huart->Tx.Tx_Pin, (GPIO_PinState)((*(huart->Tx.pBuf) >> huart->Tx.Bits) & 0x01));

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
      HAL_GPIO_WritePin(huart->Tx.pGPIOx, huart->Tx.Tx_Pin, GPIO_PIN_SET);
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

/**
 * @brief	模拟串口中断模式接收
 * @details
 * @param	huart 模拟串口句柄
 * @retval	None
 */
void HAL_SUART_TI_Receive(IoUart_HandleTypeDef *huart)
{
  if (huart->Rx.En)
  {
    switch (huart->Rx.Status)
    {
    case COM_START_BIT:
    {
      huart->Rx.Samp_Bits = (huart->Rx.Samp_Bits << 1U) | (HAL_GPIO_ReadPin(huart->Rx.pGPIOx, huart->Rx.Rx_Pin) & 0x01);

      if (++huart->Rx.Filters >= MAX_SAMPING)
      {
        if (!Get_ValidBits(huart->Rx.Samp_Bits))
        {
          huart->Rx.Status = COM_DATA_BIT;
          __HAL_TIM_SET_AUTORELOAD(huart->Rx.Timer_Handle, huart->Rx.Sam_Times[huart->Rx.Filters]);
        }
        else
        {
          huart->Rx.Status = COM_STOP_BIT;
        }
        huart->Rx.Samp_Bits = 0U;
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
      huart->Rx.Samp_Bits = (huart->Rx.Samp_Bits << 1U) | (HAL_GPIO_ReadPin(huart->Rx.pGPIOx, huart->Rx.Rx_Pin) & 0x01);

      if (++huart->Rx.Filters >= MAX_SAMPING)
      {
        huart->Rx.Data |= (Get_ValidBits(huart->Rx.Samp_Bits) & 0x01) << huart->Rx.Bits;
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
        huart->Rx.Samp_Bits = 0U;
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
      huart->Rx.Samp_Bits = (huart->Rx.Samp_Bits << 1U) | (HAL_GPIO_ReadPin(huart->Rx.pGPIOx, huart->Rx.Rx_Pin) & 0x01);

      if (++huart->Rx.Filters >= MAX_SAMPING)
      { /*Stop bit received correctly*/
        if (Get_ValidBits(huart->Rx.Samp_Bits))
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
        huart->Rx.Samp_Bits = 0U;
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

/**
 * @brief	模拟串口RX接收中断处理
 * @details
 * @param	GPIO_Pin 触发中断引脚
 * @retval	None
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  IoUart_HandleTypeDef *huart = &S_Uart1;

  if (GPIO_Pin == huart->Rx.Rx_Pin)
  {
    if (huart->Rx.Status == COM_NONE_BIT)
    {
      huart->Rx.En = true;
      // huart->Tx.En = false; /////
      huart->Rx.Status = COM_START_BIT;
      /*Close falling edge interrupt*/
      HAL_NVIC_DisableIRQ(huart->Rx.IRQn);
      __HAL_TIM_SET_COUNTER(huart->Rx.Timer_Handle, 0U);
      __HAL_TIM_SET_AUTORELOAD(huart->Rx.Timer_Handle, huart->Rx.Sam_Times[Start_Recv]);
      HAL_TIM_Base_Start_IT(huart->Rx.Timer_Handle);
    }
#if defined(USING_DEBUG)
    // shellPrint(&shell, "Received a falling edge!\r\n");
#endif
  }
}
