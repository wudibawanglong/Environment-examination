#ifndef __BEEP_H__
#define __BEEP_H__
#include "stm32f4xx.h" 
#include "sys.h"

void BEEP_Init(void);//初始化

#define BEEP PFout(8)

#endif
