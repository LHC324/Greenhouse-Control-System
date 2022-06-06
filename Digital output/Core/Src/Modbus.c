/*
 * ModbusSlave.c
 *
 *  Created on: 2022年04月08日
 *      Author: LHC
 */

#include "Modbus.h"
#include "usart.h"
#include "tool.h"

/*定义Modbus对象*/
pModbusHandle Modbus_Object;
ModbusPools Spool;
#if !defined(USING_FREERTOS)
// uint8_t pTxbuf[MOD_TX_BUF_SIZE];
// uint8_t pRxbuf[MOD_TX_BUF_SIZE];
#endif

/*静态函数声明*/
static void Modbus_TI_Recive(pModbusHandle pd, DMA_HandleTypeDef *hdma);
static void Modbus_Poll(pModbusHandle pd);
static void Modbus_Send(pModbusHandle pd, enum Using_Crc crc);
#if defined(USING_MASTER)
static void Modbus_46H(pModbusHandle pd, uint16_t regaddr, uint8_t *pdata, uint8_t datalen);
#endif
static bool Modbus_Operatex(pModbusHandle pd, uint16_t addr, uint8_t *pdata, uint8_t len);
#if defined(USING_COIL) || defined(USING_INPUT_COIL)
static void Modus_ReadXCoil(pModbusHandle pd);
static void Modus_WriteCoil(pModbusHandle pd);
#endif
#if defined(USING_INPUT_REGISTER) || defined(USING_HOLD_REGISTER)
static void Modus_ReadXRegister(pModbusHandle pd);
static void Modus_WriteHoldRegister(pModbusHandle pd);
#endif
static void Modus_ReportSeverId(pModbusHandle pd);

void Modbus_Handle(void)
{
    Modbus_Object->Mod_Poll(Modbus_Object);
}

/**
 * @brief  创建Modbus协议站对象
 * @param  pd 需要初始化对象指针
 * @param  ps 初始化数据指针
 * @retval None
 */
static void Create_ModObject(pModbusHandle *pd, pModbusHandle ps)
{
    if (!ps)
        return;
    (*pd) = (pModbusHandle)CUSTOM_MALLOC(sizeof(MdbusHandle));
    if (!(*pd))
        CUSTOM_FREE(*pd);
    // #if defined(USING_FREERTOS)
    uint8_t *pTxbuf = (uint8_t *)CUSTOM_MALLOC(ps->Master.TxSize);
    if (!pTxbuf)
    {
#if defined(USING_DEBUG)
        Debug("pTxbuf Creation failed!\r\n");
#endif
        CUSTOM_FREE(pTxbuf);
        return;
    }
    uint8_t *pRxbuf = (uint8_t *)CUSTOM_MALLOC(ps->Slave.RxSize);
    if (!pRxbuf)
    {
#if defined(USING_DEBUG)
        Debug("pRxbuf Creation failed!\r\n");
#endif
        CUSTOM_FREE(pRxbuf);
        return;
    }

    // #endif

    memset(pTxbuf, 0x00, ps->Master.TxSize);
    memset(pRxbuf, 0x00, ps->Slave.RxSize);
#if defined(USING_DEBUG)
#if defined(USING_FREERTOS)
    shellPrint(Shell_Object, "Dwin[%d]_handler = 0x%p\r\n", ps->Id, *pd);
#else
    Debug("Modbus[%d]_handler = 0x%p\r\n", ps->Slave_Id, *pd);
#endif
#endif
    (*pd)->Slave_Id = ps->Slave_Id;
    (*pd)->Mod_TI_Recive = Modbus_TI_Recive;
    (*pd)->Mod_Poll = Modbus_Poll;
    (*pd)->Mod_Transmit = Modbus_Send;
#if defined(USING_MASTER)
    (*pd)->Mod_Code46H = Modbus_46H;
#endif
    (*pd)->Mod_Operatex = Modbus_Operatex;
#if defined(USING_COIL) || defined(USING_INPUT_COIL)
    (*pd)->Mod_ReadXCoil = Modus_ReadXCoil;
    (*pd)->Mod_WriteCoil = Modus_WriteCoil;
#endif
#if defined(USING_INPUT_REGISTER) || defined(USING_HOLD_REGISTER)
    (*pd)->Mod_ReadXRegister = Modus_ReadXRegister;
    (*pd)->Mod_WriteHoldRegister = Modus_WriteHoldRegister;
#endif
    (*pd)->Mod_ReportSeverId = Modus_ReportSeverId;
    (*pd)->Master.pTbuf = pTxbuf;
    (*pd)->Master.TxCount = 0U;
    (*pd)->Master.TxSize = ps->Master.TxSize;
    (*pd)->Slave.pRbuf = pRxbuf;
    (*pd)->Slave.RxSize = ps->Slave.RxSize;
    (*pd)->Slave.RxCount = 0U;
    (*pd)->Slave.pPools = ps->Slave.pPools;
    // (*pd)->Slave.pMap = ps->Slave.pMap;
    // (*pd)->Slave.Events_Size = ps->Slave.Events_Size;
    (*pd)->Slave.pHandle = ps->Slave.pHandle;
#if !defined(USING_FREERTOS)
    (*pd)->Slave.Recive_FinishFlag = false;
#endif
    (*pd)->huart = ps->huart;
}

/**
 * @brief  销毁modbus对象
 * @param  pd 需要初始化对象指针
 * @retval None
 */
void Free_ModObject(pModbusHandle *pd)
{
    if (*pd)
    {
        CUSTOM_FREE((*pd)->Master.pTbuf);
        CUSTOM_FREE((*pd)->Slave.pRbuf);
        CUSTOM_FREE((*pd));
    }
}

/**
 * @brief  初始化Modbus协议库
 * @retval None
 */
void MX_ModbusInit(void)
{
#define SLAVE_MAX 0x0F
    /*读板标志*/
    static bool Read_Flag = false;
    // extern Save_HandleTypeDef Save_Flash;
    MdbusHandle Modbus;
    uint8_t slave_id = Get_CardId() & 0x0F;

    // Modbus.Slave_Id = SLAVE_ADDRESS;
#if defined(USING_DEBUG)
    Debug("Board id is 0x%02x, slave id is 0x%02x.\r\n", Get_CardId(), slave_id);
#endif

    /*从站ID通过卡槽编码确定*/
    Modbus.Slave_Id = (slave_id <= SLAVE_MAX) ? slave_id : SLAVE_MAX + 1U;
    // Modbus.Slave_Id = 0x00;
    Modbus.Master.TxSize = MOD_TX_BUF_SIZE;
    Modbus.Slave.RxSize = MOD_RX_BUF_SIZE;
    // Modbus.Slave.pMap = Modbus_ObjMap;
    // Modbus.Slave.Events_Size = Modbus_EventSize;
    Modbus.Slave.pHandle = &Read_Flag;
    /*定义迪文屏幕使用目标串口*/
    Modbus.huart = &huart1;
    // memset(&Spool, 0x00, sizeof(ModbusPools));
    /*寄存器池*/
    Modbus.Slave.pPools = &Spool;
/*使用屏幕接收处理时*/
#define TYPEDEF_STRUCT bool
    Create_ModObject(&Modbus_Object, &Modbus);
}

/**
 * @brief  Modbus在接收中中断接收数据
 * @param  pd 迪文屏幕对象句柄
 * @retval None
 */
static void Modbus_TI_Recive(pModbusHandle pd, DMA_HandleTypeDef *hdma)
{
    /*Gets the idle flag so that the idle flag is set*/
    if ((__HAL_UART_GET_FLAG(pd->huart, UART_FLAG_IDLE) != RESET))
    {
        /*Clear idle interrupt flag*/
        __HAL_UART_CLEAR_IDLEFLAG(pd->huart);
        if (pd && (pd->Slave.pRbuf))
        {
            /*Stop DMA transmission to prevent busy receiving data and interference during data processing*/
            HAL_UART_DMAStop(pd->huart);
            /*Get the number of untransmitted data in DMA*/
            /*Number received = buffersize - the number of data units remaining in the current DMA channel transmission */
            pd->Slave.RxCount = pd->Slave.RxSize - __HAL_DMA_GET_COUNTER(hdma);
            /*Reopen DMA reception*/
            HAL_UART_Receive_DMA(pd->huart, pd->Slave.pRbuf, pd->Slave.RxSize);
        }
#if defined(USING_FREERTOS)
        /*After opening the serial port interrupt, the semaphore has not been created*/
        if (Recive_Uart1Handle != NULL)
        {
            /*Notification task processing*/
            osSemaphoreRelease(Recive_Uart1Handle);
        }
#else
        pd->Slave.Recive_FinishFlag = true;
#endif
    }
}

#define MOD_WORD 1U
#define MOD_DWORD 2U
/*获取主机号*/
#define Get_ModId(__obj) ((__obj)->Slave.pRbuf[0U])
/*获取Modbus功能号*/
#define Get_ModFunCode(__obj) ((__obj)->Slave.pRbuf[1U])
/*获取Modbus协议数据*/
#define Get_Data(__ptr, __s, __size) \
    ((__size) < 2U ? (((__ptr)->Slave.pRbuf[__s] << 8U) | ((__ptr)->Slave.pRbuf[__s + 1U])) : (((__ptr)->Slave.pRbuf[__s] << 24U) | ((__ptr)->Slave.pRbuf[__s + 1U] << 16U) | ((__ptr)->Slave.pRbuf[__s + 2U] << 8U) | ((__ptr)->Slave.pRbuf[__s + 3U])))

/**
 * @brief  Modbus接收数据解析
 * @param  pd 迪文屏幕对象句柄
 * @retval None
 */
static void Modbus_Poll(pModbusHandle pd)
{
    uint16_t crc16 = 0x0000;
#if !defined(USING_FREERTOS)
    if (pd->Slave.Recive_FinishFlag)
    {
        pd->Slave.Recive_FinishFlag = false;
#endif /*首次调度时RXcount值被清零，导致计算crc时地址越界*/
        if (pd->Slave.RxCount > 2U)
            crc16 = Get_Crc16(pd->Slave.pRbuf, pd->Slave.RxCount - 2U, 0xffff);
#if defined(USING_DEBUG)
        Debug("rxcount = %d,crc16 = 0x%X.\r\n", pd->Slave.RxCount, (uint16_t)((crc16 >> 8U) | (crc16 << 8U)));
#endif
        /*检查是否是目标从站*/
        if ((Get_ModId(pd) == pd->Slave_Id) && (Get_Data(pd, pd->Slave.RxCount - 2U, MOD_WORD) == ((uint16_t)((crc16 >> 8U) | (crc16 << 8U)))))
        {
#if defined(USING_DEBUG)
            Debug("Data received!\r\n");
            for (uint8_t i = 0; i < pd->Slave.RxCount; i++)
            {
                Debug("prbuf[%d] = 0x%X\r\n", i, pd->Slave.pRbuf[i]);
            }
#endif
            switch (Get_ModFunCode(pd))
            {
#if defined(USING_COIL) || defined(USING_INPUT_COIL)
            case ReadCoil:
            case ReadInputCoil:
            {
                pd->Mod_ReadXCoil(pd);
            }
            break;
            case WriteCoil:
            case WriteCoils:
            {
                pd->Mod_WriteCoil(pd);
            }
            break;
#endif
#if defined(USING_INPUT_REGISTER) || defined(USING_HOLD_REGISTER)
            case ReadHoldReg:
            case ReadInputReg:
            {
                pd->Mod_ReadXRegister(pd);
            }
            break;
            case WriteHoldReg:
            case WriteHoldRegs:
            {
                pd->Mod_WriteHoldRegister(pd);
            }
			break;
#endif
            case ReportSeverId:
            {
                pd->Mod_ReportSeverId(pd);
                /**/
                if (pd->Slave.pHandle)
                {
                    *(TYPEDEF_STRUCT *)pd->Slave.pHandle = true;
                }
            }
            break;
            default:
                break;
            }
        }
        memset(pd->Slave.pRbuf, 0x00, pd->Slave.RxCount);
        pd->Slave.RxCount = 0U;
#if !defined(USING_FREERTOS)
    }
#endif
    /*检查帧头是否符合要求*/
    //     if ((pd->Slave.pRbuf[0] == 0x5A) && (pd->Slave.pRbuf[1] == 0xA5))
    //     {
    //         uint16_t addr = Get_Data(pd, 4U, MOD_DWORD);
    // #if defined(USING_DEBUG)
    //         // shellPrint(Shell_Object, "addr = 0x%x\r\n", addr);
    // #endif
    //         for (uint8_t i = 0; i < pd->Slave.Events_Size; i++)
    //         {
    //             if (pd->Slave.pMap[i].addr == addr)
    //             {
    //                 if (pd->Slave.pMap[i].event)
    //                     pd->Slave.pMap[i].event(pd, &i);
    //                 break;
    //             }
    //         }
    //     }
    // #if defined(USING_DEBUG)
    //     for (uint16_t i = 0; i < pd->Slave.RxCount; i++)
    //     // shellPrint(Shell_Object, "pRbuf[%d] = 0x%x\r\n", i, pd->Slave.pRbuf[i]);
    // #endif
    // memset(pd->Slave.pRbuf, 0x00, pd->Slave.RxCount);
    // pd->Slave.RxCount = 0U;
}

/*获取寄存器类型*/
#define Get_RegType(__obj, __type) \
    ((__type) < InputRegister ? (__obj)->Slave.pPools->Coils : (__obj)->Slave.pPools->InputRegister)

/*获取寄存器地址*/
#if defined(USING_COIL) && defined(USING_INPUT_COIL) && defined(USING_INPUT_REGISTER) && defined(USING_HOLD_REGISTER)
#define Get_RegAddr(__obj, __type, __addr) \
    ((__type) == Coil ? (uint8_t *)&(__obj)->Slave.pPools->Coils[__addr] : ((__type) == InputCoil ? (uint8_t *)&(__obj)->Slave.pPools->InputCoils[__addr] : ((__type) == InputRegister ? (uint8_t *)&(__obj)->Slave.pPools->InputRegister[__addr] : (uint8_t *)&(__obj)->Slave.pPools->HoldRegister[__addr])))
#elif defined(USING_COIL) || defined(USING_INPUT_COIL)
#define Get_RegAddr(__obj, __type, __addr) \
    ((__type) == Coil ? (uint8_t *)&(__obj)->Slave.pPools->Coils[__addr] : (uint8_t *)&(__obj)->Slave.pPools->InputCoils[__addr])
#elif defined(USING_INPUT_REGISTER) || defined(USING_HOLD_REGISTER)
#define Get_RegAddr(__obj, __type, __addr) \
    ((__type) == InputRegister ? (uint8_t *)&(__obj)->Slave.pPools->InputRegister[__addr] : (uint8_t *)&(__obj)->Slave.pPools->HoldRegister[__addr])
#else
#define Get_RegAddr(__obj, __type, __addr) (__obj, __type, __addr)
#endif

/**
 * @brief  Modbus协议读取/写入寄存器
 * @param  pd 需要初始化对象指针
 * @param  regaddr 寄存器地址[寄存器起始地址从1开始]
 * @param  pdat 数据指针
 * @param  len  读取数据长度
 * @retval None
 */
static bool Modbus_Operatex(pModbusHandle pd, uint16_t addr, uint8_t *pdata, uint8_t len)
{
    // uint16_t offset = pd->Slave.Reg_Type > Coil ? (pd->Slave.Reg_Type + 10000U) : 1U;
    uint8_t max = pd->Slave.Reg_Type < InputRegister ? REG_POOL_SIZE : REG_POOL_SIZE * 2U;
    // uint8_t reg_addr = addr - 1U, *pDest, *pSou;
    uint8_t *pDest, *pSou;
    // typeof(Get_RegType(pd, pd->Slave.Reg_Type)) *paddr;
    bool ret = false;
#if defined(USING_DEBUG)
    // if (addr < 1U)
    // {
    //     Debug("Error: Register address must be > = 1.\r\n");
    // }
#endif
    // if (reg_addr < max)
    if ((addr < max) && (len < max))
    {
#if defined(USING_COIL) || defined(USING_INPUT_COIL) || defined(USING_INPUT_REGISTER) || defined(USING_HOLD_REGISTER)
        if (pd->Slave.Operate == Read)
        {
            pDest = pdata, pSou = Get_RegAddr(pd, pd->Slave.Reg_Type, addr);
            // pd->Slave.pPools->Coils[0] = 0x000;
            // pDest = (uint8_t *)Get_RegType(pd, pd->Slave.Reg_Type);
        }
        else
        {
            pDest = Get_RegAddr(pd, pd->Slave.Reg_Type, addr), pSou = pdata;
        }
#endif
#if defined(USING_DEBUG)
        // Debug("pdest[%p] = 0x%X, psou[%p]= 0x%X, len= %d.\r\n", pDest, *pDest, pSou, *pSou, len);
#endif
        if (memcpy(pDest, pSou, len))
            ret = true;
    }
    return ret;
}

/**
 * @brief  Modbus协议主站有人云拓展46指令
 * @param  pd 需要初始化对象指针
 * @param  regaddr 寄存器地址
 * @param  pdata 数据指针
 * @param  datalen 数据长度
 * @retval None
 */
#if defined(USING_MASTER)
static void Modbus_46H(pModbusHandle pd, uint16_t regaddr, uint8_t *pdata, uint8_t datalen)
{
#define MASTER_FUNCTION_CODE 0x46
    uint8_t buf[] = {pd->Slave_Id, MASTER_FUNCTION_CODE, regaddr >> 8U, regaddr,
                     (datalen / 2U) >> 8U, (datalen / 2U), datalen};

    memset(pd->Master.pTbuf, 0x00, pd->Master.TxSize);
    pd->Master.TxCount = 0U;
    memcpy(pd->Master.pTbuf, buf, sizeof(buf));
    pd->Master.TxCount += sizeof(buf);
    memcpy(&pd->Master.pTbuf[pd->Master.TxCount], pdata, datalen);
    pd->Master.TxCount += datalen;

    pd->Mod_Transmit(pd, UsedCrc);
}
#endif

/**
 * @brief  Modbus协议读取线圈和输入线圈状态(0x01\0x02)
 * @param  pd 需要初始化对象指针
 * @retval None
 */
#if defined(USING_COIL) || defined(USING_INPUT_COIL)
static void Modus_ReadXCoil(pModbusHandle pd)
{
#define Byte_To_Bits 8U
    uint8_t len = Get_Data(pd, 4U, MOD_WORD);
    // uint8_t bytes = len % Byte_To_Bits > 0 ? len / Byte_To_Bits + 1U : len / Byte_To_Bits;
    uint8_t bytes = len / Byte_To_Bits + !!(len % Byte_To_Bits);
    // uint8_t bytes = (len + Byte_To_Bits - 1U) / Byte_To_Bits;
    uint8_t *prbits = (uint8_t *)CUSTOM_MALLOC(len);
    // uint8_t bits = 0x00;
    if (!prbits)
        goto __exit;

    memset(prbits, 0x00, len);
    memset(pd->Master.pTbuf, 0x00, pd->Master.TxSize);
    pd->Master.TxCount = 0U;
    memcpy(pd->Master.pTbuf, pd->Slave.pRbuf, 2U);
    pd->Master.TxCount += 2U;
    pd->Master.pTbuf[pd->Master.TxCount++] = bytes;
    /*通过功能码寻址寄存器*/
    pd->Slave.Reg_Type = Get_ModFunCode(pd) == ReadCoil ? Coil : InputCoil;
    pd->Slave.Operate = Read;
    pd->Mod_Operatex(pd, Get_Data(pd, 2U, MOD_WORD), prbits, len);
#if defined(USING_DEBUG)
    for (uint8_t i = 0; i < len; i++)
        Debug("prbits[%d] = 0x%X, len= %d.\r\n", i, prbits[i], len);
#endif
    for (uint8_t i = 0; i < bytes; i++)
    {
        for (uint8_t j = 0; j < Byte_To_Bits && (i * Byte_To_Bits + j) < len; j++)
        {
            uint8_t bit = (prbits[i * Byte_To_Bits + j] & 0x01);
            if (bit)
                pd->Master.pTbuf[pd->Master.TxCount] |= (bit << j);
            else
                pd->Master.pTbuf[pd->Master.TxCount] &= ~(bit << j);
#if defined(USING_DEBUG)
            Debug("pTbuf[%d] = 0x%X, j= %d.\r\n", i, pd->Master.pTbuf[pd->Master.TxCount], j);
#endif
        }
        pd->Master.TxCount++;
    }
#if defined(USING_DEBUG)
    Debug("pd->Master.TxCount = %d.\r\n", pd->Master.TxCount);
#endif
    pd->Mod_Transmit(pd, UsedCrc);
__exit:
    CUSTOM_FREE(prbits);
}

/**
 * @brief  Modbus协议写线圈/线圈组(0x05\0x0F)
 * @param  pd 需要初始化对象指针
 * @retval None
 */
static void Modus_WriteCoil(pModbusHandle pd)
{
    uint8_t *pdata = NULL, len = 0x00;
    enum Using_Crc crc;

    /*通过功能码寻址寄存器*/
    pd->Slave.Reg_Type = Coil;
    pd->Slave.Operate = Write;
    /*写单个线圈*/
    if (Get_ModFunCode(pd) == WriteCoil)
    {
        uint8_t wbit = !!(Get_Data(pd, 4U, MOD_WORD) == 0xFF00);
        len = 1U;
        pdata = &wbit;
        pd->Master.TxCount = pd->Slave.RxCount;
        crc = NotUsedCrc;
    }
    /*写多个线圈*/
    else
    {
        len = Get_Data(pd, 4U, MOD_WORD);
        // pdata = (uint8_t *)CUSTOM_MALLOC(len);
        /*利用发送缓冲区空间暂存数据*/
        pdata = pd->Master.pTbuf;

        for (uint8_t i = 0; i < len; i++)
        {
            pdata[i] = (pd->Slave.pRbuf[7U + i / Byte_To_Bits] >> (i % Byte_To_Bits)) & 0x01;
        }
        pd->Master.TxCount = 6U;
        crc = UsedCrc;
    }
#if defined(USING_DEBUG)
    Debug("pdata = 0x%X, len= %d.\r\n", *pdata, len);
#endif
    if (pdata)
        pd->Mod_Operatex(pd, Get_Data(pd, 2U, MOD_WORD), pdata, len);
    memset(pd->Master.pTbuf, 0x00, pd->Master.TxSize);
    /*请求数据原路返回*/
    memcpy(pd->Master.pTbuf, pd->Slave.pRbuf, pd->Master.TxCount);
    pd->Mod_Transmit(pd, crc);
}
#endif

/**
 * @brief  Modbus协议读输入寄存器/保持寄存器(0x03\0x04)
 * @param  pd 需要初始化对象指针
 * @retval None
 */
#if defined(USING_INPUT_REGISTER) || defined(USING_HOLD_REGISTER)
static void Modus_ReadXRegister(pModbusHandle pd)
{
    uint8_t len = Get_Data(pd, 4U, MOD_WORD) * sizeof(uint16_t);
    uint8_t *prdata = (uint8_t *)CUSTOM_MALLOC(len);

    if (!prdata)
        goto __exit;
    memset(prdata, 0x00, len);
    memset(pd->Master.pTbuf, 0x00, pd->Master.TxSize);
    pd->Master.TxCount = 0U;
    memcpy(pd->Master.pTbuf, pd->Slave.pRbuf, 2U);
    pd->Master.TxCount += 2U;
    pd->Master.pTbuf[pd->Master.TxCount] = len;
    pd->Master.TxCount += sizeof(len);
    /*通过功能码寻址寄存器*/
    pd->Slave.Reg_Type = Get_ModFunCode(pd) == ReadHoldReg ? HoldRegister : InputRegister;
    pd->Slave.Operate = Read;
    pd->Mod_Operatex(pd, Get_Data(pd, 2U, MOD_WORD), prdata, len);
    memcpy(&pd->Master.pTbuf[pd->Master.TxCount], prdata, len);
    pd->Master.TxCount += len;

    pd->Mod_Transmit(pd, UsedCrc);
__exit:
    CUSTOM_FREE(prdata);
}

/**
 * @brief  Modbus协议写保持寄存器/多个保持寄存器(0x06/0x10)
 * @param  pd 需要初始化对象指针
 * @retval None
 */
static void Modus_WriteHoldRegister(pModbusHandle pd)
{
    uint8_t *pdata = NULL, len = 0x00;
    enum Using_Crc crc;

    pd->Slave.Reg_Type = HoldRegister;
    pd->Slave.Operate = Write;
    /*写单个保持寄存器*/
    if (Get_ModFunCode(pd) == WriteHoldReg)
    {
        // uint16_t data = Get_Data(pd, 4U, MOD_WORD);
        // pdata = (uint8_t *)&data;
        len = sizeof(uint16_t);
        /*改变数据指针*/
        pdata = &pd->Slave.pRbuf[4U];
        pd->Master.TxCount = pd->Slave.RxCount;
        crc = NotUsedCrc;
    }
    else
    {
        len = pd->Slave.pRbuf[6U];
        /*改变数据指针*/
        pdata = &pd->Slave.pRbuf[7U];
        pd->Master.TxCount = 6U;
        crc = UsedCrc;
    }
    if (pdata)
        pd->Mod_Operatex(pd, Get_Data(pd, 2U, MOD_WORD), pdata, len);
    memset(pd->Master.pTbuf, 0x00, pd->Master.TxSize);
    /*请求数据原路返回*/
    memcpy(pd->Master.pTbuf, pd->Slave.pRbuf, pd->Master.TxCount);
    pd->Mod_Transmit(pd, crc);
}
#endif

/**
 * @brief  Modbus协议上报一些特定信息
 * @param  pd 需要初始化对象指针
 * @retval None
 */
static void Modus_ReportSeverId(pModbusHandle pd)
{
    // extern TIM_HandleTypeDef htim1;
    // HAL_TIM_Base_Stop_IT(&htim1);
    memset(pd->Master.pTbuf, 0x00, pd->Master.TxSize);
    pd->Master.TxCount = 0U;
    memcpy(pd->Master.pTbuf, pd->Slave.pRbuf, 2U);
    pd->Master.TxCount += 2U;
    pd->Master.pTbuf[pd->Master.TxCount++] = sizeof(uint8_t);
    /*读取卡槽与板卡编码*/
    pd->Master.pTbuf[pd->Master.TxCount++] = Get_CardId();
    pd->Mod_Transmit(pd, UsedCrc);
    // HAL_TIM_Base_Start_IT(&htim1);
}

/**
 * @brief  Modbus协议发送
 * @param  pd 需要初始化对象指针
 * @retval None
 */
static void Modbus_Send(pModbusHandle pd, enum Using_Crc crc)
{
    if (crc == UsedCrc)
    {
        uint16_t crc16 = Get_Crc16(pd->Master.pTbuf, pd->Master.TxCount, 0xffff);

        memcpy(&pd->Master.pTbuf[pd->Master.TxCount], (uint8_t *)&crc16, sizeof(crc16));
        pd->Master.TxCount += sizeof(crc16);
    }

    HAL_UART_Transmit_DMA(pd->huart, pd->Master.pTbuf, pd->Master.TxCount);
    while (__HAL_UART_GET_FLAG(pd->huart, UART_FLAG_TC) == RESET)
    {
    }
}
