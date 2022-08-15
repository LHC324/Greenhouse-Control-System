/*
 * Dwin.h
 *
 *  Created on: Nov 19, 2020
 *      Author: play
 */

#ifndef INC_DWIN_H_
#define INC_DWIN_H_

#include "tool.h"

/*迪文屏幕带CRC校验*/
#define USING_CRC 1

#define RX_BUF_SIZE 128
#define TX_BUF_SIZE 128

#define WRITE_CMD 0x82		 //写
#define READ_CMD 0x83		 //读
#define PAGE_CHANGE_CMD 0x84 //页面切换
#define TOUCH_CMD 0xD4		 //触摸动作

/*迪文屏幕页面*/
#define MAIN_PAGE 0x03
#define DIGITAL_INPUT_PAGE 0x04
#define DIFITAL_OUTPUT_PAGE 0x05
#define ANALOG_INPUT_PAGE 0x06
#define ANALOG_OUTPUT_PAGE 0x07
#define NONE_PAGE 0x08
#define COMMUNICATION_PAGE 0x0F
#define ERROR_PAGE 0x10
#define RESET_POEWR_NOTE_PAGE 28U
#define NEXT_DELAT_TIMES 50U

#define USER_NAME_ADDR 0x1000	  //用户名地址
#define USER_PASSWORD_ADDR 0x1001 //用户密码
#define LOGIN_SURE_ADDR 0x1002	  //登录确认地址
// #define HELP_ADDR 0x1003			//帮助按钮地址
#define CANCEL_ADDR 0x1003	   //注销地址
#define ERROR_NOTE_ADDR 0x1004 //错误图标提示地址

#define ANALOG_INPUT1_ADDR 0x100E //模拟量1输入地址
#define ANALOG_INPUT2_ADDR 0x1010 //模拟量2输入地址
#define ANALOG_INPUT3_ADDR 0x1012 //模拟量3输入地址
#define ANALOG_INPUT4_ADDR 0x1014 //模拟量4输入地址
#define ANALOG_INPUT5_ADDR 0x1016 //模拟量5输入地址
#define ANALOG_INPUT6_ADDR 0x1018 //模拟量6输入地址
#define ANALOG_INPUT7_ADDR 0x101A //模拟量7输入地址
#define ANALOG_INPUT8_ADDR 0x101C //模拟量8输入地址

#define ANALOG_OUTPUT1_ADDR 0x1020 //模拟量1输出地址
#define ANALOG_OUTPUT2_ADDR 0x1022 //模拟量2输出地址
#define ANALOG_OUTPUT3_ADDR 0x1024 //模拟量3输出地址
#define ANALOG_OUTPUT4_ADDR 0x1026 //模拟量4输出地址
#define ANALOG_OUTPUT5_ADDR 0x1028 //模拟量5输出地址
#define ANALOG_OUTPUT6_ADDR 0x102A //模拟量6输出地址
#define ANALOG_OUTPUT7_ADDR 0x102C //模拟量7输出地址
#define ANALOG_OUTPUT8_ADDR 0x102E //模拟量8输出地址
/*参数存储区*/
#define PRESSURE_OUT_ADDR 0x1068	//压力输出地址
#define DIGITAL_INPUT_ADDR 0x1005	//数字量输入地址
#define DIGITAL_OUTPUT_ADDR 0x1006	//数字量输出地址
#define SS_SIGNAL_ADDR 0x1008		//启停信号地址
#define ATX_STATE_ADDR 0x1009		// AT模块状态地址
#define USER_TAP_ADDR 0x100A		//用户阀地址
#define RESET_CARD_INFO_ADDR 0x100B //复位板卡信息地址
#define SURE_CARD_INFO_ADDR 0x100C	//确认板卡信息地址
#define ANALOG_INPUT_ADDR 0x100E	//模拟量输入地址
#define ANALOG_OUTPUT_ADDR 0x1020	//模拟量输出地址
#define PARAM_SETTING_ADDR 0x1030	//迪文屏幕后台参数设定地址
/*阈值设定界面*/
#define PTANK_MAX_ADDR 0x1030	//储槽压力上限地址
#define PTANK_MIN_ADDR 0x1032	//储槽压力下限地址
#define PVAOUT_MAX_ADDR 0x1034	//汽化器出口压力上限地址
#define PVAOUT_MIN_ADDR 0x1036	//汽化器出口压力下限地址
#define PGAS_MAX_ADDR 0x1038	//气站出口压力上限地址
#define PGAS_MIN_ADDR 0x103A	//气站出口压力下限地址
#define LTANK_MAX_ADDR 0x103C	//储槽液位上限地址
#define LTANK_MIN_ADDR 0x103E	//储槽液位下限地址
#define PTOLE_MAX_ADDR 0x1040	//压力容差上限地址
#define PTOLE_MIN_ADDR 0x1042	//压力容差下限地址
#define LEVEL_MAX_ADDR 0x1044	//液位容差上限地址
#define LEVEL_MIN_ADDR 0x1046	//液位容差下限地址
#define SPSFS_MAX_ADDR 0x1048	//启动模式时：储槽泄压启动值上限地址
#define SPSFS_MIN_ADDR 0x104A	//启动模式时：储槽泄压启动值下限地址
#define PSVA_START_ADDR 0x104C	//启动模式时：汽化器出口压力泄压启动地址
#define PSVAP_STOP_ADDR 0x104E	//启动模式时:汽化器出口压力泄压停止地址
#define PBACK_MAX_ADDR 0x1050	// B2-B1回压差上限地址
#define PBACK_MIN_ADDR 0x1052	//储槽回压差下限地址
#define PPVAP_START_ADDR 0x1054 //停机模式时汽化器出口压力泄压启动地址
#define PPVAP_STOP_ADDR 0x1056	//停机模式时:汽化器出口压力泄压停止值
#define SPSFE_MAX_ADDR 0x1058	//停机模式时：储槽泄压启动值上限地址
#define SPSFE_MIN_ADDR 0x105A	//停机模式时：储槽泄压启动值下限地址
#define PTANK_LIMIT_ADDR 0x105C //安全策略储槽压力极限
#define LTANK_LIMIT_ADDR 0x105E //安全策略储槽液位极限
#define TANK_HEAD_HEIGHT 0x1060 //储槽封头高度
#define TANK_CY_RADIUS 0x1062	//储槽圆柱半径
#define TANK_CY_LENGTH 0x1064	//储槽圆柱长度
#define TANK_FL_DENSITY 0x1066	//储槽液体密度
#define RESTORE_ADDR 0x1007		//恢复出厂设置地址
#define BOARD_TYPE_ADDR 0x1080	//板卡类型显示地址
#define ERROR_CODE_ADDR 0x1091	//错误代码地址
#define ERROR_ANMATION 0x1092	//错误动画地址
#define RSURE_CODE 0x00F1		//恢复出厂设置确认键值
#define RCANCEL_CODE 0x00F0		//注销键值
/*板卡卡槽地址*/
#define CARD_SLOT1_ADDR 0x1080
#define CARD_SLOT2_ADDR 0x1081
#define CARD_SLOT3_ADDR 0x1082
#define CARD_SLOT4_ADDR 0x1083
#define CARD_SLOT5_ADDR 0x1084
#define CARD_SLOT6_ADDR 0x1085
#define CARD_SLOT7_ADDR 0x1086
#define CARD_SLOT8_ADDR 0x1087
#define CARD_SLOT9_ADDR 0x1088
#define CARD_SLOT10_ADDR 0x1089
#define CARD_SLOT11_ADDR 0x108A
#define CARD_SLOT12_ADDR 0x108B
#define CARD_SLOT13_ADDR 0x108C
#define CARD_SLOT14_ADDR 0x108D
#define CARD_SLOT15_ADDR 0x108E
#define CARD_SLOT16_ADDR 0x108F
/*板卡类型显示基地址*/
#define CARD_TYPE_BASE_ADDR 0x1080
/*同类型板卡区分地址*/
#define CARD_SAMETYPE_DIFF_ADDR 0x1090
/*通信板卡数据地址*/
#define CARD_COMM_REPORT_ADDR 0x10D0
/*提示页面地址*/
#define NOTE_PAGE_ADDR 0x10D6

typedef struct Dwin_HandleTypeDef *pDwinHandle;
typedef struct Dwin_HandleTypeDef DwinHandle;
typedef void (*pfunc)(pDwinHandle, uint8_t *);
typedef struct
{
	uint32_t addr;
	float upper;
	float lower;
	/*预留外部数据结构接口*/
	// void *pHandle;
	pfunc event;
} DwinMap;

struct Dwin_HandleTypeDef
{
	uint8_t Id;
	void (*Dw_Transmit)(pDwinHandle);
	void (*Dw_Write)(pDwinHandle, uint16_t, uint8_t *, uint16_t);
	void (*Dw_Read)(pDwinHandle, uint16_t, uint8_t);
	void (*Dw_Page)(pDwinHandle, uint16_t);
	void (*Dw_Poll)(pDwinHandle);
	void (*Dw_Error)(pDwinHandle, uint8_t, uint8_t);
	void (*Dw_Delay)(uint32_t);
	struct
	{
		uint8_t *pTbuf;
		uint16_t TxSize;
		uint16_t TxCount;
	} Master;
	struct
	{
		// uint8_t *pRbuf;
		// uint16_t RxSize;
		// uint16_t RxCount;
		/*预留外部数据结构接口*/
		void *pHandle;
		void *pHandle1;
		void *pHandle2;
		DwinMap *pMap;
		uint16_t Events_Size;
	} Slave;
	DmaHandle Uart;
	// UART_HandleTypeDef *huart;
} __attribute__((aligned(4)));

extern pDwinHandle Dwin_Object;
#define Dwin_Handler(obj) (obj->Dw_Poll(obj))
#define Dwin_Recive_Buf(obj) (obj->Uart.pRbuf)
#define Dwin_Recive_Len(obj) (obj->Uart.RxCount)
#define Dwin_Rx_Size(obj) (obj->Uart.RxSize)
extern void MX_DwinInit(void);

#endif /* INC_DWIN_H_ */
