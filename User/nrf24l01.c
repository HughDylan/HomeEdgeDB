#include "nrf24l01.h"
#include "Delay.h"

// 硬件引脚宏定义
#define NRF_CSN_LOW()   GPIO_ResetBits(GPIOA, GPIO_Pin_3)
#define NRF_CSN_HIGH()  GPIO_SetBits(GPIOA, GPIO_Pin_3)
#define NRF_CE_LOW()    GPIO_ResetBits(GPIOB, GPIO_Pin_0)
#define NRF_CE_HIGH()   GPIO_SetBits(GPIOB, GPIO_Pin_0)

#define NRF_CMD_W_TX_PAYLOAD    0xA0
#define NRF_CMD_R_RX_PAYLOAD    0x61
#define NRF_CMD_FLUSH_TX        0xE1
#define NRF_CMD_FLUSH_RX        0xE2

static const uint8_t NRF_ADDR[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};

static uint8_t NRF_SPI_Swap(uint8_t byte) {
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) != SET);
    SPI_I2S_SendData(SPI1, byte);
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) != SET);
    return SPI_I2S_ReceiveData(SPI1);
}

static void NRF_WriteBuf(uint8_t reg, const uint8_t *buf, uint8_t len) {
    uint8_t i;
    NRF_CSN_LOW();
    NRF_SPI_Swap(reg | 0x20);
    for (i = 0; i < len; i++) NRF_SPI_Swap(buf[i]);
    NRF_CSN_HIGH();
}

static void NRF_ReadBuf(uint8_t reg, uint8_t *buf, uint8_t len) {
    uint8_t i;
    NRF_CSN_LOW();
    NRF_SPI_Swap(reg);
    for (i = 0; i < len; i++) buf[i] = NRF_SPI_Swap(0xFF);
    NRF_CSN_HIGH();
}

static void SPI1_Init_ForNRF(void) {
    GPIO_InitTypeDef g;
    SPI_InitTypeDef s;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_SPI1, ENABLE);

    g.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7; 
    g.GPIO_Mode = GPIO_Mode_AF_PP;
    g.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &g);

    g.GPIO_Pin = GPIO_Pin_6; 
    g.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &g);

    g.GPIO_Pin = GPIO_Pin_4; 
    g.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &g);
    GPIO_SetBits(GPIOA, GPIO_Pin_4);

    s.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    s.SPI_Mode = SPI_Mode_Master;
    s.SPI_DataSize = SPI_DataSize_8b;
    s.SPI_CPOL = SPI_CPOL_Low;
    s.SPI_CPHA = SPI_CPHA_1Edge;
    s.SPI_NSS = SPI_NSS_Soft;
    s.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32; 
    s.SPI_FirstBit = SPI_FirstBit_MSB;
    s.SPI_CRCPolynomial = 7;
    SPI_Init(SPI1, &s);
    SPI_Cmd(SPI1, ENABLE);
}

void NRF_Init(void) {
    GPIO_InitTypeDef g;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);

    g.GPIO_Pin = GPIO_Pin_3; 
    g.GPIO_Mode = GPIO_Mode_Out_PP;
    g.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &g);

    g.GPIO_Pin = GPIO_Pin_0; 
    GPIO_Init(GPIOB, &g);

    NRF_CSN_HIGH();
    NRF_CE_LOW();

    SPI1_Init_ForNRF();
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

void NRF_TxMode(void) {
    uint8_t cfg;
    NRF_CE_LOW();
    NRF_WriteReg(0x00, 0x0E); 
    NRF_WriteReg(0x01, 0x01); 
    NRF_WriteReg(0x02, 0x01); 
    NRF_WriteReg(0x03, 0x03); 
    NRF_WriteReg(0x04, 0x1A); 
    NRF_WriteReg(0x05, 0x4C); // ★ 频道设为 0x4C (76)
    NRF_WriteReg(0x06, 0x06); 
    
    NRF_WriteBuf(0x10, NRF_ADDR, 5); 
    NRF_WriteBuf(0x0A, NRF_ADDR, 5); 
    NRF_WriteReg(0x11, 0x20); 
    NRF_WriteReg(0x07, 0x70); 
    
    cfg = NRF_ReadReg(0x00); 
    cfg |= 0x02;             
    cfg &= ~0x01;            
    NRF_WriteReg(0x00, cfg);
    
    Delay_ms(2);
    NRF_CE_HIGH();
}

uint8_t NRF_RxMode(void)
{
    uint8_t cfg;
    uint8_t addr[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};

    NRF_CE_LOW();

    /* 清 FIFO */
    NRF_CSN_LOW();
    NRF_SPI_Swap(0xE2); /* FLUSH_RX */
    NRF_CSN_HIGH();

    NRF_WriteReg(0x00, 0x08); /* 先掉电，慢配置 */
    NRF_WriteReg(0x01, 0x01); /* EN_AA: 通道0自动ACK */
    NRF_WriteReg(0x02, 0x01); /* EN_RXADDR: 通道0 */
    NRF_WriteReg(0x03, 0x03); /* SETUP_AW: 5字节地址 */
    NRF_WriteReg(0x04, 0x1A); /* SETUP_RETR: 500us, 重发10次 */
    NRF_WriteReg(0x05, 0x4C); /* RF_CH: 频道76, 必须和发射板一样 */
    NRF_WriteReg(0x06, 0x06); /* RF_SETUP: 1Mbps, 0dBm, 必须一样 */
    NRF_WriteBuf(0x0A, addr, 5); /* RX_ADDR_P0: 地址必须和发射板TX_ADDR一样 */
    NRF_WriteReg(0x11, 0x20); /* RX_PW_P0: 32字节, 必须一样 */
    NRF_WriteReg(0x07, 0x70); /* 清标志 */

    /* ★ 按位操作：只改 bit1/bit0, CRC 位保持出厂值, 自动和发射板一致 */
    cfg = NRF_ReadReg(0x00);
    cfg |= 0x03; /* PWR_UP = 1, PRIM_RX = 1 */
    NRF_WriteReg(0x00, cfg);

    Delay_ms(2);
    NRF_CE_HIGH(); /* 常驻高 = 开始监听 */

    /* 写后自检 */
    if ((NRF_ReadReg(0x00) & 0x03) != 0x03) return 0x00;
    if (NRF_ReadReg(0x05) != 0x4C) return 0x05;
    if (NRF_ReadReg(0x06) != 0x06) return 0x06;
    if (NRF_ReadReg(0x11) != 0x20) return 0x11;

    return 0;
}

/* ==================== 发一包数据（带清空 FIFO 防护） ==================== */
uint8_t NRF_Send(const uint8_t *data, uint8_t len) {
    uint8_t st, i;
    uint32_t guard;

    if (len > 32) len = 32;
    NRF_CE_LOW();

    /* ★ 每次发之前清空 FIFO（极其关键！） */
    NRF_CSN_LOW();
    NRF_SPI_Swap(NRF_CMD_FLUSH_TX);
    NRF_CSN_HIGH();

    NRF_WriteReg(0x07, 0x70); // 清标志

    NRF_CSN_LOW();
    NRF_SPI_Swap(NRF_CMD_W_TX_PAYLOAD);
    for (i = 0; i < len; i++) NRF_SPI_Swap(data[i]);
    for (; i < 32; i++) NRF_SPI_Swap(0x00);
    NRF_CSN_HIGH();

    NRF_CE_HIGH();
    Delay_us(20);
    NRF_CE_LOW();

    guard = 0;
    while (1) {
        st = NRF_ReadReg(0x07);
        if (st & 0x20) { NRF_WriteReg(0x07, 0x20); return 0; } // 成功
        if (st & 0x10) { // MAX_RT
            NRF_WriteReg(0x07, 0x10);
            /* ★ 失败时再清一次 FIFO */
            NRF_CSN_LOW(); NRF_SPI_Swap(NRF_CMD_FLUSH_TX); NRF_CSN_HIGH(); 
            return 2;
        }
        if (++guard > 200000) return 3;
    }
}

uint8_t NRF_Recv(uint8_t *buf, uint8_t len) {
    uint8_t st = NRF_ReadReg(0x07);
    if ((st & 0x40) == 0) return 1;

    if (len > 32) len = 32;
    NRF_CE_LOW();
    NRF_ReadBuf(NRF_CMD_R_RX_PAYLOAD, buf, len);
    NRF_WriteReg(0x07, 0x40); // 清 RX_DR
    NRF_CE_HIGH();
    
    return 0; 
}
