#ifndef __BL2_FLASHFS_DRIVER_H__
#define __BL2_FLASHFS_DRIVER_H__

#include "service_processor_BL2_data.h"
#include "bl2_spi_flash.h"

int flashfs_drv_init(FLASH_FS_BL2_INFO_t * restrict flash_fs_bl2_info, const FLASH_FS_BL1_INFO_t * restrict flash_fs_bl1_info);
int flashfs_drv_get_config_data(void * buffer);
int flashfs_drv_get_file_size(ESPERANTO_FLASH_REGION_ID_t region_id, uint32_t * size);
int flashfs_drv_read_file(ESPERANTO_FLASH_REGION_ID_t region_id, uint32_t offset, void * buffer, uint32_t buffer_size);
int flashfs_drv_reset_boot_counters(void);
int flashfs_drv_increment_completed_boot_count(void);

#endif
