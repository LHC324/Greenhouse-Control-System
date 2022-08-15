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
        (*pl)->Schedule.Ready ? vListInitialise((*pl)->Schedule.Ready) : CUSTOM_FREE((*pl)->Schedule.Ready);
        (*pl)->Schedule.Block = (List_t *)CUSTOM_MALLOC(sizeof(List_t));
        (*pl)->Schedule.Block ? vListInitialise((*pl)->Schedule.Block) : CUSTOM_FREE((*pl)->Schedule.Block);
        (*pl)->Schedule.First_Flag = false;
        (*pl)->Schedule.Period = 0;
        (*pl)->Schedule.Event_Id = 0;
        (*pl)->Master.pTbuf = (uint8_t *)CUSTOM_MALLOC(ps->Master.TxSize);
        (*pl)->Master.pTbuf ? (void)memset((*pl)->Master.pTbuf, 0, ps->Master.TxSize) : CUSTOM_FREE((*pl)->Master.pTbuf);
        (*pl)->Slave.pRbuf = (uint8_t *)CUSTOM_MALLOC(ps->Slave.RxSize);
        (*pl)->Slave.pRbuf ? (void)memset((*pl)->Slave.pRbuf, 0, ps->Slave.RxSize) : CUSTOM_FREE((*pl)->Master.pTbuf);
        (*pl)->Master.TxSize = ps->Master.TxSize;
        (*pl)->Master.TxCount = 0;
        (*pl)->Slave.RxSize = ps->Slave.RxSize;
        (*pl)->Slave.RxCount = 0;
        (*pl)->Slave.bSemaphore = ps->Slave.bSemaphore;
        (*pl)->Check = ps->Check;
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
        L_Map[i].Frame_Head.Device_Addr.Val = i + 1U;
        L_Map[i].Frame_Head.Channel = i + 1U;
        L_Map[i].Slave_Id = i + 1U;
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
        .Check = {
            .State = L_None,
            .Counter = 0,
            .OverTimes = SUSPEND_TIMES,
        },
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
 * @brief	添加列表项到列表
 * @details
 * @param	None
 * @retval	None
 */
void Add_ListItem(List_t *list, TickType_t data)
{
    ListItem_t *p = NULL;
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
 * @brief	从列表中移除列表
 * @details
 * @param	None
 * @retval	列表中剩余的列表项
 */
UBaseType_t Remove_ListItem(List_t *list, TickType_t data)
{
    UBaseType_t items;
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
 * @brief	获取一个列表项
 * @details
 * @param	None
 * @retval	目标列表项
 */
ListItem_t *Get_OneListItem(List_t *list, ListItem_t **p)
{
    // static ListItem_t *p = NULL;

    if (list == NULL)
    {
        return NULL;
    }
    *p = *p ? (*p) : ((ListItem_t *)&list->xListEnd);
    *p = ((*p)->pxNext->xItemValue == portMAX_DELAY) ? ((ListItem_t *)list->xListEnd.pxNext) : ((*p)->pxNext);

    return *p;
}

/**
 * @brief	查找目标设备（在线/离线）设备
 * @details
 * @param	None
 * @retval	None
 */
TickType_t Get_OnlineDevice(pLoraHandle pl, List_t *list)
{
    TickType_t data;
    if (pl && list)
    {
        ListItem_t *p = Get_OneListItem(list, &pl->Schedule.Ready_Iter);
        if (p)
        {
            data = listGET_LIST_ITEM_VALUE(p);
            if (data == portMAX_DELAY)
            {
                return LORA_NULL_ID;
            }
            return data;
        }
    }
    return LORA_NULL_ID;
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
        /*遍历L101Map,找到对应从站*/
        // for (Lora_Map *p = pl->pMap; p && p < pl->pMap + pl->MapSize; p++)
        {
            // if ((Get_LoraId(pl) == p->Slave_Id) && (Get_Data(pl, pl->Slave.RxCount - 2U, MOD_WORD) ==
            //                                         ((uint16_t)((crc16 >> 8U) | (crc16 << 8U)))))
            uint8_t slave_id = pl->Schedule.Event_Id + 1U;
            if ((Get_LoraId(pl) == slave_id) && (Get_Data(pl, pl->Slave.RxCount - 2U, MOD_WORD) ==
                                                 ((uint16_t)((crc16 >> 8U) | (crc16 << 8U)))))
            {
                if (pl->Schedule.Event_Id < LORA_NULL_ID)
                {
                    !Find_ListItem(pl->Schedule.Ready, pl->Schedule.Event_Id) ? Add_ListItem(pl->Schedule.Ready,
                                                                                             pl->Schedule.Event_Id)
                                                                              : (void)false;
                    /*此处极限容纳32个从机状态:取离散输入的后32字节空间作为状态存储*/
                    if (pl->Schedule.Event_Id < sizeof(pd->Slave.pPools->InputCoils) / 2U)
                    {
                        if (pd->Slave.pPools->InputCoils[LORA_STATE_OFFSET + pl->Schedule.Event_Id] != 0xFF)
                            *(bool *)pd->Slave.pHandle = false;
                        pd->Slave.pPools->InputCoils[LORA_STATE_OFFSET + pl->Schedule.Event_Id] = 0xFF;
                    }
                }
                Check_FirstFlag(pl);
                /*对应从站正确响应*/
                pl->Check.State = L_OK;
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
                        uint16_t target_addr = pl->Schedule.Event_Id * DIGITAL_INPUT_NUMBERS;
                        if (target_addr > (DIGITAL_INPUT_NUMBERS * A_TYPE_SLAVE_MAX_ADDR))
                        {
#if defined(USING_DEBUG)
                            shellPrint(Shell_Object, "@Error:Input register address out of range!\r\n");
#endif
                            return;
                        }

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
                pl->Check.State = L_Error;
        }
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
    uint8_t buf[] = {pm->Frame_Head.Device_Addr.V.H, pm->Frame_Head.Device_Addr.V.L,
                     pm->Frame_Head.Channel, pm->Slave_Id, 0x05, 0x00, 0x00,
                     0x00, 0x00, 0x00, 0x00};
    if (pd && pm)
    {
        if (pm->Slave_Id < B_TYPE_SLAVE_START_ADDR)
        {
            // buf[4] = 0x02, buf[6] = (pm->Slave_Id - DIGITAL_INPUT_OFFSET), buf[8] = 0x08;
            buf[4] = 0x02, buf[8] = 0x08;
        }
        else
        {
            uint8_t data = 0;
            pd->Slave.Reg_Type = Coil;
            pd->Slave.Operate = Read;
#if !defined(USING_TEST)
            /*读取对应寄存器*/
            if (!pd->Mod_Operatex(pd, (pm->Slave_Id - DIGITAL_OUTPUT_OFFSET), &data, sizeof(data)))
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
            buf[4] = 0x05, buf[7] = data;
        }
        pl->Master.TxCount = sizeof(buf);
        uint16_t crc = Get_Crc16(&buf[sizeof(pm->Frame_Head) - 1U], sizeof(buf) - 5U, 0xffff);
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

    switch (pl->Check.State)
    { /*首次发出事件或者从机响应成功*/
    case L_None:
    case L_OK:
    {
        /*确保L101模块当前处于空闲状态*/
        if (!pl->Get_Lora_Status(pl))
        {
            return;
        }
        /*清除超时计数器*/
        pl->Check.Counter = 0;
        pl->Schedule.Event_Id = pl->Schedule.First_Flag ? (uint8_t)Get_OnlineDevice(pl, pl->Schedule.Ready)
                                                        : pl->Schedule.Event_Id;
        /*禁止接收引脚*/
        HAL_GPIO_WritePin(pl->Cs.pGPIOx, pl->Cs.Gpio_Pin, GPIO_PIN_SET);
        /*去掉首次扫描完毕的第一个静默周期*/
        pl->Schedule.Event_Id < pl->MapSize ? pl->Lora_MakeFrame(pl, &pl->pMap[pl->Schedule.Event_Id]),
            pl->Check.State = L_Wait        : false;
        /*使能接收引脚*/
        HAL_GPIO_WritePin(pl->Cs.pGPIOx, pl->Cs.Gpio_Pin, GPIO_PIN_RESET);
        /*每调度一个周期就绪列表，就检测阻塞列表中一个从机*/
        pl->Schedule.First_Flag && (pl->Schedule.Event_Id < pl->MapSize) ? pl->Schedule.Period++ : false;
        /*转换状态*/
        // pl->Check.State = L_Wait;

        if ((pl->Schedule.Period > pl->Schedule.Ready->uxNumberOfItems) ||
            (!pl->Schedule.Ready->uxNumberOfItems && pl->Schedule.First_Flag))
        {
            pl->Schedule.Period = 0;
            /*判断是否有离线设备*/
            if (listCURRENT_LIST_LENGTH(pl->Schedule.Block))
            {
                /*循环的从阻塞列表中移除一个列表项*/
                ListItem_t *p = Get_OneListItem(pl->Schedule.Block, &pl->Schedule.Block_Iter);
                if (p && (p->xItemValue != portMAX_DELAY))
                {
                    /*加入就绪列表*/
                    Add_ListItem(pl->Schedule.Ready, listGET_LIST_ITEM_VALUE(p));
                    /*从阻塞列表中移除*/
                    Remove_ListItem(pl->Schedule.Block, listGET_LIST_ITEM_VALUE(p));
                }
#if defined(USING_DEBUG)
                shellPrint(Shell_Object, "p = 0x%p,\r\n", p);
                // shellPrint(Shell_Object, "\r\nschdule !\r\n");
                shellPrint(Shell_Object, "\r\nBlocK_List[%d]:", pl->Schedule.Block->uxNumberOfItems);
                for (p = (ListItem_t *)pl->Schedule.Block->xListEnd.pxNext; p->xItemValue != portMAX_DELAY;
                     p = p->pxNext)
                {
                    shellPrint(Shell_Object, "%d ", listGET_LIST_ITEM_VALUE(p));
                }
                // shellPrint(Shell_Object, "p->xItemValue = 0X%0X\r\n", p->xItemValue);
                shellPrint(Shell_Object, "\r\n\r\nReady_List[%d]:", pl->Schedule.Ready->uxNumberOfItems);
                for (p = (ListItem_t *)pl->Schedule.Ready->xListEnd.pxNext; p->xItemValue != portMAX_DELAY;
                     p = p->pxNext)
                {
                    shellPrint(Shell_Object, "%d ", listGET_LIST_ITEM_VALUE(p));
                }
                shellPrint(Shell_Object, "\r\n\r\nOS remaining heap = %dByte, Mini heap = %dByte\r\n",
                           xPortGetFreeHeapSize(), xPortGetMinimumEverFreeHeapSize());
#endif
            }
        }
    }
    break;
    case L_Wait:
    {
        /*接收超时，不是接收错误导致的超时*/
        if (++pl->Check.Counter >= pl->Check.OverTimes)
        {
            pl->Check.Counter = 0;
            pl->Check.State = L_TimeOut;
        }
    }
    break;
    case L_Error:
    case L_TimeOut:
    {
        /*当前设备不在阻塞列表中*/
        !Find_ListItem(pl->Schedule.Block, pl->Schedule.Event_Id) &&
                (pl->Schedule.Event_Id != LORA_NULL_ID)
            ? Add_ListItem(pl->Schedule.Block,
                           pl->Schedule.Event_Id)
            : (void)false;
        /*从就绪列表中移除:非首次扫描状态(就绪列表非空)*/
        pl->Schedule.First_Flag &&listCURRENT_LIST_LENGTH(pl->Schedule.Ready)
            ? Remove_ListItem(pl->Schedule.Ready, pl->Schedule.Event_Id)
            : false;
        /*目标从机离线*/
        if (pd && (pl->Schedule.Event_Id < sizeof(pd->Slave.pPools->InputCoils) / 2U))
        {
            if (pd->Slave.pPools->InputCoils[LORA_STATE_OFFSET + pl->Schedule.Event_Id] != 0x00)
                *(bool *)pd->Slave.pHandle = false;
            pd->Slave.pPools->InputCoils[LORA_STATE_OFFSET + pl->Schedule.Event_Id] = 0x00;
        }

        /*在此处递增事件，而不是在L_OK中*/
        Check_FirstFlag(pl);
        pl->Check.State = L_None;
    }
    break;
    default:
        break;
    }
}
