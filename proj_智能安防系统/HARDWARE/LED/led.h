#ifndef __LED_H
#define __LED_H

#include "stm32f4xx.h" 
void LED_Init(void);//≥ı ºªØ

#define LED1 PFout(9)
#define LED2 PFout(10)
#define LED3 PEout(13)
#define LED4 PEout(14)

#endif
