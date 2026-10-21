#include "TimeBase.h"

static volatile uint32_t sys_tick_ms = 0;

void TimeBase_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    TIM_InitStructure.TIM_Period = 10000 - 1;
    TIM_InitStructure.TIM_Prescaler = 7200 - 1;
    TIM_InitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_InitStructure);
    
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    TIM_Cmd(TIM2, ENABLE);
}

uint32_t TimeBase_GetMs(void) { return sys_tick_ms; }

void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        sys_tick_ms++;
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }
}
