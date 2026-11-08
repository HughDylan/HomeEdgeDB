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

// 替换旧的 config 函数声明
bool app_fdb_save_threshold(uint8_t lux_thr);
uint8_t app_fdb_get_threshold(void);
// 追加一条传感器记录（直接传 *10 的整数，避免单片机处理浮点数的卡顿）
int app_fdb_append_sensor(uint8_t node, int t10, int h10, int lux);

// 统计当前数据库里一共有多少条记录
uint32_t app_fdb_count(void);

#endif
