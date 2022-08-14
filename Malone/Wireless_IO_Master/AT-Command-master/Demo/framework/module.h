/******************************************************************************
 * @brief    ϵͳģ�����(����ϵͳ��ʼ��,ʱ��Ƭ��ѯϵͳ)
 *
 * Copyright (c) 2017~2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2016-06-24     Morro        �������
 * 2020-05-23     Morro        ������������,��ֹģ����������
 * 2020-06-28     Morro        ����is_timeout��ʱ�жϽӿ�
 ******************************************************************************/

#ifndef _MODULE_H_
#define _MODULE_H_

#include "comdef.h"
#include <stdbool.h>

/*ģ���ʼ����*/
typedef struct {
    const char *name;               //ģ������
    void (*init)(void);             //��ʼ���ӿ�
}init_item_t;

/*��������*/
typedef struct {
    const char *name;               //ģ������    
    void (*handle)(void);           //��ʼ���ӿ�
    unsigned int interval;          //��ѯ���
    unsigned int *timer;            //ָ��ʱ��ָ��
}task_item_t;

#define __module_initialize(name,func,level)           \
    USED ANONY_TYPE(const init_item_t, init_tbl_##func)\
    SECTION("init.item."level) = {name,func}

/*
 * @brief       ����ע��
 * @param[in]   name    - �������� 
 * @param[in]   handle  - ��ʼ������(void func(void){...})
  * @param[in]  interval- ��ѯ���(ms)
 */
#define task_register(name, handle,interval)                \
    static unsigned int __task_timer_##handle;              \
    USED ANONY_TYPE(const task_item_t, task_item_##handle)  \
    SECTION("task.item.1") =                                \
    {name,handle, interval, &__task_timer_##handle}

/*
 * @brief       ģ���ʼ��ע��
 * @param[in]   name    - ģ������ 
 * @param[in]   func    - ��ʼ����ں���(void func(void){...})
 */
#define system_init(name,func)  __module_initialize(name,func,"1")
#define driver_init(name,func)  __module_initialize(name,func,"2")
#define module_init(name,func)  __module_initialize(name,func,"3")

void systick_increase(unsigned int ms);
unsigned int get_tick(void);
bool is_timeout(unsigned int start, unsigned int timeout);
void module_task_init(void);
void module_task_process(void);
    
#endif
