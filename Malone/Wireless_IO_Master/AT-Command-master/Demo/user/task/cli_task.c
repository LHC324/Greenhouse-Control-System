/******************************************************************************
 * @brief    ����������
 *
 * Copyright (c) 2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2020/07/11     Morro        
 ******************************************************************************/
#include "module.h"
#include "tty.h"
#include "cli.h"

static cli_obj_t cli;                               /*�����ж��� */

/* 
 * @brief       �����������ʼ��
 * @return      none
 */ 
static void cli_task_init(void)
{
    cli_port_t p = {tty.write, tty.read};           /*��д�ӿ� */
    
    cli_init(&cli, &p);                             /*��ʼ�������ж��� */
     
    cli_enable(&cli);
    
    cli_exec_cmd(&cli,"sysinfo");                   /*��ʾϵͳ��Ϣ*/
}

/* 
 * @brief       ������������
 * @return      none
 */ 
static void cli_task_process(void)
{
    cli_process(&cli);
}

module_init("cli", cli_task_init);                  
task_register("cli", cli_task_process, 10);          /*ע������������*/
