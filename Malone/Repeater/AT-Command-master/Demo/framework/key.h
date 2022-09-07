/******************************************************************************
 * @brief    ������������
 *
 * Copyright (c) 2017~2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2017-08-10     Morro        Initial version
 ******************************************************************************/

#ifndef _KEY_H_
#define _KEY_H_

#include "module.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LONG_PRESS_TIME     1500                   /*����ȷ��ʱ�� ------------*/
#define KEY_DEBOUNCE_TIME   20                     /*��������ʱ�� ------------*/

#define KEY_PRESS           0                      /*�̰� --------------------*/
#define KEY_LONG_DOWN       1                      /*�������� ----------------*/
#define KEY_LONG_UP         2                      /*�����ͷ� ----------------*/

/*���������� -----------------------------------------------------------------*/
typedef struct key_t {
    /*
     *@brief     ����������ȡ�ӿ�
     *@param[in] none
     *@return    0  - ����δ����, ��0 - ��������
     */
    int  (*readkey)(void);  
    /*
     *@brief     �����¼���������
     *@param[in] type - �¼�����(KEY_SHORT_PRESS - �̰�, KEY_LONG_PRESS - ����)
     *@param[in] duration ����ʱ��,������Ч
     *@return    none
     */    
    void (*event)(int type, unsigned int duration);/*�����¼����� ------------*/
    unsigned int tick;                             /*�δ��ʱ�� --------------*/
    struct key_t *next;                            /*������һ���������������� */
}key_t;

bool key_create(key_t *key, int (*readkey)(void),  /*��������*/
               void (*event)(int type, unsigned int duration));
void key_scan_process(void);                       /*����ɨ�账��*/

#ifdef __cplusplus
}
#endif

#endif
