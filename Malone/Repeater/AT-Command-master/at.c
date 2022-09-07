/**
  ******************************************************************************
  * @brief        AT����ͨ�Ź���(OS�汾)
  *
  * Copyright (c) 2020, <morro_luo@163.com>
  *
  * SPDX-License-Identifier: Apache-2.0
  *
  * Change Logs: 
  * Date           Author       Notes 
  * 2020-01-02     Morro        Initial version. 
  * 2021-02-01     Morro        ֧��URC�ص��н�������.
  * 2021-02-05     Morro        1.�޸�struct at_obj,ȥ������������
  *                             2.ɾ�� at_obj_destroy�ӿ�
  * 2021-03-21     Morro        ɾ��at_obj�е�at_work_ctx_t��,�����ڴ�ʹ��
  * 2021-04-08     Morro        ����ظ��ͷ��ź�������������ֵȴ���ʱ������
  * 2021-05-15     Morro        �Ż�URCƥ�����
  * 2021-07-20     Morro        ������,���ִ��at_do_workʱurc�������ݶ�ʧ����
  * 2021-11-20     Morro        �����ַ��������,���������urc�¼��ж�ȡ����ʱ
  *                             ���º������ݶ�ʧ����
  ******************************************************************************
  */
#include "at.h"
#include "comdef.h"
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>

#define MAX_AT_LOCK_TIME   60 * 1000     //AT��������ʱ��

static void urc_recv_process(at_obj_t *at, const char *buf, unsigned int size);

/** 
  * @brief    Ĭ�ϵ��Խӿ�
  */
static void nop_dbg(const char *fmt, ...){}


/**
  * @brief    ����ַ���
  */
static void put_string(at_obj_t *at, const char *s)
{
    while (*s != '\0')
        at->adap.write(s++, 1);
}

/**
  * @brief    ����ַ���(������)
  */
static void put_line(at_obj_t *at, const char *s)
{
    put_string(at, s);
    put_string(at, "\r\n");    
    at->adap.debug("->\r\n%s\r\n", s);
}


/**
  * @brief    ��ҵ������
  */
static unsigned int at_work_read(struct at_work_ctx *e, void *buf, unsigned int len)
{ 
    at_obj_t *at = e->at;
    len = at->adap.read(buf, len);
    urc_recv_process(at, buf, len);             //�ݽ���URC���д���
    return len;
}

/**
  * @brief    ��ҵд����
  */
static unsigned int at_work_write(struct at_work_ctx *e, const void *buf, unsigned int len)
{
    return e->at->adap.write(buf, len);
}

//��ӡ���
static void at_work_print(struct at_work_ctx *e, const char *cmd, ...)
{
    va_list args;
    va_start(args, cmd);
    char buf[MAX_AT_CMD_LEN];
    vsnprintf(buf, sizeof(buf), cmd, args);
    put_line(e->at, buf);
    va_end(args);	
}

//�ȴ�AT������Ӧ
static at_return wait_resp(at_obj_t *at, at_respond_t *r)
{        
    at->ret   = AT_RET_TIMEOUT;
    at->resp_timer = at_get_ms();    
    at->rcv_cnt = 0;                   //��ս��ջ���
    at->resp  = r;
    at_sem_wait(at->completed, r->timeout);
    at->adap.debug("<-\r\n%s\r\n", r->recvbuf);
    at->resp = NULL;
    return at->ret;
}

/**
  * @brief       �ȴ����յ�ָ����
  * @param[in]   resp    - �ڴ������մ�(��"OK",">")
  * @param[in]   timeout - �ȴ���ʱʱ��
  */
at_return wait_recv(struct at_work_ctx *e, const char *resp, unsigned int timeout)
{
    char buf[64];
    int cnt = 0, len;
    at_obj_t *at = e->at;    
    at_return ret = AT_RET_TIMEOUT;
    unsigned int timer = at_get_ms();
    while (at_get_ms() - timer < timeout) {
        len = e->read(e, &buf[cnt], sizeof(buf) - cnt);
        if (len > 0) {
            cnt += len;
            buf[cnt] = '\0';
            if (strstr(buf, resp)) {
                ret =  AT_RET_OK;
                break;
            } else if (strstr(buf, "ERROR")) {
                ret =  AT_RET_ERROR;
                break;
            }
        } else
            at_delay(1);
    }
    at->adap.debug("%s\r\n", buf);
    return ret;
}

/**
  * @brief       ����AT������
  * @param[in]   adap - AT�ӿ�������
  */
void at_obj_init(at_obj_t *at, const at_adapter_t *adap)
{
    at->adap    = *adap;
    at->rcv_cnt = 0;    
    at->send_lock = at_sem_new(1);
    at->recv_lock = at_sem_new(1);
    at->completed = at_sem_new(0);
    at->urc_item  = NULL;
    if (at->adap.debug == NULL)
        at->adap.debug = nop_dbg;
    
}

/**
  * @brief       ִ������
  * @param[in]   fmt    - ��ʽ�����
  * @param[in]   r      - ��Ӧ����,�����NULL, Ĭ�Ϸ���OK��ʾ�ɹ�,�ȴ�5s
  * @param[in]   args   - �������б�
  */
at_return at_do_cmd(at_obj_t *at, at_respond_t *r, const char *cmd)
{
    at_return ret;
    char      defbuf[64];
    at_respond_t  default_resp = {"OK", defbuf, sizeof(defbuf), 5000};
    if (r == NULL) {
        r = &default_resp;                 //Ĭ����Ӧ      
    }
    if (!at_sem_wait(at->send_lock, r->timeout)) {
        return AT_RET_TIMEOUT;    
    }
    at->busy  = true;
    
    while (at->urc_cnt) {
        at_delay(10);
    }
    put_line(at, cmd);
    ret = wait_resp(at, r); 
    at_sem_post(at->send_lock);
    at->busy  = false;
    return ret;    
}

/**
  * @brief       ִ��AT��ҵ
  * @param[in]   at    - AT������
  * @param[in]   work  - ��ҵ��ں���(���� - int (*)(at_work_ctx_t *))
  * @param[in]   params- ��ҵ����
  * @return      ������work�ķ���ֵ
  */
int at_do_work(at_obj_t *at, at_work work, void *params)
{
    at_work_ctx_t ctx;
    int ret;
    if (!at_sem_wait(at->send_lock, MAX_AT_LOCK_TIME)) {
        return AT_RET_TIMEOUT;
    }
    if (!at_sem_wait(at->recv_lock, MAX_AT_LOCK_TIME))
        return AT_RET_TIMEOUT; 
    
    at->busy  = true;
    //����at_work_ctx_t
    ctx.params    = params;
    ctx.printf    = at_work_print;
    ctx.read      = at_work_read;
    ctx.write     = at_work_write;
    ctx.wait_resp = wait_recv;  
    ctx.at        = at;
     
    at->rcv_cnt = 0;
    ret = work(&ctx);
    at_sem_post(at->recv_lock);
    at_sem_post(at->send_lock);
    at->busy  = false;
    return ret;
}

/**
  * @brief       �ָ���Ӧ��
  * @param[in]   recvbuf  - ���ջ����� 
  * @param[out]  lines    - ��Ӧ������
  * @param[in]   separator- �ָ��(, \n)
  * @return      ����
  */
int at_split_respond_lines(char *recvbuf, char *lines[], int count, char separator)
{
    char *s = recvbuf;
    size_t i = 0;      
    if (s == NULL || lines == NULL) 
        return 0;     
        
    lines[i++] = s;    
    while(*s && i < count) {       
        if (*s == ',') {
            *s = '\0';                                       
            lines[i++] = s + 1;                           /*ָ����һ���Ӵ�*/
        }
        s++;        
    }    
    return i;
}
/**
  * @brief       ��URC�������в�ѯURC��
  * @param[in]   urcline - URC��
  * @return      true - ����ʶ�𲢴���, false - δʶ��URC
  */
const urc_item_t *find_urc_item(at_obj_t *at, char *urc_buf, unsigned int size)
{
    const urc_item_t *tbl = at->adap.utc_tbl;
    int i;
    if (size < 2)
        return NULL;
    for (i = 0; i < at->adap.urc_tbl_count; i++) {
        if (strstr(urc_buf, tbl->prefix))
            return tbl;  
        tbl++;
    }
    return NULL;
}

/**
  * @brief       urc ���������
  * @param[in]   urcline - URC��
  */
static void urc_handler_entry(at_obj_t *at,  char *urcline, unsigned int size)
{
    at_urc_ctx_t context = {at->adap.read, urcline, at->adap.urc_bufsize, size};
    at->adap.debug("<=\r\n%s\r\n", urcline);            
    at->urc_item->handler(&context);                       /* �ݽ����ϲ㴦�� */      
}

/**
  * @brief       urc ���մ���
  * @param[in]   ch  - �����ַ�
  * @return      none
  */
static void urc_recv_process(at_obj_t *at, const char *buf, unsigned int size)
{
    register char *urc_buf;	
    int ch;
    urc_buf  = at->adap.urc_buf;
    
    //���ճ�ʱ����,Ĭ��MAX_URC_RECV_TIMEOUT
    if (at->urc_cnt > 0 && at_istimeout(at->urc_timer, MAX_URC_RECV_TIMEOUT)) {
        urc_buf[at->urc_cnt] = '\0';
        if (at->urc_cnt > 2)
            at->adap.debug("urc recv timeout=>%s\r\n", urc_buf);
        at->urc_cnt  = 0;
        at->urc_item = NULL;		
    }    
	
    while (size--) {
        at->urc_timer = at_get_ms(); 
        ch =  *buf++;
        urc_buf[at->urc_cnt++] = ch;
        
        if (strchr(SPEC_URC_END_MARKS, ch) || ch == '\0') {      //��������б�
            urc_buf[at->urc_cnt] = '\0';    
            if (at->urc_item == NULL)
                at->urc_item = find_urc_item(at, urc_buf, at->urc_cnt);       
            if (at->urc_item != NULL){
                if (strchr(at->urc_item->end_mark, ch)) {        //ƥ��������  
                    urc_handler_entry(at, urc_buf, at->urc_cnt);
                    at->urc_cnt  = 0;
                    at->urc_item = NULL;
                }
            } else if (ch == '\r' || ch == '\n'|| ch == '\0') {
                if (at->urc_cnt > 2 && !at->busy) {
                    at->adap.debug("%s\r\n", urc_buf);          //δʶ�𵽵�URC
                }
                at->urc_cnt = 0;                
            }
        } else if (at->urc_cnt >= at->adap.urc_bufsize) {         //�������
            at->urc_cnt = 0;
            at->urc_item = NULL;
        }
    }
}

/**
  * @brief       ������Ӧ֪ͨ
  * @return      none
  */
static void resp_notification(at_obj_t *at, at_return ret)
{
    at->ret = ret;    
    at->resp = NULL;

    at_sem_post(at->completed);
}

/**
  * @brief       ָ����Ӧ���մ���
  * @param[in]   buf  - ���ջ�����
  * @param[in]   size - ���������ݳ���
  * @return      none
  */
static void resp_recv_process(at_obj_t *at, const char *buf, unsigned int size)
{
    char *rcv_buf;
    unsigned short rcv_size;	
    at_respond_t *resp = at->resp;
    
    if (resp == NULL)                                    //����������
        return;
    
    if (size) {
        rcv_buf  = (char *)resp->recvbuf;
        rcv_size = resp->bufsize;

        if (at->rcv_cnt + size >= rcv_size) {             //�������
            at->rcv_cnt = 0;
            at->adap.debug("Receive overflow:%s", rcv_buf);
        }
        /*�����յ������ݷ���rcv_buf�� ---------------------------------------------*/
        memcpy(rcv_buf + at->rcv_cnt, buf, size);
        at->rcv_cnt += size;
        rcv_buf[at->rcv_cnt] = '\0';
        
        if (strstr(rcv_buf, resp->matcher)) {            //����ƥ��
            resp_notification(at, AT_RET_OK);
            return;
        } else if (strstr(rcv_buf, "ERROR")) {
            resp_notification(at, AT_RET_ERROR);
            return;
        }
    } 
    
    if (at_istimeout(at->resp_timer, resp->timeout))    //���ճ�ʱ
        resp_notification(at, AT_RET_TIMEOUT);
    else if (at->suspend)                                //ǿ����ֹ
        resp_notification(at, AT_RET_ABORT);

}

/**
  * @brief       ATæ�ж�
  * @return      true - ��ATָ�������������ִ����
  */
bool at_obj_busy(at_obj_t *at)
{
    return !at->busy && at_istimeout(at->urc_timer, 2000);
}

/**
  * @brief       ����AT��ҵ
  * @return      none
  */
void at_suspend(at_obj_t *at)
{
    at->suspend = 1;
}

/**
  * @brief       �ָ�AT��ҵ
  * @return      none
  */
void at_resume(at_obj_t *at)
{
    at->suspend = 0;
}

/**
  * @brief       AT����
  * @return      none
  */
void at_process(at_obj_t *at)
{
    char buf[1];
    unsigned int len;
    if (!at_sem_wait(at->recv_lock, MAX_AT_LOCK_TIME))
        return;
    do {
        len = at->adap.read(buf, sizeof(buf));
        urc_recv_process(at, buf,len);
        resp_recv_process(at, buf, len);
    } while (len);
    
    at_sem_post(at->recv_lock);
}
