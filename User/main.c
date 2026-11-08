#include "stm32f10x.h"
#include <stdio.h>
#include "Delay.h"
#include "AD.h"
#include "OLED.h"
#include "DHT22.h" // 注意这里大写，保证和你的文件名一致

int main(void) {
    uint8_t err;
    float t = 0.0f, h = 0.0f;
    int t10, h10;
    uint16_t raw;
    char line[20];

    Delay_Init();
    AD_Init();
    OLED_Init();
    OLED_Clear();

    OLED_ShowString(2, 1, "HomeEdgeDB");

    while (1) {
        /* ---- DHT22: 关中断，保护微秒时序 ---- */
        __disable_irq();
        err = DHT_Read(&t, &h);
        __enable_irq();

        if (err == 0) {
            t10 = (int)(t * 10.0f + 0.5f);
            h10 = (int)(h * 10.0f + 0.5f);

            sprintf(line, "T:%d.%d C   ", t10 / 10, t10 % 10);
            OLED_ShowString(3, 1, line);

            sprintf(line, "H:%d.%d %%   ", h10 / 10, h10 % 10);
            OLED_ShowString(4, 1, line);
        } 
        else {
            sprintf(line, "DHT Err:%02d ", err);
            OLED_ShowString(4, 1, line);
        }

        /* ---- 光敏 ---- */
        raw = AD_GetValue(1);
        sprintf(line, "L:%04d       ", raw);
        OLED_ShowString(1, 1, line);

        Delay_ms(2000);  /* DHT22 要求 >= 2 秒 */
    }
}
