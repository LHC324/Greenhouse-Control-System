/*******************************************************************************
* @brief	      ��ʽ����(queue link)����
*
* Copyright (c) 2017~2020, <morro_luo@163.com>
*
* SPDX-License-Identifier: Apache-2.0
* Change Logs 
* Date           Author       Notes 
* 2016-06-24     Morro        ��ʼ��
* 2018-03-17     Morro        ���Ӷ���Ԫ��ͳ�ƹ���
*******************************************************************************/
#ifndef _QLINK_H_
#define _QLINK_H_


#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h> 

/*��ʽ���н�� ---------------------------------------------------------------*/
struct qlink_node {
    struct qlink_node *next;
};

/*��ʽ���й����� -------------------------------------------------------------*/
struct qlink {
    unsigned int  count;
    struct qlink_node *front, *rear;        
};

/*******************************************************************************
 * @brief       ��ʼ����ʽ����
 * @param[in]   q     - ���й�����
 * @return      none
 ******************************************************************************/
static inline void qlink_init(struct qlink *q)
{
    q->front = q->rear= NULL;
    q->count = 0;
}

/*******************************************************************************
 * @brief       ��Ӳ���
 * @param[in]   q     - ���й�����
 * @return      nond
 ******************************************************************************/
static inline void qlink_put(struct qlink *q, struct qlink_node *n)
{   
    if (q->count == 0)
        q->front = n;
    else 
        q->rear->next = n;
    q->rear = n;
    n->next = NULL; 
    q->count++;
}

/*******************************************************************************
 * @brief       Ԥ���Ӳ���(��ȡ���׽��)
 * @param[in]   q     - ���й�����
 * @return      nond
 ******************************************************************************/
static inline struct qlink_node *qlink_peek(struct qlink *q)
{
    return q->front;
}

/*******************************************************************************
 * @brief       ���Ӳ���
 * @param[in]   q     - ���й�����
 * @return      nond
 ******************************************************************************/
static inline struct qlink_node *qlink_get(struct qlink *q)
{
    struct qlink_node *n;
    if (q->count == 0)
        return NULL;
    n = q->front;
    q->front = q->front->next;
    q->count--;
    return n;
}

/*******************************************************************************
 * @brief       ����Ԫ�ظ���
 * @param[in]   q     - ���й�����
 * @return      nond
 ******************************************************************************/
static inline int qlink_count(struct qlink *q)
{
    return q->count;
}

#ifdef __cplusplus
}
#endif

#endif
