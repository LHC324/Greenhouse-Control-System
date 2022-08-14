/******************************************************************************
 * @brief    ͨ�ö��й���
 *
 * Copyright (c) 2017~2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2017-03-19     Morro        Initial version. 
 ******************************************************************************/
#include "queue.h"
#include <stddef.h>
#include <string.h>

/* @brief      ����һ������
 * @param[in]  q    - ���й�����
 * @param[in]  base - ������������ַ
 * @param[in]  size - ��������������Ԫ�ظ���
 * @param[in]  element_size - ����Ԫ�ش�С(sizeof(type))
 * @return     true - �����ɹ�, false - ����ʧ��
 */
bool queue_create(queue_t *q, void *base, int size, int element_size)
{
    if (q == NULL || base == NULL || size== 0 || element_size == 0)
        return false;
    q->base         = base;
    q->size         = size;
    q->element_size = element_size;
    q->front        = q->rear = 0;
    return true;
}


/* @brief      �ж϶����Ƿ�Ϊ��
 * @return    true - ����Ϊ��, false - ���зǿ�
 */
bool queue_is_empty(queue_t *q)
{
    return q->front == q->rear;
}

/* @brief      �ж϶����Ƿ���
 * @return     true - ���з���, false - ����δ��
 */
bool queue_is_full(queue_t *q)
{
    return (q->rear + 1) % q->size == q->front;
}

/* @brief      ���(���Ԫ������β)
 * @param[in]  element  - ����Ԫ��
 * @return     true - �ɹ����, false - ������
 */
bool queue_put(queue_t *q,  void *element)
{
    if (queue_is_full(q))
        return false;
    memcpy((unsigned char *)q->base + q->rear * q->element_size, element, 
           q->element_size);
    q->rear = (q->rear + 1) % q->size;
    return true;
}

/* @brief      ����(�Ƴ�����Ԫ��)
 * @param[in]  element  - ָ�����Ԫ�صĵ�ַ
 * @return     true - �ɹ�����, false - ����Ϊ��
 */
bool queue_get(queue_t *q,  void *element)
{
    if (queue_is_empty(q))
        return false;   
    memcpy(element, (unsigned char *)q->base + q->front * q->element_size,
          q->element_size);
    q->front = (q->front + 1) % q->size;
    return true;
}

/* @brief      ��ȡ����Ԫ�ص�ַ(Ԥ��ȡ)
 * @param[in]  element  - ָ�����Ԫ�صĵ�ַ
 * @return     true - ��ȡ�ɹ�, false - ��ȡʧ�ܶ���Ϊ��
 */
bool queue_peek(queue_t *q, void **element)
{
    if (queue_is_empty(q))
        return false;   
    *element = (void *)((unsigned char *)q->base + q->front * 
              q->element_size);
    return true;
}

/* @brief      ɾ����βԪ��
 * @return     true - �ɹ��Ƴ�, false -  ����Ϊ��, 
 */
bool queue_del(queue_t *q)
{
    if (queue_is_empty(q))
        return false;   
    q->front = (q->front + 1) % q->size;
    return true;
}

/* @brief      ��ȡ����Ԫ�ظ���
 * @return    ����Ԫ�ظ���
 */
int queue_size(queue_t *q)
{
    return (q->rear + q->size - q->front ) % q->size;
}

/* @brief      ��ն���
 * @return    none
 */
void queue_clr(queue_t *q)
{
    q->front = q->rear = 0;
}
