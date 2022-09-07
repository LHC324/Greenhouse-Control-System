#include "main.h"
#include "rtthread.h"
#include "shell.h"
#include "at_usr.h"
#include "mdrtuslave.h"
#include "io_signal.h"
#include "shell_port.h"

/**
 * @brief 
 * ① 串口1接收为不定长+DMA+空闲中断
 * ② 串口1发送为DMA发送，在rt-thread定时器回调函数中进行远程发送数据组包
 * ③ ADC2没有DMA通道，采用ADC采集完成定时中断
 * ④ ADC1通道0和ADC2通道1均是采集8个点数据取平均值
 * ⑤ 添加AT解析器
 * ⑥ 增加一路模拟串口用于调试
 */

/*定时器的控制块*/
#define SOFT_TIMER0 0x00
#define TIMER0_MS 1000
static rt_timer_t timer1;

/*shell线程参数*/
#define SHELL_STATCK_SIZE 512
#define SHELL_PRIORITY 30
#define SHELL_TIMESLICE 5
static rt_thread_t shell_thread = RT_NULL;

/*AT线程参数*/
#define AT_STATCK_SIZE 512
#define AT_PRIORITY 25
#define AT_TIMESLICE 5
static rt_thread_t at_thread = RT_NULL;

/*Modbus线程参数*/
#define MDRTUS_STATCK_SIZE 1024
#define MDRTUS_PRIORITY 15
#define MDRTUS_TIMESLICE 5
static rt_thread_t mdrtus_thread = RT_NULL;

/*外部信号读取线程参数*/
#define READIO_STATCK_SIZE 512
#define READIO_PRIORITY 5
#define READIO_TIMESLICE 5
static rt_thread_t readio_thread = RT_NULL;

/*创建shell互斥信号量*/
rt_mutex_t shellMutexHandle = RT_NULL;

/*局部任务函数声明*/
static void timer_callback(void *parameter);
static void shell_task_entry(void *parameter);
static void at_task_entry(void *parameter);
static void mdrtus_task_entry(void *parameter);
static void readio_task_entry(void *parameter);

/*初始化线程函数*/
void MX_RT_Thread_Init(void)
{
	/*创建定时器1周期定时器 */
	timer1 = rt_timer_create("timer1", timer_callback,
							 SOFT_TIMER0, TIMER0_MS,
							 RT_TIMER_FLAG_PERIODIC);
	/*启动定时器1 */
	if (timer1 != RT_NULL)
		rt_timer_start(timer1);

	shell_thread = rt_thread_create("shell",
									shell_task_entry, &shell,
									SHELL_STATCK_SIZE,
									SHELL_PRIORITY,
									SHELL_TIMESLICE);
	/*如果获得线程控制块,启动这个线程 */
	if (shell_thread != RT_NULL)
		rt_thread_startup(shell_thread);

	at_thread = rt_thread_create("at",
									at_task_entry, RT_NULL,
									AT_STATCK_SIZE,
									AT_PRIORITY,
									AT_TIMESLICE);
	/*如果获得线程控制块,启动这个线程 */
	if (at_thread != RT_NULL)
		rt_thread_startup(at_thread);

	mdrtus_thread = rt_thread_create("modbus",
									 mdrtus_task_entry, RT_NULL,
									 MDRTUS_STATCK_SIZE,
									 MDRTUS_PRIORITY,
									 MDRTUS_TIMESLICE);
	/*如果获得线程控制块,启动这个线程 */
	if (mdrtus_thread != RT_NULL)
		rt_thread_startup(mdrtus_thread);

	readio_thread = rt_thread_create("read_io",
									 readio_task_entry, RT_NULL,
									 READIO_STATCK_SIZE,
									 READIO_PRIORITY,
									 READIO_TIMESLICE);
	/*如果获得线程控制块,启动这个线程 */
	if (readio_thread != RT_NULL)
		rt_thread_startup(readio_thread);
}

/*定时器超时函数 */
void timer_callback(void *parameter)
{	/*所有软件定时器，共用此回调函数*/
	uint16_t timer_id = *(uint16_t *)parameter;
}

/*Shell任务*/
void shell_task_entry(void *parameter)
{
	while (1)
	{
		shellTask(parameter);
	} 
}

/*AT任务*/
void at_task_entry(void *parameter)
{
	while (1)
	{
		// at_process(&at);
		rt_thread_delay(2);
	} 
}

/*Mdrtus任务*/
void mdrtus_task_entry(void *parameter)
{
	while (1)
	{
		mdRTU_Handler();
		rt_thread_delay(2);
	}
}

/*获取外部输入信号任务*/
void readio_task_entry(void *parameter)
{
	while (1)
	{
		Io_Digital_Handle();
		Io_Analog_Handle();
		rt_thread_delay(2);
	}
}
