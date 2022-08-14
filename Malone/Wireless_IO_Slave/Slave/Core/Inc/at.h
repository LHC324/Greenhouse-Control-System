#ifndef _AT_H_
#define _AT_H_

//#include "at_util.h"
#include <stdbool.h>

/* 单行最大命令长度 */
#define MAX_AT_CMD_LEN 128

/* 单行urc接收超时时间*/
#define MAX_URC_RECV_TIMEOUT 300

/* 指定的URC 结束标记列表 */
#define SPEC_URC_END_MARKS ":,\r\n"
#define AT_CMD_END_MARK_CRLF "\r\n"
#define AT_CMD_END_MARK_CR "\r"
#define AT_CMD_END_MARK_LF "\n"
#define AT_CMD_OK "OK"
#define AT_CMD_ERROR "ERR"

/*AT等待时间*/
#define AT_WAITTIMES 500U
/*设置/查询模块 AT 命令回显设置*/
#define SECHO "OFF"
/*查询/设置串口参数*/
#define BAUD_PARA "115200,8,1,NONE,NFC"
/*设置查询工作模式*/
#define SWORK_MODE "FP"
/*查询设置休眠模式*/
#define SPOWER_MODE "RUN"
/*空闲时间:3~240 单位秒（默认 20）*/
#define SIMT "20"
/*time： 500,1000,1500,2000,2500,3000,3500,4000ms（默认 2000）*/
#define SWTM "2000"
/*class： 1~10（默认 10）*/
/*速率对应关系（速率为理论峰值，实际速度要较小一些）：
 1: 268bps
 2: 488bps
 3: 537bps
 4: 878bps
 5: 977bps
 6: 1758bps
 7: 3125bps
 8: 6250bps
 9: 10937bps
 10: 21875bps*/
#define SSPD "10"
/*addr： 0~65535（默认 0）65535 为广播地址，同信道同速率的模块都能接收*/
#define SADDR "7"
/*L102-L 工作频段：(398+ch)MHz*/
#define SCH "7"
/**/
#define SFEC "ON"
/*10~20（默认 20db）不推荐使用小功率发送，其电源利用效率不高*/
#define SPWR "20"
/*time： 0~15000ms（默认 500）仅在 LR/LSR 模式下有效，表示进入接收状态所持续的最长时间，当速率等级较慢的时候应适
当的增加该值以保证数据不会被截断。LSR 模式下如果该值设置为 0 则模块发送数据后不开启接收。*/
#define SRTO "500"
/*time： 相邻数据包间隔，范围：100~60000ms*/
#define SSQT "1000"
/*key：16 字节 HEX 字符串*/
#define SKEY "30313233343536373839414243444546"
/*设置/查询快速进入低功耗使能标志,sta： 1 为打开，0 为关闭。*/
#define SPFLAG "0"
/*设置/查询快速进入低功耗数据
Data：123456（默认 123456）。
Style：ascii、hex（默认 ascii）。*/
#define SPDATE "123456,hex"
/*设置/查询发送完成回复标志,sta：1 为打开，0 为关闭。*/
#define SSENDOK "0"

typedef enum
{
    CONF_MODE = 0x00,
    FREE_MODE,
    UNKOWN_MODE,
    USER_ESC,
    CONF_ERROR,
    CONF_TOMEOUT,
    CONF_SUCCESS,
    INPUT_ERROR,
    CMD_MODE, //
    CMD_SURE,
    SET_ECHO,
    SET_UART,
    WORK_MODE,
    POWER_MODE,
    SET_TIDLE,
    SET_TWAKEUP,
    SPEED_GRADE,
    TARGET_ADDR,
    CHANNEL,
    CHECK_ERROR,
    TRANS_POWER,
    SET_OUTTIME,
    // SET_KEY,
    RESTART,
    SIG_STREN,
    EXIT_CMD,
    RECOVERY,
    SELECT_NID,
    SELECT_VER,
    LOW_PFLAG,
    LOW_PDATE,
    FINISH_FLAG,
    EXIT_CONF,
    NO_CMD
} At_InfoList;

typedef struct AT_HandleTypeDef *pAtHandle;
typedef struct AT_HandleTypeDef AtHandle;
struct AT_HandleTypeDef
{
    At_InfoList Name;
    char *pSend;
    char *pRecv;
    void (*event)(char *);
} __attribute__((aligned(4)));

extern void At_GetSlaveId(void);

#endif
