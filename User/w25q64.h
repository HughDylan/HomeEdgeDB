#ifndef __W25Q64_H
#define __W25Q64_H

#include "stm32f10x.h"
#include <stdint.h>

void W25Q64_Init(void);
uint32_t W25Q64_ReadID(void);
void W25Q64_Read(uint32_t addr, uint8_t *buf, uint32_t len);
void W25Q64_Write(uint32_t addr, const uint8_t *buf, uint32_t len);
void W25Q64_EraseSector(uint32_t addr);

#endif
