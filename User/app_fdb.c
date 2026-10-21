#include "app_fdb.h"
#include "flashdb.h"
#include "fal.h"
#include "w25q64.h"
#include <stdio.h>
#include <string.h>

// 将FAL底层挂载到W25Q64
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

static struct fdb_kvdb kvdb;
static struct fdb_tsdb tsdb;
static uint32_t tick = 0;

static fdb_time_t get_time(void) { return (fdb_time_t)tick++; }

void app_fdb_init(void) {
    fal_init();
    fdb_kvdb_init(&kvdb, "env", "fdb_kvdb1", NULL, NULL);
    fdb_tsdb_init(&tsdb, "tsdb", "fdb_tsdb1", get_time, 64, NULL);
    printf("FlashDB Init OK\r\n");
}

void app_fdb_save_sensor(uint8_t node_id, sensor_data_t *data) {
    struct fdb_blob blob;
    char buf[80];
    int len = snprintf(buf, sizeof(buf), "N%d,T%.1f,H%.1f,L%d,M%d,R%d",
                       node_id, data->temp, data->humi, data->lux, data->motion, data->relay);

    if (fdb_tsl_append(&tsdb, fdb_blob_make(&blob, buf, (size_t)len)) != FDB_NO_ERR) {
        fdb_tsl_clean(&tsdb);
        fdb_tsl_append(&tsdb, fdb_blob_make(&blob, buf, (size_t)len));
    }
    printf("[SAVED] %s\r\n", buf);
}

static bool dump_cb(fdb_tsl_t tsl, void *arg) {
    struct fdb_blob blob;
    char buf[80];
    fdb_tsl_to_blob(tsl, fdb_blob_make(&blob, buf, sizeof(buf) - 1));
    buf[blob.size < sizeof(buf) ? blob.size : sizeof(buf) - 1] = '\0';
    printf("[T=%u] %s\r\n", (unsigned)tsl->time, buf);
    return false;
}

void app_fdb_dump(void) {
    printf("---- History Begin ----\r\n");
    fdb_tsl_iter(&tsdb, dump_cb, NULL);
    printf("---- History End ----\r\n");
}
