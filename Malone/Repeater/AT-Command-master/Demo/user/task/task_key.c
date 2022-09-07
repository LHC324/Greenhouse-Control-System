/******************************************************************************
 * @brief    ��������(��ʾkeyģ��ʹ��)
 *
 * Copyright (c) 2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2020/07/03     Morro  
 ******************************************************************************/
#include "led.h"
#include "key.h"
#include "module.h"
#include "public.h"

static key_t key;                                    /*��������*/

static void key_event(int type, unsigned int duration);

/* 
 * @brief       ��ȡ������ƽ״̬
 * @return      0 | 1
 */ 
static int readkey(void)
{
    return GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_4) == 0;
}

/* 
 * @brief       ���� io��ʼ��
 *              PC4 -> key;
 * @return      none
 */ 
static void key_io_init(void)
{
    gpio_conf(GPIOC, GPIO_Mode_IN, GPIO_PuPd_UP, GPIO_Pin_4);
    key_create(&key, readkey, key_event);            /*��������*/
}

/* 
 * @brief       �����¼�����
 * @return      type - 
 */
static void key_event(int type, unsigned int duration)
{
    if (type == KEY_PRESS)
        led_ctrl(LED_TYPE_GREEN, LED_MODE_FAST, 3);  /*�̰�,�̵���3��*/
    else if (type == KEY_LONG_DOWN)
        led_ctrl(LED_TYPE_GREEN, LED_MODE_ON, 0);    /*����,�̵Ƴ���*/
    else if (type == KEY_LONG_UP)
        led_ctrl(LED_TYPE_GREEN, LED_MODE_OFF, 0);   /*�������ͷ�,�̵ƹر�*/
}

driver_init("key", key_io_init);                     /*������ʼ��*/
task_register("key", key_scan_process, 20);          /*����ɨ������, 20ms��ѯ1��*/
