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

#ifndef __BL2_SPI_FLASH_FS_H__
#define __BL2_SPI_FLASH_FS_H__

#include "bl2_spi_flash.h"

int flash_fs_init(FLASH_FS_BL2_INFO_t *restrict flash_fs_bl2_info,
                  const FLASH_FS_BL1_INFO_t *restrict flash_fs_bl1_info);
int flash_fs_set_active_partition(bool primary);
int flash_fs_get_config_data(void *buffer);
int flash_fs_get_file_size(ESPERANTO_FLASH_REGION_ID_t region_id, uint32_t *size);
int flash_fs_read_file(ESPERANTO_FLASH_REGION_ID_t region_id, uint32_t offset, void *buffer,
                       uint32_t buffer_size);
int flash_fs_write_partition(uint32_t partition_address, void *buffer, uint32_t buffer_size);
int flash_fs_erase_partition(uint32_t partition_address, uint32_t partition_size);
int flash_update_partition(void *buffer, uint64_t buffer_size);
int flash_fs_swap_priority_counter(void);
int flash_fs_get_boot_counters(uint32_t *attempted_boot_counter, uint32_t *completed_boot_counter);
int flash_fs_increment_completed_boot_count(void);
int flash_fs_increment_attempted_boot_count(void);
int flash_fs_get_manufacturer_name(char *mfg_name);
int flash_fs_get_part_number(char *part_number);
int flash_fs_get_serial_number(char *ser_number);
int flash_fs_get_module_rev(char *module_rev);
int flash_fs_get_memory_size(char *mem_size);
int flash_fs_get_form_factor(char *form_factor);

#endif
