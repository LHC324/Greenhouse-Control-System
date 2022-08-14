/******************************************************************************
 * @brief    wifi任务(AT-command演示, 使用的模组是M169WI-FI)
 *
 * Copyright (c) 2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2021-01-20     Morro        初版
 * 2021-03-03     Morro        增加URC使用案例
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
 * @brief   定义AT控制器
 */
static at_obj_t at;

/* 
 * @brief   wifi 数据接收缓冲区
 */
static unsigned char wifi_recvbuf[256];

/* 
 * @brief   wifi URC接收缓冲区
 */
static unsigned char wifi_urcbuf[128];

/* 
 * @brief   wifi URC表
 */
static const utc_item_t urc_table[] = {
    "ready",             wifi_ready_handler,
    "WIFI CONNECTED:",   wifi_connected_handler,
    "WIFI DISCONNECTED", wifi_disconnected_handler,
};

/* 
 * @brief   AT适配器
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
 * @brief   wifi开机就绪事件
 */
void wifi_ready_handler(char *recvbuf, int size)
{
    printf("WIFI ready...\r\n");
}

/* 
 * @brief   wifi连接事件
 */
static void wifi_connected_handler(char *recvbuf, int size)
{
    printf("WIFI connection detected...\r\n");
}
/* 
 * @brief   wifi断开连接事件
 */
static void wifi_disconnected_handler(char *recvbuf, int size)
{
    printf("WIFI disconnect detected...\r\n");
}

/* 
 * @brief   打开wifi
 */
void wifi_open(void)
{
    GPIO_SetBits(GPIOA, GPIO_Pin_4);
    printf("wifi open\r\n");
}
/* 
 * @brief   关闭wifi
 */
void wifi_close(void)
{
    GPIO_ResetBits(GPIOA, GPIO_Pin_4);
    printf("wifi close\r\n");
}


/* 
 * @brief   WIFI重启任务状态机
 * @return  true - 退出状态机, false - 保持状态机,
 */
static int wifi_reset_work(at_env_t *e)
{
    at_obj_t *a = (at_obj_t *)e->params;
    switch (e->state) {
    case 0:                                //关闭WIFI电源
        wifi_close();
        e->reset_timer(a);
        e->state++;
        break;
    case 1:
        if (e->is_timeout(a, 2000))       //延时等待2s
            e->state++;
        break;
    case 2:
        wifi_open();                       //重启启动wifi
        e->state++;
        break;
    case 3:
        if (e->is_timeout(a, 5000))       //大约延时等待5s至wifi启动
            return true;  
        break;
    }
    return false;
}
/* 
 * @brief   wifi 通信异常处理
 */
static void at_error(void)
{
    printf("wifi AT communication error\r\n");
    //执行重启作业
    at_do_work(&at, wifi_reset_work, &at);        
}

/* 
 * @brief    初始化回调
 */
static void at_init_callbatk(at_response_t *r)
{    
    if (r->ret == AT_RET_OK ) {
        printf("wifi Initialization successfully...\r\n");
        
        /* 查询版本号*/
        wifi_query_version();
        
    } else 
        printf("wifi Initialization failure...\r\n");
}

/* 
 * @brief    wifi初始化命令表
 */
static const char *wifi_init_cmds[] = {
    "AT+GPIO_WR=1,1\r\n",
    "AT+GPIO_WR=2,0\r\n",
    "AT+GPIO_WR=3,1\r\n",
    NULL
};


/* 
 * @brief    wifi初始化
 */
void wifi_init(void)
{
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA , ENABLE);
    gpio_conf(GPIOA, GPIO_Mode_OUT, GPIO_PuPd_NOPULL, GPIO_Pin_4);
    
    wifi_uart_init(115200);
    at_obj_init(&at, &at_adapter);
         
    //启动WIFI
    at_do_work(&at, wifi_reset_work, &at);        
    
    //初始化wifi
    at_send_multiline(&at, at_init_callbatk, wifi_init_cmds);  
    
    //GPIO测试
    at_send_singlline(&at, NULL, "AT+GPIO_TEST_EN=1\r\n");  
    
}driver_init("wifi", wifi_init); 



/* 
 * @brief    wifi任务(10ms 轮询1次)
 */
void wifi_task(void)
{
    at_poll_task(&at);
}task_register("wifi", wifi_task, 10);


/** 非标准AT例子----------------------------------------------------------------
 *  以查询版本号为例:
 * ->  AT+VER\r\n
 * <-  VERSION:M169-YH01
 *  
 */

//方式1, 使用at_do_cmd接口

/* 
 * @brief    自定义AT发送器
 */
static void at_ver_sender(at_env_t *e)
{
    e->printf(&at, "AT+VER\r\n");
}
/* 
 * @brief    版本查询回调
 */
static void query_version_callback(at_response_t *r)
{
    if (r->ret == AT_RET_OK ) {
        printf("wifi version info : %s\r\n", r->recvbuf);
    } else 
        printf("wifi version query failure...\r\n");
}

/* 填充AT命令*/
static const at_cmd_t at_cmd_ver = {
    at_ver_sender,                   //自定义AT发送器
    "VERSION:",                      //接收匹配前缀
    query_version_callback,          //查询回调
    3, 3000                          //重试次数及超时时间
};

/* 
 * @brief    执行版本查询命令
 */
void wifi_query_version(void)
{
    at_do_cmd(&at, NULL, &at_cmd_ver);
}
