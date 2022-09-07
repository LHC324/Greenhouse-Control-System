/*
 * ModbusMaster.c
 *
 *  Created on: 2022年1月4日
 *      Author: play
 */
#include "ModbusMaster.h"
#include "usart.h"

MODS_T g_tModS;

static void MODS_SendWithCRC(uint8_t *_pBuf, uint8_t _ucLen);


/**
 * @brief  取得16bitCRC校验码
 * @param  ptr   当前数据串指针
 * @param  length  数据长度
 * @param  init_dat 校验所用的初始数据
 * @retval 16bit校验码
 */
uint16_t Get_Crc16(uint8_t *ptr, uint16_t length, uint16_t init_dat)
{
	uint16_t i = 0;
	uint16_t j = 0;
	uint16_t crc16 = init_dat;

	for (i = 0; i < length; i++)
	{
		crc16 ^= *ptr++;

		for (j = 0; j < 8; j++)
		{
			if (crc16 & 0x0001)
			{
				crc16 = (crc16 >> 1) ^ 0xa001;
			}
			else
			{
				crc16 = crc16 >> 1;
			}
		}
	}
	return (crc16);
}

/**
 * @brief  大小端数据类型交换
 * @note   对于一个单精度浮点数的交换仅仅需要2次
 * @param  pData 数据
 * @param  start 开始位置
 * @param  length  数据长度
 * @retval None
 */
void Endian_Swap(uint8_t *pData, uint8_t start, uint8_t length)
{
	uint8_t i = 0;
	uint8_t tmp = 0;
	uint8_t count = length / 2U;
	uint8_t end = start + length - 1U;

	for (; i < count; i++)
	{
		tmp = pData[start + i];
		pData[start + i] = pData[end - i];
		pData[end - i] = tmp;
	}
}

/**
 * @brief  有人云自定义46指令
 * @param  slaveaddr 从站地址
 * @param  regaddr 寄存器开始地址
 * @param  reglength 寄存器长度
 * @param  dat 数据
 * @retval None
 */
void MOD_46H(uint8_t slaveaddr, uint16_t regaddr, uint16_t reglen, uint8_t datalen, uint8_t* dat)
{
    uint8_t i;

    g_tModS.TxCount = 0;
    g_tModS.TxBuf[g_tModS.TxCount++] = slaveaddr;
    g_tModS.TxBuf[g_tModS.TxCount++] = 0x46;
    g_tModS.TxBuf[g_tModS.TxCount++] = regaddr >> 8;
    g_tModS.TxBuf[g_tModS.TxCount++] = regaddr;
    g_tModS.TxBuf[g_tModS.TxCount++] = reglen >> 8;
    g_tModS.TxBuf[g_tModS.TxCount++] = reglen;
    g_tModS.TxBuf[g_tModS.TxCount++] = datalen;

    for(i = 0; i < datalen; i++)
    {
        g_tModS.TxBuf[g_tModS.TxCount++] = dat[i];
    }

    MODS_SendWithCRC(g_tModS.TxBuf, g_tModS.TxCount);
}

/**
 * @brief  带CRC的发送从站数据
 * @param  _pBuf 数据缓冲区指针
 * @param  _ucLen 数据长度
 * @retval None
 */
void MODS_SendWithCRC(uint8_t *_pBuf, uint8_t _ucLen)
{
    uint16_t crc;
    uint8_t buf[MOD_TX_BUF_SIZE];

    memcpy(buf, _pBuf, _ucLen);
    crc = Get_Crc16(_pBuf, _ucLen, 0xffff);
    buf[_ucLen++] = crc;
    buf[_ucLen++] = crc >> 8;
    
    HAL_UART_Transmit(&MD_UART, buf, _ucLen, 0xffff);
}
