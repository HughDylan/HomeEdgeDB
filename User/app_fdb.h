#ifndef APP_FDB_H
#define APP_FDB_H
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    float   temp;
    float   humi;
    int     lux;
    uint8_t motion;
    uint8_t relay;
} sensor_data_t;

void app_fdb_init(void);
void app_fdb_save_sensor(uint8_t node_id, sensor_data_t *data);
void app_fdb_dump(void);
#endif
