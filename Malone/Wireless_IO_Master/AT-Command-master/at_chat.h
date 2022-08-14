/******************************************************************************
 * @brief        ATָ��ͨ�Ź���
 *
 * Copyright (c) 2020~2021, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apathe-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2020-01-02     Morro        ����
 * 2021-01-20     Morro        ����debug���Խӿ�, �������δ��ʼ�������δ��������
 *                             ��ͨ���������������Զ�������Ƚӿ�
 * 2021-03-03     Morro        ���δ���URC����������Ƶ����ӡ���ճ�ʱ������
 ******************************************************************************/

#ifndef _ATCHAT_H_
#define _ATCHAT_H_

#include <list.h>
#include <stdbool.h>


/*���AT����� --------------------------------------------------------------*/
#define MAX_AT_CMD_LEN          128

/* debug ��ӡ�ӿ� ------------------------------------------------------------*/
#include <stdio.h>
#define AT_DEBUG(...)          printf("[AT]:");printf(__VA_ARGS__) /*do{}while(0)*/

/* ��ȡϵͳ�δ�(ms) -----------------------------------------------------------*/
#include "platform.h"
#define AT_GET_TICK()           get_tick()

struct at_obj;

/*urc������ -----------------------------------------------------------------*/
typedef struct {
    const char *prefix;  //��Ҫƥ���ͷ��
    void (*handler)(char *recvbuf, int size); 
}utc_item_t;

/*AT�ӿ������� ---------------------------------------------------------------*/
typedef struct {
    unsigned int (*write)(const void *buf, unsigned int len); /* ���ͽӿ�*/
    unsigned int (*read)(void *buf, unsigned int len);        /* ���սӿ�*/
    void         (*error)(void);                              /* ATִ���쳣�¼�*/
	utc_item_t    *utc_tbl;                                   /* urc ��*/
	unsigned char *urc_buf;                                   /* urc���ջ�����*/
    unsigned char *recv_buf;                                  /* ���ݻ�����*/
	unsigned short urc_tbl_count;                             /* urc�������*/
	unsigned short urc_bufsize;                               /* urc��������С*/
    unsigned short recv_bufsize;                              /* ���ջ�������С*/
}at_adapter_t;

/*AT��ҵ���л���*/
typedef struct {
	int         i,j,state;                                    
	void        *params;
    void        (*reset_timer)(struct at_obj *at); 
	bool        (*is_timeout)(struct at_obj *at, unsigned int ms); /*ʱ�����ж�*/
	void        (*printf)(struct at_obj *at, const char *fmt, ...);
	char *      (*find)(struct at_obj *at, const char *expect);
    char *      (*recvbuf)(struct at_obj *at);                 /* ָ����ջ�����*/
    unsigned int(*recvlen)(struct at_obj *at);                 /* �������ܳ���*/
    void        (*recvclr)(struct at_obj *at);                 /* ��ս��ջ�����*/
    bool        (*abort)(struct at_obj *at);                   /* ��ִֹ��*/
}at_env_t;

/*AT������Ӧ��*/
typedef enum {
    AT_RET_OK = 0,                                             /* ִ�гɹ�*/
    AT_RET_ERROR,                                              /* ִ�д���*/
    AT_RET_TIMEOUT,                                            /* ��Ӧ��ʱ*/
	AT_RET_ABORT,                                              /* ǿ����ֹ*/
}at_return;

/*AT��Ӧ */
typedef struct {
    void           *param;                                     
	char           *recvbuf;                                   /* ���ջ�����*/
	unsigned short  recvcnt;                                   /* �������ݳ���*/
    at_return       ret;                                       /* ATִ�н��*/
}at_response_t;

typedef void (*at_callbatk_t)(at_response_t *r);               /* AT ִ�лص�*/
 
/*AT״̬ (��ǰ�汾δ��) ------------------------------------------------------*/
typedef enum {
    AT_STATE_IDLE,                                             /* ����״̬*/
    AT_STATE_WAIT,                                             /* �ȴ�ִ��*/
    AT_STATE_EXEC,                                             /* ����ִ��*/
}at_work_state;

/*AT��ҵ��*/
typedef struct {
    unsigned int  state : 3;
    unsigned int  type  : 3;                                 /* ��ҵ����*/
    unsigned int  abort : 1; 
    void          *param;                                    /* ͨ�ò���*/
	void          *info;                                     /* ͨ����Ϣָ��*/
    struct list_head node;                                   /* ������*/
}at_item_t;

/*AT������ ------------------------------------------------------------------*/
typedef struct at_obj{
	at_adapter_t            adap;
    at_env_t                env;                             /* ��ҵ���л���*/
	at_item_t               items[10];                       /* �������10����ҵ*/
    at_item_t               *cursor;                         /* ��ǰ��ҵ��*/
    struct list_head        ls_ready, ls_idle;               /* ����,������ҵ��*/
	unsigned int            timer;
	unsigned int            urc_timer;                       /* urc���ռ�ʱ��*/
	at_return               ret; 
	//urc���ռ���, ������Ӧ���ռ�����
	unsigned short          urc_cnt, recv_cnt;
	unsigned char           suspend: 1;
}at_obj_t;

typedef struct {
    void (*sender)(at_env_t *e);                            /*�Զ��巢���� */
    const char *matcher;                                    /*����ƥ�䴮 */
    at_callbatk_t  cb;                                      /*��Ӧ���� */
    unsigned char  retry;                                   /*�������Դ��� */
    unsigned short timeout;                                 /*���ʱʱ�� */
}at_cmd_t;

void at_obj_init(at_obj_t *at, const at_adapter_t *);

bool at_send_singlline(at_obj_t *at, at_callbatk_t cb, const char *singlline);

bool at_send_multiline(at_obj_t *at, at_callbatk_t cb, const char **multiline);

bool at_do_cmd(at_obj_t *at, void *params, const at_cmd_t *cmd);

bool at_do_work(at_obj_t *at, int (*work)(at_env_t *e), void *params);

void at_item_abort(at_item_t *it);                          /*��ֹ��ǰ��ҵ*/
         
bool at_obj_busy(at_obj_t *at);                              /*æ�ж�*/

void at_suspend(at_obj_t *at);

void at_resume(at_obj_t *at);

void at_poll_task(at_obj_t *at);


#endif
