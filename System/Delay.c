#include "stm32f10x.h"

/* 直接用地址访问 DWT，不依赖 core_cm3.h 里有没有定义 DWT 结构体 */
#define DEMCR_REG       (*(volatile uint32_t *)0xE000EDFCUL)
#define DWT_CTRL_REG    (*(volatile uint32_t *)0xE0001000UL)
#define DWT_CYCCNT_REG  (*(volatile uint32_t *)0xE0001004UL)

void Delay_Init(void)
{
    DEMCR_REG      |= (1UL << 24);   /* TRCENA：打开跟踪总开关 */
    DWT_CYCCNT_REG  = 0;             /* 计数器清零 */
    DWT_CTRL_REG   |= 1UL;           /* CYCCNTENA：开始计数 */
}

void Delay_us(uint32_t xus)
{
    uint32_t start = DWT_CYCCNT_REG;
    uint32_t ticks = xus * 72;       /* 72MHz → 1 微秒 = 72 个周期 */
    while ((DWT_CYCCNT_REG - start) < ticks);
}

void Delay_ms(uint32_t xms)
{
    while (xms--) Delay_us(1000);
}

void Delay_s(uint32_t xs)
{
    while (xs--) Delay_ms(1000);
}
