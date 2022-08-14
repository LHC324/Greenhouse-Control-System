#include "at_usr.h"

#if defined(USING_AT)
/*定义AT控制器对象*/
at_obj_t at;
/*URC主动上报缓冲区*/
static char urc_buf[URC_SIZE];
/*定义URC表*/
urc_item_t utc_tbl[] = {
	{"+CSQ: ", AT_CMD_END_MARK_CRLF, NULL},
	{" ", AT_CMD_END_MARK_CRLF, NULL}};

/*AT适配器接口*/
const at_adapter_t adap = {
	// .urc_buf     = urc_buf,
	// .urc_bufsize = sizeof(urc_buf),
	// .utc_tbl     = utc_tbl,
	// .urc_tbl_count = sizeof(utc_tbl) / sizeof(urc_item_t),
	// /*debug调试接口*/
	// .debug       = at_debug,
	// /*适配GPRS模块的串口读写接口*/
	// .write       = uart_write,
	// .read        = uart_read
	0};
#endif
