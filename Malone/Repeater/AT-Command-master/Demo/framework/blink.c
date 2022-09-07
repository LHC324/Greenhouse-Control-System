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
#include "module.h"                      /*get_tick()*/
#include "blink.h"                       
#include <stddef.h>
#include <string.h>

static blink_dev_t *head = NULL;         /*ͷ��� -*/

/*
 * @brief       ����blink�豸
 * @param[in]   dev    - �豸
 * @param[in]   ioctrl - IO���ƺ���
 * @return      none
 */
void blink_dev_create(blink_dev_t *dev, void (*ioctrl)(bool enable))
{
    blink_dev_t *tail = head;  
    memset(dev, 0, sizeof(blink_dev_t));
    dev->ioctrl = ioctrl;    
    dev->next = NULL;
    if (head == NULL) {
        head = dev;
        return;
    }
    while (tail->next != NULL)
        tail = tail->next;
    tail->next = dev;
}


/* 
 * @brief       blink �豸����
 * @param[in]   name    - �豸����
 * @param[in]   ontime - ����ʱ��(�����ֵΪ0�����ùر�)
 * @param[in]   offtime- �ر�ʱ��
 * @param[in]   repeats- �ظ�����(0��ʾ����ѭ��)
 * @return      none
 */
void blink_dev_ctrl(blink_dev_t *dev, int ontime, int offtime, int repeats)
{
    dev->ontime  = ontime;
    dev->offtime = offtime + ontime;                  
    dev->repeats = repeats;
    dev->tick    = get_tick();
    dev->count   = 0;
    if (ontime  == 0) {
        dev->ioctrl(false);
        dev->enable  = false;
    }        
}

/*
 * @brief       blink�豸����
 * @param[in]   none
 * @return      none
 */
void blink_dev_process(void)
{
    blink_dev_t *dev;
    for (dev = head; dev != NULL; dev = dev->next) {
        if (dev->ontime == 0) {
            continue;
        } else if (get_tick() - dev->tick < dev->ontime) {
            if (!dev->enable) {
                dev->enable = true;
                dev->ioctrl(true);
            }
        } else if(get_tick() - dev->tick < dev->offtime) {    /**/
            if (dev->enable) {
                dev->enable = false;
                dev->ioctrl(false);
            }
        } else {
            dev->tick = get_tick();
			if (dev->repeats) {
				if (++dev->count >= dev->repeats) {
					dev->ontime = 0;
					dev->ioctrl(false);
					dev->enable = false;
				}
			}
        }
    }
}
