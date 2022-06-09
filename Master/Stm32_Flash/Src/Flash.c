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
 * @用户flash区域：0-7页(256KB/扇区)
 * @系统参数区： 7扇区
 * @校准系数存放区域：None
 */
/*===================================================================================*/

/*
 *  初始化FLASH
 */
void MX_FLASH_Init(void)
{
	HAL_FLASH_Unlock();
	/*Enable disconnection in flash programming error*/
	__HAL_FLASH_ENABLE_IT_BANK1(FLASH_IT_ALL_BANK1);
	__HAL_FLASH_ENABLE_IT_BANK2(FLASH_IT_ALL_BANK2);
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
						   FLASH_FLAG_PGSERR | FLASH_FLAG_WRPERR);
	HAL_FLASH_Lock();
}

/**
 * 读FLASH
 * @param  Address 地址
 * @note   为了兼容各种数据类型，按字节读取
 * @param  pBuf  存放读取的数据
 * @param  Size    要读取的数据大小，单位字节
 * @return         读出成功的字节数
 */
bool FLASH_Read(uint32_t Address, void *pBuf, uint32_t Size)
{
	// uint8_t *pdata = (uint8_t *)pBuf;

	// /* 非法地址 */
	// if (Address < STM32FLASH_BASE || (Address > STM32FLASH_END) || Size == 0 || pBuf == NULL)
	// 	return false;

	// for (uint32_t i = 0; i < Size; i++)
	// {
	// 	pdata[i] = *(__IO uint16_t *)Address++;
	// 	// Address += 1U;
	// }

	for (uint8_t *pdata = (uint8_t *)pBuf; pdata < (uint8_t *)pBuf + Size; pdata++)
	{
		*pdata = *(__IO uint32_t *)Address++;
	}
	return true;
}

/**
 * @brief  Returns the Flash sector Number of the address
 * @param  None
 * @retval The Flash sector Number of the address
 */
static uint32_t FLASH_If_GetSectorNumber(uint32_t Address)
{
	uint32_t sector = 0;

	if ((Address < ADDR_FLASH_SECTOR_1_BANK1) && (Address >= ADDR_FLASH_SECTOR_0_BANK1))
	{
		sector = FLASH_SECTOR_0;
	}
	else if ((Address < ADDR_FLASH_SECTOR_2_BANK1) && (Address >= ADDR_FLASH_SECTOR_1_BANK1))
	{
		sector = FLASH_SECTOR_1;
	}
	else if ((Address < ADDR_FLASH_SECTOR_3_BANK1) && (Address >= ADDR_FLASH_SECTOR_2_BANK1))
	{
		sector = FLASH_SECTOR_2;
	}
	else if ((Address < ADDR_FLASH_SECTOR_4_BANK1) && (Address >= ADDR_FLASH_SECTOR_3_BANK1))
	{
		sector = FLASH_SECTOR_3;
	}
	else if ((Address < ADDR_FLASH_SECTOR_5_BANK1) && (Address >= ADDR_FLASH_SECTOR_4_BANK1))
	{
		sector = FLASH_SECTOR_4;
	}
	else if ((Address < ADDR_FLASH_SECTOR_6_BANK1) && (Address >= ADDR_FLASH_SECTOR_5_BANK1))
	{
		sector = FLASH_SECTOR_5;
	}
	else if ((Address < ADDR_FLASH_SECTOR_7_BANK1) && (Address >= ADDR_FLASH_SECTOR_6_BANK1))
	{
		sector = FLASH_SECTOR_6;
	}
	else if ((Address < ADDR_FLASH_SECTOR_0_BANK2) && (Address >= ADDR_FLASH_SECTOR_7_BANK1))
	{
		sector = FLASH_SECTOR_7;
	}
	else if ((Address < ADDR_FLASH_SECTOR_1_BANK2) && (Address >= ADDR_FLASH_SECTOR_0_BANK2))
	{
		sector = FLASH_SECTOR_0;
	}
	else if ((Address < ADDR_FLASH_SECTOR_2_BANK2) && (Address >= ADDR_FLASH_SECTOR_1_BANK2))
	{
		sector = FLASH_SECTOR_1;
	}
	else if ((Address < ADDR_FLASH_SECTOR_3_BANK2) && (Address >= ADDR_FLASH_SECTOR_2_BANK2))
	{
		sector = FLASH_SECTOR_2;
	}
	else if ((Address < ADDR_FLASH_SECTOR_4_BANK2) && (Address >= ADDR_FLASH_SECTOR_3_BANK2))
	{
		sector = FLASH_SECTOR_3;
	}
	else if ((Address < ADDR_FLASH_SECTOR_5_BANK2) && (Address >= ADDR_FLASH_SECTOR_4_BANK2))
	{
		sector = FLASH_SECTOR_4;
	}
	else if ((Address < ADDR_FLASH_SECTOR_6_BANK2) && (Address >= ADDR_FLASH_SECTOR_5_BANK2))
	{
		sector = FLASH_SECTOR_5;
	}
	else if ((Address < ADDR_FLASH_SECTOR_7_BANK2) && (Address >= ADDR_FLASH_SECTOR_6_BANK2))
	{
		sector = FLASH_SECTOR_6;
	}
	else /*if((Address < USER_FLASH_END_ADDRESS) && (Address >= ADDR_FLASH_SECTOR_7_BANK2))*/
	{
		sector = FLASH_SECTOR_7;
	}

	return sector;
}

/**
 * 写FLASH
 * @param  Address    写入起始地址，！！！要求32字节对齐！！！
 * @param  pBuf     待写入的数据，！！！要求32字节对齐！！！
 * @param  Size 要写入的数据量，单位：半字，！！！要求32字节对齐！！！
 * @return            实际写入的数据量，单位：字节
 */
uint32_t FLASH_Write(uint32_t Address, const uint32_t *pBuf, uint32_t Size)
{
#define FLASH_WRITE_SIZE 32U
	/*初始化FLASH_EraseInitTypeDef*/
	FLASH_EraseInitTypeDef pEraseInit;
	/* Get the sector where start the user flash area */
	uint32_t FirstSector = FLASH_If_GetSectorNumber(Address);
	/*设置SectorError*/
	uint32_t SectorError = 0;
	/*按32字节对齐*/
	// uint32_t counts = (Size + FLASH_WRITE_SIZE - 1U) / (FLASH_WRITE_SIZE / sizeof(uint32_t));
	// uint32_t actual_size = Size % FLASH_WRITE_SIZE ? ((Size / FLASH_WRITE_SIZE) * FLASH_WRITE_SIZE + FLASH_WRITE_SIZE): Size;
	// uint32_t actual_size = Size % FLASH_WRITE_SIZE ? (Size + FLASH_WRITE_SIZE) : Size;

	/* 非法地址 */
	if ((Address < STM32FLASH_BASE) || (Address > STM32FLASH_END) ||
		(Size == 0) || (pBuf == NULL) || (Address % FLASH_WRITE_SIZE))
		return false;

	/*关闭中断*/
	// __set_FAULTMASK(1);
	__disable_irq();
	/*1、解锁FLASH*/
	HAL_FLASH_Unlock();
	/*2、擦除FLASH*/
	pEraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
	pEraseInit.Sector = FirstSector;
	// pEraseInit.NbSectors = FLASH_If_GetSectorNumber(USER_FLASH_LAST_PAGE_ADDRESS) - FirstSector + 1; // 8 - FirstSector;
	pEraseInit.NbSectors = 1;
	pEraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;
	/*清除flash标志位*/
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
						   FLASH_FLAG_PGSERR | FLASH_FLAG_WRPERR);

	if (Address < ADDR_FLASH_SECTOR_0_BANK2)
	{
		pEraseInit.Banks = FLASH_BANK_1;
		if (HAL_FLASHEx_Erase(&pEraseInit, &SectorError) != HAL_OK)
		{
			/* Error occurred while sector erase */
			// SectorError = 0x3AA3;
			goto __exit;
			// return false;
		}

		/* Mass erase of second bank */
		pEraseInit.TypeErase = FLASH_TYPEERASE_MASSERASE;
		pEraseInit.Banks = FLASH_BANK_2;
		if (HAL_FLASHEx_Erase(&pEraseInit, &SectorError) != HAL_OK)
		{
			/* Error occurred while sector erase */
			// return false;
			// SectorError = 0xA33A;
			goto __exit;
		}
	}
	else
	{
		pEraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
		pEraseInit.Banks = FLASH_BANK_2;
		if (HAL_FLASHEx_Erase(&pEraseInit, &SectorError) != HAL_OK)
		{
			/* Error occurred while sector erase */
			// SectorError = 0x4AA4;
			goto __exit;
			// return false;
		}
	}

	/*3、对FLASH烧写*/
	/*size是按字节，写入是按32字*/
	for (uint32_t *p = (uint32_t *)pBuf, addr = Address; addr < Address + Size; p += 8U, addr += FLASH_WRITE_SIZE)
	{
		/* Device voltage range supposed to be [2.7V to 3.6V], the operation will
		be done by word */
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, addr, (uint32_t)p) == HAL_OK)
		{
			/* Check the written value */
			if (*(__IO uint32_t *)(addr) != *p)
			{
				/* Flash content doesn't match SRAM content */
				SectorError = 0x5AA5;
				break;
			}
		}
		else
		{
			/* Error occurred while writing data in Flash memory */
			SectorError = 0xA55A;
			break;
		}
	}
__exit:
	/*Check for ECC errors and clear the flash error caused by ECC verification*/
	// if ((!__HAL_FLASH_GET_FLAG(FLASH_SR_SNECCERR)) && (SectorError <= FLASH_SECTOR_7))
	// {
	// 	FLASH_Erase_Sector(SectorError, pEraseInit.Banks, pEraseInit.VoltageRange);

	// 	/* Wait for last operation to be completed */
	// 	FLASH_WaitForLastOperation(50000, FLASH_BANK_2);

	// 	/* If the erase operation is completed, disable the SER Bit */
	// 	FLASH->CR2 &= (~(FLASH_CR_SER | FLASH_CR_SNB));
	// 	/* Mass erase to be done */
	// 	// FLASH_MassErase(pEraseInit.VoltageRange, SectorError);

	// 	__HAL_FLASH_CLEAR_FLAG(FLASH_SR_SNECCERR);
	// }
	/*4、锁住FLASH*/
	HAL_FLASH_Lock();
	/*打开中断*/
	// __set_FAULTMASK(0);
	__enable_irq();

	return SectorError;
}
