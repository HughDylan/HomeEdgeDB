#include "nrf24l01.h"
#include "w25q64.h" // 为了借用 SPI_SwapByte
#include "Delay.h"

/* 初始化 CE( PB0 ) 和 CSN( PA3 ) */
void NRF_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 开启 GPIOA 和 GPIOB 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);
    
    // CSN(PA3) 和 CE(PB0) 配为推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // 初始状态：CSN拉高（释放总线），CE拉低（待机）
    NRF_CSN_HIGH();
    NRF_CE_LOW();
    Delay_ms(100); // 等模块上电稳定
}

/* 读寄存器 */
uint8_t NRF_ReadReg(uint8_t reg) {
    uint8_t val;
    NRF_CSN_LOW();
    SPI_SwapByte(reg);             // 发寄存器地址（读命令）
    val = SPI_SwapByte(0xFF);      // 读回数据
    NRF_CSN_HIGH();
    return val;
}

/* 写寄存器 */
void NRF_WriteReg(uint8_t reg, uint8_t val) {
    NRF_CSN_LOW();
    SPI_SwapByte(reg | 0x20);      // 发寄存器地址 | 0x20（写命令）
    SPI_SwapByte(val);             // 发数据
    NRF_CSN_HIGH();
}
