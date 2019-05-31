#ifndef __ESPERANTO_FLASH_IMAGE_H__
#define __ESPERANTO_FLASH_IMAGE_H__

#include <stdint.h>
#include <assert.h>

#define FLASH_PAGE_SIZE 4096

typedef enum ESPERANTO_FLASH_REGION_ID {
    ESPERANTO_FLASH_REGION_ID_INVALID = 0,
    ESPERANTO_FLASH_REGION_ID_PRIORITY_DESIGNATOR,
    ESPERANTO_FLASH_REGION_ID_BOOT_COUNTERS,
    ESPERANTO_FLASH_REGION_ID_CONFIGURATION_DATA,
    ESPERANTO_FLASH_REGION_ID_VAULTIP_FW,
    ESPERANTO_FLASH_REGION_ID_SP_CERTIFICATES,
    ESPERANTO_FLASH_REGION_ID_SW_CERTIFICATES,
    ESPERANTO_FLASH_REGION_ID_COMM_CERTIFICATES,
    ESPERANTO_FLASH_REGION_ID_SP_BL1,
    ESPERANTO_FLASH_REGION_ID_SP_BL2,
    ESPERANTO_FLASH_REGION_ID_PCIE_CONFIG,
    ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING,
    ESPERANTO_FLASH_REGION_ID_MASTER_MINION_EXEC,
    ESPERANTO_FLASH_REGION_ID_MASTER_MINION_USER,
    ESPERANTO_FLASH_REGION_ID_WORKER_MINION_EXEC,
    ESPERANTO_FLASH_REGION_ID_MAXION_BL1,
    ESPERANTO_FLASH_REGION_ID_AFTER_LAST_SUPPORTED_VALUE,
    ESPERANTO_FLASH_REGION_ID_FFFFFFFF = 0xFFFFFFFF // this should force the compiler to use 32-bit data type for the enum
} ESPERANTO_FLASH_REGION_ID_t;

#define ESPERANTO_PARTITION_TAG 0x54524150 // "PART"

typedef struct ESPERANTO_FLASH_PARTITION_HEADER {
    uint32_t partition_tag;
    uint32_t partition_header_size;     // size of the partition header (struct ESPERANTO_FLASH_PARTITION_HEADER) in bytes
    uint32_t partition_size;            // size of the partition in FLASH_PAGE_SIZE blocks
    uint32_t partition_image_version;
    uint32_t region_info_size;          // size of the region info (struct ESPERANTO_REGION_INFO) in bytes
    uint32_t regions_count;
    uint32_t reserved;
    uint32_t partition_header_checksum;
} ESPERANTO_FLASH_PARTITION_HEADER_t;

static_assert(32 == sizeof(ESPERANTO_FLASH_PARTITION_HEADER_t), "sizeof(ESPERANTO_FLASH_PARTITION_HEADER_t) is not 32!");

#define ESPERANTO_REGION_TAG 0x4E474552 // "REGN"

typedef struct ESPERANTO_REGION_INFO {
    ESPERANTO_FLASH_REGION_ID_t region_id;
    uint32_t region_offset;
    uint32_t region_reserved_size;      // size of the region in FLASH_PAGE_SIZE blocks
    uint32_t region_info_checksum;
} ESPERANATO_REGION_INFO_t;

static_assert(16 == sizeof(ESPERANATO_REGION_INFO_t), "sizeof(ESPERANATO_REGION_INFO_t) is not 16!");

#define ESPERANTO_MAX_REGIONS_COUNT ((FLASH_PAGE_SIZE - sizeof(ESPERANTO_FLASH_PARTITION_HEADER_t))/sizeof(ESPERANATO_REGION_INFO_t))

#define ESPERANTO_FILE_TAG 0x454C4946 // "FILE"

typedef struct ESPERANTO_FILE_INFO {
    uint32_t file_header_tag;
    uint32_t file_header_size;
    uint32_t file_size;                 // size of the file in bytes
    uint32_t file_header_crc;
} ESPERANATO_FILE_INFO_t;

static_assert(16 == sizeof(ESPERANATO_FILE_INFO_t), "sizeof(ESPERANATO_FILE_INFO_t) is not 16!");

#endif
