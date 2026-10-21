#ifndef __SENSOR_H
#define __SENSOR_H

#include "stm32f10x.h"

void Sensor_Init(void);
void Sensor_Update(void);        /* 每 1 秒调一次 */

float Sensor_GetTemp(void);      /* 摄氏度 */
uint8_t Sensor_GetLight(void);   /* 0~100 百分比 */
uint16_t Sensor_GetRawTemp(void);/* 获取温度原始ADC值 */

#endif
