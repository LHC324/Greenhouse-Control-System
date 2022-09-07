/*
 * myflash.c
 *
 *  Created on: 2022年01月23日
 *      Author: LHC
 */

#include "Flash.h"
#include "string.h"

/*===================================================================================*/
/* Flash 分配
* @用户flash区域：0-125页(1KB/页)
* @系统参数区： 126页
* @校准系数存放区域：127页
*/
/*===================================================================================*/

/*
 *  初始化FLASH
 */
void FLASH_Init(void)
{
	HAL_FLASH_Unlock();
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR);
	HAL_FLASH_Lock();
}

/**
 * 读FLASH
 * @param  Address 地址
 * @note   为了兼容各种数据类型，按字节读取
 * @param  Buffer  存放读取的数据
 * @param  Size    要读取的数据大小，单位字节
 * @return         读出成功的字节数
 */
bool FLASH_Read(uint32_t Address, void *Buffer, uint32_t Size)
{
	uint8_t *pdata = (uint8_t *)Buffer;

	/* 非法地址 */
	if (Address < STM32FLASH_BASE || (Address > STM32FLASH_END) || Size == 0 || Buffer == NULL)
		return false;
	
	for(uint32_t i = 0; i < Size; i++)
	{
		pdata[i] = *(__IO uint16_t*)Address;
		Address += 1U;
	}

	return true;
}

/**
 * 写FLASH
 * @param  Address    写入起始地址，！！！要求2字节对齐！！！
 * @param  Buffer     待写入的数据，！！！要求2字节对齐！！！
 * @param  Size 要写入的数据量，单位：半字，！！！要求2字节对齐！！！
 * @return            实际写入的数据量，单位：字节
 */
uint32_t FLASH_Write(uint32_t Address, const uint16_t *Buffer, uint32_t Size)
{   
	/*初始化FLASH_EraseInitTypeDef*/
    FLASH_EraseInitTypeDef pEraseInit;
    /*设置PageError*/
    uint32_t PageError = 0;
	
	/* 非法地址 */
	if (Address < STM32FLASH_BASE || (Address > STM32FLASH_END) || Size == 0 || Buffer == NULL)
		return false;
	
	/*1、解锁FLASH*/
    HAL_FLASH_Unlock();
    /*2、擦除FLASH*/
	pEraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
    pEraseInit.PageAddress = Address;
    pEraseInit.NbPages = 1U;
	/*清除flash标志位*/
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR);
    HAL_FLASHEx_Erase(&pEraseInit, &PageError);
    /*3、对FLASH烧写*/
    uint16_t TempBuf = 0;
    for(uint32_t i = 0; i < Size; i++)
    {
        TempBuf = *(Buffer + i);
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD , Address + i * 2, TempBuf);
		/*写完数据之后，再读出来校验*/
		if ((*(__IO uint16_t*) (Address + i * 2) != TempBuf))
		{
			return true;
		}
		// Usart1_Printf("addr is  %d\r\n", Address);	
    }
    /*4、锁住FLASH*/
    HAL_FLASH_Lock();
	
	return PageError;
}

