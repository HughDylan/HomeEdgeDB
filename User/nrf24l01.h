#ifndef __NRF24L01_H
#define __NRF24L01_H

#include "stm32f10x.h"

void NRF_Init(void);
uint8_t NRF_ReadReg(uint8_t reg);
void NRF_WriteReg(uint8_t reg, uint8_t val);

uint8_t NRF_TxMode(void);
uint8_t NRF_RxMode(void);  /* 返回 0 = 全部写入正确 */
uint8_t NRF_Send(const uint8_t *data, uint8_t len); /* 返回 0=成功 2=重发耗尽 3=超时 */
uint8_t NRF_Recv(uint8_t *buf, uint8_t len); /* 返回 0=收到 1=没收到 */

#endif
