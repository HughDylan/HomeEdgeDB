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

// ================= 新增：时间戳恢复功能 =================
/* 遍历找出库里最大的时间 */
static bool max_time_cb(fdb_tsl_t tsl, void *arg) {
    fdb_time_t *p = (fdb_time_t *)arg;
    if (tsl->time > *p) *p = (fdb_time_t)tsl->time;
    return false; // false = 继续遍历
}

/* 上电时把 tick 推到库中最后一条之后 */
static void tsdb_resume_time(void) {
    fdb_time_t last = 0;
    fdb_tsl_iter(&tsdb, max_time_cb, &last);
    tick = last + 1;
    printf("TSDB resume time = %u\r\n", (unsigned)tick);
}
// ========================================================

// ================= FlashDB 初始化 =================
void app_fdb_init(void) {
    fdb_err_t err;

    //printf("W25Q64 ID = %06X\r\n", (unsigned)W25Q64_ReadID());
    //printf("fal_init  = %d\r\n", fal_init());

    err = fdb_kvdb_init(&kvdb, "env", "fdb_kvdb1", NULL, NULL);
    printf("KVDB init = %d\r\n", err);

    err = fdb_tsdb_init(&tsdb, "tsdb", "fdb_tsdb1", get_time, 64, NULL);
    printf("TSDB init = %d\r\n", err);

    // 初始化成功后，调用恢复时间函数
    // 初始化成功后，先清空旧数据，再恢复时间
    if (err == FDB_NO_ERR) {
    
    tsdb_resume_time();         // 原本就有的这句保留
        }
	printf("CONFIG : LUX=%u\r\n",(unsigned)app_fdb_get_threshold());
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
    size_t n;

    // 1. 获取元信息
    fdb_tsl_to_blob(tsl, &blob);

    // 2. 告诉它读到哪里
    blob.buf = buf;
    blob.size = sizeof(buf) - 1;

    // 3. 核心读取（注意这里加了 tsdb，也就是库对象）
    fdb_blob_read((fdb_db_t)&tsdb, &blob);

    // 4. 计算长度并封口
    n = blob.saved.len;
    if (n > sizeof(buf) - 1) n = sizeof(buf) - 1;
    buf[n] = '\0';

    // 5. 打印十六进制
    printf("[T=%u] %s\r\n" , (unsigned)tsl->time, buf);
    

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

/* ================== KVDB 配置存取 ================== */
#define THR_DEFAULT  30

bool app_fdb_save_threshold(uint8_t lux_thr)
{
    char val[8];
    if (lux_thr > 100) return false; // 限制范围，防止乱设
    snprintf(val, sizeof(val), "%u", (unsigned)lux_thr);
    
    // 写入 KVDB
    return fdb_kv_set(&kvdb, "lux_thr", val) == FDB_NO_ERR;
}

uint8_t app_fdb_get_threshold(void)
{
    // 注意这里！直接调 fdb_kv_get
    const char *val_str = fdb_kv_get(&kvdb, "lux_thr"); 
    
    if (val_str != NULL) {
        //  核心坑点：必须立刻 atoi 存成数字！千万不能存指针！
        return (uint8_t)atoi(val_str);
    }
    return THR_DEFAULT; // 如果没存过，返回默认值 30
}

