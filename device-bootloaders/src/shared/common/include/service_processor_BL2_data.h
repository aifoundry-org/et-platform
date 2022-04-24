#ifndef __SERVICE_PROCESSOR_BL2_DATA_H__
#define __SERVICE_PROCESSOR_BL2_DATA_H__

#include <stdint.h>
#include <stdbool.h>
#include "service_processor_spi_flash.h"
#include "esperanto_flash_image.h"

#include "esperanto_signed_image_format/executable_image.h"

#include "service_processor_ROM_data.h"
#include "service_processor_BL1_data.h"

#define SERVICE_PROCESSOR_BL2_DATA_VERSION 0x00000001

typedef struct ESPERANTO_PARTITION_BL2_INFO_s {
    // partition header loaded from flash
    ESPERANTO_FLASH_PARTITION_HEADER_t header;

    // regions table loaded from flash
    ESPERANATO_REGION_INFO_t regions_table[ESPERANTO_MAX_REGIONS_COUNT];

    // indexes of regions used by the SP BL2
    uint32_t priority_designator_region_index;
    uint32_t boot_counters_region_index;
    uint32_t configuration_data_region_index;
    uint32_t vaultip_fw_region_index;
    uint32_t pcie_config_region_index;
    uint32_t sp_certificates_region_index;
    uint32_t sp_bl1_region_index;
    uint32_t sp_bl2_region_index;
    uint32_t dram_training_region_index;
    uint32_t machine_minion_region_index;
    uint32_t master_minion_region_index;
    uint32_t worker_minion_region_index;
    uint32_t maxion_bl1_region_index;
    uint32_t dram_training_payload_800mhz_region_index;
    uint32_t dram_training_payload_933mhz_region_index;
    uint32_t dram_training_payload_1067mhz_region_index;
    uint32_t dram_training_2d_region_index;
    uint32_t dram_training_2d_payload_800mhz_region_index;
    uint32_t dram_training_2d_payload_933mhz_region_index;
    uint32_t dram_training_2d_payload_1067mhz_region_index;

    // priority and boot counters data
    PAGE_DATA_t priority_designator_region_data;
    PAGE_DATA_t boot_counters_region_data;

    // partition priority counter value read from the PRIORITY DESIGNATOR region
    uint32_t priority_counter;
    // attempted and completed boot counters values read from the BOOT COUNTERS region
    uint32_t attempted_boot_counter;
    uint32_t completed_boot_counter;

    bool partition_valid; // if true, no errors were encountered when scanning the partition
} ESPERANTO_PARTITION_BL2_INFO_t;

/* Esperanto flash config data */
typedef struct __attribute__((__packed__)) ESPERANTO_CONFIG_DATA {
    char        manuf_name[16];
    uint32_t    part_num;
    uint64_t    serial_num;                 
    uint8_t     mem_size;
    uint32_t    module_rev;
    uint8_t     form_factor;
    uint32_t    fw_release_rev;
    uint8_t     padding[26];
} ESPERANTO_CONFIG_DATA_t;

static_assert(64 == sizeof(ESPERANTO_CONFIG_DATA_t), "sizeof(ESPERANTO_CONFIG_DATA_t) is not 64!");

/* Esperanto flash config header */
typedef struct ESPERANTO_CONFIG_HEADER {
    uint32_t tag;
    uint32_t version;
    uint64_t hash;                 
} ESPERANTO_CONFIG_HEADER_t;

static_assert(16 == sizeof(ESPERANTO_CONFIG_HEADER_t), "sizeof(ESPERANTO_CONFIG_HEADER_t) is not 16!");

typedef struct FLASH_FS_BL2_INFO_s {
    union {
        SPI_FLASH_ID_t flash_id;
        uint32_t flash_id_u32;
    };
    uint32_t flash_size;
    ESPERANTO_PARTITION_BL2_INFO_t partition_info[2];
    uint32_t active_partition;
    uint32_t other_partition_valid;
    uint32_t configuration_region_address;
    // @cabul: Need to add file_info for this if we want to use flash_fs_read_file

    ESPERANTO_CONFIG_HEADER_t asset_config_header;
    ESPERANTO_CONFIG_DATA_t   asset_config_data;

    ESPERANATO_FILE_INFO_t pcie_config_file_info;
    ESPERANATO_FILE_INFO_t vaultip_firmware_file_info;
    ESPERANATO_FILE_INFO_t sp_certificates_file_info;
    ESPERANATO_FILE_INFO_t sp_bl1_file_info;
    ESPERANATO_FILE_INFO_t sp_bl2_file_info;

    ESPERANATO_FILE_INFO_t dram_training_file_info;
    ESPERANATO_FILE_INFO_t machine_minion_file_info;
    ESPERANATO_FILE_INFO_t master_minion_file_info;
    ESPERANATO_FILE_INFO_t worker_minion_file_info;
    ESPERANATO_FILE_INFO_t maxion_bl1_file_info;
    ESPERANATO_FILE_INFO_t dram_training_payload_800mhz_file_info;
    ESPERANATO_FILE_INFO_t dram_training_payload_933mhz_file_info;
    ESPERANATO_FILE_INFO_t dram_training_payload_1067mhz_file_info;
    ESPERANATO_FILE_INFO_t dram_training_2d_file_info;
    ESPERANATO_FILE_INFO_t dram_training_2d_payload_800mhz_file_info;
    ESPERANATO_FILE_INFO_t dram_training_2d_payload_933mhz_file_info;
    ESPERANATO_FILE_INFO_t dram_training_2d_payload_1067mhz_file_info;
} FLASH_FS_BL2_INFO_t;

typedef struct SERVICE_PROCESSOR_BL2_DATA_s {
    uint32_t service_processor_bl2_data_size;
    uint32_t service_processor_bl2_version;
    uint32_t service_processor_bl1_version;
    uint32_t service_processor_rom_version;
    uint32_t sp_gpio_pins;
    uint32_t vaultip_coid_set;
    uint16_t spi_controller_rx_baudrate_divider;
    uint16_t spi_controller_tx_baudrate_divider;
    uint8_t service_processor_bl1_image_file_version_major;
    uint8_t service_processor_bl1_image_file_version_minor;
    uint8_t service_processor_bl1_image_file_version_revision;

    FLASH_FS_BL2_INFO_t flash_fs_bl2_info;

    ESPERANTO_RAW_IMAGE_FILE_HEADER_t pcie_config_header;
    ESPERANTO_CERTIFICATE_t sp_certificates[2];
    ESPERANTO_IMAGE_FILE_HEADER_t sp_bl1_header;
    ESPERANTO_IMAGE_FILE_HEADER_t sp_bl2_header;
    ESPERANTO_RAW_IMAGE_FILE_HEADER_t dram_training_header;

    ESPERANTO_CERTIFICATE_t sw_certificates[2];
    uint32_t sw_certificates_loaded;
    ESPERANTO_IMAGE_FILE_HEADER_t machine_minion_header;
    ESPERANTO_IMAGE_FILE_HEADER_t master_minion_header;
    ESPERANTO_IMAGE_FILE_HEADER_t worker_minion_header;
    ESPERANTO_IMAGE_FILE_HEADER_t maxion_bl1_header;
} SERVICE_PROCESSOR_BL2_DATA_t;

#endif
