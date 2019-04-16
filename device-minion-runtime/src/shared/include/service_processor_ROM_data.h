#ifndef __SERVICE_PROCESSOR_ROM_DATA_H__
#define __SERVICE_PROCESSOR_ROM_DATA_H__

#include <stdint.h>
#include "service_processor_spi_flash.h"
#include "../../firmware-tools/esperanto-flash-tool/include/esperanto_flash_image.h"

typedef struct ESPERANTO_PARTITION_INFO_s {
    ESPERANTO_FLASH_PARTITION_HEADER_t header;
    ESPERANATO_REGION_INFO_t regions_table[ESPERANTO_MAX_REGIONS_COUNT];
    uint32_t priority_designator_region_address;
    uint32_t priority_counter;
    uint32_t boot_counters_region_address;
    uint32_t attempted_boot_counter;
    uint32_t completed_boot_counter;
} ESPERANTO_PARTITION_INFO_t;

typedef struct FLASH_FS_ROM_INFO_s {
    SPI_FLASH_ID_t flash_id;
    uint32_t flash_size;
    ESPERANTO_PARTITION_INFO_t partition_info[2];
    uint32_t active_partition;
    uint32_t configuration_region_address;
    ESPERANATO_FILE_INFO_t vaultip_firmware_file_info;
    ESPERANATO_FILE_INFO_t sp_certificates_file_info;
    ESPERANATO_FILE_INFO_t sp_bl1_file_info;
} FLASH_FS_ROM_INFO_t;

typedef struct SERVICE_PROCESSOR_ROM_DATA_s {
    uint32_t service_processor_rom_data_size;
    uint32_t service_processor_rom_version;
    FLASH_FS_ROM_INFO_t flash_fs_rom_info;
} SERVICE_PROCESSOR_ROM_DATA_t;

#endif
