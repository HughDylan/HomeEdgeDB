#include "w25q64.h"
#include "Delay.h"

#define W25Q_CS_LOW()   GPIO_ResetBits(GPIOA, GPIO_Pin_4)
#define W25Q_CS_HIGH()  GPIO_SetBits(GPIOA, GPIO_Pin_4)

// SPI1 发送并接收一个字节
static uint8_t SPI_SwapByte(uint8_t byte) {
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(SPI1, byte);
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
    return SPI_I2S_ReceiveData(SPI1);
}

void W25Q64_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef SPI_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_SPI1, ENABLE);

    // PA4 (CS), PA5 (SCK), PA7 (MOSI)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // PA6 (MISO)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 上拉输入
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4; 
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(SPI1, &SPI_InitStructure);

    SPI_Cmd(SPI1, ENABLE);
    W25Q_CS_HIGH();
}

static void W25Q_WaitBusy(void) {
    W25Q_CS_LOW();
    SPI_SwapByte(0x05);
    while ((SPI_SwapByte(0xFF) & 0x01) == 0x01);
    W25Q_CS_HIGH();
}

static void W25Q_WriteEnable(void) {
    W25Q_CS_LOW();
    SPI_SwapByte(0x06);
    W25Q_CS_HIGH();
}

uint32_t W25Q64_ReadID(void) {
    uint32_t id = 0;
    W25Q_CS_LOW();
    SPI_SwapByte(0x9F);
    id |= SPI_SwapByte(0xFF) << 16;
    id |= SPI_SwapByte(0xFF) << 8;
    id |= SPI_SwapByte(0xFF);
    W25Q_CS_HIGH();
    return id;
}

void W25Q64_Read(uint32_t addr, uint8_t *buf, uint32_t len) {
    W25Q_CS_LOW();
    SPI_SwapByte(0x03);
    SPI_SwapByte(addr >> 16);
    SPI_SwapByte(addr >> 8);
    SPI_SwapByte(addr);
    while (len--) *buf++ = SPI_SwapByte(0xFF);
    W25Q_CS_HIGH();
}

void W25Q64_Write(uint32_t addr, const uint8_t *buf, uint32_t len) {
    while (len > 0) {
        uint32_t page_remain = 256 - (addr % 256);
        uint32_t write_len = (len < page_remain) ? len : page_remain;
        W25Q_WriteEnable();
        W25Q_CS_LOW();
        SPI_SwapByte(0x02);
        SPI_SwapByte(addr >> 16);
        SPI_SwapByte(addr >> 8);
        SPI_SwapByte(addr);
        for (uint32_t i = 0; i < write_len; i++) SPI_SwapByte(buf[i]);
        W25Q_CS_HIGH();
        W25Q_WaitBusy();
        addr += write_len; buf += write_len; len -= write_len;
    }
}

void W25Q64_EraseSector(uint32_t addr) {
    W25Q_WriteEnable();
    W25Q_CS_LOW();
    SPI_SwapByte(0x20);
    SPI_SwapByte(addr >> 16);
    SPI_SwapByte(addr >> 8);
    SPI_SwapByte(addr);
    W25Q_CS_HIGH();
    W25Q_WaitBusy();
}
