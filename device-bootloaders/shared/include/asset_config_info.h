#ifndef __ASSET_CONFIG_INFO__
#define __ASSET_CONFIG_INFO__

typedef struct asset_config_info_s {
    uint32_t part_num;
    uint64_t serial_num;
    uint8_t mem_size;
    uint32_t module_rev;
    uint8_t form_factor;
} asset_config_info_t;

#endif // __ASSET_CONFIG_INFO__
