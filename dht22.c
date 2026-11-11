#include "dht22.h"
#include "Delay.h"

#define DHT_PORT GPIOB
#define DHT_PIN  GPIO_Pin_1

// 切换输出模式
static void DHT_SetOut(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = DHT_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD; 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT_PORT, &GPIO_InitStructure);
}

// 切换输入模式
static void DHT_SetIn(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = DHT_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT_PORT, &GPIO_InitStructure);
}

void DHT22_Init(void) {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    DHT_SetOut();
    GPIO_SetBits(DHT_PORT, DHT_PIN); 
}

// 读一个字节，带超时保护
static uint8_t DHT_ReadByte(uint8_t *ok) {
    uint8_t i, dat = 0;
    uint32_t guard;
    
    for (i = 0; i < 8; i++) {
        guard = 0;
        while (GPIO_ReadInputDataBit(DHT_PORT, DHT_PIN) == Bit_RESET) {
            if (++guard > 30000) { *ok = 0; return 0; }
        }
        Delay_us(40);
        dat <<= 1;
        if (GPIO_ReadInputDataBit(DHT_PORT, DHT_PIN) == Bit_SET) {
            dat |= 1;
            guard = 0;
            while (GPIO_ReadInputDataBit(DHT_PORT, DHT_PIN) == Bit_SET) {
                if (++guard > 30000) { *ok = 0; return 0; }
            }
        }
    }
    return dat;
}

// 读取温湿度主函数
uint8_t DHT_Read(float *temp, float *humi) {
    uint8_t buf[5] = {0};
    uint8_t i, ok = 1;
    uint32_t guard;

    DHT_SetOut();
    GPIO_ResetBits(DHT_PORT, DHT_PIN);
    Delay_ms(2); 
    GPIO_SetBits(DHT_PORT, DHT_PIN);
    Delay_us(30);
    
    DHT_SetIn();
    
    guard = 0;
    while (GPIO_ReadInputDataBit(DHT_PORT, DHT_PIN) == Bit_SET) {
        if (++guard > 10000) return 1; 
    }
    guard = 0;
    while (GPIO_ReadInputDataBit(DHT_PORT, DHT_PIN) == Bit_RESET) {
        if (++guard > 10000) return 2; 
    }
    guard = 0;
    while (GPIO_ReadInputDataBit(DHT_PORT, DHT_PIN) == Bit_SET) {
        if (++guard > 10000) return 3; 
    }

    for (i = 0; i < 5; i++) {
        buf[i] = DHT_ReadByte(&ok);
        if (!ok) return 4;
    }

    if ((uint8_t)(buf[0] + buf[1] + buf[2] + buf[3]) != buf[4]) return 5; 

    *humi = (float)(((uint16_t)buf[0] << 8) | buf[1]) / 10.0f;
    if (buf[2] & 0x80) *temp = -(float)(((uint16_t)buf[2] & 0x7F) << 8 | buf[3]) / 10.0f;
    else               *temp = (float)(((uint16_t)buf[2] << 8) | buf[3]) / 10.0f;

    Delay_us(50);
    return 0;
}
