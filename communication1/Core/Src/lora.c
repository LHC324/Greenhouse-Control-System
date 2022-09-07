#include "lora.h"
#include "usart.h"
#include "shell_port.h"
#include "Modbus.h"
//#include "mdrtuslave.h"
//#include "io_signal.h"

/*定义lora模块对象*/
pLoraHandle Lora_Object;
/*Lora函数事件图*/
Lora_Map L_Map[SLAVE_MAX_NUMBER];

/*静态函数声明*/
static bool inline Get_Lora_Status(pLoraHandle pl);
static void Set_Lora_FactoryMode(void);
static void Lora_TI_Recive(pLoraHandle pl, DMA_HandleTypeDef *hdma);
static void Lora_Recive_Poll(pLoraHandle pl);
static void Lora_Tansmit_Poll(pLoraHandle pl);
static bool Lora_MakeFrame(pLoraHandle pl, Lora_Map *pm);
static void Lora_Send(pLoraHandle pl, uint8_t *pdata, uint16_t size);
static bool Lora_Check_InputCoilState(uint8_t *p_current, uint8_t *p_last, uint8_t size);
static void Lora_CallBack(pLoraHandle pl, void *pdata);

/**
 * @brief  创建Lora模块对象
 * @param  pl 需要初始化对象指针
 * @param  ps 初始化数据指针
 * @retval None
 */
static void Create_ModObject(pLoraHandle *pl, pLoraHandle ps)
{
    (*pl) = (pLoraHandle)CUSTOM_MALLOC(sizeof(Lora_Handle));
#if defined(USING_DEBUG)
    uint8_t ret = 0;
#endif
    if (*pl && ps)
    {
        (*pl)->MapSize = ps->MapSize;
        (*pl)->pMap = ps->pMap;
        (*pl)->Lora_Recive_Poll = Lora_Recive_Poll;
        (*pl)->Lora_Transmit_Poll = Lora_Tansmit_Poll;
        (*pl)->Get_Lora_Status = Get_Lora_Status;
        (*pl)->Set_Lora_Factory = Set_Lora_FactoryMode;
        (*pl)->Lora_TI_Recive = Lora_TI_Recive;
        (*pl)->Lora_MakeFrame = Lora_MakeFrame;
        (*pl)->Loara_Send = Lora_Send;
        (*pl)->Lora_Check_InputCoilState = Lora_Check_InputCoilState;
        (*pl)->Loara_CallBack = Lora_CallBack;
        (*pl)->Schedule.Ready = (List_t *)CUSTOM_MALLOC(sizeof(List_t));
        (*pl)->Schedule.Ready ? vListInitialise((*pl)->Schedule.Ready)
                              : CUSTOM_FREE((*pl)->Schedule.Ready);
        (*pl)->Schedule.Block = (List_t *)CUSTOM_MALLOC(sizeof(List_t));
        (*pl)->Schedule.Block ? vListInitialise((*pl)->Schedule.Block)
                              : CUSTOM_FREE((*pl)->Schedule.Block);
        (*pl)->Schedule.First_Flag = false;
        (*pl)->Schedule.Period = 0;
        (*pl)->Schedule.Event_Id = 0;
        // (*pl)->Schedule.Ready.pIter = (ListItem_t *)&(*pl)->Schedule.Ready.pList->xListEnd;
        // (*pl)->Schedule.Block.pIter = (ListItem_t *)&(*pl)->Schedule.Block.pList->xListEnd;
        (*pl)->Master.pTbuf = (uint8_t *)CUSTOM_MALLOC(ps->Master.TxSize);
        (*pl)->Master.pTbuf ? (void)memset((*pl)->Master.pTbuf, 0, ps->Master.TxSize) : CUSTOM_FREE((*pl)->Master.pTbuf);
        (*pl)->Slave.pRbuf = (uint8_t *)CUSTOM_MALLOC(ps->Slave.RxSize);
        (*pl)->Slave.pRbuf ? (void)memset((*pl)->Slave.pRbuf, 0, ps->Slave.RxSize) : CUSTOM_FREE((*pl)->Master.pTbuf);
        (*pl)->Master.TxSize = ps->Master.TxSize;
        (*pl)->Master.TxCount = 0;
        (*pl)->Slave.RxSize = ps->Slave.RxSize;
        (*pl)->Slave.RxCount = 0;
        (*pl)->Slave.bSemaphore = ps->Slave.bSemaphore;
        // (*pl)->Check = ps->Check;
        (*pl)->huart = ps->huart;
        (*pl)->Cs = ps->Cs;
        (*pl)->pHandle = ps->pHandle;
#if defined(USING_DEBUG)
        shellPrint(Shell_Object, "Lora_handler = 0x%p,ret = %d.\r\n", *pl, ret);
#endif
    }
}

/**
 * @brief  销毁Lora对象
 * @param  pl 需要初始化对象指针
 * @retval None
 */
void Free_LoraObject(pLoraHandle *pl)
{
    if (*pl)
    {
        CUSTOM_FREE((*pl)->Schedule.Ready);
        CUSTOM_FREE((*pl)->Schedule.Block);
        CUSTOM_FREE((*pl)->Master.pTbuf);
        CUSTOM_FREE((*pl)->Slave.pRbuf);
        CUSTOM_FREE(*pl);
    }
}

/**
 * @brief	Lora模块初始化
 * @details
 * @param	None
 * @retval	None
 */
void MX_Lora_Init(void)
{
    extern osSemaphoreId Lora_ReciveHandle;

    for (uint8_t i = 0; i < SLAVE_MAX_NUMBER; i++)
    {
#if defined(USING_REPEATER_MODE)
        L_Map[i].Frame_Head.Device_Addr.Val = 1U;
        L_Map[i].Frame_Head.Channel = 1U;
#else
        L_Map[i].Frame_Head.Device_Addr.Val = i + 1U;
        L_Map[i].Frame_Head.Channel = i + 1U;
#endif
        // L_Map[i].Slave_Id = i + 1U;

#if (USING_INDEPENDENT_STRUCT)
        L_Map[i].Check.State = L_None;
        L_Map[i].Check.Counter = 0;
        L_Map[i].Check.OverTimes = SUSPEND_TIMES;
        L_Map[i].Check.Schedule_counts = 0;
#else
        L_Map[i].Schedule_counts = 0;
#endif
    }
    Lora_Handle lora = {
        .MapSize = sizeof(L_Map) / sizeof(Lora_Map),
        .pMap = L_Map,
        .Schedule = {
            .Period = 0,
        },
        .Master = {
            .Id = 0x00,
            .pTbuf = NULL,
            .TxSize = LORA_TX_BUF_SIZE,
        },
        .Slave = {
            .bSemaphore = Lora_ReciveHandle,
            .pRbuf = NULL,
            .RxSize = LORA_RX_BUF_SIZE,
        },
#if (!USING_INDEPENDENT_STRUCT)
        .Check = {
            .State = L_None,
            .Counter = 0,
            .OverTimes = SUSPEND_TIMES,
        },
#endif
        .huart = &huart2,
        .Cs = {
            .pGPIOx = CS0_LORA1_GPIO_Port,
            .Gpio_Pin = CS0_LORA1_Pin,
        },
        .pHandle = Modbus_Object,
    };
    Create_ModObject(&Lora_Object, &lora);
}

/**
 * @brief	获取Lora模块当前是否空闲
 * @details	无线发送数据时拉低，用于指示发送繁忙状态
 * @param	None
 * @retval	ture/fale
 */
bool inline Get_Lora_Status(pLoraHandle pl)
{
    return ((bool)HAL_GPIO_ReadPin(LORA_STATUS_GPIO_Port, LORA_STATUS_Pin));
}

/**
 * @brief	Lora模块恢复出厂设置
 * @details	拉低 3s 以上恢复出厂设置
 * @param	None
 * @retval	None
 */
void Set_Lora_FactoryMode(void)
{
    HAL_GPIO_WritePin(LORA_RELOAD_GPIO_Port, LORA_RELOAD_Pin, GPIO_PIN_RESET);
#if defined(USING_RTTHREAD)
    rt_thread_mdelay(3500);
#else
    osDelay(3500);
#endif
    HAL_GPIO_WritePin(LORA_RELOAD_GPIO_Port, LORA_RELOAD_Pin, GPIO_PIN_SET);
#if defined(USING_DEBUG)
    shellPrint(Shell_Object, "@Loara: Factory reset was successful.\r\n");
#endif
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), Lora_factory,
                 Set_Lora_FactoryMode, config);

/**
 * @brief  Lora模块在接收中断中接收数据
 * @param  pl Lora对象句柄
 * @retval None
 */
void Lora_TI_Recive(pLoraHandle pl, DMA_HandleTypeDef *hdma)
{
    /*Gets the idle flag so that the idle flag is set*/
    if ((__HAL_UART_GET_FLAG(pl->huart, UART_FLAG_IDLE) != RESET))
    {
        /*Clear idle interrupt flag*/
        __HAL_UART_CLEAR_IDLEFLAG(pl->huart);
        if (pl && (pl->Slave.pRbuf))
        {
            /*Stop DMA transmission to prevent busy receiving data and interference during data processing*/
            HAL_UART_DMAStop(pl->huart);
            /*Get the number of untransmitted data in DMA*/
            /*Number received = buffersize - the number of data units remaining in the current DMA channel transmission */
            pl->Slave.RxCount = pl->Slave.RxSize - __HAL_DMA_GET_COUNTER(hdma);
            /*Reopen DMA reception*/
            HAL_UART_Receive_DMA(pl->huart, pl->Slave.pRbuf, pl->Slave.RxSize);
        }
#if defined(USING_FREERTOS)
        /*After opening the serial port interrupt, the semaphore has not been created*/
        if (pl->Slave.bSemaphore)
        {
            /*Notification task processing*/
            osSemaphoreRelease((osSemaphoreId)pl->Slave.bSemaphore);
        }
#endif
    }
}

/**
 * @brief	从列表中查找指定列表项
 * @details
 * @param	None
 * @retval	None
 */
ListItem_t *Find_ListItem(List_t *list, TickType_t data)
{
    ListItem_t *p = list->xListEnd.pxNext;

    for (; p->xItemValue != portMAX_DELAY; p = p->pxNext)
    {
        if (listGET_LIST_ITEM_VALUE(p) == data)
        {
            return p;
        }
    }
    return NULL;
}

/**
 * @brief	添加列表项到列表
 * @details
 * @param	None
 * @retval	None
 */
void Add_ListItem(List_t *list, TickType_t data)
{
    // ListItem_t *p = NULL;
    /*先从对应列表中查找到列表项，存在：则不添加*/
    ListItem_t *p = Find_ListItem(list, data);
    if (p)
        return;
    p = (ListItem_t *)CUSTOM_MALLOC(sizeof(ListItem_t));
    if (p == NULL)
    {
        CUSTOM_FREE(p);
        return;
    }
    vListInitialiseItem(p);
    listSET_LIST_ITEM_VALUE(p, data);
    /*按照data大小升序排列*/
    // vListInsert(list, p);
    vListInsertEnd(list, p);
}

/**
 * @brief	从列表中移除列表
 * @details
 * @param	None
 * @retval	列表中剩余的列表项
 */
UBaseType_t Remove_ListItem(List_t *list, TickType_t data)
{
    UBaseType_t items;

    if (!listCURRENT_LIST_LENGTH(list))
        return 0;
    /*先从对应列表中查找到列表项，在移除*/
    ListItem_t *p = Find_ListItem(list, data);
    if (p && (p->xItemValue != portMAX_DELAY))
    {
        items = uxListRemove(p);
        /*释放链表节点*/
        CUSTOM_FREE(p);
        return items;
    }
    return 0;
}

/**
 * @brief	查找目标设备（在线/离线）设备
 * @details
 * @param	None
 * @retval	None
 */
ListItem_t *Get_OneDevice(pLoraHandle pl, List_t *plist)
{
    List_t *const pxConstList = (plist);

    if (pl && plist)
    {
        if (!listCURRENT_LIST_LENGTH(plist))
            return NULL;

        /* Increment the index to the next item and return the item, ensuring */
        /* we don't return the marker used at the end of the list.  */
        (pxConstList)->pxIndex = (pxConstList)->pxIndex->pxNext;
        if ((void *)(pxConstList)->pxIndex == (void *)&((pxConstList)->xListEnd))
        {
            (pxConstList)->pxIndex = (pxConstList)->pxIndex->pxNext;
        }
        /*记录本次调度位置*/
        // listx->pIter = (pxConstList)->pxIndex;
#if defined(USING_DEBUG)
        shellPrint(Shell_Object, "@note:uxNumberOfItems = %d,data = %d.\r\n",
                   list->uxNumberOfItems, p->xItemValue);
#endif

        return (pxConstList)->pxIndex;
    }
    return NULL;
}

/**
 * @brief  Check whether the status of each switch changes
 * @param  p_current current state
 * @param  p_last Last status
 * @param  size  Detection length
 * @retval true/false
 */
bool Lora_Check_InputCoilState(uint8_t *p_current, uint8_t *p_last, uint8_t size)
{
    bool ret = false;
    if (p_current && p_last && size)
    {
        for (uint8_t i = 0; i < size; i++)
        {
            if (p_current[i] != p_last[i])
            {
                p_last[i] = p_current[i];
                ret = true;
            }
        }
    }
    return ret;
}

/**
 * @brief	Lora模块回调函数
 * @details
 * @param	None
 * @retval	None
 */
void Lora_CallBack(pLoraHandle pl, void *pdata)
{
    pModbusHandle pd = (pModbusHandle)pl->pHandle;
    bool *pflag = (bool *)pd->Slave.pHandle;
    if (pl && pdata)
    {
        static uint8_t rbits[DIGITAL_INPUT_NUMBERS];
        pd->Slave.Reg_Type = InputCoil;
        pd->Slave.Operate = Read;
        /*读取对应寄存器*/
        if (!pd->Mod_Operatex(pd, (pl->Schedule.Event_Id * DIGITAL_INPUT_NUMBERS), rbits, sizeof(rbits)))
        {
#if defined(USING_DEBUG)
            shellPrint(Shell_Object, "Coil reading failed!\r\n");
#endif
            return;
        }
        if (pl->Lora_Check_InputCoilState((uint8_t *)pdata, rbits, DIGITAL_INPUT_NUMBERS) && pflag)
        {
            *pflag = false;
        }
    }
}

/**
 * @brief	主站处理从站发来的数据
 * @details
 * @param	None
 * @retval	None
 */
void Lora_Recive_Poll(pLoraHandle pl)
{
    /*确认收到地址与目标从机地址相同*/
    if (pl->Slave.RxCount > 2U)
    {
        pModbusHandle pd = (pModbusHandle)pl->pHandle;
#if defined(USING_DEBUG)
        shellPrint(Shell_Object, "\r\nLora_Buf[%d]:", pl->Slave.RxCount);
        for (uint8_t i = 0; i < pl->Slave.RxCount; i++)
        {
            shellPrint(Shell_Object, "%02X ", pl->Slave.pRbuf[i]);
        }
        shellPrint(Shell_Object, "\r\n\r\n");
#endif
        uint16_t crc16 = Get_Crc16(pl->Slave.pRbuf, pl->Slave.RxCount - 2U, 0xffff);
#if defined(USING_DEBUG)
        shellPrint(Shell_Object, "la_rxcount = %d,crc16 = %#X.\r\n", pl->Slave.RxCount,
                   (uint16_t)((crc16 >> 8U) | (crc16 << 8U)));
#endif
        // if ((Get_LoraId(pl) == p->Slave_Id) && (Get_Data(pl, pl->Slave.RxCount - 2U, MOD_WORD) ==
        //                                         ((uint16_t)((crc16 >> 8U) | (crc16 << 8U)))))
        // uint8_t slave_id = pl->Schedule.Event_Id + 1U;
        // if ((Get_LoraId(pl) == slave_id) && (Get_Data(pl, pl->Slave.RxCount - 2U, MOD_WORD) ==
        //                                      ((uint16_t)((crc16 >> 8U) | (crc16 << 8U)))))
        uint8_t slave_id = Get_LoraId(pl), event_id = (slave_id - 1U);
        /*过滤离散输入自己请求数据*/
        // if (!pl->Slave.pRbuf[2] && (slave_id < B_TYPE_SLAVE_START_ADDR))
        //     return;
#if defined(USING_DEBUG)
        shellPrint(Shell_Object, "\r\n@note:current response event_id[%d].\r\n", event_id);
#endif
        if (Get_Data(pl, pl->Slave.RxCount - 2U, MOD_WORD) == ((uint16_t)((crc16 >> 8U) | (crc16 << 8U))))
        {
            if (event_id < pl->MapSize)
            {
                Add_ListItem(pl->Schedule.Ready, event_id);
                /*从阻塞列表中移除*/
                Remove_ListItem(pl->Schedule.Block, event_id);
                /*此处极限容纳32个从机状态:取离散输入的后32字节空间作为状态存储*/
                if (pd->Slave.pPools->InputCoils[LORA_STATE_OFFSET + event_id] != 0x01)
                {
                    *(bool *)pd->Slave.pHandle = false;
                    pd->Slave.pPools->InputCoils[LORA_STATE_OFFSET + event_id] = 0x01;
                }

#if (USING_INDEPENDENT_STRUCT)
                pl->pMap[event_id].Check.Schedule_counts = 0;
#else
                pl->pMap[event_id].Schedule_counts = 0;
#endif
            }
            Check_FirstFlag(pl);
/*对应从站正确响应*/
#if (USING_INDEPENDENT_STRUCT)
            pl->pMap[event_id].Check.State = L_OK;
#else
            pl->Check.State = L_OK;
#endif
            /*按分区写数据到目标寄存器：数字输入响应帧*/
            if ((slave_id < B_TYPE_SLAVE_START_ADDR))
            {
                /*数据分离*/
                uint8_t wbits[DIGITAL_INPUT_NUMBERS];
#if defined(USING_DEBUG)
                shellPrint(Shell_Object, "\r\nslave[%d]:\r\n", slave_id);
#endif
                for (uint8_t i = 0; i < DIGITAL_INPUT_NUMBERS; i++)
                {
                    wbits[i] = ((pl->Slave.pRbuf[3U] >> (i % 8U)) & 0x01);
#if defined(USING_DEBUG)
                    shellPrint(Shell_Object, "%d  ", wbits[i]);
#endif
                }
                if (pd)
                {
                    /*Lora回调函数：检测输入线圈状态是否产生变化*/
                    pl->Loara_CallBack(pl, wbits);
                    pd->Slave.Reg_Type = InputCoil;
                    pd->Slave.Operate = Write;
                    /*写入对应寄存器*/
                    // if (!pd->Mod_Operatex(pd, (p->Slave_Id - 1U) * DIGITAL_INPUT_NUMBERS, wbits,
                    //                       DIGITAL_INPUT_NUMBERS))
                    /*pl->Schedule.Event_Id在Check_FirstFlag(pl)后++*/
                    uint16_t target_addr = event_id * DIGITAL_INPUT_NUMBERS;
                    if (target_addr > (DIGITAL_INPUT_NUMBERS * A_TYPE_SLAVE_MAX_ADDR))
                    {
#if defined(USING_DEBUG)
                        shellPrint(Shell_Object, "@Error:Input register address[%d] out of range!\r\n", target_addr);
#endif
                        return;
                    }
#if defined(USING_DEBUG)
                    shellPrint(Shell_Object, "@note: addr = %d.\r\n", target_addr);
#endif
                    if (!pd->Mod_Operatex(pd, target_addr, wbits, DIGITAL_INPUT_NUMBERS))
                    {
#if defined(USING_DEBUG)
                        shellPrint(Shell_Object, "Input coil write failed!\r\n");
#endif
                    }
                }
            }
            /*数字输出响应帧*/
            else
            {
            }
            // break;
        }
        else
#if (USING_INDEPENDENT_STRUCT)
            pl->pMap[event_id].Check.State = L_Error;
#else
            pl->Check.State = L_Error;
#endif
    }
    // __exit:
    Clear_LoraBuffer(pl, Slave, R);
}

/**
 * @brief	位变量组帧
 * @details 主机设备号为0，信道为0
 * @note    |---目标节点地址（2B）---|---信道（1B）---|---从机地址---|---Data---|---CRC---|
 * @param	pl Lora对象句柄
 * @retval	None
 */
// #define USING_TEST
bool Lora_MakeFrame(pLoraHandle pl, Lora_Map *pm)
{
    pModbusHandle pd = (pModbusHandle)pl->pHandle;
    uint8_t slave_id = pl->Schedule.Event_Id + 1U;
#if !defined(USING_TRANSPARENT_MODE)
//#if defined(USING_REPEATER_MODE)
//    uint8_t buf[] = {pm->Frame_Head.Device_Addr.V.H, pm->Frame_Head.Device_Addr.V.L,
//                     pm->Frame_Head.Channel, slave_id | 0x80, 0x05, 0x00, 0x00,
//                     0x00, 0x00, 0x00, 0x00};
//#else
	uint8_t buf[] = {pm->Frame_Head.Device_Addr.V.H, pm->Frame_Head.Device_Addr.V.L,
                     pm->Frame_Head.Channel, slave_id, 0x05, 0x00, 0x00,
                     0x00, 0x00, 0x00, 0x00};				 
//#endif
#else
    uint8_t buf[] = {slave_id, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#endif
#if defined(USING_DEBUG)
    shellPrint(Shell_Object, "@Warning:Slave_Id = %d.\r\n", pm->Slave_Id);
#endif
    if (pd && pm)
    {
        if (slave_id < B_TYPE_SLAVE_START_ADDR)
        {
#if !defined(USING_TRANSPARENT_MODE)
            buf[4] = 0x02, buf[8] = 0x08;
#else
            buf[1] = 0x02, buf[5] = 0x08;
#endif
        }
        else
        {
            uint8_t data = 0;
            pd->Slave.Reg_Type = Coil;
            pd->Slave.Operate = Read;
#if !defined(USING_TEST)
            /*读取对应寄存器*/
            if (!pd->Mod_Operatex(pd, (slave_id - DIGITAL_OUTPUT_OFFSET), &data, sizeof(data)))
            {
#if defined(USING_DEBUG)
                shellPrint(Shell_Object, "Coil reading failed!\r\n");
#endif
                return false;
            }
            // buf[4] = 0x05, buf[6] = (pm->Slave_Id - DIGITAL_OUTPUT_OFFSET), buf[7] = data;
#else
            static uint8_t counts = 0;
            if (++counts >= 5)
            {
                counts = 0;
                data ^= 1;
            }
#endif
#if !defined(USING_TRANSPARENT_MODE)
            buf[4] = 0x05, buf[7] = data;
#else
            buf[1] = 0x05, buf[3] = data;
#endif
        }
        pl->Master.TxCount = sizeof(buf);
#if !defined(USING_TRANSPARENT_MODE)
        uint16_t crc = Get_Crc16(&buf[sizeof(pm->Frame_Head) - 1U], sizeof(buf) - 5U, 0xffff);
#else
        uint16_t crc = Get_Crc16(buf, sizeof(buf) - 2U, 0xffff);
#endif
#if defined(USING_REPEATER_MODE)
	 buf[3]	|= 0x80;
#endif
        /*拷贝两个字节CRC到临时缓冲区*/
        memcpy(&buf[sizeof(buf) - sizeof(uint16_t)], &crc, sizeof(uint16_t));
        memcpy(pl->Master.pTbuf, buf, pl->Master.TxCount);
        pl->Loara_Send(pl, pl->Master.pTbuf, pl->Master.TxCount);
    }

    return true;
}

/**
 * @brief	对Lora模块发送数据
 * @details
 * @param	None
 * @retval	None
 */
void Lora_Send(pLoraHandle pl, uint8_t *pdata, uint16_t size)
{
    if (!pl)
        return;
    HAL_UART_Transmit_DMA(pl->huart, pdata, size);
    while (__HAL_UART_GET_FLAG(pl->huart, UART_FLAG_TC) == RESET)
    {
    }
    Clear_LoraBuffer(pl, Master, T);
}

/**
 * @brief	主站轮询发送数据给从站
 * @details
 * @param	None
 * @retval	None
 */
void Lora_Tansmit_Poll(pLoraHandle pl)
{
    pModbusHandle pd = (pModbusHandle)pl->pHandle;
    if (!pl || !pl->Schedule.Ready || !pl->Schedule.Block || !pl->Cs.pGPIOx)
        return;

#if (USING_INDEPENDENT_STRUCT)
    switch (pl->pMap[pl->Schedule.Event_Id].Check.State)
#else
    switch (pl->Check.State)
#endif
    { /*首次发出事件或者从机响应成功*/
    case L_None:
    case L_OK:
    {
#if !defined(USING_ASHINING)
        /*确保L101模块当前处于空闲状态*/
        if (!pl->Get_Lora_Status(pl))
        {
            //            return;
            osDelay(1);
        }
#else
        while (!pl->Get_Lora_Status(pl))
        {
            osDelay(1);
        }
        osDelay(2);
#endif
        /*清除超时计数器*/

#if (USING_INDEPENDENT_STRUCT)
        pl->pMap[pl->Schedule.Event_Id].Check.Counter = 0;
#else
        pl->Check.Counter = 0;
#endif
        // pl->Schedule.Event_Id = pl->Schedule.First_Flag ? (uint8_t)Get_OnlineDevice(pl, pl->Schedule.Ready)
        //                                                 : pl->Schedule.Event_Id;
        if (pl->Schedule.First_Flag)
        {
            ListItem_t *p = Get_OneDevice(pl, pl->Schedule.Ready);
            pl->Schedule.Event_Id = p ? (uint8_t)listGET_LIST_ITEM_VALUE(p) : LORA_NULL_ID;
            pl->Schedule.Period++;
#if defined(USING_DEBUG)
            shellPrint(Shell_Object, "@Warning:Schedule_Event_Id = %d.\r\n", pl->Schedule.Event_Id);
#endif
        }
        /*禁止接收引脚*/
        HAL_GPIO_WritePin(pl->Cs.pGPIOx, pl->Cs.Gpio_Pin, GPIO_PIN_SET);
        /*去掉首次扫描完毕的第一个静默周期*/
        pl->Schedule.Event_Id < pl->MapSize
#if (USING_INDEPENDENT_STRUCT)
            ? pl->Lora_MakeFrame(pl, &pl->pMap[pl->Schedule.Event_Id]),
            pl->pMap[pl->Schedule.Event_Id].Check.State = L_Wait : false;
#else
            ? pl->Lora_MakeFrame(pl, &pl->pMap[pl->Schedule.Event_Id]),
            pl->Check.State = L_Wait : false;
#endif
        /*使能接收引脚*/
        HAL_GPIO_WritePin(pl->Cs.pGPIOx, pl->Cs.Gpio_Pin, GPIO_PIN_RESET);

        /*每调度一个周期就绪列表，就检测阻塞列表中一个从机*/
        if ((pl->Schedule.Period > listCURRENT_LIST_LENGTH(pl->Schedule.Ready)) ||
            (!listCURRENT_LIST_LENGTH(pl->Schedule.Ready) && pl->Schedule.First_Flag))
        {
            pl->Schedule.Period = 0;
            /*循环的从阻塞列表中移除一个列表项*/
            ListItem_t *p = Get_OneDevice(pl, pl->Schedule.Block);
            /*加入就绪列表*/
            Add_ListItem(pl->Schedule.Ready, listGET_LIST_ITEM_VALUE(p));
            /*从阻塞列表中移除*/
            Remove_ListItem(pl->Schedule.Block, listGET_LIST_ITEM_VALUE(p));
#if defined(USING_DEBUG)
            // shellPrint(Shell_Object, "p = 0x%p,\r\n", p);
            // shellPrint(Shell_Object, "\r\nschdule !\r\n");
            shellPrint(Shell_Object, "\r\n\r\n@note:BlocK_List[%d],current_idex[%d]:_________Start\r\n",
                       listCURRENT_LIST_LENGTH(pl->Schedule.Block), listGET_LIST_ITEM_VALUE(pl->Schedule.Block->pxIndex));
            for (p = (ListItem_t *)pl->Schedule.Block->xListEnd.pxNext; p->xItemValue != portMAX_DELAY;
                 p = p->pxNext)
            {
                shellPrint(Shell_Object, "%d ", listGET_LIST_ITEM_VALUE(p));
            }
            shellPrint(Shell_Object, "\r\n@note:Ready_List[%d],current_idex[%d]:\r\n",
                       listCURRENT_LIST_LENGTH(pl->Schedule.Ready), listGET_LIST_ITEM_VALUE(pl->Schedule.Ready->pxIndex));
            for (p = (ListItem_t *)pl->Schedule.Ready->xListEnd.pxNext; p->xItemValue != portMAX_DELAY;
                 p = p->pxNext)
            {
                shellPrint(Shell_Object, "%d ", listGET_LIST_ITEM_VALUE(p));
            }
            shellPrint(Shell_Object, "\r\nOS remaining heap = %dByte, Mini heap = %dByte_________End\r\n\r\n",
                       xPortGetFreeHeapSize(), xPortGetMinimumEverFreeHeapSize());
#endif
        }
    }
    break;
    case L_Wait:
    {
#if (USING_INDEPENDENT_STRUCT)
        /*接收超时，不是接收错误导致的超时*/
        if (++pl->pMap[pl->Schedule.Event_Id].Check.Counter >=
            pl->pMap[pl->Schedule.Event_Id].Check.OverTimes)
        {
            pl->pMap[pl->Schedule.Event_Id].Check.Counter = 0;
            pl->pMap[pl->Schedule.Event_Id].Check.State = L_TimeOut;
        }
#else
        /*接收超时，不是接收错误导致的超时*/
        if (++pl->Check.Counter >= pl->Check.OverTimes)
        {
            pl->Check.Counter = 0;
            pl->Check.State = L_TimeOut;
        }
#endif
    }
    break;
    case L_Error:
    case L_TimeOut:
    {
        uint8_t actual_site = LORA_STATE_OFFSET + pl->Schedule.Event_Id;
        /*目标从机离线*/
        if (pd && (pl->Schedule.Event_Id > pl->MapSize))
            goto __start_next;
        /*添加到当前设备到塞列表中*/
        Add_ListItem(pl->Schedule.Block, pl->Schedule.Event_Id);
        /*从就绪列表中移除:非首次扫描状态(就绪列表非空)*/
        pl->Schedule.First_Flag
            ? Remove_ListItem(pl->Schedule.Ready, pl->Schedule.Event_Id)
            : false;
#if defined(USING_DEBUG)
        shellPrint(Shell_Object, "@Warning:Timeout_Event_Id = %d.\r\n", pl->Schedule.Event_Id);
#endif
        if (pd->Slave.pPools->InputCoils[actual_site] != 0x00)
        {
            *(bool *)pd->Slave.pHandle = false;
            pd->Slave.pPools->InputCoils[actual_site] = 0x00;
            // pl->pMap[pl->Schedule.Event_Id].schedule_counts++;
            // pl->pMap[pl->Schedule.Event_Id].Check.Schedule_counts++;
        }
        /*连续三轮调度均检测不到目标从机响应：则认为直接离线*/
#if (USING_INDEPENDENT_STRUCT)
        if (++pl->pMap[pl->Schedule.Event_Id].Check.Schedule_counts > SCHEDULING_COUNTS)
#else
        if (++pl->pMap[pl->Schedule.Event_Id].Schedule_counts > SCHEDULING_COUNTS)
#endif
        {

#if (USING_INDEPENDENT_STRUCT)
            pl->pMap[pl->Schedule.Event_Id].Check.Schedule_counts = 0;
#else
            pl->pMap[pl->Schedule.Event_Id].Schedule_counts = 0;
#endif
            *(bool *)pd->Slave.pHandle = false;
            /*对于输入型从机：离线时清除输入寄存器*/
            if (pl->Schedule.Event_Id < A_TYPE_SLAVE_MAX_ADDR)
            {
                memset(&pd->Slave.pPools->InputCoils[pl->Schedule.Event_Id * DIGITAL_INPUT_NUMBERS],
                       0x00, DIGITAL_INPUT_NUMBERS);
            }
        }

    __start_next:
        /*在此处递增事件，而不是在L_OK中*/
        Check_FirstFlag(pl);

#if (USING_INDEPENDENT_STRUCT)
        pl->pMap[pl->Schedule.Event_Id].Check.State = L_None;
#else
        pl->Check.State = L_None;
#endif
    }
    break;
    default:
        break;
    }
}
