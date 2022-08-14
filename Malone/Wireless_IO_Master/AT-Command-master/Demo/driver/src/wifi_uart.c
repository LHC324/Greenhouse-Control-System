/******************************************************************************
 * @brief    wifi����ͨ������
 *
 * Copyright (c) 2021, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2021-01-20     Morro        ��ʼ�汾
 ******************************************************************************/
#include "stm32f4xx.h"
#include "ringbuffer.h"
#include "wifi_uart.h"
#include "public.h"
#include <string.h>

#if (TTY_RXBUF_SIZE & (TTY_RXBUF_SIZE - 1)) != 0 
    #error "TTY_RXBUF_SIZE must be power of 2!"
#endif

#if (TTY_TXBUF_SIZE & (TTY_TXBUF_SIZE - 1)) != 0 
    #error "TTY_RXBUF_SIZE must be power of 2!"
#endif

static unsigned char rxbuf[WIFI_RXBUF_SIZE];         /*���ջ����� */
static unsigned char txbuf[WIFI_TXBUF_SIZE];         /*���ͻ����� */
static ring_buf_t rbsend, rbrecv;                    /*�շ�����������*/

/*
 * @brief	    wifi���ڳ�ʼ��
 * @param[in]   baudrate - ������
 * @return 	    none
 */
void wifi_uart_init(int baud_rate)
{
    ring_buf_init(&rbsend, txbuf, sizeof(txbuf));/*��ʼ�����λ����� */
    ring_buf_init(&rbrecv, rxbuf, sizeof(rxbuf)); 
    
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB , ENABLE);
    
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_USART3);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_USART3);  
    
    gpio_conf(GPIOB, GPIO_Mode_AF, GPIO_PuPd_NOPULL, GPIO_Pin_10 | GPIO_Pin_11);
    
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
    uart_conf(USART3, baud_rate);                    /*��������*/
    
    nvic_conf(USART3_IRQn, 1, 1);

}

/*
 * @brief	    �򴮿ڷ��ͻ�������д�����ݲ���������
 * @param[in]   buf       -  ���ݻ���
 * @param[in]   len       -  ���ݳ���
 * @return 	    ʵ��д�볤��(�����ʱ��������,�򷵻�len)
 */
unsigned int wifi_uart_write(const void *buf, unsigned int len)
{   
    unsigned int ret;
    ret = ring_buf_put(&rbsend, (unsigned char *)buf, len);  
    USART_ITConfig(USART3, USART_IT_TXE, ENABLE);
    return ret; 
}

/*
 * @brief	    ��ȡ���ڽ��ջ�����������
 * @param[in]   buf       -  ���ݻ���
 * @param[in]   len       -  ���ݳ���
 * @return 	    (ʵ�ʶ�ȡ����)������ջ���������Ч���ݴ���len�򷵻�len���򷵻ػ���
 *              ����Ч���ݵĳ���
 */
unsigned int wifi_uart_read(void *buf, unsigned int len)
{
    return ring_buf_get(&rbrecv, (unsigned char *)buf, len);
}

/*���ջ�������*/
bool wifi_uart_rx_empty(void)
{
    return ring_buf_len(&rbrecv) == 0;
}

/*
 * @brief	    ����1�շ��ж�
 * @param[in]   none
 * @return 	    none
 */
void USART3_IRQHandler(void)
{     
    unsigned char data;
    if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET) {
        data = USART_ReceiveData(USART3);
        ring_buf_put(&rbrecv, &data, 1);           /*�����ݷ�����ջ�����*/             
    }
    if (USART_GetITStatus(USART3, USART_IT_TXE) != RESET) {
        if (ring_buf_get(&rbsend, &data, 1))      /*�ӻ�������ȡ������---*/
            USART_SendData(USART3, data);            
        else{
            USART_ITConfig(USART3, USART_IT_TXE, DISABLE);    
        }
    }
    if (USART_GetITStatus(USART3, USART_IT_ORE_RX) != RESET) {
        data = USART_ReceiveData(USART3);        
    }
}
