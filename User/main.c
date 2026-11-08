#include "stm32f10x.h"
#include <stdio.h>
#include "Delay.h"
#include "AD.h"
#include "OLED.h"
#include "DHT22.h"
#include "w25q64.h"
#include "app_fdb.h"

int main(void)
{
    uint8_t  err;
    float    t = 0.0f, h = 0.0f;
    int      t10 = 0, h10 = 0;
    uint16_t raw;
    uint32_t rec;          /* 当前记录条数 */
    uint8_t  tick = 0;
    char     line[20];

    // 初始化
    Delay_Init();
    W25Q64_Init();
    app_fdb_init();
    AD_Init();
    OLED_Init();
    OLED_Clear();

    /* 启动时只统计一次；之后靠自增，不再每条遍历全表 */
    rec = app_fdb_count();

    while (1)
    {
        /* DHT22: 关中断读，保护微秒时序 */
        __disable_irq();
        err = DHT_Read(&t, &h);
        __enable_irq();

        raw = AD_GetValue(1);  /* PA1 光敏 */

        if (err == 0)
        {
            t10 = (int)(t * 10.0f + 0.5f);
            h10 = (int)(h * 10.0f + 0.5f);
        }

        /* 每 5 次 (约 10 秒) 入库一条 */
        if (err == 0 && ++tick >= 5)
        {
            tick = 0;
            /* 写成功才自增 */
            if (app_fdb_append_sensor(0, t10, h10, (int)raw) == 0) {
                rec++; 
            }
        }

        /* ===== 屏幕只有 4 行，刚好四项 ===== */
        
        // 第 1 行: 光照
        sprintf(line, "L:%04d       ", raw);
        OLED_ShowString(1, 1, line);

        // 第 2 行: 温度
        if (err == 0) sprintf(line, "T:%d.%d C   ", t10 / 10, t10 % 10);
        else          sprintf(line, "DHT Err:%02d ", err);
        OLED_ShowString(2, 1, line);

        // 第 3 行: 湿度
        if (err == 0) sprintf(line, "H:%d.%d %%   ", h10 / 10, h10 % 10);
        else          sprintf(line, "            ");
        OLED_ShowString(3, 1, line);

        // 第 4 行: 记录条数
        sprintf(line, "REC:%lu      ", (unsigned long)rec);
        OLED_ShowString(4, 1, line);

        Delay_ms(2000);
    }
}
