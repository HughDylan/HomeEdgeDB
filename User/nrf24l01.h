#ifndef __NRF24L01_H
#define __NRF24L01_H

#include "stm32f10x.h"

void NRF_Init(void);
uint8_t NRF_ReadReg(uint8_t reg);
void NRF_WriteReg(uint8_t reg, uint8_t val);

// 新增：收发模式切换与收发函数
void NRF_TxMode(void);
uint8_t NRF_RxMode(void);
uint8_t NRF_Send(const uint8_t *data, uint8_t len);
uint8_t NRF_Recv(uint8_t *buf, uint8_t len);

#endif
