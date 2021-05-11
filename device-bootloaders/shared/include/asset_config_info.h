#ifndef __ASSET_CONFIG_INFO__
#define __ASSET_CONFIG_INFO__

#define ASSET_CONFIG_PART_NUM_SIZE    sizeof(uint32_t)
#define ASSET_CONFIG_SERIAL_NUM_SIZE  sizeof(uint64_t)
#define ASSET_CONFIG_MEM_SIZE_SIZE    sizeof(uint8_t)
#define ASSET_CONFIG_MODULE_REV_SIZE  sizeof(uint32_t)
#define ASSET_CONFIG_FORM_FACTOR_SIZE sizeof(uint8_t)

#define ASSET_CONFIG_HEADER_SIZE (sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint64_t))

typedef struct asset_config_header_s {
    uint32_t tag;
    uint32_t version;
    uint64_t hash;
} asset_config_header_t;

typedef struct asset_config_info_s {
    uint32_t part_num;
    uint64_t serial_num;
    uint8_t mem_size;
    uint32_t module_rev;
    uint8_t form_factor;
} asset_config_info_t;

#endif // __ASSET_CONFIG_INFO__
