#include "sys.h"
#include "includes.h"
#include "delay.h"
#include "usart.h"
#include "rtc.h"

extern OS_FLAG_GRP	g_flag_grp;					//事件标志组的对象
extern volatile uint32_t g_run_time;
void rtc_init(void)
{
	EXTI_InitTypeDef	EXTI_InitStructure;
	NVIC_InitTypeDef	NVIC_InitStructure;
	
	RTC_TimeTypeDef  	RTC_TimeStructure;
	RTC_DateTypeDef  	RTC_DateStructure;
	RTC_InitTypeDef  	RTC_InitStructure;	
	//使能rtc的硬件时钟
	
	/* Enable the PWR clock，使能电源管理时钟 */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
	
	/* Allow access to RTC，允许访问RTC */
	PWR_BackupAccessCmd(ENABLE);

#if 0
	//打开LSE振荡时钟
	RCC_LSEConfig(RCC_LSE_ON);

	//检查LSE振荡时钟是否就绪
	while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET);
	
	//为RTC选择LSE作为时钟源
	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
	
#else
	
	RCC_LSICmd(ENABLE);
	
	/* Wait till LSI is ready */  
	while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)
	{
	}
	
	/* Select the RTC Clock Source */
	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
#endif
	
	/* Enable the RTC Clock，使能RTC的硬件时钟 */
	RCC_RTCCLKCmd(ENABLE);
	
	/* Wait for RTC APB registers synchronisation，等待所有RTC相关寄存器就绪 */
	RTC_WaitForSynchro();
	
	//配置频率1Hz
	/* ck_spre(1Hz) = RTCCLK(LSE) /(uwAsynchPrediv + 1)*(uwSynchPrediv + 1)*/
	//32768Hz/(127+1)/(255+1)=1Hz
	RTC_InitStructure.RTC_AsynchPrediv = 127;
	RTC_InitStructure.RTC_SynchPrediv = 255;
	RTC_InitStructure.RTC_HourFormat = RTC_HourFormat_24;
	RTC_Init(&RTC_InitStructure);
	
	
	//配置日期
	RTC_DateStructure.RTC_Year = 0x21;				//2021年
	RTC_DateStructure.RTC_Month = RTC_Month_March;	//3月份
	RTC_DateStructure.RTC_Date = 0x17;				//17号
	RTC_DateStructure.RTC_WeekDay = RTC_Weekday_Wednesday;//星期3
	RTC_SetDate(RTC_Format_BCD, &RTC_DateStructure);


	//配置时间 17:14:50
	RTC_TimeStructure.RTC_H12     = RTC_H12_PM;
	RTC_TimeStructure.RTC_Hours   = 0x10;
	RTC_TimeStructure.RTC_Minutes = 0x14;
	RTC_TimeStructure.RTC_Seconds = 0x50; 
	RTC_SetTime(RTC_Format_BCD, &RTC_TimeStructure);  
	
	
	RTC_WakeUpCmd(DISABLE);
	
	//唤醒时钟源的硬件时钟频率为1Hz
	RTC_WakeUpClockConfig(RTC_WakeUpClock_CK_SPRE_16bits);
	
	//只进行一次计数
	RTC_SetWakeUpCounter(0);
	
	RTC_WakeUpCmd(ENABLE);

	//配置中断的触发方式：唤醒中断
	RTC_ITConfig(RTC_IT_WUT, ENABLE);
	
	
	RTC_ClearFlag(RTC_FLAG_WUTF);
	
	EXTI_ClearITPendingBit(EXTI_Line22);
	EXTI_InitStructure.EXTI_Line = EXTI_Line22;			//
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;//RTC手册规定
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	//优先级
	NVIC_InitStructure.NVIC_IRQChannel = RTC_WKUP_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

void rtc_alarm_set(RTC_AlarmTypeDef RTC_AlarmStructure)
{
	/* 关闭闹钟，若不关闭，配置闹钟触发的中断有BUG，无论怎么配置，只要到00秒，则触发中断*/
	RTC_AlarmCmd(RTC_Alarm_A, DISABLE);
	
	/* Configure the RTC Alarm A register，配置RTC的A闹钟 */
	RTC_SetAlarm(RTC_Format_BCD, RTC_Alarm_A, &RTC_AlarmStructure);
	
	/* Enable the alarm ，让RTC的闹钟A工作*/
	RTC_AlarmCmd(RTC_Alarm_A, ENABLE);
	
}


void rtc_alarm_a_init(void)
{
	EXTI_InitTypeDef	EXTI_InitStructure;
	NVIC_InitTypeDef	NVIC_InitStructure;	
	
	/* Enable RTC Alarm A Interrupt，允许RTC的A闹钟触发中断 */
	RTC_ITConfig(RTC_IT_ALRA, ENABLE);
	
	/* 清空标志位 */
	RTC_ClearFlag(RTC_FLAG_ALRAF);
	
	EXTI_ClearITPendingBit(EXTI_Line17);
	EXTI_InitStructure.EXTI_Line = EXTI_Line17;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);


	/*使能闹钟的中断 */
	NVIC_InitStructure.NVIC_IRQChannel = RTC_Alarm_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	RTC_AlarmCmd(RTC_Alarm_A, DISABLE);
}



void  RTC_WKUP_IRQHandler(void)
{
	OS_ERR err;
	
	//进入中断
	OSIntEnter(); 
	
	//检测标志位
	if(RTC_GetITStatus(RTC_IT_WUT) == SET)
	{
		g_run_time++;
		//设置事件标志组
		OSFlagPost(&g_flag_grp,FLAG_GRP_RTC_WAKEUP,OS_OPT_POST_FLAG_SET,&err);		
		
		//清空标志位	
		RTC_ClearITPendingBit(RTC_IT_WUT);
		
		EXTI_ClearITPendingBit(EXTI_Line22);
	}
	
	//退出中断
	OSIntExit();	
}


void RTC_Alarm_IRQHandler(void)
{
	OS_ERR err;
	
	//进入中断
	OSIntEnter(); 

	
	//检测标志位
	if(RTC_GetITStatus(RTC_IT_ALRA) == SET)
	{
		//设置事件标志组
		OSFlagPost(&g_flag_grp,FLAG_GRP_RTC_ALARM_A,OS_OPT_POST_FLAG_SET,&err);		

		//清空标志位	
		RTC_ClearITPendingBit(RTC_IT_ALRA);
		
		EXTI_ClearITPendingBit(EXTI_Line17);
	}
	
	//退出中断
	OSIntExit();	
}
uint8_t DEC2BCD(uint8_t dec)
{
	//(((data/10)<<4) + (data%10));
	return (dec/10*16)+(dec%10);
}
