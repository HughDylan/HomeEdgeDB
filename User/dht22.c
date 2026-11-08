#include "dht22.h"
#include "Delay.h"

// ⚠️ 根据图2，引脚定义在 PB1
#define DHT_PORT GPIOB
#define DHT_PIN  GPIO_Pin_1

// 切换输出模式 (开漏输出，防止总线冲突)
static void DHT_SetOut(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = DHT_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD; 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT_PORT, &GPIO_InitStructure);
}

// 切换输入模式 (上拉输入，空闲时保持高电平)
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

// 读一个字节 (8位)，带超时保护
static uint8_t DHT_ReadByte(uint8_t *ok) {
    uint8_t i, dat = 0;
    uint32_t guard;
    
    for (i = 0; i < 8; i++) {
        guard = 0;
        // ① 等待低电平结束
        while (GPIO_ReadInputDataBit(DHT_PORT, DHT_PIN) == Bit_RESET) {
            if (++guard > 30000) { *ok = 0; return 0; }
        }
        
        // ② 高电平开始后 40us 采样
        Delay_us(40);
        
        dat <<= 1;
        // 如果 40us 后还是高电平，说明这1位是1 (高电平持续70us)
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

// 主函数：读一次温湿度
uint8_t DHT_Read(float *temp, float *humi) {
    uint8_t buf[5] = {0};
    uint8_t i, ok = 1;
    uint32_t guard;

    // 1. 主机发送命令：拉低至少 2ms 后释放
    DHT_SetOut();
    GPIO_ResetBits(DHT_PORT, DHT_PIN);
    Delay_ms(2); 
    GPIO_SetBits(DHT_PORT, DHT_PIN);
    Delay_us(40);
    
    // 2. 切换为输入，等模块应答
    DHT_SetIn();
    
    guard = 0;
    while (GPIO_ReadInputDataBit(DHT_PORT, DHT_PIN) == Bit_SET) {
        if (++guard > 10000) return 1; // 一直高，模块没应答
    }
    
    guard = 0;
    while (GPIO_ReadInputDataBit(DHT_PORT, DHT_PIN) == Bit_RESET) {
        if (++guard > 10000) return 2; // 应答的低电平超时
    }
    
    guard = 0;
    while (GPIO_ReadInputDataBit(DHT_PORT, DHT_PIN) == Bit_SET) {
        if (++guard > 10000) return 3; // 应答的高电平超时
    }

    // 3. 读 40 位 = 5 个字节
    for (i = 0; i < 5; i++) {
        buf[i] = DHT_ReadByte(&ok);
        if (!ok) return 4;
    }

    // 4. 校验：前 4 字节相加的低 8 位 == 第 5 字节
    if ((uint8_t)(buf[0] + buf[1] + buf[2] + buf[3]) != buf[4]) {
        return 5; 
    }

    // 5. 换算
    *humi = (float)(((uint16_t)buf[0] << 8) | buf[1]) / 10.0f;
    
    // 最高位是符号位
    if (buf[2] & 0x80) {
        *temp = -(float)(((uint16_t)buf[2] & 0x7F) << 8 | buf[3]) / 10.0f;
    } else {
        *temp = (float)(((uint16_t)buf[2] << 8) | buf[3]) / 10.0f;
    }

    Delay_us(50); // 让总线回到空闲高电平
    return 0;
}
/* 只读 5 个原始字节，不做校验，专门用来调试 */
uint8_t DHT_ReadRaw(uint8_t *buf5) {
    uint8_t i, ok;
    uint32_t guard;

    DHT_SetOut();
    GPIO_ResetBits(DHT_PORT, DHT_PIN);
    Delay_ms(2);
    GPIO_SetBits(DHT_PORT, DHT_PIN);
    Delay_us(30);

    DHT_SetIn();

    guard = 0;
    while (GPIO_ReadInputDataBit(DHT_PORT, DHT_PIN) == Bit_SET)
        if (++guard > 100000) return 1;
        
    guard = 0;
    while (GPIO_ReadInputDataBit(DHT_PORT, DHT_PIN) == Bit_RESET)
        if (++guard > 100000) return 2;
        
    guard = 0;
    while (GPIO_ReadInputDataBit(DHT_PORT, DHT_PIN) == Bit_SET)
        if (++guard > 100000) return 3;

    for (i = 0; i < 5; i++) {
        buf5[i] = DHT_ReadByte(&ok);
        if (!ok) return 4;
    }
    
    Delay_us(50);
    return 0;
}
