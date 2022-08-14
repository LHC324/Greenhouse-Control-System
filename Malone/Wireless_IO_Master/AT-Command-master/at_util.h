/******************************************************************************
 * @brief    atģ��OS�����ֲ�ӿ�
 *
 * Copyright (c) 2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2020-01-02     Morro        ����
 ******************************************************************************/

#ifndef _ATUTIL_H_
#define _ATUTIL_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef void *at_sem_t;                                /*�ź���*/

/*
 * @brief	   ��ȡ��ǰϵͳ������
 */
static inline unsigned int at_get_ms(void)
{
    /*Add  here...*/
    return 0;
}
/*
 * @brief	   ��ʱ�ж�
 * @retval     true | false
 */
static inline bool at_istimeout(unsigned int start_time, unsigned int timeout)
{
    return at_get_ms() - start_time > timeout;
}

/*
 * @brief	   ������ʱ
 * @retval     none
 */
static inline void at_delay(uint32_t ms)
{
    
}
/*
 * @brief	   �����ź�
 * @retval     none
 */
static inline at_sem_t at_sem_new(int value)
{
    return NULL;
}
/*
 * @brief	   �ȴ��ź�
 * @retval     none
 */
static inline bool at_sem_wait(at_sem_t s, uint32_t timeout)
{
    return false;
}

/*
 * @brief	   �ͷ��ź�
 * @retval     none
 */  
static inline void at_sem_post(at_sem_t s)
{
    
}

/*
 * @brief	   ���ٵ��ź�
 * @retval     none
 */  
static inline void at_sem_free(at_sem_t s)
{
    
}

#endif