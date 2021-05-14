/*-------------------------------------------------------------------------
* Copyright (C) 2018, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

#ifndef __BL1_SPI_FLASH_FS_H__
#define __BL1_SPI_FLASH_FS_H__

#include "bl1_spi_flash.h"

int flash_fs_init(FLASH_FS_BL1_INFO_t *restrict flash_fs_bl1_info,
                  const FLASH_FS_ROM_INFO_t *restrict flash_fs_rom_info);
int flash_fs_set_active_partition(bool primary);
int flash_fs_get_config_data(void *buffer);
int flash_fs_get_file_size(ESPERANTO_FLASH_REGION_ID_t region_id, uint32_t *size);
int flash_fs_read_file(ESPERANTO_FLASH_REGION_ID_t region_id, uint32_t offset, void *buffer,
                       uint32_t buffer_size);
int flash_fs_increment_attempted_boot_count(void);

#endif
