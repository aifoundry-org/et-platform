#ifndef __ESPERANTO_PCIE_CONFIG_HEADER_H__
#define __ESPERANTO_PCIE_CONFIG_HEADER_H__

#include <stdint.h>
#include "certificate.h"

#define CURRENT_PCIE_CONFIG_FILE_HEADER_TAG 0x65494350 // 'PCIe'
#define CURRENT_PCIE_CONFIG_FILE_VERSION_TAG 1

#define MAXIMUM_STAGE_1_DATA_SIZE 3072
#define MAXIMUM_STAGE_2_DATA_SIZE 32768
#define MAXIMUM_STAGE_3_DATA_SIZE 3072

typedef struct ESPERANTO_PCIE_CONFIG_HEADER_s {
    HEADER_TAG_t header_tag;
    VERSION_TAG_t version_tag;
    uint32_t checksum_lo;
    uint32_t checksum_hi;
    uint32_t total_file_size;
    uint32_t stage_1_data_size;
    uint32_t stage_2_data_size;
    uint32_t stage_3_data_size;
} ESPERANTO_PCIE_CONFIG_HEADER_t;

#endif
