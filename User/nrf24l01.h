#ifndef __NRF24L01_H
#define __NRF24L01_H

#include "stm32f10x.h"

void NRF_Init(void);
uint8_t NRF_ReadReg(uint8_t reg);
void NRF_WriteReg(uint8_t reg, uint8_t val);

#endif
