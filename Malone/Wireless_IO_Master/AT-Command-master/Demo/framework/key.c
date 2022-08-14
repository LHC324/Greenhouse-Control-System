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
#include "key.h"
#include <stddef.h>

static key_t *keyhead = NULL;                              /*��������ͷ���*/

/*******************************************************************************
 * @brief       ����һ������
 * @param[in]   key     - ����������
 * @param[in]   readkey - �����������Ժ���ָ��
 * @param[in]   event   - �����¼�������ָ��
 * @return      true    - ����ʧ��, false - �����ɹ�
 ******************************************************************************/
bool key_create(key_t *key, int (*readkey)(void), 
               void (*event)(int type, unsigned int duration))
{
    key_t *keytail = keyhead;
    if (key == NULL || readkey == NULL || event == NULL)
        return 0;    
    key->event    = event;
    key->readkey  = readkey;
    key->next     = NULL;
    if (keyhead == NULL) {
        keyhead = key;
        return 1;
    }
    while (keytail->next != NULL)                          /*ת����β*/
        keytail = keytail->next;
    keytail->next = key;
    return 1;
}

/*******************************************************************************
 * @brief       ����ɨ�账��
 * @return      none
 ******************************************************************************/
void key_scan_process(void)
{
    key_t *k;
    for (k = keyhead; k != NULL; k = k->next) {        
        if (k->readkey()) {
            if (k->tick) {
                if (is_timeout(k->tick, LONG_PRESS_TIME))    /*�����ж� */   
                    k->event(KEY_LONG_DOWN, get_tick() - k->tick);           
            } else {
                k->tick = get_tick();                        /*��¼�״ΰ���ʱ��*/
            }
        } else if (k->tick) {              
            if (is_timeout(k->tick, LONG_PRESS_TIME)) {      /*�����ͷ� */            
                k->event(KEY_LONG_UP, get_tick() - k->tick);
            }
            
            /*�̰��ͷŲ��� ---------------------------------------------------*/
            if (is_timeout(k->tick, KEY_DEBOUNCE_TIME) && 
                !is_timeout(k->tick, LONG_PRESS_TIME)) {
                k->event(KEY_PRESS, get_tick() - k->tick);
            }
            
            k->tick = 0;
        }        
    }
}

