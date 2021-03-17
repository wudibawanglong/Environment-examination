#include "sys.h"
#include "usart.h"	
 
#include "includes.h"					//ucos 使用	  

volatile uint8_t  g_usart3_buf[128];	// 串口3接收的数据指令缓冲区	
volatile uint32_t g_usart3_cnt;			// 串口3数据接收长度

extern OS_Q g_queue_usart3;
//加入以下代码,支持printf函数,而不需要选择use MicroLIB	  

#pragma import(__use_no_semihosting)             
//标准库需要的支持函数                 
struct __FILE 
{ 
	int handle; 
}; 

FILE __stdout;       
//定义_sys_exit()以避免使用半主机模式    
void _sys_exit(int x) 
{ 
	x = x; 
} 
//重定义fputc函数 
int fputc(int ch, FILE *f)
{ 	
	while((USART3->SR&0X40)==0);//循环发送,直到发送完毕   
	USART3->DR = (u8) ch;      
	return ch;
}

 
//初始化IO 串口3
//baud:波特率
void usart_init(u32 baud)
{
   //GPIO端口设置
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB,ENABLE); //使能GPIOA时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3,ENABLE);//使能USART3时钟
 
	//串口3对应引脚复用映射
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource11,GPIO_AF_USART3); //GPIOA9复用为USART3
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource10,GPIO_AF_USART3); //GPIOA10复用为USART3
	
	//USART3端口配置
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_10; //GPIOA9与GPIOA10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//速度50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //上拉
	GPIO_Init(GPIOB,&GPIO_InitStructure); //初始化PB11，PB10

   //USART3 初始化设置
	USART_InitStructure.USART_BaudRate = baud;//波特率设置
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
	USART_Init(USART3, &USART_InitStructure); //初始化串口3
	
	USART_Cmd(USART3, ENABLE);  //使能串口3
	
	USART_ClearFlag(USART3, USART_FLAG_TC);
	

	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);//开启相关中断

	//Usart1 NVIC 配置
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;//串口1中断通道
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器、

	
}


void USART3_IRQHandler(void)                	//串口3中断服务程序
{
	uint8_t d=0;

	uint8_t usart3_flag_complete=0;				//串口3数据接收完毕标志位(未完成:0	完成:1)
	
	OS_ERR err;
	
	//进入中断
	OSIntEnter();    

	if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)  //接收中断(接收到的数据必须是0x0d 0x0a结尾)
	{
		//接收串口数据
		d=USART_ReceiveData(USART3);	
		
		g_usart3_buf[g_usart3_cnt] = d;
		
		g_usart3_cnt++;

		if(d == '#'|| g_usart3_cnt>= sizeof(g_usart3_buf))
		{
			usart3_flag_complete=1;
		}		
		
		
		//清空串口接收中断标志位
		USART_ClearITPendingBit(USART3, USART_IT_RXNE);
	} 
	
	//退出中断
	OSIntExit();  
	
	//发送消息队列
	if(usart3_flag_complete)
	{
		OSQPost(&g_queue_usart3, (void *)g_usart3_buf, g_usart3_cnt, OS_OPT_POST_FIFO, &err);
		
		g_usart3_cnt=0;
		usart3_flag_complete=0;
		
		if(err != OS_ERR_NONE)
		{
			printf("[USART3_IRQHandler]OSQPost error code %d\r\n",err);
		}
	
	}	
	
	  	
} 


 



