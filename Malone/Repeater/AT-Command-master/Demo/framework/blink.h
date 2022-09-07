/******************************************************************************
 * @brief    ������˸����(dev, motor, buzzer)���豸(dev, motor, buzzer)����
 *
 * Copyright (c) 2019, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2019-04-01     Morro        Initial version
 ******************************************************************************/
#ifndef _BLINK_H_
#define _BLINK_H_

#include "module.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

/*Blink �豸����*/
typedef struct blink_dev {
    unsigned short ontime;                                /*����ʱ��*/
    unsigned short offtime;                               /*�ر�ʱ��*/
    unsigned short repeats;                               /*�ظ�����*/
    unsigned char  count;
    unsigned char  enable;
    unsigned int   tick;
    void (*ioctrl)(bool enable);                          /*io���ƽӿ�*/
    struct blink_dev *next;
}blink_dev_t;

void blink_dev_create(blink_dev_t *dev, void (*ioctrl)(bool enable));

void blink_dev_ctrl(blink_dev_t *dev, int ontime, int offtime, int repeat);

void blink_dev_process(void);

#ifdef __cplusplus
}
#endif

#endif
