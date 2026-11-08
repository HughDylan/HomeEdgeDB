#include "OLED.h"
#include "Delay.h"

#define OLED_GPIO     GPIOB
#define OLED_SCL      GPIO_Pin_8
#define OLED_SDA      GPIO_Pin_9

/* ?? 0~9 ???,???? 5 ?,?? 8 ?(bit0 ????) */
static const uint8_t NUM_FONT[10][5] = {
    {0x3E,0x51,0x49,0x45,0x3E},   /* 0 */
    {0x00,0x42,0x7F,0x40,0x00},   /* 1 */
    {0x42,0x61,0x51,0x49,0x46},   /* 2 */
    {0x21,0x41,0x45,0x4B,0x31},   /* 3 */
    {0x18,0x14,0x12,0x7F,0x10},   /* 4 */
    {0x27,0x45,0x45,0x45,0x39},   /* 5 */
    {0x3C,0x4A,0x49,0x49,0x30},   /* 6 */
    {0x01,0x71,0x09,0x05,0x03},   /* 7 */
    {0x36,0x49,0x49,0x49,0x36},   /* 8 */
    {0x06,0x49,0x49,0x29,0x1E},   /* 9 */
};

static void I2C_Delay(void)
{
    volatile uint8_t i = 30;
    while (i--);
}

static void OLED_GPIO_Init(void)
{
    GPIO_InitTypeDef s;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    s.GPIO_Mode  = GPIO_Mode_Out_OD;
    s.GPIO_Speed = GPIO_Speed_50MHz;
    s.GPIO_Pin   = OLED_SCL | OLED_SDA;
    GPIO_Init(OLED_GPIO, &s);

    GPIO_SetBits(OLED_GPIO, OLED_SCL | OLED_SDA);
    Delay_ms(100);
}

static void I2C_Start(void)
{
    GPIO_SetBits(OLED_GPIO, OLED_SDA);
    GPIO_SetBits(OLED_GPIO, OLED_SCL);
    I2C_Delay();
    GPIO_ResetBits(OLED_GPIO, OLED_SDA);
    I2C_Delay();
    GPIO_ResetBits(OLED_GPIO, OLED_SCL);
}

static void I2C_Stop(void)
{
    GPIO_ResetBits(OLED_GPIO, OLED_SDA);
    I2C_Delay();
    GPIO_SetBits(OLED_GPIO, OLED_SCL);
    I2C_Delay();
    GPIO_SetBits(OLED_GPIO, OLED_SDA);
    I2C_Delay();
}

static void I2C_SendByte(uint8_t byte)
{
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        if (byte & 0x80) GPIO_SetBits(OLED_GPIO, OLED_SDA);
        else             GPIO_ResetBits(OLED_GPIO, OLED_SDA);
        byte <<= 1;
        I2C_Delay();
        GPIO_SetBits(OLED_GPIO, OLED_SCL);
        I2C_Delay();
        GPIO_ResetBits(OLED_GPIO, OLED_SCL);
    }
    /* ? 9 ?:?? SDA,???(?????,????) */
    GPIO_SetBits(OLED_GPIO, OLED_SDA);
    I2C_Delay();
    GPIO_SetBits(OLED_GPIO, OLED_SCL);
    I2C_Delay();
    GPIO_ResetBits(OLED_GPIO, OLED_SCL);
}

void OLED_WriteCmd(uint8_t cmd)
{
    I2C_Start();
    I2C_SendByte(0x78);      /* 0x3C << 1 */
    I2C_SendByte(0x00);      /* 0x00 = ?? */
    I2C_SendByte(cmd);
    I2C_Stop();
}

void OLED_WriteData(uint8_t dat)
{
    I2C_Start();
    I2C_SendByte(0x78);
    I2C_SendByte(0x40);      /* 0x40 = ?? */
    I2C_SendByte(dat);
    I2C_Stop();
}

void OLED_SetPos(uint8_t page, uint8_t x)
{
    OLED_WriteCmd(0xB0 + page);            /* ???? */
    OLED_WriteCmd(0x00 + (x & 0x0F));      /* ???? 4 ? */
    OLED_WriteCmd(0x10 + ((x >> 4) & 0x0F));/* ???? 4 ? */
}

void OLED_Clear(void)
{
    uint8_t p, i;
    for (p = 0; p < 8; p++)
    {
        OLED_SetPos(p, 0);
        for (i = 0; i < 128; i++) OLED_WriteData(0x00);
    }
}

void OLED_Init(void)
{
    OLED_GPIO_Init();
    Delay_ms(200);

    OLED_WriteCmd(0xAE);
    OLED_WriteCmd(0xD5); OLED_WriteCmd(0x80);
    OLED_WriteCmd(0xA8); OLED_WriteCmd(0x3F);
    OLED_WriteCmd(0xD3); OLED_WriteCmd(0x00);
    OLED_WriteCmd(0x40);
    OLED_WriteCmd(0x8D); OLED_WriteCmd(0x14);   /* ???,??? */
    OLED_WriteCmd(0x20); OLED_WriteCmd(0x02);
    OLED_WriteCmd(0xA1);
    OLED_WriteCmd(0xC8);
    OLED_WriteCmd(0xDA); OLED_WriteCmd(0x12);
    OLED_WriteCmd(0x81); OLED_WriteCmd(0xCF);
    OLED_WriteCmd(0xD9); OLED_WriteCmd(0xF1);
    OLED_WriteCmd(0xDB); OLED_WriteCmd(0x40);
    OLED_WriteCmd(0xA4);
    OLED_WriteCmd(0xA6);
    OLED_WriteCmd(0xAF);

    OLED_Clear();
}

/* ????:page 0~7,x ??,digits ??? */
void OLED_ShowNumber(uint8_t page, uint8_t x, uint32_t num, uint8_t digits)
{
    uint8_t i, d;
    uint8_t buf[10];

    if (digits > 10) digits = 10;

    for (i = 0; i < digits; i++)
    {
        buf[digits - 1 - i] = num % 10;
        num /= 10;
    }

    OLED_SetPos(page, x);
    for (i = 0; i < digits; i++)
    {
        d = buf[i];
        OLED_WriteData(NUM_FONT[d][0]);
        OLED_WriteData(NUM_FONT[d][1]);
        OLED_WriteData(NUM_FONT[d][2]);
        OLED_WriteData(NUM_FONT[d][3]);
        OLED_WriteData(NUM_FONT[d][4]);
        OLED_WriteData(0x00);           /* ???????? */
    }
}
