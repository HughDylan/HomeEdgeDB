#ifndef __DHT22_H
#define __DHT22_H

#include "stm32f10x.h"

void DHT22_Init(void);
uint8_t DHT_Read(float *temp, float *humi);

#endif
