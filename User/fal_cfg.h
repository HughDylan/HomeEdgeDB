#ifndef _FAL_CFG_H_
#define _FAL_CFG_H_
#include "fal.h"

extern const struct fal_flash_dev w25q64_flash;
#define FAL_FLASH_DEV_TABLE { &w25q64_flash, }
#define FAL_PART_TABLE_FLASH_DEV_NAME  "w25q64"
#define FAL_PART_TABLE_END_OFFSET  (8 * 1024 *1024)
#define FAL_PART_TABLE \
{ \
    {FAL_PART_MAGIC_WORD, "fdb_kvdb1", "w25q64", 0, 4*1024*1024, 0}, \
    {FAL_PART_MAGIC_WORD, "fdb_tsdb1", "w25q64", 4*1024*1024, 4*1024*1024, 0}, \
}
#endif
