#ifndef _FAL_CFG_H_
#define _FAL_CFG_H_

#include "fal.h"

extern const struct fal_flash_dev w25q64_flash;

#define FAL_FLASH_DEV_TABLE  \
{                            \
    &w25q64_flash,           \
}

// 👇 告诉 FAL 库我们要用自定义的分区表
#define FAL_PART_HAS_TABLE_CFG
#define FAL_PART_TABLE_FLASH_DEV_NAME       "w25q64"

// 👇 把分区表藏到最后 1KB，绝不越界，也不覆盖数据
#define FAL_PART_TABLE_END_OFFSET           (1 * 1024)

// 👇 黄金比例：KVDB 512KB，TSDB 7.5MB
#define FAL_PART_TABLE                                                            \
{                                                                                 \
    {FAL_PART_MAGIC_WORD, "fdb_kvdb1", "w25q64", 0,          512 * 1024, 0},      \
    {FAL_PART_MAGIC_WORD, "fdb_tsdb1", "w25q64", 512 * 1024, 7 * 1024 * 1024 + 512 * 1024, 0}, \
}

#endif
