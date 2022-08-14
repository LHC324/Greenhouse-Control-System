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
#include "cli.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>

static const cmd_item_t cmd_tbl_start SECTION("cli.cmd.0") = {0};
static const cmd_item_t cmd_tbl_end SECTION("cli.cmd.4") = {0};    
/*
 * @brief       ��������
 * @param[in]   keyword - ����ؼ���
 * @return      ������
 */ 
static const cmd_item_t *find_cmd(const char *keyword)
{                   
	const cmd_item_t *it;
    for (it = &cmd_tbl_start + 1; it < &cmd_tbl_end; it++) {
		if (!strcasecmp(keyword, it->name))
		    return it;
    }
	return NULL;
}

/*******************************************************************************
 * @brief      �ַ����ָ�  - ��Դ�ַ������ҳ�������separatorָ���ķָ���
 *                            (��',')���滻���ַ���������'\0'�γ��Ӵ���ͬʱ��list
 *                            ָ���б��е�ÿһ��ָ��ֱ�ָ��һ���Ӵ�
 * @example 
 *             input=> s = "abc,123,456,,fb$"  
 *             separator = ",$"
 *            
 *             output=>s = abc'\0' 123'\0' 456'\0' '\0' fb'\0''\0'
 *             list[0] = "abc"
 *             list[1] = "123"
 *             list[2] = "456"
 *             list[3] = ""
 *             list[4] = "fb"
 *             list[5] = ""
 *
 * @param[in] str             - Դ�ַ���
 * @param[in] separator       - �ָ��ַ��� 
 * @param[in] list            - �ַ���ָ���б�
 * @param[in] len             - �б���
 * @return    listָ���б���������������ʾ�򷵻�6
 ******************************************************************************/  
static size_t strsplit(char *s, const char *separator, char *list[],  size_t len)
{
    size_t count = 0;      
    if (s == NULL || list == NULL || len == 0) 
        return 0;     
        
    list[count++] = s;    
    while(*s && count < len) {       
        if (strchr(separator, *s) != NULL) {
            *s = '\0';                                       
            list[count++] = s + 1;                           /*ָ����һ���Ӵ�*/
        }
        s++;        
    }    
    return count;
}

/*
 *@brief ��ӡһ����ʽ���ַ��������ڿ���̨
 *@retval 
 */
static void cli_print(cli_obj_t *obj, const char *format, ...)
{
	va_list args;
    int len;
	char buf[CLI_MAX_CMD_LEN + CLI_MAX_CMD_LEN / 2];
	va_start (args, format);
	len = vsnprintf (buf, sizeof(buf), format, args);
    obj->write(buf, len);
}


/*
 * @brief       cli ��ʼ��
 * @param[in]   p - cli�����ӿ�
 * @return      none
 */
void cli_init(cli_obj_t *obj, const cli_port_t *p)
{
    obj->read   = p->read;
    obj->write  = p->write;
    obj->print  = cli_print;
    obj->enable = true;
}

/*
 * @brief       ����cli����ģʽ(cli��ʱ�Զ������û����������)
 * @param[in]   none
 * @return      none
 **/
void cli_enable(cli_obj_t *obj)
{
    obj->enable = true;
}

/*
 * @brief       �˳�cli����ģʽ(cli��ʱ���ٴ����û����������)
 * @param[in]   none
 * @return      none
 **/
void cli_disable (cli_obj_t *obj)
{
    obj->enable = false;
}

/*
 * @brief       ������
 * @param[in]   line - ������
 * @return      none
 **/
static void process_line(cli_obj_t *obj)
{
    char *argv[CLI_MAX_ARGS];
    int   argc;    
    const cmd_item_t *it;
    argc = strsplit(obj->recvbuf, " ,",argv, CLI_MAX_ARGS);
    if ((it = find_cmd(argv[0])) == NULL) {
        obj->print(obj, "Unknown command '%s' - try 'help'\r\n", argv[0]);
        return;
    }
    it->handler(obj, argc, argv);
}

/*
 * @brief       ִ��һ������(����cli�Ƿ�����,����ִ��)
 * @param[in]   none
 * @return      none
 **/
void cli_exec_cmd(cli_obj_t *obj, const char *cmd)
{
    snprintf(obj->recvbuf, CLI_MAX_CMD_LEN, "%s", cmd);
    process_line(obj);
}

/*
 * @brief       �����д������
 * @param[in]   none
 * @return      none
 **/
void cli_process(cli_obj_t *obj)
{
    
    int i;
    if (!obj->read || !obj->enable)
        return;
    i = obj->recvcnt;
    obj->recvcnt += obj->read(&obj->recvbuf[i], CLI_MAX_CMD_LEN - i);
    while (i < obj->recvcnt) {
        if (obj->recvbuf[i] == '\r' || obj->recvbuf[i] == '\n') {    /*��ȡ1��*/
            obj->recvbuf[i] = '\0';
            process_line(obj);
            obj->recvcnt = 0;
        }
        i++;
    }
}


/*******************************************************************************
* @brief	   ����Ƚ���
* @param[in]   none
* @return 	   �ο�strcmp
*******************************************************************************/
static int cmd_item_comparer(const void *item1,const void *item2)
{
    cmd_item_t *it1 = *((cmd_item_t **)item1); 
    cmd_item_t *it2 = *((cmd_item_t **)item2);    
    return strcmp(it1->name, it2->name);
}

/*
 * @brief	   ��������
 */
static int do_help (struct cli_obj *s, int argc, char *argv[])
{
	int i,j, count;
    const cmd_item_t *item_start = &cmd_tbl_start + 1; 
    const cmd_item_t *item_end   = &cmd_tbl_end;	
	const cmd_item_t *cmdtbl[CLI_MAX_CMDS];
    
    if (argc == 2) {
        if ((item_start = find_cmd(argv[1])) != NULL) 
        {
            s->print(s, item_start->brief);                  /*����ʹ����Ϣ----*/
            s->print(s, "\r\n");   
        }
        return 0;
    }
    for (i = 0; i < item_end - item_start && i < CLI_MAX_ARGS; i++)
        cmdtbl[i] = &item_start[i];
    count = i;
    /*������������� ---------------------------------------------------------*/
    qsort(cmdtbl, i, sizeof(cmd_item_t*), cmd_item_comparer);    		        
    s->print(s, "\r\n");
    for (i = 0; i < count; i++) {
        s->print(s, cmdtbl[i]->name);                        /*��ӡ������------*/
        /*�������*/
        j = strlen(cmdtbl[i]->name);
        if (j < 10)
            j = 10 - j;
            
        while (j--)
            s->print(s, " ");
            
        s->print(s, "- "); 
		s->print(s, cmdtbl[i]->brief);                       /*����ʹ����Ϣ----*/
        s->print(s, "\r\n");        
    }
	return 1;
}
 /*ע��������� ---------------------------------------------------------------*/
cmd_register("help", do_help, "list all command.");   
cmd_register("?",    do_help, "alias for 'help'");

