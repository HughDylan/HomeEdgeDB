#ifndef __AD_H
#define __AD_H
#include "stm32f10x.h"

extern uint16_t AD_Value[4];

void AD_Init(void);
//注意这里：加了参数ADC_Channel,用来告诉函数都那个通道
uint16_t AD_GetValue(uint8_t ADC_Channel);
#endif
