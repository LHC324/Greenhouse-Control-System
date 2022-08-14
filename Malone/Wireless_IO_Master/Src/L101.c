#include "L101.h"
#include "usart.h"
#if defined(USING_RTTHREAD)
#include "rtthread.h"
#else
#include "cmsis_os.h"
#endif
#include "shell_port.h"
#include "mdrtuslave.h"
#include "io_signal.h"

/*定义L101临时组包缓冲区*/
// static uint8_t g_pFBuffer[PF_TX_SIZE] = {0};

/*建立lora模块各节点间调度数据结构*/
typedef struct
{
    List_t *Ready;
    List_t *Block;
    bool First_Flag;
} L101_Schedule __attribute__((aligned(4)));

static L101_Schedule *pLs = NULL;
static ListItem_t *g_Item[2U] = {NULL, NULL};

extern osThreadId shellHandle;
extern osThreadId mdbusHandle;
extern osTimerId Timer1Handle;
/*静态函数声明*/
static uint8_t Set_BitFrame(L101_HandleTypeDef *pL);

/*L101事件处理映射图*/
L101_HandleTypeDef L101_Map[EXTERN_DIGITAL_MAX] = {
    {.Sdevice_Addr = 0x01, .Schannel = 0x01, .Slave_Id = 0x01, .Digital_Addr = 0x0000, .Crc16 = 0, .Analog_Addr = 0x0000, .Check = {L_None, 3U, 0}, .func = Set_BitFrame},
    {.Sdevice_Addr = 0x02, .Schannel = 0x02, .Slave_Id = 0x02, .Digital_Addr = 0x0001, .Crc16 = 0, .Analog_Addr = 0x0001, .Check = {L_None, 3U, 0}, .func = Set_BitFrame},
    {.Sdevice_Addr = 0x03, .Schannel = 0x03, .Slave_Id = 0x03, .Digital_Addr = 0x0002, .Crc16 = 0, .Analog_Addr = 0x0002, .Check = {L_None, 3U, 0}, .func = Set_BitFrame},
    {.Sdevice_Addr = 0x04, .Schannel = 0x04, .Slave_Id = 0x04, .Digital_Addr = 0x0003, .Crc16 = 0, .Analog_Addr = 0x0003, .Check = {L_None, 3U, 0}, .func = Set_BitFrame},
    {.Sdevice_Addr = 0x05, .Schannel = 0x05, .Slave_Id = 0x05, .Digital_Addr = 0x0004, .Crc16 = 0, .Analog_Addr = 0x0004, .Check = {L_None, 3U, 0}, .func = Set_BitFrame},
    {.Sdevice_Addr = 0x06, .Schannel = 0x06, .Slave_Id = 0x06, .Digital_Addr = 0x0005, .Crc16 = 0, .Analog_Addr = 0x0005, .Check = {L_None, 3U, 0}, .func = Set_BitFrame},
    {.Sdevice_Addr = 0x07, .Schannel = 0x07, .Slave_Id = 0x07, .Digital_Addr = 0x0006, .Crc16 = 0, .Analog_Addr = 0x0006, .Check = {L_None, 3U, 0}, .func = Set_BitFrame},
    {.Sdevice_Addr = 0x08, .Schannel = 0x08, .Slave_Id = 0x08, .Digital_Addr = 0x0007, .Crc16 = 0, .Analog_Addr = 0x0007, .Check = {L_None, 3U, 0}, .func = Set_BitFrame},
};

/**
 * @brief  初始化调度列表
 * @param  None
 * @retval None
 */
void Slist_Init(void)
{
    pLs = (L101_Schedule *)pvPortMalloc(sizeof(L101_Schedule));
    if (pLs != NULL)
    {
        pLs->Ready = (List_t *)pvPortMalloc(sizeof(List_t));
        pLs->Ready ? (vListInitialise(pLs->Ready)) : (vPortFree(pLs->Ready));
        pLs->Block = (List_t *)pvPortMalloc(sizeof(List_t));
        pLs->Block ? (vListInitialise(pLs->Block)) : (vPortFree(pLs->Block));
        pLs->First_Flag = false;
    }
    else
    {
        vPortFree(pLs);
    }
}

/**
 * @brief  取得16bitCRC校验码
 * @param  ptr   当前数据串指针
 * @param  length  数据长度
 * @param  init_dat 校验所用的初始数据
 * @retval 16bit校验码
 */
static uint16_t Get_Crc16(uint8_t *ptr, uint16_t length, uint16_t init_dat)
{
    uint16_t i = 0;
    uint16_t j = 0;
    uint16_t crc16 = init_dat;

    for (i = 0; i < length; i++)
    {
        crc16 ^= *ptr++;

        for (j = 0; j < 8; j++)
        {
            crc16 = (crc16 & 0x0001) ? ((crc16 >> 1) ^ 0xa001) : (crc16 >> 1U);
        }
    }
    return (crc16);
}

/**
 * @brief  大小端数据类型交换
 * @note   对于一个单精度浮点数的交换仅仅需要2次
 * @param  pData 数据
 * @param  start 开始位置
 * @param  length  数据长度
 * @retval None
 */
void Endian_Swap(uint8_t *pData, uint8_t start, uint8_t length)
{
    uint8_t i = 0;
    uint8_t tmp = 0;
    uint8_t count = length / 2U;
    uint8_t end = start + length - 1U;

    for (; i < count; i++)
    {
        tmp = pData[start + i];
        pData[start + i] = pData[end - i];
        pData[end - i] = tmp;
    }
}

/**
 * @brief	获取Lora模块当前是否空闲
 * @details	无线发送数据时拉低，用于指示发送繁忙状态
 * @param	None
 * @retval	ture/fale
 */
bool inline Get_L101_Status(void)
{
    return ((bool)HAL_GPIO_ReadPin(STATUS_GPIO_Port, STATUS_Pin));
}

/**
 * @brief	Lora模块恢复出厂设置
 * @details	拉低 3s 以上恢复出厂设置
 * @param	None
 * @retval	None
 */
void Set_L101_FactoryMode(void)
{
    HAL_GPIO_WritePin(RELOAD_GPIO_Port, RELOAD_Pin, GPIO_PIN_RESET);
#if defined(USING_RTTHREAD)
    rt_thread_mdelay(3500);
#else
    osDelay(3500);
#endif
    HAL_GPIO_WritePin(RELOAD_GPIO_Port, RELOAD_Pin, GPIO_PIN_SET);
}

/**
 * @brief	确定Lora模块工作的方式
 * @details
 * @param	handler:modbus主机/从机句柄
 * @retval	true:定点模式;fasle:shell
 */
bool Check_Mode(ModbusRTUSlaveHandler handler)
{
    ReceiveBufferHandle pB = handler->receiveBuffer;

    return (((pB->count > 0) && (pB->buf[0] == ENTER_CODE)) ? (false) : (true));
}

/**
 * @brief   进入shell模式
 * @details
 * @param	None
 * @retval	None
 */
void Shell_Mode(void)
{
    __HAL_UART_DISABLE_IT(&huart1, UART_IT_IDLE);
    HAL_NVIC_DisableIRQ(USART1_IRQn);
    if (HAL_UART_DMAStop(&huart1) != HAL_OK)
    {
        return;
    }
    /*挂起Modbus任务*/
    // osThreadSuspend(mdbusHandle);
    /*恢复shell任务*/
    osThreadResume(shellHandle);
    /*停止发送定时器*/
    osTimerStop(Timer1Handle);
}

/**
 * @brief   退出shell模式
 * @details
 * @param	None
 * @retval	None
 */
#if defined(USING_L101)
uint8_t Exit_Shell(void)
{
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
    if (HAL_UART_Receive_DMA(&huart1, mdRTU_Recive_Buf(Master_Object), MODBUS_PDU_SIZE_MAX) != HAL_OK)
    {
        return 0xFF;
    }
    /*恢复modbus任务*/
    // osThreadResume(mdbusHandle);
    /*挂起shell任务*/
    osThreadSuspend(shellHandle);
    /*打开发送定时器*/
    osTimerStart(Timer1Handle, MDTASK_SENDTIMES);
    return 0x00;
}
#if defined(USING_DEBUG)
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), exit_shell, Exit_Shell, exit);
#endif
#endif

/**
 * @brief	位变量组帧
 * @details 主机设备号为0，信道为0
 * @note    |---目标节点地址（2B）---|---信道（1B）---|---从机地址---|---Data---|---CRC---|
 * @param	None
 * @retval	None
 */
uint8_t Set_BitFrame(L101_HandleTypeDef *pL)
{
    mdBit Coil_Bit;
    mdSTATUS ret = mdFALSE;
    Frame_Head pF = {.Addr.Val = 0x0000};
    /*读取对应寄存器地址*/
    // ret = g_Mdmaster->registerPool->mdReadCoil(g_Mdmaster->registerPool, pL->Digital_Addr, &Coil_Bit);
    ret = mdRTU_ReadCoil(Master_Object, pL->Digital_Addr, Coil_Bit);

    mdU8 buf[] = {0, 0, pL->Schannel, pL->Slave_Id, 0x05, pL->Digital_Addr >> 8U,
                  pL->Digital_Addr, (uint8_t)(Coil_Bit & 0x00FF),
                  0x00, 0x00, 0x00};
    /*读取寄存器正确*/
    if (ret == mdTRUE)
    {
        /*得到从站设备地址*/
        pF.Addr.V.H = pL->Sdevice_Addr << 8U;
        pF.Addr.V.L = pL->Sdevice_Addr;
        /*拷贝前2个字节到临时缓冲区*/
        memcpy(&buf[0], (mdU8 *)&pF, sizeof(pF));
        /*计算CRC:memcpy拷贝属于大端*/
        pL->Crc16 = Get_Crc16(&buf[sizeof(pF) + 1U], sizeof(buf) - 5U, 0xffff);
        /*拷贝两个字节CRC到临时缓冲区*/
        memcpy(&buf[sizeof(buf) - sizeof(pL->Crc16)], (mdU8 *)&pL->Crc16, sizeof(pL->Crc16));
        /*数据传输到从站*/
        // g_Mdmaster->mdRTUSendString(g_Mdmaster, buf, sizeof(buf));
        mdRTU_SendString(Master_Object, buf, sizeof(buf));
        /*传输完成一个从站后考虑适当的延时*/
    }

    return ret;
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
    p = (ListItem_t *)pvPortMalloc(sizeof(ListItem_t));
    if (p == NULL)
    {
        vPortFree(p);
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
        vPortFree(p);
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
 * @brief	查找在线设备
 * @details
 * @param	None
 * @retval	None
 */
TickType_t Get_OnlineDevice(List_t *list)
{
    TickType_t data;
    if (list != NULL)
    {
        data = listGET_LIST_ITEM_VALUE(Get_OneListItem(list, &g_Item[0]));
        if (data == portMAX_DELAY)
        {
            return 0;
        }
        return data;
    }

    return 0;
}

/**
 * @brief	主站发送数据给从站
 * @details
 * @param	None
 * @retval	None
 */
void Master_Poll(void)
{
    L101_HandleTypeDef *pL = NULL;
    static uint16_t event_x = 0;
    static uint16_t sum = 0;
    static bool option_flag = false;

    /*首次上电扫描所有从机状态*/
    event_x = (pLs->First_Flag && option_flag) ? (option_flag = false, Get_OnlineDevice(pLs->Ready)) : (event_x);

#if defined(USING_DEBUG)
    // shellPrint(&shell, "\r\nevent_x = %d\r\n", event_x);
#endif
    if (event_x >= LEVENTS)
    {
        return;
    }
    pL = &L101_Map[event_x];

    if ((pL == NULL) || (pLs == NULL))
    {
        return;
    }
#if defined(USING_DEBUG)
    // shellPrint(&shell, "\r\np[%d],Check.State = %d\r\n", event_x, pL->Check.State);
#endif
    switch (pL->Check.State)
    { /*首次发出事件或者从机响应成功*/
    case L_None:
    case L_OK:
    { /*清除超时计数器*/
        pL->Check.Counter = 0;
        /*确保L101模块当前处于空闲状态*/
        if (!Get_L101_Status())
        {
            return;
        }
        pL->func(pL);
        // L101_Map[1].func(&L101_Map[1]);
        /*保障首次事件不被错过*/
        if (pL->Check.State == L_OK)
        { /*释放操作信号*/
            option_flag = true;
            /*当前设备在就绪列表中不存在*/
            if (Find_ListItem(pLs->Ready, event_x) == NULL)
            { /*检测到回应的设备加入就绪列表*/
                Add_ListItem(pLs->Ready, event_x);
            }
            if (pLs->First_Flag == false)
            {
                /*一个周期扫描结束*/
                pLs->First_Flag = (++event_x >= LEVENTS) ? (event_x = 0, true) : false;
            }
        }
        /*转换状态*/
        pL->Check.State = L_Wait;
    }
    break;
    case L_Wait:
    {
        /*接收超时，不是接收错误导致的超时*/
        if (++pL->Check.Counter >= pL->Check.Times)
        {
            pL->Check.Counter = 0;
            pL->Check.State = L_TimeOut;
        }
#if defined(USING_DEBUG)

#endif
    }
    break;
    case L_Error:
    case L_TimeOut:
    {
        // pL->Check.State = L_OK;
        /*当前设备不在阻塞列表中*/
        if (Find_ListItem(pLs->Block, event_x) == NULL)
        {
            Add_ListItem(pLs->Block, event_x);
        }
        /*从就绪列表中移除:非首次扫描状态*/
        if (pLs->First_Flag)
        { /*当前错误设备处于离线*/
            Remove_ListItem(pLs->Ready, event_x);
        }
        else
        {
            /*一个周期扫描结束*/
            pLs->First_Flag = (++event_x >= LEVENTS) ? (event_x = 0, true) : false;
        }
        pL->Check.State = L_None;
    }
    break;
    default:
        break;
    }
    if (pLs->First_Flag)
    {
        ListItem_t *p = NULL;
        /*1个None+3Wait+1Timeout,增加一个就绪列表项消耗*/
        if (++sum >= pL->Check.Times + 2U + pLs->Ready->uxNumberOfItems)
        {
            sum = 0;
            option_flag = true;
            /*判断是否有离线设备*/
            if (listCURRENT_LIST_LENGTH(pLs->Block))
            {
                /*循环的从阻塞列表中移除一个列表项*/
                p = Get_OneListItem(pLs->Block, &g_Item[1]);
                /*加入就绪列表*/
                Add_ListItem(pLs->Ready, listGET_LIST_ITEM_VALUE(p));
                /*从阻塞列表中移除*/
                Remove_ListItem(pLs->Block, listGET_LIST_ITEM_VALUE(p));
#if defined(USING_DEBUG)
                // shellPrint(&shell, "p = 0x%p,\r\n", p);
                // // shellPrint(&shell, "\r\nschdule !\r\n");
                // shellPrint(&shell, "\r\nBlocK_List:\r\n");
                // for (p = (ListItem_t *)pLs->Block->xListEnd.pxNext; p->xItemValue != portMAX_DELAY; p = p->pxNext)
                // {
                //     shellPrint(&shell, "Bnums = %d,value[0x%p] = %d\r\n", pLs->Block->uxNumberOfItems, p, listGET_LIST_ITEM_VALUE(p));
                // }
                // shellPrint(&shell, "p->xItemValue = 0X%0X\r\n", p->xItemValue);
                // shellPrint(&shell, "\r\nReady_List:\r\n");
                // for (p = (ListItem_t *)pLs->Ready->xListEnd.pxNext; p->xItemValue != portMAX_DELAY; p = p->pxNext)
                // {
                //     shellPrint(&shell, "Rnums = %d,value[0x%p] = %d\r\n", pLs->Ready->uxNumberOfItems, p, listGET_LIST_ITEM_VALUE(p));
                // }
                // shellPrint(&shell, "OS remaining heap = %dByte, Mini heap = %dByte\r\n",
                //    xPortGetFreeHeapSize(), xPortGetMinimumEverFreeHeapSize());
#endif
            }
        }
    }
}
