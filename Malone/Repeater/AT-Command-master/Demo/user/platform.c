/******************************************************************************
 * @brief    ƽ̨��س�ʼ��
 *
 * Copyright (c) 2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ******************************************************************************/
#include "module.h"
#include "public.h"
#include "config.h"
#include "platform.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "tty.h"

/*
 * @brief	   ϵͳ�δ��ж�
 * @param[in]   none
 * @return 	   none
 */
void SysTick_Handler(void)
{
    systick_increase(SYS_TICK_INTERVAL);
}

/*
 * @brief	   �ض���printf
 */
int fputc(int c, FILE *f)
{       
    tty.write(&c, 1);
    while (!tty.rx_isempty()) {}   
    while (tty.tx_isfull()) {}                           //��ֹ��LOG
    return c;
}

/*
 * @brief	   Ӳ��������ʼ��
 * @param[in]   none
 * @return 	   none
 */
static void bsp_init(void)
{    
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  
    tty.init(115200);
    SystemCoreClockUpdate();
 	SysTick_Config(SystemCoreClock / (1000 / SYS_TICK_INTERVAL));   //����ϵͳʱ��
	NVIC_SetPriority(SysTick_IRQn, 0);
    
}system_init("bsp", bsp_init); 
