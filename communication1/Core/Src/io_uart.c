#include "io_uart.h"
#include "tim.h"
#include "shell_port.h"
#if defined(USING_RTTHREAD)
#include "rtthread.h"
#else
#include "cmsis_os.h"
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
    Get_BaudTable(&S_Uart1, User_BaudRate);
    S_Uart1.Check_Type = NONE;
    S_Uart1.Tx.Finsh_Flag = false;
    S_Uart1.Tx.Status = COM_NONE_BIT;
    S_Uart1.Tx.Timer_Handle = &htim6;
    S_Uart1.Rx.Finsh_Flag = false;
    S_Uart1.Rx.Status = COM_NONE_BIT;
    S_Uart1.Rx.Timer_Handle = &htim14;
    S_Uart1.Rx.IRQn = EXTI4_15_IRQn;
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
            /*Cotex-M0 没有basepri寄存器，在xPortSysTickHandler中会频闭所有中断源*/
            BaseType_t xSchedulerState = xTaskGetSchedulerState();
            (xSchedulerState != taskSCHEDULER_RUNNING) ? __enable_irq() : (void)false;
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
        __HAL_GPIO_EXTI_CLEAR_IT(IO_UART_RX_Pin);
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
 * @brief	模拟串口RX接收中断处理
 * @details
 * @param	GPIO_Pin 触发中断引脚
 * @retval	None
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    IoUart_HandleTypeDef *huart = &S_Uart1;

    if (GPIO_Pin == IO_UART_RX_Pin)
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
