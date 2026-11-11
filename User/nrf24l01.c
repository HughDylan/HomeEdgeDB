#include "nrf24l01.h"
#include "Delay.h"

#define NRF_CSN_LOW()   GPIO_ResetBits(GPIOA, GPIO_Pin_3)
#define NRF_CSN_HIGH()  GPIO_SetBits(GPIOA, GPIO_Pin_3)
#define NRF_CE_LOW()    GPIO_ResetBits(GPIOB, GPIO_Pin_0)
#define NRF_CE_HIGH()   GPIO_SetBits(GPIOB, GPIO_Pin_0)

/* ---------- SPI1 换一个字节 ---------- */
static uint8_t NRF_SPI_Swap(uint8_t byte)
{
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) != SET);
    SPI_I2S_SendData(SPI1, byte);
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) != SET);
    return SPI_I2S_ReceiveData(SPI1);
}

/* ---------- 独立配 SPI1：2.25MHz + PA4 强制拉高 ---------- */
static void SPI1_Init_ForNRF(void)
{
    GPIO_InitTypeDef g;
    SPI_InitTypeDef  s;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_SPI1, ENABLE);

    g.GPIO_Pin   = GPIO_Pin_5 | GPIO_Pin_7;      /* SCK / MOSI */
    g.GPIO_Mode  = GPIO_Mode_AF_PP;
    g.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &g);

    g.GPIO_Pin  = GPIO_Pin_6;                    /* MISO */
    g.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &g);

    g.GPIO_Pin  = GPIO_Pin_4;                    /* PA4 = W25Q64 CS，钉高防抢 */
    g.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &g);
    GPIO_SetBits(GPIOA, GPIO_Pin_4);

    s.SPI_Direction         = SPI_Direction_2Lines_FullDuplex;
    s.SPI_Mode              = SPI_Mode_Master;
    s.SPI_DataSize          = SPI_DataSize_8b;
    s.SPI_CPOL              = SPI_CPOL_Low;
    s.SPI_CPHA              = SPI_CPHA_1Edge;
    s.SPI_NSS               = SPI_NSS_Soft;
    s.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32;   /* 72/32 = 2.25MHz */
    s.SPI_FirstBit          = SPI_FirstBit_MSB;
    s.SPI_CRCPolynomial     = 7;
    SPI_Init(SPI1, &s);
    SPI_Cmd(SPI1, ENABLE);
}

/* ---------- 初始化引脚 ---------- */
void NRF_Init(void)
{
    GPIO_InitTypeDef g;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);

    g.GPIO_Pin   = GPIO_Pin_3;                   /* CSN */
    g.GPIO_Mode  = GPIO_Mode_Out_PP;
    g.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &g);

    g.GPIO_Pin = GPIO_Pin_0;                     /* CE */
    GPIO_Init(GPIOB, &g);

    NRF_CSN_HIGH();
    NRF_CE_LOW();

    SPI1_Init_ForNRF();
    Delay_ms(100);
}

/* ---------- 读 / 写单个寄存器 ---------- */
uint8_t NRF_ReadReg(uint8_t reg)
{
    uint8_t val;
    NRF_CSN_LOW();
    NRF_SPI_Swap(reg);
    val = NRF_SPI_Swap(0xFF);
    NRF_CSN_HIGH();
    return val;
}

void NRF_WriteReg(uint8_t reg, uint8_t val)
{
    NRF_CSN_LOW();
    NRF_SPI_Swap(reg | 0x20);
    NRF_SPI_Swap(val);
    NRF_CSN_HIGH();
}

/* ---------- 读 / 写多字节缓冲区 ---------- */
static void NRF_WriteBuf(uint8_t reg, const uint8_t *buf, uint8_t len)
{
    uint8_t i;
    NRF_CSN_LOW();
    NRF_SPI_Swap(reg | 0x20);
    for (i = 0; i < len; i++) NRF_SPI_Swap(buf[i]);
    NRF_CSN_HIGH();
}

static void NRF_ReadBuf(uint8_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t i;
    NRF_CSN_LOW();
    NRF_SPI_Swap(reg);
    for (i = 0; i < len; i++) buf[i] = NRF_SPI_Swap(0xFF);
    NRF_CSN_HIGH();
}

/* ---------- 公共参数：两块板必须一致 ---------- */
static const uint8_t NRF_ADDR[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
#define NRF_CHANNEL   0x4C        /* 76 */
#define NRF_RF_SETUP  0x06        /* 1Mbps, 0dBm */
#define NRF_PLOAD_W   0x20        /* 32 字节 */

/* ================= 发送模式 ================= */
uint8_t NRF_TxMode(void)
{
    uint8_t addr[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
    uint8_t cfg;

    NRF_CE_LOW();

    NRF_CSN_LOW(); NRF_SPI_Swap(0xE1); NRF_CSN_HIGH();   /* FLUSH_TX */

    NRF_WriteReg(0x00, 0x08);            /* 先掉电 */
    NRF_WriteReg(0x01, 0x01);            /* EN_AA 通道0 */
    NRF_WriteReg(0x02, 0x01);            /* EN_RXADDR 通道0 */
    NRF_WriteReg(0x03, 0x03);            /* 5 字节地址 */
    NRF_WriteReg(0x04, 0x1A);            /* 500us x 10 次重发 */
    NRF_WriteReg(0x05, NRF_CHANNEL);
    NRF_WriteReg(0x06, NRF_RF_SETUP);
    NRF_WriteReg(0x11, NRF_PLOAD_W);

    NRF_WriteBuf(0x10, addr, 5);         /* TX_ADDR */
    NRF_WriteBuf(0x0A, addr, 5);         /* RX_ADDR_P0 必须与 TX_ADDR 相同 */

    NRF_WriteReg(0x07, 0x70);            /* 清标志 */

    /* ★ 位操作：只改 bit1/bit0，CRC 位保持出厂值 */
    cfg = NRF_ReadReg(0x00);
    cfg |=  0x02;                        /* PWR_UP = 1 */
    cfg &= ~0x01;                        /* PRIM_RX = 0 */
    NRF_WriteReg(0x00, cfg);
    Delay_ms(2);

    /* 写后自检 */
    if ((NRF_ReadReg(0x00) & 0x03) != 0x02) return 0x00;
    if ( NRF_ReadReg(0x05) != NRF_CHANNEL)  return 0x05;
    if ( NRF_ReadReg(0x06) != NRF_RF_SETUP) return 0x06;
    if ( NRF_ReadReg(0x11) != NRF_PLOAD_W)  return 0x11;

    return 0;
}

/* ================= 接收模式 ================= */
uint8_t NRF_RxMode(void)
{
    uint8_t addr[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
    uint8_t cfg;

    NRF_CE_LOW();

    NRF_CSN_LOW(); NRF_SPI_Swap(0xE2); NRF_CSN_HIGH();   /* FLUSH_RX */

    NRF_WriteReg(0x00, 0x08);            /* 先掉电 */
    NRF_WriteReg(0x01, 0x01);
    NRF_WriteReg(0x02, 0x01);
    NRF_WriteReg(0x03, 0x03);
    NRF_WriteReg(0x05, NRF_CHANNEL);
    NRF_WriteReg(0x06, NRF_RF_SETUP);
    NRF_WriteReg(0x11, NRF_PLOAD_W);

    NRF_WriteBuf(0x0A, addr, 5);         /* RX_ADDR_P0 */

    NRF_WriteReg(0x07, 0x70);

     /* 显式写死，不依赖出厂值:
bit0 PRIM_RX=1 接收 / bit1 PWR_UP=1 上电
bit2 CRC0=1  2字节CRC <- 必须和发射板的 0x0E 一致
bit3 EN_CRC=1  CRC使能
*/
NRF_WriteReg(0x00, 0x0F);
Delay_ms(2);

    NRF_CE_HIGH();                       /* 常驻高 = 开始监听 */

    /* 写后自检 */
    if ((NRF_ReadReg(0x00) & 0x0F) != 0x0F) return 0xEE;
    if ( NRF_ReadReg(0x05) != NRF_CHANNEL)  return 0x05;
    if ( NRF_ReadReg(0x06) != NRF_RF_SETUP) return 0x06;
    if ( NRF_ReadReg(0x11) != NRF_PLOAD_W)  return 0x11;

    return 0;
}

/* ================= 发一包 ================= */
uint8_t NRF_Send(const uint8_t *data, uint8_t len)
{
    uint8_t st, i;
    uint32_t guard;

    if (len > 32) len = 32;

    NRF_CE_LOW();

    NRF_CSN_LOW(); NRF_SPI_Swap(0xE1); NRF_CSN_HIGH();   /* 防 FIFO 死锁 */
    NRF_WriteReg(0x07, 0x70);

    NRF_CSN_LOW();
    NRF_SPI_Swap(0xA0);                                  /* W_TX_PAYLOAD */
    for (i = 0; i < len; i++) NRF_SPI_Swap(data[i]);
    for (; i < 32; i++)       NRF_SPI_Swap(0x00);
    NRF_CSN_HIGH();

    NRF_CE_HIGH();
    Delay_us(20);
    NRF_CE_LOW();

    guard = 0;
    while (1)
    {
        st = NRF_ReadReg(0x07);
        if (st & 0x20) { NRF_WriteReg(0x07, 0x20); return 0; }   /* TX_DS */
        if (st & 0x10)                                            /* MAX_RT */
        {
            NRF_WriteReg(0x07, 0x10);
            NRF_CSN_LOW(); NRF_SPI_Swap(0xE1); NRF_CSN_HIGH();
            return 2;
        }
        if (++guard > 200000) return 3;
    }
}

/* ================= 收一包 ================= */
uint8_t NRF_Recv(uint8_t *buf, uint8_t len)
{
    uint8_t st, i;

    st = NRF_ReadReg(0x07);
    if ((st & 0x40) == 0) return 1;      /* RX_DR 没置位 = 没收到 */

    if (len > 32) len = 32;

    NRF_CSN_LOW();
    NRF_SPI_Swap(0x61);                  /* R_RX_PAYLOAD */
    for (i = 0; i < len; i++) buf[i] = NRF_SPI_Swap(0xFF);
    NRF_CSN_HIGH();

    NRF_WriteReg(0x07, 0x40);            /* 清 RX_DR */
    return 0;
}
