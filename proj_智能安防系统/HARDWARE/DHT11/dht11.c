#include "dht11.h"

//初始化温湿度模块（PG9）
void dht11_init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	
	/* 1.使能对应GPIO的RCC时钟 */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);
	
	// 2.将PG9配置为输出模式，并置为高电平
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;		//引脚号
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;	//输出模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;	//推挽模式
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	//输出速率（功耗）
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;//无上下拉
	GPIO_Init(GPIOG, &GPIO_InitStructure);
	
	GPIO_SetBits(GPIOG, GPIO_Pin_9);
}

// 获取温湿度模块数据（单总线：PG9）
int8_t dht11_read_data(uint8_t *pbuf)
{
	uint32_t t=0;	//用于超时检测计数
	uint8_t data=0;	//暂存每一个字节
	int8_t i=0, j=0;//循环变量
	uint8_t check_sum;	//校验和
	GPIO_InitTypeDef  GPIO_InitStructure;
	
	// 1.将当前PG9设置为输出模式，并置为高电平
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;		//引脚号
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;	//输出模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;	//推挽模式
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	//输出速率（功耗）
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;//无上下拉
	GPIO_Init(GPIOG, &GPIO_InitStructure);
	
	PGout(9) = 1;
	
	// 2.将PG9输出低电平，并延时至少18ms
	PGout(9) = 0;
	delay_ms(20);
	
	// 3.PG9输出高电平，并延时20-40us（如30us）
	PGout(9) = 1;
	delay_us(30);
	
	// 4.再将PG9设置为输入模式
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;		//引脚号
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;	//输入模式
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;//无上下拉
	GPIO_Init(GPIOG, &GPIO_InitStructure);
	
	// 5.等待PG9变为低电平（最好添加超时处理）
	while(PGin(9))
	{
		t++;
		delay_us(1);
		if(t>=4000)	// 超时4ms
			return -1;
	}
	
	// 6.等待PG9变为高电平（最好添加超时处理）
	t=0;
	while(PGin(9) == 0)
	{
		t++;
		delay_us(1);
		if(t>=100)	// 超时100us
			return -2;
	}
	
	// 7.通过超时检测判断高电平合法性（等待变为低电平）
	t=0;
	while(PGin(9))
	{
		t++;
		delay_us(1);
		if(t>=100)	// 超时100us
			return -3;
	}
	
	// 8.接收40bit（5字节）数据（0或1）
		// 8.1 等待50us结束
	for(j=0; j<5; j++)
	{
		data = 0;	// 0000 0000
		
		for(i=7; i>=0; i--)
		{
			t=0;
			while(PGin(9) == 0)
			{
				t++;
				delay_us(1);
				if(t>=100)	// 超时100us
					return -4;
			}
				// 8.2 开始判断数据0或1
			delay_us(40);	// 40~60us左右
			if(PGin(9))
			{
				data |= 1<<i;
				
				// 等待剩余高电平持续完毕
				t=0;
				while(PGin(9))
				{
					t++;
					delay_us(1);
					if(t>=100)	// 超时100us
						return -5;
				}
			}
		}
		pbuf[j] = data;
	}
		
	// 9.校验数据（通过校验和）
	check_sum = (pbuf[0]+pbuf[1]+pbuf[2]+pbuf[3])&0xFF;
	if(check_sum != pbuf[4])
		return -6;
	
	return 0;
}
