#include "stm32f10x.h"

void LED_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; // 只用 PA2
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 默认关灯
    GPIO_SetBits(GPIOA, GPIO_Pin_2);
}

// 开灯（模拟继电器吸合）
void LED_ON(void)
{
    GPIO_ResetBits(GPIOA, GPIO_Pin_2);
}

// 关灯（模拟继电器断开）
void LED_OFF(void)
{
    GPIO_SetBits(GPIOA, GPIO_Pin_2);
}
