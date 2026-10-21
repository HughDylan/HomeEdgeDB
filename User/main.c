#include "stm32f10x.h"
#include <stdio.h>
#include "app_fdb.h"
#include "protocol.h"
#include "w25q64.h"
#include "Delay.h"
#include "OLED.h"
#include "sensor.h"
#include "Key.h"
#include "LED.h"
#include "TimeBase.h"

uint8_t rx_byte;

// 串口初始化 (保留)
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
    // 1. 系统初始化
    Serial_Init(); 
    OLED_Init();       
    Sensor_Init();     
    Key_Init();
    LED_Init();        
    TimeBase_Init();   // 启动 TIM2 定时器，提供 1ms 心跳
    app_fdb_init(); 
    
    printf("\r\n==== HomeEdgeDB Master ====\r\n");
    uint32_t id = W25Q64_ReadID();
    printf("W25Q64 ID: 0x%X\r\n", (unsigned int)id);

    OLED_ShowString(1, 1, "System Ready");

    // 2. 定义时间记录变量 (相当于闹钟)
    uint32_t last_save_time = 0;
    uint32_t last_oled_time = 0;
    uint32_t last_key_time  = 0;
    uint8_t  led_state = 0;

    while (1) {
        // 任务 A：处理串口指令 (如果有 dump 等命令，立刻处理)
        protocol_poll();
        
        // 获取系统上电至今的毫秒数
        uint32_t now = TimeBase_GetMs();

        // 任务 B：按键扫描 (每 50ms 查一次，不影响其他任务)
        if (now - last_key_time >= 50) {
            last_key_time = now;
            if (Key_GetNum() == 1) { 
                led_state = !led_state;
                if (led_state) LED_ON();
                else           LED_OFF();
                printf("[LOCAL] Key pressed, LED set to %d\r\n", led_state);
            }
        }

        // 任务 C：传感器采集并存入数据库 (每 5000ms = 5秒，保护 Flash 寿命)
        if (now - last_save_time >= 5000) { 
            last_save_time = now;
            Sensor_Update();            // 读取 ADC 硬件
            app_fdb_collect_and_save(); // 存入 FlashDB
            
            // 规则联动：光照低于 30% 自动开灯
            if (Sensor_GetLight() < 30 && led_state == 0) {
                led_state = 1;
                LED_ON();
            }
        }

        // 任务 D：OLED 屏幕刷新 (每 200ms 刷一次，丝滑不卡顿)
        if (now - last_oled_time >= 200) {
            last_oled_time = now;
            
            // 从缓存拿数据，不占用读取硬件的时间
            sensor_data_t d = app_fdb_get_last(); 
            
            OLED_ShowString(2, 1, "Temp:");
            OLED_ShowNum(2, 6, (uint32_t)d.temp, 2);
            OLED_ShowString(2, 9, "C  ");
            
            OLED_ShowString(3, 1, "Lux :");
            OLED_ShowNum(3, 6, d.lux, 3);
            OLED_ShowString(3, 10, "%  ");
            
            OLED_ShowString(4, 1, "Light:");
            if (led_state) OLED_ShowString(4, 7, "ON ");
            else           OLED_ShowString(4, 7, "OFF");
        }
    }
	
}
