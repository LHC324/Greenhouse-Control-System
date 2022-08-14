#ifndef __MDCONFIG_H__
#define __MDCONFIG_H__

#define DEBUG                       (0)
/*是否使用Modbus库*/
#define USER_MODBUS_LIB             (1)
/*不严格遵从成帧机制，保留所有帧内字符间隔大于1.5个字符时间的帧*/
#define IGNORE_LOSS_FRAME           (0)
/*帧尾的CRC检查*/
#define CRC_CHECK                   (1) 
/*ModBus数据帧位数:1bit start + 8bit data + 1bit stop*/
#define DATA_BITS                   (10)


#define MODBUS_PDU_SIZE_MIN         (4)
#define MODBUS_PDU_SIZE_MAX         (253)

#define REGISTER_WIDTH              (16)

#define COIL_OFFSET                         (1)
#define INPUT_COIL_OFFSET                   (10001)
#define INPUT_REGISTER_OFFSET               (30001)
#define HOLD_REGISTER_OFFSET                (40001)
/*每个寄存器组最大寄存器个数(占用4*REGISTER_POOL_MAX_BUFFER <REGISTER_POOL_MAX_REGISTER_NUMBER)*/
#define REGISTER_POOL_MAX_BUFFER            (32)

#define REGISTER_POOL_MAX_REGISTER_NUMBER   (256)

#endif
