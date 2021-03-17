#ifndef __DHT11_H__
#define __DHT11_H__
#include "sys.h"
#include "delay.h"
void dht11_init(void);
int8_t dht11_read_data(uint8_t *pbuf);



#endif
