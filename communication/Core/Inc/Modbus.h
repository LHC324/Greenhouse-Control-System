/*
 * ModbusSlave.h
 *
 *  Created on: 2022年04月08日
 *      Author: LHC
 */

#ifndef INC_MODBUS_H_
#define INC_MODBUS_H_

#include "main.h"

#define SLAVE_ADDRESS 0x02
#define MOD_RX_BUF_SIZE 128U
#define MOD_TX_BUF_SIZE 128U
#define REG_POOL_SIZE 16U

#define COIL_OFFSET (1)
#define INPUT_COIL_OFFSET (10001)
#define INPUT_REGISTER_OFFSET (30001)
#define HOLD_REGISTER_OFFSET (40001)

#define USING_MASTER
#define USING_COIL
#define USING_INPUT_COIL
// #define USING_INPUT_REGISTER
// #define USING_HOLD_REGISTER

#if defined(USING_COIL) && !defined(USING_INPUT_COIL)
#error Input coil not defined!
#elif !defined(USING_COIL) && defined(USING_INPUT_COIL)
#error Coil not defined!
#elif defined(USING_INPUT_REGISTER) && !defined(USING_HOLD_REGISTER)
#error Holding register not defined!
#elif !defined(USING_INPUT_REGISTER) && defined(USING_HOLD_REGISTER)
#error The input register is not defined!
#endif

/*错误码状态*/
#define RSP_OK 0			  // 成功
#define RSP_ERR_CMD 0x01	  // 不支持的功能码
#define RSP_ERR_REG_ADDR 0x02 // 寄存器地址错误
#define RSP_ERR_VALUE 0x03	  // 数据值域错误
#define RSP_ERR_WRITE 0x04	  // 写入失败

typedef enum
{
	Coil = 0x00,
	InputCoil,
	InputRegister,
	HoldRegister,
} Regsiter_Type __attribute__((aligned(1)));

typedef enum
{
	ReadCoil = 0x01,
	ReadInputCoil = 0x02,
	ReadHoldReg = 0x03,
	ReadInputReg = 0x04,
	WriteCoil = 0x05,
	WriteHoldReg = 0x06,
	WriteCoils = 0x0F,
	WriteHoldRegs = 0x10,
	ReportSeverId = 0x11,

} Function_Code __attribute__((aligned(1)));

typedef enum
{
	Read = 0x00,
	Write,
} Regsiter_Operate __attribute__((aligned(1)));

enum Using_Crc
{
	UsedCrc,
	NotUsedCrc
};

typedef struct Modbus_HandleTypeDef *pModbusHandle;
typedef struct Modbus_HandleTypeDef MdbusHandle;
typedef void (*pfunc)(pModbusHandle, uint8_t *);
typedef struct
{
	uint32_t addr;
	float upper;
	float lower;
	pfunc event;
} ModbusMap;

typedef struct
{
#if defined(USING_COIL)
	uint8_t Coils[REG_POOL_SIZE];
#endif
#if defined(USING_INPUT_COIL)
	uint8_t InputCoils[REG_POOL_SIZE];
#endif
#if defined(USING_INPUT_REGISTER)
	uint16_t InputRegister[REG_POOL_SIZE * 2U];
#endif
#if defined(USING_HOLD_REGISTER)
	uint16_t HoldRegister[REG_POOL_SIZE * 2U];
#endif
} ModbusPools;

typedef struct
{
	Regsiter_Type type;
	void *registers;
} Pool;

struct Modbus_HandleTypeDef
{
	uint8_t Slave_Id;
	void (*Mod_TI_Recive)(pModbusHandle, DMA_HandleTypeDef *);
	void (*Mod_Poll)(pModbusHandle);
	void (*Mod_Transmit)(pModbusHandle, enum Using_Crc);
#if defined(USING_MASTER)
	void (*Mod_Code46H)(pModbusHandle, uint16_t, uint8_t *, uint8_t);
#endif
	bool (*Mod_Operatex)(pModbusHandle, uint16_t, uint8_t *, uint8_t);
#if defined(USING_COIL) || defined(USING_INPUT_COIL)
	void (*Mod_ReadXCoil)(pModbusHandle);
	void (*Mod_WriteCoil)(pModbusHandle);
#endif
#if defined(USING_INPUT_REGISTER) || defined(USING_HOLD_REGISTER)
	void (*Mod_ReadXRegister)(pModbusHandle);
	void (*Mod_WriteHoldRegister)(pModbusHandle);
#endif
	void (*Mod_ReportSeverId)(pModbusHandle);
	void (*Mod_Error)(pModbusHandle, uint8_t, uint8_t);
	struct
	{
		uint8_t *pTbuf;
		uint8_t TxSize;
		uint8_t TxCount;
	} Master;
	struct
	{
		uint8_t *pRbuf;
		uint8_t RxSize;
		uint8_t RxCount;
		/*预留外部数据结构接口*/
		// void *pHandle;
		// ModbusMap *pMap;
		Regsiter_Type Reg_Type;
		ModbusPools *pPools;
		Regsiter_Operate Operate;
		// Function_Code Fun_Code;
		// Pool *pools;
		// uint16_t Events_Size;
#if !defined(USING_FREERTOS)
		bool Recive_FinishFlag;
#endif
	} Slave;
	UART_HandleTypeDef *huart;
} __attribute__((aligned(4)));

#define s sizeof(ModbusPools)

/*读线圈*/
#define SMod_ReadCoil(__obj, __addr, __data) \
	((__addr) > (COIL_OFFSET + REG_POOL_SIZE) ? false : (__data) = (__obj)->Slave.pPools->Coils[(__addr)-COIL_OFFSET], true)
/*写线圈*/
#define SMod_WriteCoil(__obj, __addr, __data) \
	((__addr) > (COIL_OFFSET + REG_POOL_SIZE) ? false : (__obj)->Slave.pPools->Coils[(__addr)-COIL_OFFSET] = (__data), true)
/*读线圈组*/
#define SMod_ReadCoilS(__obj, __saddr, __size, __pdata)                                   \
	(((__saddr)-COIL_OFFSET + (__size)) > (REG_POOL_SIZE) ? false : do {                  \
		memcpy((__pdata), (__obj)->Slave.pPools->Coils[(__saddr)-COIL_OFFSET], (__size)); \
	} while (0))
/*写线圈组*/
#define SMod_WriteCoils(__obj, __saddr, __size, __pdata)                                  \
	(((__saddr)-COIL_OFFSET + (__size)) > (REG_POOL_SIZE) ? false : do {                  \
		memcpy((__obj)->Slave.pPools->Coils[(__saddr)-COIL_OFFSET], (__pdata), (__size)); \
	} while (0),                                                                          \
	 true)

extern void MX_ModbusInit(void);
extern pModbusHandle Modbus_Object;
extern void Modbus_Handle(void);

#define Modbus_ReciveHandle(__obj, __dma) ((__obj)->Mod_TI_Recive((__obj), (__dma)))

#endif /* INC_MODBUS_H_ */
