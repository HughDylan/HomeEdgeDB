#ifndef APP_FDB_H
#define APP_FDB_H

#include <stdint.h>
#include <stdbool.h>

// 👇 这一块就是它找不到的结构体定义，必须放在函数声明前面
typedef struct
{
    float   temp;
    float   humi;
    int     lux;
    uint8_t motion;
    uint8_t relay;
} sensor_data_t;

// 函数声明
void app_fdb_init(void);
bool app_fdb_save_sensor(uint8_t node_id, sensor_data_t *data);
void app_fdb_dump(void);

// 新加的函数
void app_fdb_collect_and_save(void);
sensor_data_t app_fdb_get_last(void);

#endif
