#include "stm32f10x.h"
#include <stdio.h>
#include "app_fdb.h"
#include "protocol.h"
#include "w25q64.h"
#include "Delay.h"
#include "sensor.h"
#include "Key.h"
#include "LED.h"
#include "TimeBase.h"

uint8_t rx_byte;

void Serial_Init(void) {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART1, &USART_InitStructure);
    
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    USART_Cmd(USART1, ENABLE);
}

void USART1_IRQHandler(void) {
    if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET) {
        protocol_rx_byte(USART_ReceiveData(USART1));
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}

int fputc(int ch, FILE *f) {
    USART_SendData(USART1, (uint8_t)ch);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    return ch;
}

int main(void) {
    Serial_Init();       
    // OLED_Init();       // 屏幕坏了，先屏蔽
    Sensor_Init();     
    Key_Init();
    LED_Init();        
    TimeBase_Init();   
    app_fdb_init(); 
    
    printf("\r\n==== HomeEdgeDB Master (No Screen Mode) ====\r\n");
    
    uint32_t id = W25Q64_ReadID();
    printf("W25Q64 ID: 0x%X ", (unsigned int)id); 
    if (id == 0xEF4017) {
        printf("-> W25Q64 Ready!\r\n");
    } else {
        printf("-> W25Q64 ERROR!\r\n");
    }

    uint32_t last_save_time = 0;
    uint32_t last_key_time  = 0;
    uint8_t  led_state = 0;

    while (1) {
        protocol_poll();
        uint32_t now = TimeBase_GetMs();

        // 任务 A：按键
        if (now - last_key_time >= 50) {
            last_key_time = now;
            if (Key_GetNum() == 1) { 
                led_state = !led_state;
                if (led_state) LED_ON();
                else           LED_OFF();
                printf("[LOCAL] Key pressed, LED set to %d\r\n", led_state);
            }
        }

        // 任务 B：采集与存储
        if (now - last_save_time >= 5000) { 
            last_save_time = now;
            Sensor_Update();            
            app_fdb_collect_and_save(); 
            
            uint8_t current_threshold = app_config_get_threshold();
            if (Sensor_GetLight() < current_threshold && led_state == 0) {
                led_state = 1;
                LED_ON();
                printf("[AUTO] Light < %d%%, LED ON\r\n", current_threshold);
            }
        }
    }
}
