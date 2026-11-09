#include "stm32f10x.h"
#include <stdio.h>
#include "Delay.h"
#include "OLED.h"
#include "nrf24l01.h"

int main(void) {
    uint8_t v0, v1, v2, v5, v7;
    char line[20];

    Delay_Init();
    // 删掉了 W25Q64_Init()，因为 nRF 已经自己配了 SPI！
    NRF_Init();
    OLED_Init();
    OLED_Clear();

    while (1) {
        v0 = NRF_ReadReg(0x00); /* CONFIG    出厂 0x08 */
        v1 = NRF_ReadReg(0x01); /* EN_AA     出厂 0x3F */
        v2 = NRF_ReadReg(0x02); /* EN_RXADDR 出厂 0x03 */
        v5 = NRF_ReadReg(0x05); /* RF_CH     出厂 0x02 */
        v7 = NRF_ReadReg(0x07); /* STATUS    出厂 0x0E */

        sprintf(line, "CFG:%d AA:%d ", v0, v1);
        OLED_ShowString(1, 1, line);

        sprintf(line, "RXA:%d CH:%d ", v2, v5);
        OLED_ShowString(2, 1, line);

        sprintf(line, "ST:%d     ", v7);
        OLED_ShowString(3, 1, line);

        // 严格按照图里的判断标准
        if (v0 == 0x08 || v0 == 0x0E)
            OLED_ShowString(4, 1, "OK!             ");
        else if (v0 == 0x00 || v0 == 0xFF)
            OLED_ShowString(4, 1, "NO SIGNAL       ");
        else
            OLED_ShowString(4, 1, "ODD VALUE       ");

        Delay_ms(500);
    }
}
