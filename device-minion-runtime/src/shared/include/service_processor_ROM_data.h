#ifndef __SERVICE_PROCESSOR_ROM_DATA_H__
#define __SERVICE_PROCESSOR_ROM_DATA_H__

#include <stdint.h>
#include "service_processor_spi_flash.h"
#include "../../firmware-tools/esperanto-flash-tool/include/esperanto_flash_image.h"

typedef struct ESPERANTO_PARTITION_INFO_s {
    // partition header loaded from flash
    ESPERANTO_FLASH_PARTITION_HEADER_t header;

    // regions table loaded from flash
    ESPERANATO_REGION_INFO_t regions_table[ESPERANTO_MAX_REGIONS_COUNT];

    // indexes of regions used by the SP ROM
    uint32_t priority_designator_region_index;
    uint32_t boot_counters_region_index;
    uint32_t configuration_data_region_index;
    uint32_t vaultip_fw_region_index;
    uint32_t sp_certificates_region_index;
    uint32_t sp_bl1_region_index;

    // partition priority counter value read from the PRIORITY DESIGNATOR region
    uint32_t priority_counter;
    // attempted and completed boot counters values read from the BOOT COUNTERS region
    uint32_t attempted_boot_counter;
    uint32_t completed_boot_counter;

    bool partition_valid; // if true, no errors were encountered when scanning the partition
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
