#ifndef APP_FDB_H
#define APP_FDB_H

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    float   temp;
    float   humi;
    int     lux;
    uint8_t motion;
    uint8_t relay;
} sensor_data_t;

void app_fdb_init(void);
bool app_fdb_save_sensor(uint8_t node_id, sensor_data_t *data);
void app_fdb_dump(void);

void app_fdb_collect_and_save(void);
sensor_data_t app_fdb_get_last(void);

// 👇 新增的 KVDB 配置函数
void app_config_save_threshold(uint8_t threshold);
uint8_t app_config_get_threshold(void);

#endif
