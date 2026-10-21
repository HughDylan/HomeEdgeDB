#include "stm32f10x.h"
#include <stdio.h>
#include "app_fdb.h"
#include "protocol.h"
#include "w25q64.h"
#include "Delay.h" // 确保包含延时函数

uint8_t rx_byte; // 串口接收中断用的全局变量

// 串口初始化 (PA9 TX, PA10 RX)
void Serial_Init(void) {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // PA9 TX
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // PA10 RX
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
    
    // 开启接收中断
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    USART_Cmd(USART1, ENABLE);
}

// 串口接收中断服务函数
void USART1_IRQHandler(void) {
    if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET) {
        protocol_rx_byte(USART_ReceiveData(USART1));
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}

// printf 重定向
int fputc(int ch, FILE *f) {
    USART_SendData(USART1, (uint8_t)ch);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    return ch;
}

// 主函数
int main(void) {
    Serial_Init(); // 初始化串口
    
    printf("\r\n==== HomeEdgeDB Master (StdLib) ====\r\n");
    
    uint32_t id = W25Q64_ReadID();
    printf("W25Q64 ID: 0x%X (Should be 0xEF4017)\r\n", (unsigned int)id);

    app_fdb_init(); // 初始化 FlashDB

    printf("Send: NODE:1,T:25.6,H:60.2,L:123,M:0,R:0\r\n");
    printf("Send: dump\r\n");

    while (1) {
        protocol_poll(); // 处理串口接收的数据
        Delay_ms(10);    // 简单的延时
    }
}


