#include "nrf24l01.h"
#include "Delay.h"

// 硬件引脚宏定义
#define NRF_CSN_LOW()   GPIO_ResetBits(GPIOA, GPIO_Pin_3)
#define NRF_CSN_HIGH()  GPIO_SetBits(GPIOA, GPIO_Pin_3)
#define NRF_CE_LOW()    GPIO_ResetBits(GPIOB, GPIO_Pin_0)
#define NRF_CE_HIGH()   GPIO_SetBits(GPIOB, GPIO_Pin_0)

// 底层 SPI 收发
static uint8_t NRF_SPI_Swap(uint8_t byte) {
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) != SET);
    SPI_I2S_SendData(SPI1, byte);
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) != SET);
    return SPI_I2S_ReceiveData(SPI1);
}

// 独立配置 SPI1（慢速、模式0，彻底摆脱 W25Q64 的影响）
static void SPI1_Init_ForNRF(void) {
    GPIO_InitTypeDef g;
    SPI_InitTypeDef s;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_SPI1, ENABLE);

    /* ① SCK(PA5) 和 MOSI(PA7)：复用推挽输出 */
    g.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
    g.GPIO_Mode = GPIO_Mode_AF_PP;
    g.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &g);

    /* ② MISO(PA6)：上拉输入 */
    g.GPIO_Pin = GPIO_Pin_6;
    g.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &g);

    /* ③ ★关键：PA4 是 W25Q64 的 CS，明确拉高，别让它抢 MISO */
    g.GPIO_Pin = GPIO_Pin_4;
    g.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &g);
    GPIO_SetBits(GPIOA, GPIO_Pin_4); // 高 = W25Q64 不选中

    /* ④ SPI1 参数：慢速、模式0 */
    s.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    s.SPI_Mode = SPI_Mode_Master;
    s.SPI_DataSize = SPI_DataSize_8b;
    s.SPI_CPOL = SPI_CPOL_Low;
    s.SPI_CPHA = SPI_CPHA_1Edge;
    s.SPI_NSS = SPI_NSS_Soft;
    s.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32; // 72/32 = 2.25MHz
    s.SPI_FirstBit = SPI_FirstBit_MSB;
    s.SPI_CRCPolynomial = 7;
    SPI_Init(SPI1, &s);

    SPI_Cmd(SPI1, ENABLE);
}

void NRF_Init(void) {
    GPIO_InitTypeDef g;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);

    /* CSN = PA3 */
    g.GPIO_Pin = GPIO_Pin_3;
    g.GPIO_Mode = GPIO_Mode_Out_PP;
    g.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &g);

    /* CE = PB0 */
    g.GPIO_Pin = GPIO_Pin_0;
    GPIO_Init(GPIOB, &g);

    NRF_CSN_HIGH();
    NRF_CE_LOW();

    SPI1_Init_ForNRF(); // ★ 自己配 SPI，不依赖 W25Q64
    Delay_ms(100);
}

uint8_t NRF_ReadReg(uint8_t reg) {
    uint8_t val;
    NRF_CSN_LOW();
    NRF_SPI_Swap(reg);
    val = NRF_SPI_Swap(0xFF);
    NRF_CSN_HIGH();
    return val;
}

void NRF_WriteReg(uint8_t reg, uint8_t val) {
    NRF_CSN_LOW();
    NRF_SPI_Swap(reg | 0x20);
    NRF_SPI_Swap(val);
    NRF_CSN_HIGH();
}
