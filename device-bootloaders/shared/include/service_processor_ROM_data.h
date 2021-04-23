/*-------------------------------------------------------------------------
* Copyright (C) 2019,2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef __SERVICE_PROCESSOR_ROM_DATA_H__
#define __SERVICE_PROCESSOR_ROM_DATA_H__

/*
 * NOTE: THE SOURCE OF TRUTH FOR THIS FILE COMES FROM THE PUBLISHED BOOTROM SPEC.
 */

#include <stdint.h>
#include <stdbool.h>
#include "service_processor_spi_flash.h"

#include "esperanto_flash_image.h"

#include "esperanto_signed_image_format/executable_image.h"
#include "esperanto_signed_image_format/raw_image.h"

#define SERVICE_PROCESSOR_ROM_DATA_VERSION 0x00000001

#define SIZE_PER_ULL (sizeof(unsigned long long))
#define ULL_PER_PAGE (FLASH_PAGE_SIZE / SIZE_PER_ULL)

typedef union PAGE_DATA_u {
    unsigned long long ull[ULL_PER_PAGE];
    uint8_t b[FLASH_PAGE_SIZE];
} PAGE_DATA_t;

typedef struct ESPERANTO_PARTITION_ROM_INFO_s {
    // partition header loaded from flash
    ESPERANTO_FLASH_PARTITION_HEADER_t header;

    // regions table loaded from flash
    ESPERANATO_REGION_INFO_t regions_table[ESPERANTO_MAX_REGIONS_COUNT];

    // indexes of regions used by the SP ROM
    uint32_t priority_designator_region_index;
    uint32_t boot_counters_region_index;
    uint32_t configuration_data_region_index;
    uint32_t pcie_config_region_index;
    uint32_t vaultip_fw_region_index;
    uint32_t sp_certificates_region_index;
    uint32_t sp_bl1_region_index;

    // priority and boot counters data
    PAGE_DATA_t priority_designator_region_data;
    PAGE_DATA_t boot_counters_region_data;

    // partition priority counter value read from the PRIORITY DESIGNATOR region
    uint32_t priority_counter;
    // attempted and completed boot counters values read from the BOOT COUNTERS region
    uint32_t attempted_boot_counter;
    uint32_t completed_boot_counter;

    bool partition_valid; // if true, no errors were encountered when scanning the partition
} ESPERANTO_PARTITION_ROM_INFO_t;

typedef struct FLASH_FS_ROM_INFO_s {
    union {
        SPI_FLASH_ID_t flash_id;
        uint32_t flash_id_u32;
    };
    uint32_t flash_size;
    ESPERANTO_PARTITION_ROM_INFO_t partition_info[2];
    uint32_t active_partition;
    uint32_t other_partition_valid;
    uint32_t configuration_region_address;
    ESPERANATO_FILE_INFO_t pcie_config_file_info;
    ESPERANATO_FILE_INFO_t vaultip_firmware_file_info;
    ESPERANATO_FILE_INFO_t sp_certificates_file_info;
    ESPERANATO_FILE_INFO_t sp_bl1_file_info;
} FLASH_FS_ROM_INFO_t;

typedef struct SERVICE_PROCESSOR_ROM_DATA_s {
    uint32_t service_processor_rom_data_size;
    uint32_t service_processor_rom_version;
    uint32_t sp_gpio_pins;
    uint32_t sp_pll0_frequency;
    uint32_t sp_pll1_frequency;
    uint32_t pcie_pll0_frequency;
    uint64_t timer_raw_ticks_before_pll_turned_on;
    uint16_t spi_controller_rx_baudrate_divider;
    uint16_t spi_controller_tx_baudrate_divider;
    FLASH_FS_ROM_INFO_t flash_fs_rom_info;
    ESPERANTO_RAW_IMAGE_FILE_HEADER_t pcie_config_header;
    uint32_t * pcie_config_data;
    uint32_t * vaultip_firmware_image;
    uint32_t vaultip_coid_set;
    ESPERANTO_CERTIFICATE_t sp_certificates[2];
    ESPERANTO_IMAGE_FILE_HEADER_t sp_bl1_header;
} SERVICE_PROCESSOR_ROM_DATA_t;

#endif
