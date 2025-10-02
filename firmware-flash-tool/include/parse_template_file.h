#ifndef __PARSE_TEMPLATE_H__
#define __PARSE_TEMPLATE_H__

#include <stdint.h>
#include <stdbool.h>
#include "esperanto_flash_image.h"

typedef struct REGION_INFO {
    ESPERANTO_FLASH_REGION_ID_t id;
    uint32_t size;
    char * file_path;
} REGION_INFO_t;

typedef struct PARTITION_INFO {
    uint32_t partition_size;
    uint32_t priority;
    uint32_t attempted_boot_counter;
    uint32_t completed_boot_counter;
    uint32_t regions_count;
    REGION_INFO_t * regions;
} PARTITION_INFO_t;

typedef struct IMAGE_INFO {
    uint32_t image_size;
    PARTITION_INFO_t partitions[2];
} IMAGE_INFO_t;

typedef struct TEMPLATE_INFO {
    bool image_type;
    union {
        IMAGE_INFO_t * image;
        PARTITION_INFO_t * partition;
    };
} TEMPLATE_INFO_t;

const char * region_id_to_name(ESPERANTO_FLASH_REGION_ID_t id);
ESPERANTO_FLASH_REGION_ID_t region_name_to_id(const char * name);
int load_file(const char * file_path, char ** buffer, size_t * buffer_size);
void free_template_info(TEMPLATE_INFO_t * template);
int parse_template_file(const char * filename, TEMPLATE_INFO_t * template);

#endif // __PARSE_TEMPLATE_H__
