/****************************************************************
*名    称:智能安防系统
*作    者:zjr
*项目时间:2021/03/13-2021/03/17
*****************************************************************/

#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "oled.h"
#include "rtc.h"
#include "led.h"
#include "beep.h"
#include "bmp.h"
#include "sr04.h"
#include "dht11.h"
#include "flash.h"
#include "key_board.h"
#include "includes.h"
#include <string.h>

//任务uasrt1控制块
OS_TCB task_usart3;
void task_func_usart3(void *parg);
CPU_STK task_stk_usart3[512];			//任务堆栈，大小为512字，也就是2048字节

//任务rtc控制块
OS_TCB task_rtc;
void task_func_rtc(void *parg);
CPU_STK task_stk_rtc[512];			//任务2的任务堆栈，大小为512字，也就是2048字节

//任务rtc_alarm控制块
OS_TCB task_rtc_alarm;
void task_func_rtc_alarm(void *parg);
CPU_STK task_stk_rtc_alarm[512];			//任务3的任务堆栈，大小为512字，也就是2048字节

//任务led控制块
OS_TCB task_led;
void task_func_led(void *parg);
CPU_STK task_stk_led[512];			//任务4的任务堆栈，大小为512字，也就是2048字节

//任务beep控制块
OS_TCB task_beep;
void task_func_beep(void *parg);
CPU_STK task_stk_beep[512];			//任务5的任务堆栈，大小为512字，也就是2048字节

//任务beep控制块
OS_TCB task_beep;
void task_func_beep(void *parg);
CPU_STK task_stk_beep[512];			//任务5的任务堆栈，大小为512字，也就是2048字节

//任务sr04控制块
OS_TCB task_sr04;
void task_func_sr04(void *parg);
CPU_STK task_stk_sr04[512];			//任务6的任务堆栈，大小为512字，也就是2048字节

//任务keyboard控制块
OS_TCB task_keyboard;
void task_func_keyboard(void *parg);
CPU_STK task_stk_keyboard[512];			//任务7的任务堆栈，大小为512字，也就是2048字节

//任务bluet_check控制块
OS_TCB task_bluet_check;
void task_func_bluet_check(void *parg);
CPU_STK task_stk_bluet_check[512];			//任务8的任务堆栈，大小为512字，也就是2048字节

//任务keyvalue控制块
OS_TCB task_keyvalue;
void task_func_keyvalue(void *parg);
CPU_STK task_stk_keyvalue[512];			//任务9的任务堆栈，大小为512字，也就是2048字节


volatile uint32_t g_sr04_dis=40;		//sr04报警距离

volatile uint32_t g_run_time;			//运行时间
uint8_t login_flag = 0;					//登录标志位
char 		g_pwd_buf[11];				//flash里存储的密码
char 		pwd_in[11];					//保存输入密码数组
int8_t      num_in=0;					//已输入密码的位数

OS_FLAG_GRP	g_flag_grp;					//事件标志组的对象

OS_Q 		g_queue_usart3;				// 消息队列对象（串口3）
OS_Q 		g_queue_led;				// 消息队列对象（LED）
OS_Q 		g_queue_beep;				// 消息队列对象（BEEP）
OS_Q		g_queue_keyvalue;			// 消息队列对象（keyboard）

OS_MUTEX 	g_mutex_oled;				//互斥锁的对象oled

OS_SEM 		g_sem_led;					//信号量的对象led
OS_SEM 		g_sem_beep;					//信号量的对象beep

//主函数
int main(void)
{
	
	OS_ERR err;

	systick_init();  													//时钟初始化
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);						//中断分组配置
	
	usart_init(9600);  				 									//串口3初始化
	
	LED_Init(); 		//LED初始化	
	
	BEEP_Init();		//蜂鸣器初始化
	
	rtc_init();			//rtc初始化
	
	rtc_alarm_a_init();	//闹钟A初始化
	
	sr04_init();		//初始化sr04
	
	key_board_init();	//初始化矩阵键盘
	
	OLED_Init();		//初始化OLED
	OLED_Clear();

	/*解锁FLASH，允许操作FLASH*/
	FLASH_Unlock();

	/* 清空相应的标志位*/  
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
                   FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR); 
	//擦除flash
	//flash_erase_record();
	//char pbuf[11]={"524680"};
	//flash_write_record(pbuf,1);
	
	// 锁定Flash，保护数据内容不被更改。
	FLASH_Lock();
	
	flash_read_record(g_pwd_buf,1);
	printf("passwd:%s \r\n",g_pwd_buf);
	
	//OS初始化，它是第一个运行的函数,初始化各种的全局变量，例如中断嵌套计数器、优先级、存储器
	OSInit(&err);


	//创建usart3任务
	OSTaskCreate(	(OS_TCB *)&task_usart3,									//任务控制块，等同于线程id
					(CPU_CHAR *)"task_usart3",									//任务的名字，名字可以自定义的
					(OS_TASK_PTR)task_func_usart3,								//任务函数，等同于线程函数
					(void *)0,												//传递参数，等同于线程的传递参数
					(OS_PRIO)7,											 	//任务的优先级6		
					(CPU_STK *)task_stk_usart3,									//任务堆栈基地址
					(CPU_STK_SIZE)512/10,									//任务堆栈深度限位，用到这个位置，任务不能再继续使用
					(CPU_STK_SIZE)512,										//任务堆栈大小			
					(OS_MSG_QTY)0,											//禁止任务消息队列
					(OS_TICK)0,												//默认时间片长度																
					(void  *)0,												//不需要补充用户存储区
					(OS_OPT)OS_OPT_TASK_NONE,								//没有任何选项
					&err													//返回的错误码
				);
					
	if(err!=OS_ERR_NONE)
	{
		printf("task 1 create fail\r\n");
		
		while(1);
	
	}


	//创建实时时钟任务
	OSTaskCreate(	(OS_TCB *)&task_rtc,									//任务控制块
					(CPU_CHAR *)"task_rtc",									//任务的名字
					(OS_TASK_PTR)task_func_rtc,										//任务函数
					(void *)0,												//传递参数
					(OS_PRIO)7,											 	//任务的优先级7		
					(CPU_STK *)task_stk_rtc,									//任务堆栈基地址
					(CPU_STK_SIZE)512/10,									//任务堆栈深度限位，用到这个位置，任务不能再继续使用
					(CPU_STK_SIZE)512,										//任务堆栈大小			
					(OS_MSG_QTY)0,											//禁止任务消息队列
					(OS_TICK)0,												//默认时间片长度																
					(void  *)0,												//不需要补充用户存储区
					(OS_OPT)OS_OPT_TASK_NONE,								//没有任何选项
					&err													//返回的错误码
				);
					
	if(err!=OS_ERR_NONE)
	{
		printf("task 2 create fail\r\n");
		
		while(1);
	
	}
	
	//创建闹钟任务
	OSTaskCreate(	(OS_TCB *)&task_rtc_alarm,									//任务控制块
					(CPU_CHAR *)"task_rtc_alarm",									//任务的名字
					(OS_TASK_PTR)task_func_rtc_alarm,										//任务函数
					(void *)0,												//传递参数
					(OS_PRIO)7,											 	//任务的优先级7		
					(CPU_STK *)task_stk_rtc_alarm,									//任务堆栈基地址
					(CPU_STK_SIZE)512/10,									//任务堆栈深度限位，用到这个位置，任务不能再继续使用
					(CPU_STK_SIZE)512,										//任务堆栈大小			
					(OS_MSG_QTY)0,											//禁止任务消息队列
					(OS_TICK)0,												//默认时间片长度																
					(void  *)0,												//不需要补充用户存储区
					(OS_OPT)OS_OPT_TASK_NONE,								//没有任何选项
					&err													//返回的错误码
				);
					
	if(err!=OS_ERR_NONE)
	{
		printf("task alarm create fail\r\n");
		
		while(1);
	
	}
	
	//创建LED任务
	OSTaskCreate(	(OS_TCB *)&task_led,									//任务控制块
					(CPU_CHAR *)"task_led",									//任务的名字
					(OS_TASK_PTR)task_func_led,										//任务函数
					(void *)0,												//传递参数
					(OS_PRIO)7,											 	//任务的优先级7		
					(CPU_STK *)task_stk_led,									//任务堆栈基地址
					(CPU_STK_SIZE)512/10,									//任务堆栈深度限位，用到这个位置，任务不能再继续使用
					(CPU_STK_SIZE)512,										//任务堆栈大小			
					(OS_MSG_QTY)0,											//禁止任务消息队列
					(OS_TICK)0,												//默认时间片长度																
					(void  *)0,												//不需要补充用户存储区
					(OS_OPT)OS_OPT_TASK_NONE,								//没有任何选项
					&err													//返回的错误码
				);
					
	if(err!=OS_ERR_NONE)
	{
		printf("task led create fail\r\n");
		
		while(1);
	
	}
	
	
	//创建BEEP任务
	OSTaskCreate(	(OS_TCB *)&task_beep,									//任务控制块
					(CPU_CHAR *)"task_beep",									//任务的名字
					(OS_TASK_PTR)task_func_beep,										//任务函数
					(void *)0,												//传递参数
					(OS_PRIO)7,											 	//任务的优先级7		
					(CPU_STK *)task_stk_beep,									//任务堆栈基地址
					(CPU_STK_SIZE)512/10,									//任务堆栈深度限位，用到这个位置，任务不能再继续使用
					(CPU_STK_SIZE)512,										//任务堆栈大小			
					(OS_MSG_QTY)0,											//禁止任务消息队列
					(OS_TICK)0,												//默认时间片长度																
					(void  *)0,												//不需要补充用户存储区
					(OS_OPT)OS_OPT_TASK_NONE,								//没有任何选项
					&err													//返回的错误码
				);
					
	if(err!=OS_ERR_NONE)
	{
		printf("task BEEP create fail\r\n");
		
		while(1);
	}
	
	//创建sr04任务
	OSTaskCreate(	(OS_TCB *)&task_sr04,									//任务控制块
					(CPU_CHAR *)"task_sr04",									//任务的名字
					(OS_TASK_PTR)task_func_sr04,										//任务函数
					(void *)0,												//传递参数
					(OS_PRIO)7,											 	//任务的优先级7		
					(CPU_STK *)task_stk_sr04,									//任务堆栈基地址
					(CPU_STK_SIZE)512/10,									//任务堆栈深度限位，用到这个位置，任务不能再继续使用
					(CPU_STK_SIZE)512,										//任务堆栈大小			
					(OS_MSG_QTY)0,											//禁止任务消息队列
					(OS_TICK)0,												//默认时间片长度																
					(void  *)0,												//不需要补充用户存储区
					(OS_OPT)OS_OPT_TASK_NONE,								//没有任何选项
					&err													//返回的错误码
				);
					
	if(err!=OS_ERR_NONE)
	{
		printf("task sr04 create fail\r\n");
		
		while(1);
	}
	
	//创建keyboard任务
	OSTaskCreate(	(OS_TCB *)&task_keyboard,									//任务控制块
					(CPU_CHAR *)"task_keyboard",									//任务的名字
					(OS_TASK_PTR)task_func_keyboard,										//任务函数
					(void *)0,												//传递参数
					(OS_PRIO)7,											 	//任务的优先级7		
					(CPU_STK *)task_stk_keyboard,									//任务堆栈基地址
					(CPU_STK_SIZE)512/10,									//任务堆栈深度限位，用到这个位置，任务不能再继续使用
					(CPU_STK_SIZE)512,										//任务堆栈大小			
					(OS_MSG_QTY)0,											//禁止任务消息队列
					(OS_TICK)0,												//默认时间片长度																
					(void  *)0,												//不需要补充用户存储区
					(OS_OPT)OS_OPT_TASK_NONE,								//没有任何选项
					&err													//返回的错误码
				);
					
	if(err!=OS_ERR_NONE)
	{
		printf("task keyboard create fail\r\n");
		
		while(1);
	}
	
		//创建bluet_check任务
	OSTaskCreate(	(OS_TCB *)&task_bluet_check,									//任务控制块
					(CPU_CHAR *)"task_bluet_check",									//任务的名字
					(OS_TASK_PTR)task_func_bluet_check,										//任务函数
					(void *)0,												//传递参数
					(OS_PRIO)7,											 	//任务的优先级7		
					(CPU_STK *)task_stk_bluet_check,									//任务堆栈基地址
					(CPU_STK_SIZE)512/10,									//任务堆栈深度限位，用到这个位置，任务不能再继续使用
					(CPU_STK_SIZE)512,										//任务堆栈大小			
					(OS_MSG_QTY)0,											//禁止任务消息队列
					(OS_TICK)0,												//默认时间片长度																
					(void  *)0,												//不需要补充用户存储区
					(OS_OPT)OS_OPT_TASK_NONE,								//没有任何选项
					&err													//返回的错误码
				);
					
	if(err!=OS_ERR_NONE)
	{
		printf("task bluet_check create fail\r\n");
		
		while(1);
	}
	
	//创建keyvalue任务
	OSTaskCreate(	(OS_TCB *)&task_keyvalue,									//任务控制块
					(CPU_CHAR *)"task_keyvalue",									//任务的名字
					(OS_TASK_PTR)task_func_keyvalue,										//任务函数
					(void *)0,												//传递参数
					(OS_PRIO)7,											 	//任务的优先级7		
					(CPU_STK *)task_stk_keyvalue,									//任务堆栈基地址
					(CPU_STK_SIZE)512/10,									//任务堆栈深度限位，用到这个位置，任务不能再继续使用
					(CPU_STK_SIZE)512,										//任务堆栈大小			
					(OS_MSG_QTY)0,											//禁止任务消息队列
					(OS_TICK)0,												//默认时间片长度																
					(void  *)0,												//不需要补充用户存储区
					(OS_OPT)OS_OPT_TASK_NONE,								//没有任何选项
					&err													//返回的错误码
				);
					
	if(err!=OS_ERR_NONE)
	{
		printf("task keyvalue create fail\r\n");
		
		while(1);
	}
	
	
	//创建事件标志组，所有标志位初值为0
	OSFlagCreate(&g_flag_grp,"g_flag_grp",0,&err);
	
	// 创建消息队列
	OSQCreate(&g_queue_usart3, "g_queue_usart3", 16, &err);
	OSQCreate(&g_queue_led, "g_queue_led", 16, &err);
	OSQCreate(&g_queue_beep, "g_queue_beep", 16, &err);
	OSQCreate(&g_queue_keyvalue, "g_queue_keyvalue", 16, &err);
	
	//创建互斥锁
	//OSMutexCreate(&g_mutex_printf,	"g_mutex_printf",&err);	
	OSMutexCreate(&g_mutex_oled,	"g_mutex_oled",&err);
	
	//创建信号量，初值为0，有一个资源
	OSSemCreate(&g_sem_led,"g_sem_led",0,&err);
	OSSemCreate(&g_sem_beep,"g_sem_beep",0,&err);
	
	//启动OS，进行任务调度
	OSStart(&err);
					
					
	printf(".......\r\n");
					
	while(1);
	
}
void read_temp(void)
{
	OS_ERR err;
	uint8_t buf[5] = {0};		//温湿度数据
	char buf_temp[16]={0};
	dht11_init();			//初始化温湿度模块
	
	dht11_read_data(buf);
	printf("T:%d.%d  \r\n", buf[2], buf[3]);
	
	OSTaskSuspend(&task_rtc,&err);
	OSTaskSuspend(&task_rtc_alarm,&err);
	OSTaskSuspend(&task_sr04,&err);
	OLED_Clear();
	sprintf(buf_temp,"Temp:%d.%d",buf[2], buf[3]);
	//等待互斥锁
	OSMutexPend(&g_mutex_oled,0,OS_OPT_PEND_BLOCKING,NULL,&err);	
	OLED_ShowString(0,2,(uint8_t *)buf_temp,16);
	//立即释放互斥锁
	OSMutexPost(&g_mutex_oled,OS_OPT_POST_NONE,&err);
	delay_ms(4000);
	OLED_Clear();
	OSTaskResume(&task_rtc,&err);
	OSTaskResume(&task_rtc_alarm,&err);
	OSTaskResume(&task_sr04,&err);

}
void read_humi(void)
{
	OS_ERR err;
	 uint8_t buf[5] = {0};		//温湿度数据
	char buf_humi[16]={0};
	
	dht11_init();			//初始化温湿度模块
	dht11_read_data(buf);
	
	printf("H:%d.%d  \r\n", buf[0], buf[1]);

	OSTaskSuspend(&task_rtc,&err);
	OSTaskSuspend(&task_rtc_alarm,&err);
	OSTaskSuspend(&task_sr04,&err);
	OLED_Clear();
	sprintf(buf_humi,"Humi:%d.%d",buf[0], buf[1]);
	OSMutexPend(&g_mutex_oled,0,OS_OPT_PEND_BLOCKING,NULL,&err);
	OLED_ShowString(0,2,(uint8_t *)buf_humi,16);
	OSMutexPost(&g_mutex_oled,OS_OPT_POST_NONE,&err);
	delay_ms(4000);
	OLED_Clear();
	OSTaskResume(&task_rtc,&err);
	OSTaskResume(&task_rtc_alarm,&err);
	OSTaskResume(&task_sr04,&err);
}

void run_time(void)
{
	OS_ERR err;
	char buf_run[16]={0};		//运行时间
	
				
	OSTaskSuspend(&task_rtc,&err);
	OSTaskSuspend(&task_rtc_alarm,&err);
	OSTaskSuspend(&task_sr04,&err);
	OLED_Clear();
	printf("run time:%ds\r\n",g_run_time);
	sprintf(buf_run,"run time:%ds",g_run_time);
	OSMutexPend(&g_mutex_oled,0,OS_OPT_PEND_BLOCKING,NULL,&err);
	OLED_ShowString(0,2,(uint8_t *)buf_run,16);
	OSMutexPost(&g_mutex_oled,OS_OPT_POST_NONE,&err);
	delay_ms(3000);
	OLED_Clear();
	OSTaskResume(&task_rtc,&err);
	OSTaskResume(&task_rtc_alarm,&err);
	OSTaskResume(&task_sr04,&err);
}
void task_func_usart3(void *parg)
{
	OS_ERR err;
	OS_MSG_SIZE msg_size=0;
	RTC_TimeTypeDef  	RTC_TimeStructure;
	RTC_DateTypeDef 	RTC_DateStructure;
	RTC_AlarmTypeDef 	RTC_AlarmStructure;	
	uint8_t success_flag=0;
	char *msg = NULL;
	//申请CPU状态保存
	CPU_SR_ALLOC();	
	
	printf("task_func_usart3 is create ok\r\n");
	printf("please input the passwd!!!\r\n");

	while(1)
	{
		
		//等待消息队列
		msg = OSQPend(&g_queue_usart3, 0, OS_OPT_PEND_BLOCKING, &msg_size, NULL, &err);
		//接收到消息
		if(msg && msg_size)
		{
			//关闭串口3中断
			NVIC_DisableIRQ(USART3_IRQn);
			
			printf("msg(%d): %s\r\n", msg_size, msg);
			if(login_flag == 1)
			{
				//根据指令进行判断
				if(strstr((char *)msg,"DATE SET"))
				{
					/* 设置日期：DATE SET-2021-3-18# */
					char *p = strtok((char *)msg, "-#");
					p = strtok(NULL, "-#");		// 设置年份
					RTC_DateStructure.RTC_Year = DEC2BCD(atoi(p)-2000);
					
					p = strtok(NULL, "-#");		// 设置月份
					RTC_DateStructure.RTC_Month = DEC2BCD(atoi(p));
					p = strtok(NULL, "-#");		// 设置日期
					RTC_DateStructure.RTC_Date = DEC2BCD(atoi(p));
					RTC_DateStructure.RTC_WeekDay = 0x01;
					RTC_SetDate(RTC_Format_BCD, &RTC_DateStructure);
				}
				else if(strstr((char *)msg, "TIME SET"))
				{
					/* 设置时间：TIME SET-10:20:30# */
					char *p = strtok((char *)msg, "-#");
					p = strtok(NULL, ":#");		// 设置小时和上下午
					
					//进入临界区，原因RTC获取时间用到相同的结构体和寄存器
					OS_CRITICAL_ENTER();
					RTC_TimeStructure.RTC_Hours   = DEC2BCD(atoi(p));
					if(atoi(p) < 12)
						RTC_TimeStructure.RTC_H12     = RTC_H12_AM;
					else
						RTC_TimeStructure.RTC_H12     = RTC_H12_PM;
					p = strtok(NULL, ":#");		// 设置分钟
					RTC_TimeStructure.RTC_Minutes   = DEC2BCD(atoi(p));
					p = strtok(NULL, ":#");		// 设置秒数
					RTC_TimeStructure.RTC_Seconds   = DEC2BCD(atoi(p));
					
					RTC_SetTime(RTC_Format_BCD, &RTC_TimeStructure); 
					//退出临界区
					OS_CRITICAL_EXIT();				
				}
				else if(strstr((char *)msg, "AL SET"))
				{
					char *p = strtok((char *)msg, "-#");
					p = strtok(NULL, ":#");		// 设置小时和上下午
					OS_CRITICAL_ENTER();
					RTC_AlarmStructure.RTC_AlarmTime.RTC_Hours   = DEC2BCD(atoi(p));
					if(atoi(p) < 12)
						RTC_AlarmStructure.RTC_AlarmTime.RTC_H12     = RTC_H12_AM;
					else
						RTC_AlarmStructure.RTC_AlarmTime.RTC_H12     = RTC_H12_PM;
					p = strtok(NULL, ":#");		// 设置分钟
					RTC_AlarmStructure.RTC_AlarmTime.RTC_Minutes   = DEC2BCD(atoi(p));
					p = strtok(NULL, ":#");		// 设置秒数
					RTC_AlarmStructure.RTC_AlarmTime.RTC_Seconds   = DEC2BCD(atoi(p));
					
					//每天闹钟都会生效
					//屏蔽星期/日期，也就说闹钟每天都会生效
					RTC_AlarmStructure.RTC_AlarmMask = RTC_AlarmMask_DateWeekDay;				
		
					//设置闹钟
					rtc_alarm_set(RTC_AlarmStructure); 
					//printf("alarm:%02x:%02x:%02x \r\n",RTC_AlarmStructure.RTC_AlarmTime.RTC_Hours,RTC_AlarmStructure.RTC_AlarmTime.RTC_Minutes,RTC_AlarmStructure.RTC_AlarmTime.RTC_Seconds);
				/*	//阻塞等待互斥锁
					//OSMutexPend(&g_mutex_oled,0,OS_OPT_PEND_BLOCKING,NULL,&err);	
					//OLED_Clear();
					//OLED_DrawBMP(0,0,128,8,(uint8_t *)pic_alarm);
					
					//立即释放互斥锁
					//OSMutexPost(&g_mutex_oled,OS_OPT_POST_NONE,&err);*/
					OS_CRITICAL_EXIT();	
					printf("set alarm ok\r\n");
				}
				
				else if(strstr((char *)msg, "TEMP"))
				{
					read_temp();	
				}
				else if(strstr((char *)msg, "HUMI"))
				{
					read_humi();
				}	
				else if(strstr((char *)msg, "SR SET"))//SR SET-100#
				{
					char *p = strtok((char *)msg, "-#");
					p = strtok(NULL, "-#");
					g_sr04_dis = atoi(p);
				}
				else if(strstr((char *)msg, "RUN TIME"))//RUN TIME#
				{
					run_time();
				}
				
				else if(strstr((char *)msg, "PAW SET"))//PAW SET-123456#
				{
					char *p = strtok((char *)msg, "-#");
					p = strtok(NULL, "-#");		
					/*解锁FLASH，允许操作FLASH*/
					FLASH_Unlock();
					/* 清空相应的标志位*/  
					FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
								   FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR); 
					//擦除flash
					flash_erase_record();
					//写入密码
					flash_write_record(p,1);

					// 锁定Flash，保护数据内容不被更改。
					FLASH_Lock();

					flash_read_record(g_pwd_buf,1);
					printf("passwd2:%s \r\n",g_pwd_buf);
						
				}
				memset(msg, 0, msg_size);
				msg_size=0;
				
				//使能串口3中断
				NVIC_EnableIRQ(USART3_IRQn);
			}
			else if(login_flag == 0)
			{
				//关闭串口3中断
				NVIC_DisableIRQ(USART3_IRQn);
				
				printf("msg(%d): %s\r\n", msg_size, msg);
				char *p = strtok(msg,"#");
				char *q = p;
				int8_t num = 0;
				for(int8_t i=0; i<10; i++,q++)//验证虚位密码
				{   
					if(*q==g_pwd_buf[num])
						num++;
					else 
					{
						num=0;
						if(*q==g_pwd_buf[num])
						num++;
					}
					
					if(num==6)
					{
						login_flag=1;

						printf("passwd ok!!!\r\n");
						success_flag = 1;
						
						BEEP = 1;
						delay_ms(50);
						BEEP = 0;
						break;
					}
				}
				if(!success_flag)	//密码错误
				{
					BEEP = 1;
					delay_ms(2000);
					BEEP = 0;	
				}
				memset(msg, 0, msg_size);
				msg_size=0;
				
				//使能串口3中断
				NVIC_EnableIRQ(USART3_IRQn);
			}
			else
			{
				delay_ms(1000);
			}
		}
	}
}

void task_func_rtc(void *parg)				//实时时钟任务
{
	char buf_time[16]={0};
	char buf_date[16]={0};
	
	RTC_TimeTypeDef  	RTC_TimeStructure;
	RTC_DateTypeDef 	RTC_DateStructure;
	OS_ERR err;
	
	OS_FLAGS flags=0;
	
	printf("task_rtc is create ok\r\n");
	while(1)
	{	
		//if(login_flag)
		//{//一直阻塞等待事件标志组，等待成功后，将对应bit清0
		flags=OSFlagPend(&g_flag_grp,FLAG_GRP_RTC_WAKEUP,0,OS_OPT_PEND_FLAG_SET_ANY + OS_OPT_PEND_FLAG_CONSUME+OS_OPT_PEND_BLOCKING,NULL,&err);	
			
		if(err != OS_ERR_NONE)
		{
			printf("[task_rtc][OSFlagPend]Error Code = %d\r\n",err);
			
			continue;
		}
			
		//rtc唤醒中断
		if(flags & FLAG_GRP_RTC_WAKEUP)
		{
			
			//RTC_GetTime，获取时间
			RTC_GetTime(RTC_Format_BCD, &RTC_TimeStructure); 
			
			//获取日期
			RTC_GetDate(RTC_Format_BCD, &RTC_DateStructure);
			
			//格式化字符串
			sprintf(buf_date,"20%02x-%02x-%02x",RTC_DateStructure.RTC_Year,RTC_DateStructure.RTC_Month,RTC_DateStructure.RTC_Date);
			sprintf(buf_time,"%02x:%02x:%02x",RTC_TimeStructure.RTC_Hours,RTC_TimeStructure.RTC_Minutes,RTC_TimeStructure.RTC_Seconds);
			
			//printf("%s\r\n",buf_date);
			//printf("%s\r\n",buf_time);
			
			//阻塞等待互斥锁
			OSMutexPend(&g_mutex_oled,0,OS_OPT_PEND_BLOCKING,NULL,&err);			
			
			//oled显示日期时间
			OLED_ShowString(0,0,(uint8_t *)buf_date,16);
			OLED_ShowString(0,2,(uint8_t *)buf_time,16);
					
			//立即释放互斥锁
			OSMutexPost(&g_mutex_oled,OS_OPT_POST_NONE,&err);	
			
		}
	//}
	}
}
void task_func_rtc_alarm(void *parg)		//闹钟任务
{
	OS_ERR err;
	OS_FLAGS flag;
	
	uint32_t i=0,j=0;
	uint8_t sta=0;
	
	printf("task_alarm is create ok\r\n");	
	
	while(1)
	{
		// 等待事件标志位
		flag = OSFlagPend(&g_flag_grp, FLAG_GRP_RTC_ALARM_A, 0, OS_OPT_PEND_FLAG_SET_ANY + OS_OPT_PEND_FLAG_CONSUME+OS_OPT_PEND_BLOCKING, NULL, &err);

		//闹钟来了
		if(flag & FLAG_GRP_RTC_ALARM_A)
		{
			//挂起其他任务，目的独占OLED屏显示
			OSTaskSuspend(&task_rtc,&err);
			OSTaskSuspend(&task_usart3,&err);
			OSTaskSuspend(&task_sr04,&err);
			OLED_Display_On();
			
			delay_ms(100);
			OLED_Clear();	
			//显示图片：起床啦
			OLED_DrawBMP(0,0,128,8,(uint8_t *)pic_alarm);
			
			//LED闪烁，蜂鸣器响起
			for(j=0; j<3; j++)
			{
				for(i=0; i<4; i++)
				{
					//printf("abcdef\r\n");
					//1:蜂鸣器响，灯亮；0:蜂鸣器不响，灯灭
					if(i%2==0)
						sta=0x01;
					else sta=0x00;
				
					//控制led
					OSQPost(&g_queue_led,&sta,1,OS_OPT_POST_FIFO,&err);
					
					if(err != OS_ERR_NONE)
					{
						printf("[app_task_rtc_alarm][OSQPost]Error Code %d\r\n",err);
						break;
					}
					//等待消息队列发送完毕
					delay_ms(50);
					
					if(i%2==0)
						sta=1;
					else sta=0;				
					
					//控制蜂鸣器
					OSQPost(&g_queue_beep,&sta,1,OS_OPT_POST_FIFO,&err);
					
					if(err != OS_ERR_NONE)
					{
						printf("[app_task_rtc_alarm][OSQPost]Error Code %d\r\n",err);
						break;
					}

					//阻塞等待信号量led
					OSSemPend(&g_sem_led,0,OS_OPT_PEND_BLOCKING,NULL,&err);			
					if(err != OS_ERR_NONE)
					{
						printf("[app_task_rtc_alarm][OSSemPend]Error Code = %d\r\n",err);
						break;	
					}					
					//阻塞等待信号量beep
					OSSemPend(&g_sem_beep,0,OS_OPT_PEND_BLOCKING,NULL,&err);			
					if(err != OS_ERR_NONE)
					{
						printf("[app_task_rtc_alarm][OSSemPend]Error Code = %d\r\n",err);
						break;	
					}

					delay_ms(100);
				}
				delay_ms(500);
			}
			
			//oled清屏
			OLED_Clear();
			
			delay_ms(100);
			//OLED_Display_Off();		
			
			//恢复其他任务
			OSTaskResume(&task_rtc,&err);
			OSTaskResume(&task_usart3,&err);
			OSTaskResume(&task_sr04,&err);			
		}	
	}
}

void task_func_led(void *parg)
{
	OS_ERR err;
	OS_MSG_SIZE msg_size=0;
	char *p = NULL;
	uint8_t led_sta=0;

	printf("task_led is create ok\r\n");

	while(1)
	{
		//等待消息队列,接收其他模块发来的led控制
		p = OSQPend(&g_queue_led, 0, OS_OPT_PEND_BLOCKING, &msg_size, NULL, &err);
		
		if(p && msg_size)
		{
			//printf("led_sta: %d\r\n", *p);
			led_sta = *p;
			
			if(led_sta == 0x01)			//闹钟
				LED1 = 0 ;
			else if(led_sta == 0x00)
				LED1 = 1;
			else if(led_sta == 0x02)	//蓝牙
				LED2 = 0 ;
			else if(led_sta == 0x04)				
				LED2 = 1 ;
			else if(led_sta == 0x0E)	//sr04
				LED4 = 0 ;
			else if(led_sta == 0x08)				
				LED4 = 1 ;
			
/*			if(led_sta &0x01)
				LED1 = 0;
			else LED1 =1;
			if(led_sta &0x02)
				LED2 = 0;
			else LED2 =1;
			if(led_sta &0x04)
				LED3 = 0;
			else LED3 =1;
			if(led_sta &0x08)
				LED4 = 0;
			else LED4 =1;*/
		}
		//释放信号量，告诉对方，当前LED控制任务已经完成
		OSSemPost(&g_sem_led,OS_OPT_POST_1,&err);	
	}
}

void task_func_beep(void *parg)
{
	OS_ERR err;
	OS_MSG_SIZE msg_size=0;
	char *p = NULL;
	uint8_t beep_sta=0;

	printf("task_beep is create ok\r\n");

	while(1)
	{
		//等待消息队列,接收其他模块发来的led控制
		p = OSQPend(&g_queue_beep, 0, OS_OPT_PEND_BLOCKING, &msg_size, NULL, &err);
		
		if(p && msg_size)
		{
			//printf("beep_sta: %d\r\n", *p);
			beep_sta = *p;
			
			if(beep_sta)
				BEEP = 1;
			else BEEP =0;
		}
		//释放信号量，告诉对方，当前beep控制任务已经完成
		OSSemPost(&g_sem_beep,OS_OPT_POST_1,&err);	
	}
}

void task_func_sr04(void *parg)
{
	OS_ERR err;
	uint32_t distance = 0;
	uint8_t sta=0;
	uint32_t i;
	
	printf("task_sr04 is create ok\r\n");
	while(1)
	{
		// 获取距离数据
		distance = sr04_get_distance();
		
		if(distance == 0xFFFFFFFF)
			printf("sr04 error!\r\n");
		else if(distance >= 20 && distance <= 4000)
		{
			//数据是正确的
			//printf("distance=%dmm\r\n",distance);	
			
			if(distance <= g_sr04_dis)	//小于超声波预警距离，报警
			{
				printf("distance=%dmm\r\n",distance);	
				//LED4闪烁
				for(i=0; i<14; i++)
				{	
					//1:蜂鸣器响，灯亮；0:蜂鸣器不响，灯灭
					if(i%2==0)
						sta=0x0E;
					else sta=0x08;
				
					//控制led
					OSQPost(&g_queue_led,&sta,1,OS_OPT_POST_FIFO,&err);
					
					if(err != OS_ERR_NONE)
					{
						printf("[task_sr04][OSQPost]Error Code %d\r\n",err);
						break;
					}				
					
					//阻塞等待信号量led
					OSSemPend(&g_sem_led,0,OS_OPT_PEND_BLOCKING,NULL,&err);			
					if(err != OS_ERR_NONE)
					{
						printf("[task_sr04][OSSemPend]Error Code = %d\r\n",err);
						break;	
					}					
					
					delay_ms(50);
					
				}
				
				//挂起其他任务，目的独占OLED屏显示
				OSTaskSuspend(&task_rtc,&err);
				OSTaskSuspend(&task_usart3,&err);
				OSTaskSuspend(&task_rtc_alarm,&err);
				OLED_Display_On();
				
				delay_ms(100);
				OLED_Clear();	
				//显示图片：太近了
				OLED_DrawBMP(0,0,128,8,(uint8_t *)pic_sr04);
				delay_ms(1000);
				//oled清屏
				OLED_Clear();
				
				delay_ms(100);
				//OLED_Display_Off();		
				
				//恢复其他任务
				OSTaskResume(&task_rtc,&err);
				OSTaskResume(&task_usart3,&err);
				OSTaskResume(&task_rtc_alarm,&err);
			
			}
		}
		delay_ms(1000);
	}
}

void task_func_keyboard(void *parg)
{
	OS_ERR err;
	uint32_t key_sta=0;
	char key_old=0;
	char key_cur=0;
	
	printf("task_keyboard is create ok\r\n");
	while(1)
	{
		/* 使用状态机思想得到按键的状态 */
		switch(key_sta)
		{
			case 0://获取按下的按键
			{
				key_cur = get_key_board();	
				if(key_cur != 'N')
				{
					key_old = key_cur;
					key_sta=1;
				}
			}break;

			case 1://确认按下的按键
			{
				key_cur = get_key_board();	
				if((key_cur != 'N') && (key_cur == key_old))
				{
					printf("KEY %c Down\r\n",key_cur);
					//发送按键值到消息队列
					OSQPost(&g_queue_keyvalue,&key_cur,1,OS_OPT_POST_FIFO,&err);
					key_sta=2;
				}
			}break;
		
			case 2://获取释放的按键
			{
				key_cur = get_key_board();	
				if(key_cur == 'N')
				{
					printf("KEY %c Up\r\n",key_old);
					
					key_sta=0;
					key_old =  'N';
				}
			}break;

			default:break;
		}
	}
}

void task_func_keyvalue(void *parg)
{
	OS_ERR err;
	OS_MSG_SIZE msg_size=0;	
	char *p = NULL;
	uint32_t value=0;						//按键的值
	//uint8_t buf[5] = {0};					//温湿度数据
	uint8_t success_flag=0;
	
	uint8_t i=0;
	uint8_t num=0;							//正确的密码位数
	printf("task_keyvalue is create ok\r\n");

	while(1)
	{
		//等待消息队列
		p = OSQPend(&g_queue_keyvalue, 0, OS_OPT_PEND_BLOCKING, &msg_size, NULL, &err);
		if(p && msg_size)
		{
			value = *p;
			if(login_flag == 0)
			{
				if(value >='0' && value <='9')
				{
					pwd_in[num_in++]=value;
				}
				if(value=='#'||num_in == 10)
				{
					num = 0;
					for(i=0; i<10; i++)//验证虚位密码
					{   
						if(pwd_in[i]==g_pwd_buf[num])
							num++;
						else 
						{
							num=0;
							if(pwd_in[i]==g_pwd_buf[num])
							num++;
						}
						if(num==6)		//密码正确
						{
							login_flag = 1;
							printf("passwd ok!!!\r\n");
							success_flag = 1;
							
							BEEP = 1;
							delay_ms(50);
							BEEP = 0;
							break;
						}
					}
					if(!success_flag)	//密码错误
					{
						BEEP = 1;
						delay_ms(2000);
						BEEP = 0;	
					}
					num_in = 0;
					memset(pwd_in,0,sizeof(pwd_in));
				}
			}
			else if(login_flag == 1)
			{
				if(value=='A')
				{
					read_temp();
				}
				else if(value=='B')
				{
					read_humi();
				}
				else if(value=='C')			//超声波检测距离
				{
					uint32_t distance = 0;
					distance = sr04_get_distance();
					printf("distance=%dmm\r\n",distance);
				}
				else if(value=='D')			//运行时间
					run_time();
				else if(value=='*')			//软件复位
					NVIC_SystemReset();
			}
			else
				delay_ms(1000);
		}
	}

}

void task_func_bluet_check(void *parg)
{
	uint8_t sta=0;
	OS_ERR err;
	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_3;		//LED0和LED1对应IO口
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;			    //普通输出模式，
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;				//推挽输出，驱动LED需要电流驱动
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;		    //100MHz
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;				    //无上下拉
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	while(1)
	{
		if(PAin(3))	
		{
			sta=0x02;	
		}	
		else
		{	
			sta=0x04;
		}
		//控制led
		OSQPost(&g_queue_led,&sta,1,OS_OPT_POST_FIFO,&err);
		delay_ms(50);
		
		//阻塞等待信号量led
		OSSemPend(&g_sem_led,0,OS_OPT_PEND_BLOCKING,NULL,&err);	
	}
}
