/******************************************************************************
 * @brief    ST mcu Ƭ��flash����
 *
 * Copyright (c) 2018~2020, <morro_luo@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs: 
 * Date           Author       Notes 
 * 2018-09-09     Morro        Initial version
 ******************************************************************************/
#include "mcu_flash.h"
#include "stm32f4xx.h"
#include <string.h>

typedef struct {
    unsigned int start;
    unsigned int size;
    unsigned int secnum;
}sec_info_t;

/*������ַӳ�� ---------------------------------------------------------------*/
const sec_info_t sec_map[] = 
{
    {0x08000000, 16*1024, FLASH_Sector_0},
    {0x08004000, 16*1024, FLASH_Sector_1},
    {0x08008000, 16*1024, FLASH_Sector_2},
    {0x0800C000, 16*1024, FLASH_Sector_3},
    {0x08010000, 64*1024, FLASH_Sector_4},
    {0x08020000, 128*1024, FLASH_Sector_5},
    {0x08040000, 128*1024, FLASH_Sector_6},
    {0x08040000, 128*1024, FLASH_Sector_7}
};

/*
 * @brief       stm32 mcu �ڲ�flash��������
 * @param[in]   addr        - ��ַ
 * @param[in]   ̽�մ�С    - size
 * @return      0 - ʧ��, ��0 - �ɹ�
 */
int mcu_flash_erase(unsigned int addr, size_t size)
{ 
    int i;
    int len = sizeof(sec_map) / sizeof(sec_info_t);
    const sec_info_t *sec = &sec_map[len - 1];
    
    FLASH_Status status;
    
    /*Խ�紦��*/
    if (addr > sec->start + sec->size)
        return 0;
    
    FLASH_Unlock();
    for (i = 0; i < len; i++)
    {
        sec = &sec_map[i];
        if ( (sec->start >= addr && sec->start < addr + size) || 
             (sec->start + sec->size > addr && sec->start + sec->size <= addr + size))
        {
            //FLASH_OB_WRPConfig();
            status = FLASH_EraseSector(sec->secnum, VoltageRange_2);
            if (status != FLASH_COMPLETE)
            {
                FLASH_Lock(); 
                return 0;  
            }
                          
        }
    }
    FLASH_Lock(); 
    return 1;
}

/*
 * @brief       stm32 mcu �ڲ�flashд����
 * @param[in]   addr        - ��ַ
 * @param[in]   buf         - ���ݻ�����
 * @param[in]   д���С    - size
 * @return      0 - ʧ��, ��0 - �ɹ�
 */
int mcu_flash_write(unsigned int addr ,const void *buf, size_t size)
{
    unsigned char *p = (uint8_t *)buf;
//    unsigned int base = addr;
//    size_t tlen = size;
    int wrlen;
    FLASH_Status status = FLASH_COMPLETE;
    int ret = 0;
    FLASH_Unlock();              
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_FLAG_OPERR | 
                    FLASH_FLAG_PGAERR);     
    while (size) {
#if 0
        /*���ݶ��뷽ʽ�Ż�д�볤��*/
        if ((addr & 7) == 0 && size > 8)             /*8�ֽڶ���,��˫��д��*/  
        {
            status = FLASH_ProgramDoubleWord(addr, *((uint64_t *)p));
            if (status != FLASH_COMPLETE)
                goto _quit;
            wrlen = 8;
        }
        else if ((addr & 3) == 0 && size > 4)        /*4�ֽڶ���,����д��*/
        {
            status = FLASH_ProgramWord(addr, *((uint32_t *)p));
            if (status != FLASH_COMPLETE)
                goto _quit;
            wrlen = 4;
        }
        else if ((addr & 1) == 0 && size > 2)        /*2�ֽڶ���,������д��*/
        {
            status = FLASH_ProgramHalfWord(addr, *((uint16_t *)p));
            if (status != FLASH_COMPLETE)
                goto _quit;
            wrlen = 2;
        }
        else                                         /*���ֽ�д�� --------*/
        {
            status = FLASH_ProgramByte(addr, *((uint8_t *)p));
            if (status != FLASH_COMPLETE)
                goto _quit;
            wrlen = 1;
        }
#endif
        if ((addr & 1) == 0 && size > 2)        /*2�ֽڶ���,������д��*/
        {
            status = FLASH_ProgramHalfWord(addr, *((uint16_t *)p));
            if (status != FLASH_COMPLETE)
                goto _quit;
            wrlen = 2;
        }
        else                                         /*���ֽ�д�� --------*/
        {
            status = FLASH_ProgramByte(addr, *((uint8_t *)p));
            if (status != FLASH_COMPLETE)
                goto _quit;
            wrlen = 1;
        }        
        /*��ַƫ�� -------------------------------------------------------*/
        size -= wrlen;
        addr += wrlen;
        p    += wrlen;        
    }
_quit:

     ret = status == FLASH_COMPLETE;// && memcmp(buf, (void *)base, tlen) ? 1 : 0;     
	 FLASH_Lock();
     return ret;
}

/*
 * @brief       stm32 mcu �ڲ�flash������
 * @param[in]   addr        - ��ַ
 * @param[in]   buf         - ���ݻ�����
 * @param[in]   ��������    - size
 * @return      0 - ʧ��, ��0 - �ɹ�
 */
int mcu_flash_read(unsigned int addr ,void *buf, size_t size)
{
    memcpy(buf, (void *)addr, size);  
    return 0;
}
