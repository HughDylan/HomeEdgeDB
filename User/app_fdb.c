#include "app_fdb.h"
#include "flashdb.h"
#include "fal.h"
#include "w25q64.h"
#include "sensor.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// ================= FAL 底层接口 =================
static int init(void) { W25Q64_Init(); return 0; }
static int read(long offset, uint8_t *buf, size_t size) { W25Q64_Read(offset, buf, size); return size; }
static int write(long offset, const uint8_t *buf, size_t size) { W25Q64_Write(offset, buf, size); return size; }
static int erase(long offset, size_t size) {
    for (uint32_t i = 0; i < size; i += 4096) W25Q64_EraseSector(offset + i);
    return 0;
}

const struct fal_flash_dev w25q64_flash = {
    "w25q64", 0, 8*1024*1024, 4096, {init, read, write, erase}, 1
};

// ================= 全局变量 =================
static struct fdb_kvdb kvdb;
static struct fdb_tsdb tsdb;
static uint32_t tick = 0;
static sensor_data_t last_data = {0};

static fdb_time_t get_time(void) { return (fdb_time_t)tick++; }

// 👇 👇 👇 新增：读取最后一条记录时间的回调函数 👇 👇 👇
static bool get_last_time_cb(fdb_tsl_t tsl, void *arg) {
    *(fdb_time_t *)arg = tsl->time; 
    return true; // 返回 true 表示找到第一条（最新的）就停止遍历
}
// 👆 👆 👆 新增结束 👆 👆 👆

// ================= FlashDB 初始化 =================
void app_fdb_init(void) {
    fdb_err_t err;

    printf("W25Q64 ID = %06X\r\n", (unsigned)W25Q64_ReadID());
    printf("fal_init  = %d\r\n", fal_init());

    err = fdb_kvdb_init(&kvdb, "env", "fdb_kvdb1", NULL, NULL);
    printf("KVDB init = %d\r\n", err);

    err = fdb_tsdb_init(&tsdb, "tsdb", "fdb_tsdb1", get_time, 64, NULL);
    printf("TSDB init = %d\r\n", err);

    // 👇 👇 👇 新增：上电时恢复时间戳 👇 👇 👇
    if (err == FDB_NO_ERR) {
        fdb_time_t last_time = 0;
        fdb_tsl_iter_reverse(&tsdb, get_last_time_cb, &last_time);
        
        if (last_time > 0) {
            tick = last_time + 1; 
            printf("System Time Restored to: %u\r\n", (unsigned)tick);
        }
    }
    // 👆 👆 👆 新增结束 👆 👆 👆
}

// ================= TSDB 存储与查询 =================
bool app_fdb_save_sensor(uint8_t node_id, sensor_data_t *data) {
    struct fdb_blob blob;
    char buf[80];
    int len = snprintf(buf, sizeof(buf), "N%d,T%.1f,H%.1f,L%d,M%d,R%d",
                       node_id, data->temp, data->humi, data->lux, data->motion, data->relay);

    if (len < 0) len = 0;
    if (len >= sizeof(buf)) len = sizeof(buf) - 1;

    if (fdb_tsl_append(&tsdb, fdb_blob_make(&blob, buf, (size_t)len)) != FDB_NO_ERR) {
        fdb_tsl_clean(&tsdb); 
        fdb_tsl_append(&tsdb, fdb_blob_make(&blob, buf, (size_t)len));
    }
    printf("[SAVED] %s\r\n", buf);
    return true;
}

static bool dump_cb(fdb_tsl_t tsl, void *arg) {
    struct fdb_blob blob;
    char buf[80];
    fdb_tsl_to_blob(tsl, fdb_blob_make(&blob, buf, sizeof(buf) - 1));
    
    uint32_t real_len = blob.saved.len;
    buf[real_len] = '\0';
    
    printf("[T=%u] %s\r\n", (unsigned)tsl->time, buf);
    return false;
}

void app_fdb_dump(void) {
    printf("---- History Begin ----\r\n");
    fdb_tsl_iter(&tsdb, dump_cb, NULL);
    printf("---- History End ----\r\n");
}

// ================= 数据采集与联动 =================
void app_fdb_collect_and_save(void) {
    sensor_data_t d;
    d.temp = Sensor_GetTemp();
    d.humi = 50.0; 
    d.lux  = Sensor_GetLight();
    d.motion = 0;  
    d.relay = 0;   

    last_data = d; 
    app_fdb_save_sensor(0, &d);
}

sensor_data_t app_fdb_get_last(void) { return last_data; }

// ================= KVDB 动态配置 =================
void app_config_save_threshold(uint8_t threshold) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", threshold); 
    fdb_kv_set(&kvdb, "lux_th", buf);
    printf("[CONFIG] Threshold saved as %s\r\n", buf);
}

uint8_t app_config_get_threshold(void) {
    const char *val_str = fdb_kv_get(&kvdb, "lux_th"); 
    if (val_str != NULL && val_str[0] != '\0') {
        return (uint8_t)atoi(val_str); 
    }
    return 30; 
}
