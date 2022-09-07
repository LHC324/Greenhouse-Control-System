/*
 * ModbusMaster.h
 *
 *  Created on: 2021年1月29日
 *      Author: play
 */

#ifndef INC_MODBUSMASTER_H_
#define INC_MODBUSMASTER_H_

#include "main.h"

#define  SLAVEADDRESS	      0x02
#define  MOD_RX_BUF_SIZE      128U
#define  MOD_TX_BUF_SIZE      128U

typedef struct
{
	uint8_t RxBuf[MOD_RX_BUF_SIZE];
	uint16_t RxCount;
	uint8_t RxStatus;
	uint8_t RxNewFlag;

	uint8_t TxBuf[MOD_TX_BUF_SIZE];
	uint16_t TxCount;
}MODS_T;

extern MODS_T g_tModS;


extern void MOD_46H(uint8_t slaveaddr, uint16_t regaddr, uint16_t reglen, uint8_t datalen, uint8_t* dat);

#endif /* INC_MODBUSMASTER_H_ */
