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
    if (strncmp(line, "dump", 4) == 0) { app_fdb_dump(); return; }
    if (strncmp(line, "NODE:", 5) != 0) return;

    uint8_t node_id = 0;
    sensor_data_t d = {0};
    char *p;
    p = strstr(line, "NODE:"); if (p) node_id = (uint8_t)atoi(p + 5);
    p = strstr(line, "T:");    if (p) d.temp = (float)atof(p + 2);
    p = strstr(line, "H:");    if (p) d.humi = (float)atof(p + 2);
    p = strstr(line, "L:");    if (p) d.lux = atoi(p + 2);
    p = strstr(line, "M:");    if (p) d.motion = (uint8_t)atoi(p + 2);
    p = strstr(line, "R:");    if (p) d.relay = (uint8_t)atoi(p + 2);
    app_fdb_save_sensor(node_id, &d);
}

void protocol_poll(void) {
    char local[128];
    if (!line_ready) return;
    __disable_irq();
    memcpy(local, (void *)rx_buf, 128);
    rx_len = 0; line_ready = false;
    __enable_irq();
    parse_line(local);
}