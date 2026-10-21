#include "TimeBase.h"

static volatile uint32_t sys_tick_ms = 0;

void TimeBase_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    // 1. 开启 TIM2 时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    
    // 2. 定时器配置：72MHz主频，预分频7200，计数值10000，刚好 1ms 中断一次
    TIM_InitStructure.TIM_Period = 10000 - 1;
    TIM_InitStructure.TIM_Prescaler = 7200 - 1;
    TIM_InitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_InitStructure);
    
    // 3. 开启更新中断
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    
    // 4. 中断优先级配置
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    // 5. 启动定时器
    TIM_Cmd(TIM2, ENABLE);
}

// 提供给外部调用的接口，获取当前时间
uint32_t TimeBase_GetMs(void)
{
    return sys_tick_ms;
}

// TIM2 的中断服务函数
void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        sys_tick_ms++;
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }
}
