/******************************************************************************
 * @brief    wifi����(AT-command��ʾ, ʹ�õ�ģ����M169WI-FI)
 *
 * Copyright (c) 2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2021-01-20     Morro        ����
 * 2021-03-03     Morro        ����URCʹ�ð���
 ******************************************************************************/
#include "at_chat.h"
#include "wifi_uart.h"
#include "public.h"
#include "module.h"
#include "cli.h"
#include <stdio.h>
#include <stdbool.h>

/* Private function prototypes -----------------------------------------------*/
void wifi_open(void);
void wifi_close(void);
static void at_error(void);
void wifi_query_version(void);
void wifi_ready_handler(char *recvbuf, int size);
void wifi_connected_handler(char *recvbuf, int size);
void wifi_disconnected_handler(char *recvbuf, int size);

/* Private variables ---------------------------------------------------------*/
/* 
 * @brief   ����AT������
 */
static at_obj_t at;

/* 
 * @brief   wifi ���ݽ��ջ�����
 */
static unsigned char wifi_recvbuf[256];

/* 
 * @brief   wifi URC���ջ�����
 */
static unsigned char wifi_urcbuf[128];

/* 
 * @brief   wifi URC��
 */
static const utc_item_t urc_table[] = {
    "ready",             wifi_ready_handler,
    "WIFI CONNECTED:",   wifi_connected_handler,
    "WIFI DISCONNECTED", wifi_disconnected_handler,
};

/* 
 * @brief   AT������
 */
static const at_adapter_t  at_adapter = {
    .write    = wifi_uart_write,
    .read     = wifi_uart_read,
    .error    = at_error,
    .utc_tbl  = (utc_item_t *)urc_table,
    .urc_buf  = wifi_urcbuf,
    .recv_buf = wifi_recvbuf,
    .urc_tbl_count = sizeof(urc_table) / sizeof(urc_table[0]),
    .urc_bufsize   = sizeof(wifi_urcbuf),
    .recv_bufsize = sizeof(wifi_recvbuf)
};

/* Private functions ---------------------------------------------------------*/

/* 
 * @brief   wifi���������¼�
 */
void wifi_ready_handler(char *recvbuf, int size)
{
    printf("WIFI ready...\r\n");
}

/* 
 * @brief   wifi�����¼�
 */
static void wifi_connected_handler(char *recvbuf, int size)
{
    printf("WIFI connection detected...\r\n");
}
/* 
 * @brief   wifi�Ͽ������¼�
 */
static void wifi_disconnected_handler(char *recvbuf, int size)
{
    printf("WIFI disconnect detected...\r\n");
}

/* 
 * @brief   ��wifi
 */
void wifi_open(void)
{
    GPIO_SetBits(GPIOA, GPIO_Pin_4);
    printf("wifi open\r\n");
}
/* 
 * @brief   �ر�wifi
 */
void wifi_close(void)
{
    GPIO_ResetBits(GPIOA, GPIO_Pin_4);
    printf("wifi close\r\n");
}


/* 
 * @brief   WIFI��������״̬��
 * @return  true - �˳�״̬��, false - ����״̬��,
 */
static int wifi_reset_work(at_env_t *e)
{
    at_obj_t *a = (at_obj_t *)e->params;
    switch (e->state) {
    case 0:                                //�ر�WIFI��Դ
        wifi_close();
        e->reset_timer(a);
        e->state++;
        break;
    case 1:
        if (e->is_timeout(a, 2000))       //��ʱ�ȴ�2s
            e->state++;
        break;
    case 2:
        wifi_open();                       //��������wifi
        e->state++;
        break;
    case 3:
        if (e->is_timeout(a, 5000))       //��Լ��ʱ�ȴ�5s��wifi����
            return true;  
        break;
    }
    return false;
}
/* 
 * @brief   wifi ͨ���쳣����
 */
static void at_error(void)
{
    printf("wifi AT communication error\r\n");
    //ִ��������ҵ
    at_do_work(&at, wifi_reset_work, &at);        
}

/* 
 * @brief    ��ʼ���ص�
 */
static void at_init_callbatk(at_response_t *r)
{    
    if (r->ret == AT_RET_OK ) {
        printf("wifi Initialization successfully...\r\n");
        
        /* ��ѯ�汾��*/
        wifi_query_version();
        
    } else 
        printf("wifi Initialization failure...\r\n");
}

/* 
 * @brief    wifi��ʼ�������
 */
static const char *wifi_init_cmds[] = {
    "AT+GPIO_WR=1,1\r\n",
    "AT+GPIO_WR=2,0\r\n",
    "AT+GPIO_WR=3,1\r\n",
    NULL
};


/* 
 * @brief    wifi��ʼ��
 */
void wifi_init(void)
{
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA , ENABLE);
    gpio_conf(GPIOA, GPIO_Mode_OUT, GPIO_PuPd_NOPULL, GPIO_Pin_4);
    
    wifi_uart_init(115200);
    at_obj_init(&at, &at_adapter);
         
    //����WIFI
    at_do_work(&at, wifi_reset_work, &at);        
    
    //��ʼ��wifi
    at_send_multiline(&at, at_init_callbatk, wifi_init_cmds);  
    
    //GPIO����
    at_send_singlline(&at, NULL, "AT+GPIO_TEST_EN=1\r\n");  
    
}driver_init("wifi", wifi_init); 



/* 
 * @brief    wifi����(10ms ��ѯ1��)
 */
void wifi_task(void)
{
    at_poll_task(&at);
}task_register("wifi", wifi_task, 10);


/** �Ǳ�׼AT����----------------------------------------------------------------
 *  �Բ�ѯ�汾��Ϊ��:
 * ->  AT+VER\r\n
 * <-  VERSION:M169-YH01
 *  
 */

//��ʽ1, ʹ��at_do_cmd�ӿ�

/* 
 * @brief    �Զ���AT������
 */
static void at_ver_sender(at_env_t *e)
{
    e->printf(&at, "AT+VER\r\n");
}
/* 
 * @brief    �汾��ѯ�ص�
 */
static void query_version_callback(at_response_t *r)
{
    if (r->ret == AT_RET_OK ) {
        printf("wifi version info : %s\r\n", r->recvbuf);
    } else 
        printf("wifi version query failure...\r\n");
}

/* ���AT����*/
static const at_cmd_t at_cmd_ver = {
    at_ver_sender,                   //�Զ���AT������
    "VERSION:",                      //����ƥ��ǰ׺
    query_version_callback,          //��ѯ�ص�
    3, 3000                          //���Դ�������ʱʱ��
};

/* 
 * @brief    ִ�а汾��ѯ����
 */
void wifi_query_version(void)
{
    at_do_cmd(&at, NULL, &at_cmd_ver);
}
