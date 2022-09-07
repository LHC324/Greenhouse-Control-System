/******************************************************************************
 * @brief    tty���ڴ�ӡ����
 *
 * Copyright (c) 2015, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2015-07-03     Morro
 ******************************************************************************/
#include "stm32f4xx.h"
#include "ringbuffer.h"
#include "tty.h"
#include "public.h"
#include <string.h>

#if (TTY_RXBUF_SIZE & (TTY_RXBUF_SIZE - 1)) != 0 
    #error "TTY_RXBUF_SIZE must be power of 2!"
#endif

#if (TTY_TXBUF_SIZE & (TTY_TXBUF_SIZE - 1)) != 0 
    #error "TTY_RXBUF_SIZE must be power of 2!"
#endif


static unsigned char rxbuf[TTY_RXBUF_SIZE];         /*���ջ����� */
static unsigned char txbuf[TTY_TXBUF_SIZE];         /*���ͻ����� */
static ring_buf_t rbsend, rbrecv;                   /*�շ�����������*/

/*
 * @brief	    ���ڳ�ʼ��
 * @param[in]   baudrate - ������
 * @return 	    none
 */
static void uart_init(int baudrate)
{
    ring_buf_init(&rbsend, txbuf, sizeof(txbuf));/*��ʼ�����λ����� */
    ring_buf_init(&rbrecv, rxbuf, sizeof(rxbuf)); 
    
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA , ENABLE);
    
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);  
    
    gpio_conf(GPIOA, GPIO_Mode_AF, GPIO_PuPd_NOPULL, 
              GPIO_Pin_9 | GPIO_Pin_10);
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    uart_conf(USART1, baudrate);                    /*��������*/
    
    nvic_conf(USART1_IRQn, 1, 1);

}

/*
 * @brief	    �򴮿ڷ��ͻ�������д�����ݲ���������
 * @param[in]   buf       -  ���ݻ���
 * @param[in]   len       -  ���ݳ���
 * @return 	    ʵ��д�볤��(�����ʱ��������,�򷵻�len)
 */
static unsigned int uart_write(const void *buf, unsigned int len)
{   
    unsigned int ret;
    ret = ring_buf_put(&rbsend, (unsigned char *)buf, len);  
    USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
    return ret; 
}

/*
 * @brief	    ��ȡ���ڽ��ջ�����������
 * @param[in]   buf       -  ���ݻ���
 * @param[in]   len       -  ���ݳ���
 * @return 	    (ʵ�ʶ�ȡ����)������ջ���������Ч���ݴ���len�򷵻�len���򷵻ػ���
 *              ����Ч���ݵĳ���
 */
static unsigned int uart_read(void *buf, unsigned int len)
{
    return ring_buf_get(&rbrecv, (unsigned char *)buf, len);
}

/*���ͻ�������*/
static bool tx_isfull(void)
{
    return ring_buf_len(&rbsend) == TTY_TXBUF_SIZE;
}

/*���ջ�������*/
bool rx_isempty(void)
{
    return ring_buf_len(&rbrecv) == 0;
}

/*����̨�ӿڶ��� -------------------------------------------------------------*/
const tty_t tty = {
    uart_init,
    uart_write,
    uart_read,
    tx_isfull,
    rx_isempty
};

/*
 * @brief	    ����1�շ��ж�
 * @param[in]   none
 * @return 	    none
 */
void USART1_IRQHandler(void)
{     
    unsigned char data;
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        data = USART_ReceiveData(USART1);
        ring_buf_put(&rbrecv, &data, 1);           /*�����ݷ�����ջ�����*/             
    }
    if (USART_GetITStatus(USART1, USART_IT_TXE) != RESET) {
        if (ring_buf_get(&rbsend, &data, 1))      /*�ӻ�������ȡ������---*/
            USART_SendData(USART1, data);            
        else{
            USART_ITConfig(USART1, USART_IT_TXE, DISABLE);    
        }
    }
    if (USART_GetITStatus(USART1, USART_IT_ORE_RX) != RESET) {
        data = USART_ReceiveData(USART1);        
    }
}
