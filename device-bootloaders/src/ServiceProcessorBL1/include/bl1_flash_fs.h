/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------
*/

#ifndef __BL1_SPI_FLASH_FS_H__
#define __BL1_SPI_FLASH_FS_H__

#include "bl1_spi_flash.h"

int flash_fs_init(FLASH_FS_BL1_INFO_t *flash_fs_bl1_info);
int flash_fs_set_active_partition(bool primary);
int flash_fs_get_config_data(void *buffer);
int flash_fs_get_file_size(ESPERANTO_FLASH_REGION_ID_t region_id, uint32_t *size);
int flash_fs_read_file(ESPERANTO_FLASH_REGION_ID_t region_id, uint32_t offset, void *buffer,
                       uint32_t buffer_size);
int flash_fs_increment_attempted_boot_count(void);

#endif
