/*
 * ModbusMaster.c
 *
 *  Created on: 2022年1月7日
 *      Author: play
 */

#include "ModbusMaster.h"
#include "usart.h"

MODS_T g_tModS;

/*
*********************************************************************************************************
*	函 数 名:   GetCrc16
*	功能说明: CRC16检测
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static uint16_t GetCrc16(uint8_t *ptr, uint8_t length, uint16_t IniDat)
{
    uint8_t iix;
    uint16_t iiy;
    uint16_t crc16 = IniDat;

    for(iix = 0; iix < length; iix++)
    {
        crc16 ^= *ptr++;

        for(iiy = 0; iiy < 8; iiy++)
        {
            if(crc16 & 0x0001)
            {
                crc16 = (crc16 >> 1) ^ 0xa001;
            }
            else
            {
                crc16 = crc16 >> 1;
            }
        }
    }

    return(crc16);
}

/*
*********************************************************************************************************
*	函 数 名:  MODS_SendWithCRC
*	功能说明: 带CRC的发送从站数据
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODS_SendWithCRC(uint8_t *_pBuf, uint8_t _ucLen)
{
    uint16_t crc;
    uint8_t buf[MOD_TX_BUF_SIZE];

    memcpy(buf, _pBuf, _ucLen);
    crc = GetCrc16(_pBuf, _ucLen, 0xffff);
    buf[_ucLen++] = crc;
    buf[_ucLen++] = crc >> 8;

    HAL_UART_Transmit_DMA(&huart1, buf, _ucLen);
	while (__HAL_UART_GET_FLAG(&huart1,UART_FLAG_TC) == RESET) { }
}

/*
*********************************************************************************************************
*	函 数 名:  MOD_46H
*	功能说明: 有人云自定义46指令
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void MOD_46H(uint8_t slaveaddr, uint16_t regaddr, uint16_t reglength, uint8_t datalength, uint8_t* dat)
{
    uint8_t i;

    g_tModS.TxCount = 0;
    g_tModS.TxBuf[g_tModS.TxCount++] = slaveaddr;
    g_tModS.TxBuf[g_tModS.TxCount++] = 0x46;
    g_tModS.TxBuf[g_tModS.TxCount++] = regaddr >> 8;
    g_tModS.TxBuf[g_tModS.TxCount++] = regaddr;
    g_tModS.TxBuf[g_tModS.TxCount++] = reglength >> 8;
    g_tModS.TxBuf[g_tModS.TxCount++] = reglength;
    g_tModS.TxBuf[g_tModS.TxCount++] = datalength;

    for(i = 0; i < datalength; i++)
    {
        g_tModS.TxBuf[g_tModS.TxCount++] = dat[i];
    }

    MODS_SendWithCRC(g_tModS.TxBuf, g_tModS.TxCount);
}
