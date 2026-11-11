#include "stm32f10x.h"
#include <stdio.h>
#include "Delay.h"
#include "OLED.h"
#include "nrf24l01.h"

int main(void)
{
    uint8_t buf[32];
    uint8_t r, cfg, ch, st;
    uint32_t cnt = 0;
    char line[20];

    Delay_Init();
    OLED_Init();
    NRF_Init();
    NRF_RxMode();
    OLED_Clear();

    while (1)
    {
        r = NRF_Recv(buf, 32);
        if (r == 0)
        {
            cnt++;
            buf[31] = '\0';

            cfg = NRF_ReadReg(0x00);
            ch  = NRF_ReadReg(0x05);
            st  = NRF_ReadReg(0x07);

            /* 第1行：收到的包数 */
            sprintf(line, "RX N:%lu      ", (unsigned long)cnt);
            OLED_ShowString(1, 1, line);

            /* 第2行：CONFIG + 频道(核对用) */
            sprintf(line, "C:%02X CH:%02X ", cfg, ch);
            OLED_ShowString(2, 1, line);

            /* 第3行：STATUS (收到包时 bit6 会置1) */
            sprintf(line, "ST:%02X      ", st);
            OLED_ShowString(3, 1, line);

            /* 第4行：收到的内容截前 15 字符，一屏只放得下 */
            if (cnt > 0)
            {
                buf[15] = '\0';
                OLED_ShowString(4, 1, (char *)buf);
            }
            else
            {
                OLED_ShowString(4, 1, "waiting... ");
            }
        }
    }
}
