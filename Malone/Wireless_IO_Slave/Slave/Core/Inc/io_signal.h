#ifndef _IO_SIGNAL_H_
#define _IO_SIGNAL_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "stdbool.h"

/*定义外部数字量输入路数*/
#define EXTERN_DIGITAL_MAX 2U
/*定义外部模拟量输入路数*/
#define EXTERN_ANALOG_MAX 2U
/*数字信号量输入在内存中初始地址*/
#define DIGITAL_INPUT_START_ADDR 0x00
/*数字信号量输出在内存中初始地址*/
#define DIGITAL_OUTPUT_START_ADDR 0x00
/*模拟信号量在内存中初始地址*/
#define ANALOG_START_ADDR 0x00


extern void Io_Digital_Input(void);
extern void Io_Analog_Handle(void);
#if defined(USING_SLAVE)
extern void Io_Digital_Output(bool signal);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __IO_SIGNAL_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
