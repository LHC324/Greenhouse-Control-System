/**
 ******************************************************************************
 * @file    IAP_Main/Src/menu.c
 * @author  MCD Application Team
 * @brief   This file provides the software which contains the main menu routine.
 *          The main menu gives the options of:
 *             - downloading a new binary file,
 *             - uploading internal flash memory,
 *             - executing the binary file already loaded
 *             - configuring the write protection of the Flash sectors where the
 *               user loads his binary file.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */

/** @addtogroup STM32F1xx_IAP
 * @{
 */

/* Includes ------------------------------------------------------------------*/
//#include "main.h"
#include "common.h"
#include "flash_if.h"
#include "menu.h"
//#include "ymodem.h"
#include "usart.h"
#include "string.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define DOWNLOAD_PAGE 18
#define UPLOAD_PAGE 19
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
extern uint32_t gUpdate_flag;
extern uint32_t gJump_ApplicationAddr;
extern void Dwin_Write(uint16_t addr, uint16_t *pdata);
pFunction JumpToApplication;
uint32_t JumpAddress;
uint32_t FlashProtection = 0;
uint8_t aFileName[FILE_NAME_LENGTH];

/* Private function prototypes -----------------------------------------------*/
void SerialDownload(void);
void SerialUpload(void);

typedef enum
{
  BOOT_TEXT_INFO = 0, /*boot title*/
  MENU_TEXT_INFO,     /*Main menu interface*/
  SUBMENU1_TEXT_INFO, /*Submenu interface*/
  SUBMENU2_TEXT_INFO, /*Submenu interface*/
  ENOTE1_TEXT_INFO,   /*Note1 information*/
  ENOTE2_TEXT_INFO,   /*Note2 information*/
  ENOTE3_TEXT_INFO,   /*Note3 information*/
  ENOTE4_TEXT_INFO,   /*Note4 information*/
  ENOTE5_TEXT_INFO,   /*Note5 information*/
  ENOTE6_TEXT_INFO,   /*Note6 information*/
  ENOTE7_TEXT_INFO,   /*Note7 information*/
  ENOTE8_TEXT_INFO,   /*Note8 information*/
  ENOTE9_TEXT_INFO,   /*Note9 information*/
  ENOTE10_TEXT_INFO,  /*Note10 information*/
  SUCCESS1_TEXT_INFO, /*Success1 information*/
  SUCCESS2_TEXT_INFO, /*Success2 information*/
  WARNING1_TEXT_INFO, /*Warning1 information*/
  ERROR1_TEXT_INFO,   /*Error1 information*/
  ERROR2_TEXT_INFO,   /*Error2 information*/
  ERROR3_TEXT_INFO,   /*Error3 information*/
  ERROR4_TEXT_INFO,   /*Error4 information*/
  ERROR5_TEXT_INFO,   /*Error5 information*/
  ERROR6_TEXT_INFO,   /*Error6 information*/
} Text_Info;

/***
 *        ____              __  __                    __
 *       / __ )____  ____  / /_/ /   ____  ____ _____/ /__  _____
 *      / __  / __ $/ __ $/ __/ /   / __ $/ __ `/ __  / _ $/ ___/
 *     / /_/ / /_/ / /_/ / /_/ /___/ /_/ / /_/ / /_/ /  __/ /
 *    /_____/@____/@____/@__/_____/@____/@__,_/@__,_/@___/_/
 *
 */
static const char *BootLoaderText[] =
    {
#if BOOT_SHOW_INFO == 1
        [BOOT_TEXT_INFO] =
            "\r\n"
            "    ____              __  __                    __\r\n"
            "   / __ )____  ____  / /_/ /   ____  ____ _____/ /__  _____\r\n"
            "  / __  / __ $/ __ $/ __/ /   / __ $/ __ `/ __  / _ $/ ___/\r\n"
            " / /_/ / /_/ / /_/ / /_/ /___/ /_/ / /_/ / /_/ /  __/ /\r\n"
            "/_____/@____/@____/@__/_____/@____/@__,_/@__,_/@___/_/\r\n"
            "\r\n"
            "Build:       "__DATE__
            " "__TIME__
            "\r\n"
            "Version:     " BOOTLOADER_VERSION "\r\n"
            "Copyright:   (c) 2022 BootLoader LHC@Yunnan Zhaofu Technology Co., Ltd\r\n",
#endif
        [MENU_TEXT_INFO] =
            "#=================== Main Menu ===================#\r\n"
            "# 1 @Download image to the internal Flash.        #\r\n"
            "# 2 @Upload image from the internal Flash.        #\r\n"
            "# 3 @Execute the loaded application.              #\r\n",
        [SUBMENU1_TEXT_INFO] =
            "# 4 @Disable the write protection.                #\r\n",
        [SUBMENU2_TEXT_INFO] =
            "# 4 @Enable the write protection.                 #\r\n"
            "#=================================================#\r\n",
        /*Note class*/
        [ENOTE1_TEXT_INFO] =
            "@note 01:Start program execution......\r\n",
        [ENOTE2_TEXT_INFO] =
            "@note 02:Write Protection disabled. \r\n",
        [ENOTE3_TEXT_INFO] =
            "@note 03:Write Protection enabled. \r\n",
        [ENOTE4_TEXT_INFO] =
            "@note 04:System will now restart......\r\n",
        [ENOTE5_TEXT_INFO] =
            "@note 05:Invalid Number ! ==> The number should be either 1, 2, 3 or 4. \r\n\r\n",
        [ENOTE6_TEXT_INFO] =
            "@note 06:Waiting for the file to be sent ... (press 'a' to abort). \n\r",
        [ENOTE7_TEXT_INFO] =
            "@note 07:Plese Select Receive File. \n\r",
        [ENOTE8_TEXT_INFO] =
            "@note 08:Start restarting the system .......\n\r",
        [ENOTE9_TEXT_INFO] =
            "@note 09:Terminal response timeout .......\n\r",
        [ENOTE10_TEXT_INFO] =
            "@note 09:Waiting for terminal response .......\n\r",
        /*Success class*/
        [SUCCESS1_TEXT_INFO] =
            "@Success 01:Programming Completed ! \n\r",
        [SUCCESS2_TEXT_INFO] =
            "@Success 02:File uploaded successfully ! \n\r",
        /*Warning class*/
        [WARNING1_TEXT_INFO] =
            "@Warning 01:Aborted by user. \n\r",
        /*Error class*/
        [ERROR1_TEXT_INFO] =
            "@error 01:Flash write un-protection failed ! \r\n",
        [ERROR2_TEXT_INFO] =
            "@error 02:Flash write protection failed ! \r\n",
        [ERROR3_TEXT_INFO] =
            "@error 03:The image size is higher than the allowed space memory ! \r\n",
        [ERROR4_TEXT_INFO] =
            "@error 04:Verification failed ! \n\r",
        [ERROR5_TEXT_INFO] =
            "@error 05:Failed to receive the file ! \n\r",
        [ERROR6_TEXT_INFO] =
            "@error 06:Error Occurred while Transmitting File ! \n\r",
};

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  迪文屏幕指定页面切换
 * @param  pd 迪文屏幕对象句柄
 * @param  page 目标页面
 * @retval None
 */
static void Dwin_PageChange(uint16_t page)
{
  uint8_t data[12U] = {
      0x5A, 0xA5, 0x07 + 2U, 0x82, 0x00, 0x84, 0x5A, 0x01,
      page >> 8U, page};

  uint16_t crc16 = Get_Crc16(&data[3], sizeof(data) - sizeof(crc16) - 3U, 0xFFFF);
  memcpy(&data[10U], &crc16, sizeof(crc16));
  HAL_UART_Transmit(&SCREEN_UART, data, sizeof(data), NAK_TIMEOUT);
}
/**
 * @brief  Download a file via serial port
 * @param  None
 * @retval None
 */
void SerialDownload(void)
{
  uint8_t number[11] = {0};
  uint32_t size = 0;
  COM_StatusTypeDef result;

  /*Switch to the specified interface*/
  Dwin_PageChange(DOWNLOAD_PAGE);
  /*Waiting for the file to be sent*/
  Serial_PutString((uint8_t *)BootLoaderText[ENOTE6_TEXT_INFO]);
  result = Ymodem_Receive(&size);
  switch (result)
  {
    uint16_t dewin_addr;
  case COM_OK:
  {
    dewin_addr = 102;
    Dwin_Write(DOWNLOAD_ADDR, &dewin_addr);
  }
  break;
  default:
  {
    dewin_addr = 101;
    Dwin_Write(DOWNLOAD_ADDR, &dewin_addr);
  }
  break;
  }
  if (result == COM_OK)
  {
    HAL_Delay(200);
    /*Programming Completed*/
    Serial_PutString((uint8_t *)BootLoaderText[SUCCESS1_TEXT_INFO]);
    Serial_PutString((uint8_t *)"\n\r--------------------------------\r\n Name: ");
    Serial_PutString(aFileName);
    Int2Str(number, size);
    Serial_PutString((uint8_t *)"\r\n Size: ");
    Serial_PutString(number);
    Serial_PutString((uint8_t *)" Bytes\r\n");
    Serial_PutString((uint8_t *)"--------------------------------\r\n");

    // if (((gUpdate_flag & 0xFFFF0000) >> 16U) == UPDATE_APP1)
    // {
    //   gUpdate_flag = (((uint32_t)UPDATE_APP2 << 16U) | 0xFFFF);
    // }
    // else
    // {
    //   gUpdate_flag = (((uint32_t)UPDATE_APP1 << 16U) | 0xFFFF);
    // }
    /*Clear upgrade flag*/
    gUpdate_flag = (gUpdate_flag & 0xFFFF0000) | 0xFFFF;
    /* erase user application area */
    FLASH_If_Erase(UPDATE_SAVE_ADDRESS, FLASH_SECTOR_SIZE);
    /*Upgrade success flag will be written in flash*/
    FLASH_If_Write(UPDATE_SAVE_ADDRESS, &gUpdate_flag, 1U);
    /*Restart the system*/
    Serial_PutString((uint8_t *)BootLoaderText[ENOTE8_TEXT_INFO]);
    HAL_Delay(500);
#define RESTORE_UART
    {
      HAL_UART_MspDeInit(&UartHandle);
      HAL_UART_MspDeInit(&SCREEN_UART);
      /*Close interrupt*/
      __set_FAULTMASK(1);
      NVIC_SystemReset();
    }
  }
  else if (result == COM_LIMIT)
  {
    /*The image size is higher than the allowed space memory*/
    Serial_PutString((uint8_t *)BootLoaderText[ERROR3_TEXT_INFO]);
  }
  else if (result == COM_DATA)
  {
    /*Verification failed*/
    Serial_PutString((uint8_t *)BootLoaderText[ERROR3_TEXT_INFO]);
  }
  else if (result == COM_ABORT)
  {
    /*Aborted by user*/
    Serial_PutString((uint8_t *)BootLoaderText[WARNING1_TEXT_INFO]);
  }
  else
  {
    /*Failed to receive the file*/
    Serial_PutString((uint8_t *)BootLoaderText[ERROR5_TEXT_INFO]);
  }
}

/**
 * @brief  Upload a file via serial port.
 * @param  None
 * @retval None
 */
void SerialUpload(void)
{
  uint8_t status = 0;
  uint16_t dewin_addr;

  /*Switch to the specified interface*/
  Dwin_PageChange(UPLOAD_PAGE);
  /*Select Receive File*/
  Serial_PutString((uint8_t *)BootLoaderText[ENOTE7_TEXT_INFO]);

  HAL_UART_Receive(&UartHandle, &status, 1, Get_Ms(RX_TIMEOUT));
  if (status == CRC16)
  {
    /* Transmit the flash image through ymodem protocol */
    status = Ymodem_Transmit((uint8_t *)gJump_ApplicationAddr, (const uint8_t *)FLASH_INTERNAL_IMAGE_NAME, USER_FLASH_SIZE);

    if (status != 0)
    {
      /*Error Occurred while Transmitting File*/
      Serial_PutString((uint8_t *)BootLoaderText[ERROR6_TEXT_INFO]);
      dewin_addr = 103;
      Dwin_Write(DOWNLOAD_ADDR, &dewin_addr);
    }
    else
    {
      /*File uploaded successfully*/
      Serial_PutString((uint8_t *)BootLoaderText[SUCCESS2_TEXT_INFO]);
      dewin_addr = 104;
      Dwin_Write(DOWNLOAD_ADDR, &dewin_addr);
    }
  }
}

/**
 * @brief  Display the Main Menu on HyperTerminal
 * @param  None
 * @retval None
 */
void Main_Menu(void)
{
#define INPUT_COUNTS 3U
  uint8_t key = 0;
  /*Input count*/
  uint8_t input_count = 0;

#if BOOT_SHOW_INFO == 1
  Serial_PutString((uint8_t *)BootLoaderText[BOOT_TEXT_INFO]);
#endif
  /* Test if any sector of Flash memory where user application will be loaded is write protected */
  FlashProtection = FLASH_If_GetWriteProtectionStatus();

  while (1)
  {
    /*The serial port must be reinitialized,
    otherwise the terminal cannot respond after it is stuck.*/
    MX_USART2_UART_Init();
    /*Hardware watchdog feed dog*/
    // HAL_GPIO_TogglePin(WDI_GPIO_Port, WDI_Pin);
    /*Display menu*/
    Serial_PutString((uint8_t *)BootLoaderText[MENU_TEXT_INFO]);

    if (FlashProtection != FLASHIF_PROTECTION_NONE)
    {
      /*Disable the write protection*/
      Serial_PutString((uint8_t *)BootLoaderText[SUBMENU1_TEXT_INFO]);
    }
    else
    {
      /*Enable the write protection*/
      Serial_PutString((uint8_t *)BootLoaderText[SUBMENU2_TEXT_INFO]);
    }
    /* Clean the input path */
    // __HAL_UART_FLUSH_DRREGISTER(&UartHandle);
    /* Receive key */
    if ((HAL_UART_Receive(&UartHandle, &key, 1, Get_Ms(EXIT_TIMEOUT)) == HAL_TIMEOUT) ||
        (input_count >= INPUT_COUNTS))
    {
      /*Command waiting timeout, clear the upgrade flag and restart automatically*/
      Serial_PutString((uint8_t *)BootLoaderText[ENOTE9_TEXT_INFO]);
      input_count = 0;
      key = '3';
    }
    // else
    // {
    //   Serial_PutString((uint8_t *)BootLoaderText[ENOTE10_TEXT_INFO]);
    //   HAL_Delay(1000);
    // }
    /* Clean the input path */
    // __HAL_UART_FLUSH_DRREGISTER(&UartHandle);

    switch (key)
    {
    case '1':
      /*Enter the correct clear count value*/
      input_count = 0;
      /* Download user application in the Flash */
      SerialDownload();
      break;
    case '2':
      /*Enter the correct clear count value*/
      input_count = 0;
      /*Close interrupt*/
      /* Upload user application from the Flash */
      SerialUpload();
      break;
    case '3':
      /*Enter the correct clear count value*/
      input_count = 0;
      /*Upgrade failed. Clear the upgrade flag and jump manually*/
      // gUpdate_flag = ((gUpdate_flag & 0xFFFF0000) | 0xFFFF);
      //	  gUpdate_flag = (*(__IO uint32_t *)UPDATE_SAVE_ADDRESS);
      if (((gUpdate_flag & 0xFFFF0000) >> 16U) == UPDATE_APP1)
      {
        gUpdate_flag = (((uint32_t)UPDATE_APP2 << 16U) | 0xFFFF);
      }
      else
      {
        gUpdate_flag = (((uint32_t)UPDATE_APP1 << 16U) | 0xFFFF);
      }
      /* erase user application area */
      FLASH_If_Erase(UPDATE_SAVE_ADDRESS, FLASH_SECTOR_SIZE);
      /*Upgrade success flag will be written in flash*/
      FLASH_If_Write(UPDATE_SAVE_ADDRESS, &gUpdate_flag, 1U);
      /*The system starts to restart*/
      Serial_PutString((uint8_t *)BootLoaderText[ENOTE8_TEXT_INFO]);
#define RESTORE_UART
      {
        HAL_UART_MspDeInit(&UartHandle);
        HAL_UART_MspDeInit(&SCREEN_UART);
        /*Close interrupt*/
        __set_FAULTMASK(1);
        NVIC_SystemReset();
      }

      // /*Start program execution*/
      // Serial_PutString((uint8_t *)BootLoaderText[ENOTE1_TEXT_INFO]);
      // /* execute the new program */
      // JumpAddress = *(__IO uint32_t *)(gJump_ApplicationAddr + 4);
      // /* Jump to user application */
      // JumpToApplication = (pFunction)JumpAddress;
      // /* Initialize user application's Stack Pointer */
      // __set_MSP(*(__IO uint32_t *)gJump_ApplicationAddr);
      // /*Close interrupt*/
      // __set_FAULTMASK(1);
      // JumpToApplication();
      break;
    case '4':
      /*Enter the correct clear count value*/
      input_count = 0;
      if (FlashProtection != FLASHIF_PROTECTION_NONE)
      {
        /* Disable the write protection */
        if (FLASH_If_WriteProtectionConfig(FLASHIF_WRP_DISABLE) == FLASHIF_OK)
        {
          /*Write Protection disabled*/
          Serial_PutString((uint8_t *)BootLoaderText[ENOTE2_TEXT_INFO]);
          /*System will now restart*/
          Serial_PutString((uint8_t *)BootLoaderText[ENOTE4_TEXT_INFO]);
          /* Launch the option byte loading */
          HAL_FLASH_OB_Launch();
        }
        else
        {
          /*Flash write un-protection failed*/
          Serial_PutString((uint8_t *)BootLoaderText[ERROR1_TEXT_INFO]);
        }
      }
      else
      {
        if (FLASH_If_WriteProtectionConfig(FLASHIF_WRP_ENABLE) == FLASHIF_OK)
        {
          /*Write Protection enabled*/
          Serial_PutString((uint8_t *)BootLoaderText[ENOTE2_TEXT_INFO]);
          /*System will now restart*/
          Serial_PutString((uint8_t *)BootLoaderText[ENOTE3_TEXT_INFO]);
          /* Launch the option byte loading */
          HAL_FLASH_OB_Launch();
        }
        else
        {
          /*Error: Flash write protection failed*/
          Serial_PutString((uint8_t *)BootLoaderText[ERROR1_TEXT_INFO]);
        }
      }
      break;
    default:
      input_count++;
      /*Write Protection disabled*/
      Serial_PutString((uint8_t *)BootLoaderText[ENOTE5_TEXT_INFO]);
      break;
    }
  }
}

/**
 * @}
 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
