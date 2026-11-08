#ifndef __DHT22_H
#define __DHT22_H

#include "stm32f10x.h"

void DHT22_Init(void);
// 👇 名字必须叫 DHT_Read
uint8_t DHT_Read(float *temp, float *humi);
uint8_t DHT_ReadRaw(uint8_t *buf5);
#endif
