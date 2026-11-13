#include "stm32f10x.h"
#include <stdio.h>
#include "delay.h"
#include "OLED.h"
#include "nrf24l01.h"

void OLED_WriteCommand(uint8_t Command);

int main(void)
{
    uint8_t buf[32];
    uint8_t self, cfg, ch, aa, rxa, pw, st, ad;
    uint32_t cnt = 0;
    char line[27];

    Delay_Init();
    Delay_ms(100);
    OLED_Init();
    OLED_WriteCommand(0xA1);
    OLED_WriteCommand(0xC8);
    OLED_WriteCommand(0xAF);

    NRF_Init();
    self = NRF_RxMode();
    OLED_Clear();

    while (1)
    {
        GPIO_SetBits(GPIOB, GPIO_Pin_0);  /* CE 每圈强制拉高，防松线 */
        if (NRF_Recv(buf, 32) == 0) cnt++;

        cfg = NRF_ReadReg(0x00);     /* CONFIG */
        ch = NRF_ReadReg(0x05);      /* RF_CH */
        aa = NRF_ReadReg(0x01);      /* EN_AA */
        rxa = NRF_ReadReg(0x02);     /* EN_RXADDR */
        pw = NRF_ReadReg(0x11);      /* RX_PW_P0 */
        ad = NRF_ReadReg(0x0A);      /* RX_ADDR_P0首字节 */
        st = NRF_ReadReg(0x07);      /* STATUS */

        sprintf(line, "C:%02X H:%02X A:%02X R:%02X", cfg, ch, aa, rxa);
        line[16] = '\0';
        OLED_ShowString(1, 1, line);

        sprintf(line, "AD:%02X W:%02X S:%02X", ad, pw, self);
        line[16] = '\0';
        OLED_ShowString(2, 1, line);

        sprintf(line, "ST:%02X N:%lu", st, (unsigned long)cnt);
        line[16] = '\0';
        OLED_ShowString(3, 1, line);

        if (cnt > 0) { buf[15] = '\0'; OLED_ShowString(4, 1, (char *)buf); }

        Delay_ms(200); /* 5次/秒，不刷爆 I2C */
    }
}
