#ifndef __OLED_H
#define __OLED_H

#include "stm32f10x.h"

void OLED_Init(void);
void OLED_Clear(void);
void OLED_SetPos(uint8_t page, uint8_t x);
void OLED_ShowNumber(uint8_t page, uint8_t x, uint32_t num, uint8_t digits);
void OLED_WriteCmd(uint8_t cmd);
void OLED_WriteData(uint8_t dat);
#endif
