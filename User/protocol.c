#include "protocol.h"
#include "app_fdb.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static volatile char rx_buf[128];
static volatile uint16_t rx_len = 0;
static volatile bool line_ready = false;

void protocol_rx_byte(uint8_t byte) {
    if (line_ready) return;
    if (byte == '\r') return;
    if (byte == '\n') {
        rx_buf[rx_len] = '\0';
        line_ready = true;
        return;
    }
    if (rx_len < 127) rx_buf[rx_len++] = (char)byte;
    else rx_len = 0;
}

static void parse_line(char *line) {
    // 1. 处理 dump 指令
    if (strncmp(line, "dump", 4) == 0) { app_fdb_dump(); return; }
    
    // 2. 处理动态配置指令 SET:LUX:XX
    if (strncmp(line, "SET:LUX:", 8) == 0) {
        uint8_t new_th = (uint8_t)atoi(line + 8);
        if (new_th > 0 && new_th <= 100) {
            app_config_save_threshold(new_th);
        } else {
            printf("[ERROR] Invalid threshold!\r\n");
        }
        return;
    }

    // 3. 处理节点数据上报
    if (strncmp(line, "NODE:", 5) != 0) return;

    uint8_t node_id = 0;
    sensor_data_t d = {0};
    char *p;

    p = strstr(line, "NODE:"); if (p == NULL) return;
    node_id = (uint8_t)atoi(p + 5);
    if (node_id < 1 || node_id > 3) return; // 限制节点号 1-3

    // 防脏数据：缺任何一个字段直接丢弃
    p = strstr(line, "T:"); if (p == NULL) return; d.temp = (float)atof(p + 2);
    p = strstr(line, "H:"); if (p == NULL) return; d.humi = (float)atof(p + 2);
    p = strstr(line, "L:"); if (p == NULL) return; d.lux = atoi(p + 2);
    p = strstr(line, "M:"); if (p == NULL) return; d.motion = (uint8_t)atoi(p + 2);
    p = strstr(line, "R:"); if (p == NULL) return; d.relay = (uint8_t)atoi(p + 2);
    
    app_fdb_save_sensor(node_id, &d);
}

void protocol_poll(void) {
    char local[128];
    if (!line_ready) return;
    
    // 关中断，安全拷贝
    __disable_irq();
    memcpy(local, (void *)rx_buf, 128);
    rx_len = 0; 
    line_ready = false;
    __enable_irq();
    
    parse_line(local);
}
