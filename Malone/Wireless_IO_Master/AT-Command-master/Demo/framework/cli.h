/******************************************************************************
 * @brief    �����д���
 *
 * Copyright (c) 2015-2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
* 2015-06-09      Morro        ����
*                             
* 2017-07-04      Morro        �Ż��ֶηָ��
* 
* 2020-07-05      Morro        ʹ��cli_obj_t����, ֧�ֶ������Դ����
 ******************************************************************************/
#ifndef _CMDLINE_H_
#define _CMDLINE_H_

#include "comdef.h"

#define CLI_MAX_CMD_LEN           64             /*�����г���*/                 
#define CLI_MAX_ARGS              16             /*����������*/
#define CLI_MAX_CMDS              32             /*�����������������*/

struct cli_obj;

/*�������*/
typedef struct {
	char	   *name;		                         /*������*/
    /*��������*/   
	int        (*handler)(struct cli_obj *o, int argc, char *argv[]);   
    const char *brief;                               /*������*/
}cmd_item_t;

#define __cmd_register(name,handler,brief)\
    USED ANONY_TYPE(const cmd_item_t,__cli_cmd_##handler)\
    SECTION("cli.cmd.1") = {name, handler, brief}
    
/*******************************************************************************
 * @brief     ����ע��
 * @params    name      - ������
 * @params    handler   - ��������
 *            ����:int (*handler)(struct cli_obj *s, int argc, char *argv[]);   
 * @params    brief     - ʹ��˵��
 */                 
#define cmd_register(name,handler,brief)\
    __cmd_register(name,handler,brief)

/*cli �ӿڶ��� -------------------------------------------------------------*/
typedef struct {
    unsigned int (*write)(const void *buf, unsigned int len);
    unsigned int (*read) (void *buf, unsigned int len);
}cli_port_t;

/*�����ж���*/
typedef struct cli_obj {
    unsigned int (*write)(const void *buf, unsigned int len);
    unsigned int (*read) (void *buf, unsigned int len);
    void         (*print)(struct cli_obj *this, const char *fmt, ...); 
    char           recvbuf[CLI_MAX_CMD_LEN + 1];  /*������ջ�����*/
    unsigned short recvcnt;                       /*�����ճ���*/    
    unsigned       enable : 1;
}cli_obj_t;

void cli_init(cli_obj_t *obj, const cli_port_t *p);

void cli_enable(cli_obj_t *obj);

void cli_disable (cli_obj_t *obj);

void cli_exec_cmd(cli_obj_t *obj, const char *cmd);

void cli_process(cli_obj_t *obj);



#endif	/* __CMDLINE_H */
