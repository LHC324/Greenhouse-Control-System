#ifndef _IO_SIGNAL_H_
#define _IO_SIGNAL_H_

#ifdef __cplusplus
extern "C"
{
#endif

/*定义外部数字量输入路数*/
#define EXTERN_DIGITALIN_MAX 8U
/*定义外部模拟量输入路数*/
#define EXTERN_ANALOGIN_MAX 8U
/*定义外部数字量输出路数*/
#define EXTERN_DIGITALOUT_MAX 8U
/*定义外部模拟量输入路数*/
#define EXTERN_ANALOGOUT_MAX 8U
/*输入数字信号量在内存中初始地址*/
#define INPUT_DIGITAL_START_ADDR 0x00
/*输出数字信号量在内存中初始地址*/
#define OUT_DIGITAL_START_ADDR 0x00
/*输入模拟信号量在内存中初始地址*/
#define INPUT_ANALOG_START_ADDR 0x00
/*输出模拟信号量在内存中初始地址*/
#define OUT_ANALOG_START_ADDR 0x00

    extern void Read_Digital_Io(void);
    extern void Read_Analog_Io(void);
    extern void Write_Digital_IO(void);
    extern void Write_Analog_IO(void);

#ifdef __cplusplus
}
#endif

#endif /* __IO_SIGNAL_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
