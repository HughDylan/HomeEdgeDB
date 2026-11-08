#ifndef __NRF24L01_H
#define __NRF24L01_H

#include "stm32f10x.h"

// 根据图1，CE 接 PB0，CSN 接 PA3
#define NRF_CE_LOW()    GPIO_ResetBits(GPIOB, GPIO_Pin_0)
#define NRF_CE_HIGH()   GPIO_SetBits(GPIOB, GPIO_Pin_0)
#define NRF_CSN_LOW()   GPIO_ResetBits(GPIOA, GPIO_Pin_3)
#define NRF_CSN_HIGH()  GPIO_SetBits(GPIOA, GPIO_Pin_3)

void NRF_Init(void);
uint8_t NRF_ReadReg(uint8_t reg);
void NRF_WriteReg(uint8_t reg, uint8_t val);

#endif
