#include "beep.h"

void BEEP_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	
	/* 1.使能对应GPIO的RCC时钟 */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);
	
	/* 2.配置GPIO并初始化 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;		//引脚号
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;	//输出模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;	//推挽
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	//输出速率（功耗）
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;//无上下拉(按键已经自带上拉)
	GPIO_Init(GPIOF, &GPIO_InitStructure);
	
	BEEP = 0;
}

