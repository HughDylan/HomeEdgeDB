#ifndef __SENSOR_H
#define __SENSOR_H

#include "stm32f10x.h"

void Sensor_Init(void);
void Sensor_Update(void);
float Sensor_GetTemp(void);
uint8_t Sensor_GetLight(void);

#endif
