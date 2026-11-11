#include "stm32f10x.h"
#include <stdio.h>
#include "Delay.h"
#include "OLED.h"
#include "nrf24l01.h"

int main(void)
{
    uint8_t  buf[32];
    uint8_t  r, self, cfg, ch, st;
    uint32_t cnt = 0;
    char     line[20];

    Delay_Init();
    OLED_Init();
    NRF_Init();
    self = NRF_RxMode();          /* ★ 自检结果 */
    OLED_Clear();

    while (1)
    {
        r = NRF_Recv(buf, 32);
        if (r == 0)
        {
            cnt++;
            buf[31] = '\0';
        }

        cfg = NRF_ReadReg(0x00);
        ch  = NRF_ReadReg(0x05);
        st  = NRF_ReadReg(0x07);

        /* 第1行：带版本号，一眼确认是新程序 */
        sprintf(line, "RXv2 S:%02X N:%lu", self, (unsigned long)cnt);
        OLED_ShowString(1, 1, line);

        sprintf(line, "C:%02X CH:%02X ", cfg, ch);
        OLED_ShowString(2, 1, line);

        sprintf(line, "ST:%02X       ", st);
        OLED_ShowString(3, 1, line);

        if (cnt > 0)
        {
            buf[15] = '\0';
            OLED_ShowString(4, 1, (char *)buf);
        }
        else
        {
            OLED_ShowString(4, 1, "waiting...");
        }
    }
}
