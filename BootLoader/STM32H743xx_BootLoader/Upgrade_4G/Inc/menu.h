
/**
 ******************************************************************************
 * @file    IAP_Main/Inc/menu.h
 * @author  MCD Application Team
 * @brief   This file provides all the headers of the menu functions.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MENU_H_
#define __MENU_H_

/* Includes ------------------------------------------------------------------*/
//#include "flash_if.h"
#include "ymodem.h"
//#include "main.h"

#define BOOTLOADER_VERSION "1.1.0"
#define FLASH_INTERNAL_IMAGE_NAME "Uploaded_GreenhouseController.bin"

#define BOOT_SHOW_INFO 1

/*display text*/
//#define BOOT_TEXT_INFO 0
//#define MENU_TEXT_INFO 1
#define SCREEN_UART huart4
#define DOWNLOAD_ADDR 0x1093
//#define SUBMENU_TEXT_INFO 2

/* Imported variables --------------------------------------------------------*/
extern uint8_t aFileName[FILE_NAME_LENGTH];

/* Private variables ---------------------------------------------------------*/
typedef void (*pFunction)(void);

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void Main_Menu(void);
extern uint16_t Get_Crc16(uint8_t *ptr, uint16_t length, uint16_t init_dat);

#endif /* __MENU_H */
