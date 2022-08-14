#include "L101.h"
#if defined(USING_RTTHREAD)
#include "rtthread.h"
#else
#include "cmsis_os.h"
#endif
#include "shell_port.h"
#include "mdrtuslave.h"
#include "io_signal.h"


/*静态函数声明*/
static uint8_t Set_BitFrame(L101_HandleTypeDef *pL);

/*L101事件处理映射图*/
L101_HandleTypeDef L101_Map[EXTERN_DIGITAL_MAX] =
{
    {.Sdevice_Addr = 0x01, .Schannel = 0x01, .Slave_Id = 0x01, .Digital_Addr = 0x0000, .Crc16 = 0, .Analog_Addr = 0x0000, .Check = {L_None, 3U, 0}, .func = Set_BitFrame},
    {.Sdevice_Addr = 0x02, .Schannel = 0x02, .Slave_Id = 0x02, .Digital_Addr = 0x0001, .Crc16 = 0, .Analog_Addr = 0x0001, .Check = {L_None, 3U, 0}, .func = Set_BitFrame},
};


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
 * @brief	位变量组帧
 * @details 主机设备号为0，信道为0
 * @param	None
 * @retval	None
 */
uint8_t Set_BitFrame(L101_HandleTypeDef *pL)
{
    mdBit Coil_Bit;
    mdSTATUS ret = mdFALSE;
    Frame_Head pF = {.Addr.Val = 0x0000};
    /*读取对应寄存器地址*/
    ret = mdhandler->registerPool->mdReadCoil(mdhandler->registerPool, pL->Digital_Addr, &Coil_Bit);

    mdU8 buf[] = {0, 0, pL->Schannel, pL->Slave_Id, 0x05, pL->Digital_Addr >> 8U,
                  pL->Digital_Addr, (uint8_t)(Coil_Bit & 0x00FF),
                  0x00, 0x00, 0x00};
    /*读取寄存器错误*/
    if (ret != mdTRUE)
    {
        goto __exit;
    }
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
    mdhandler->mdRTUSendString(mdhandler, buf, sizeof(buf));
    /*传输完成一个从站后考虑适当的延时*/
__exit:
    return ret;
}

#if defined(USING_MASTER)
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

    pL = &L101_Map[event_x];
    if (pL == NULL)
    {
        return;
    }
#if defined(USING_DEBUG)
        // shellPrint(&shell,"pL->Check.State = %d\r\n",pL->Check.State);
#endif
    switch (pL->Check.State)
    { /*首次发出事件或者从机响应成功*/
    case L_None:
    case L_OK:
    { /*清除超时计数器*/
        pL->Check.Counter = 0;
        pL->func(pL);
        /*保障首次事件不被错过*/
        if (pL->Check.State == L_OK)
        {
           event_x = (++event_x) % LEVENTS; 
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
    {
        pL->Check.State = L_OK;
    }
    break;
    case L_TimeOut:
    {
        pL->Check.State = L_OK;
    }
    break;
    default:
        break;
    }
}
#else

#endif
