#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stddef.h>

#include "esperanto_flash_image.h"
#include "esperanto_flash_tool.h"
#include "parse_template_file.h"
#include "esperanto_flash_image_create.h"
#include "esperanto_flash_image_view.h"
#include "esperanto_flash_image_util.h"

#define SIZE_IN_MB (1024 * 1024)

static int verify_flash_image(TEMPLATE_INFO_t *template, const uint8_t * file_data, uint32_t file_size);

static ESPERANATO_REGION_INFO_t regions_map[ESPERANTO_MAX_REGIONS_COUNT];

static int reserve_region_index(uint32_t * next_available_region_index, ESPERANTO_FLASH_REGION_ID_t id) {
    uint32_t n;

    for (n = 0; n < ESPERANTO_MAX_REGIONS_COUNT; n++) {
        if (ESPERANTO_FLASH_REGION_ID_INVALID == regions_map[n].region_id) {
            regions_map[n].region_id = id;
            *next_available_region_index = n;
            return 0;
        }
    }
    return -1;
}

static uint32_t get_regions_count(void) {
    uint32_t n;

    for (n = 0; n < ESPERANTO_MAX_REGIONS_COUNT; n++) {
        if (ESPERANTO_FLASH_REGION_ID_INVALID == regions_map[n].region_id) {
            return n;
        }
    }
    return ESPERANTO_MAX_REGIONS_COUNT;
}

static int get_region_index(const PARTITION_INFO_t * partition_info, ESPERANTO_FLASH_REGION_ID_t id, uint32_t * index) {
    uint32_t n;
    for (n = 0; n < partition_info->regions_count; n++) {
        if (partition_info->regions[n].id == id) {
            *index = n;
            return 0;
        }
    }
    return -1;
}

static int set_counter_field(uint8_t * field, uint32_t field_size, uint32_t value) {
    uint32_t bytes, bits, mask;
    if (value > (8 * field_size)) {
        return -1;
    }
    bits = value % 8;
    bytes = value / 8;
    memset(field, 0xFF, field_size);
    memset(field, 0, bytes);
    if (bits > 0) {
        mask = (1u << bits) - 1;
        field[bytes] = (uint8_t)(field[bytes] & ~mask);
    }
    return 0;
}

static int create_partition(const PARTITION_INFO_t * partition_info, uint8_t * partition_data, uint32_t partition_size) {
    ESPERANTO_FLASH_PARTITION_HEADER_t partition_header;
    ESPERANATO_FILE_INFO_t file_info;
    uint32_t priority_designator_region_index = 0xFFFFFFFF;
    uint32_t boot_counters_region_index = 0xFFFFFFFF;
    uint32_t configuration_data_region_index = 0xFFFFFFFF;
    uint32_t region_index;
    uint32_t region_size;
    uint32_t n;
    char * file_data = NULL;
    size_t file_size;
    uint32_t next_free_offset = 1;

    if (partition_info->priority > (8 * FLASH_PAGE_SIZE)) {
        fprintf(stderr, "Error in create_partition: partition priority value too large!\n");
        return -1;
    }
    if (partition_info->attempted_boot_counter > (4 * FLASH_PAGE_SIZE)) {
        fprintf(stderr, "Error in create_partition: partition attempted boot counter value too large!\n");
        return -1;
    }
    if (partition_info->completed_boot_counter > (4 * FLASH_PAGE_SIZE)) {
        fprintf(stderr, "Error in create_partition: partition completed boot counter value too large!\n");
        return -1;
    }

    memset(regions_map, 0, sizeof(regions_map));

    // if the JSON template did not specify the priority designator region, reserve it now
    if (0 != get_region_index(partition_info, ESPERANTO_FLASH_REGION_ID_PRIORITY_DESIGNATOR, &priority_designator_region_index)) {
        if (0 != reserve_region_index(&priority_designator_region_index, ESPERANTO_FLASH_REGION_ID_PRIORITY_DESIGNATOR)) {
            fprintf(stderr, "Error in create_partition: failed to allocate the priority designator region!\n");
            return -1;
        }
        regions_map[priority_designator_region_index].region_offset = next_free_offset;
        next_free_offset++;
        regions_map[priority_designator_region_index].region_reserved_size = 1;
    }

    // if the JSON template did not specify the boot counters region, reserve it now
    if (0 != get_region_index(partition_info, ESPERANTO_FLASH_REGION_ID_BOOT_COUNTERS, &boot_counters_region_index)) {
        if (0 != reserve_region_index(&boot_counters_region_index, ESPERANTO_FLASH_REGION_ID_BOOT_COUNTERS)) {
            fprintf(stderr, "Error in create_partition: failed to allocate the boot counters region!\n");
            return -1;
        }
        regions_map[boot_counters_region_index].region_offset = next_free_offset;
        next_free_offset++;
        regions_map[boot_counters_region_index].region_reserved_size = 1;
    }

    // if the JSON template did not specify the configuration data region, reserve it now
    if (0 != get_region_index(partition_info, ESPERANTO_FLASH_REGION_ID_CONFIGURATION_DATA, &configuration_data_region_index)) {
        if (0 != reserve_region_index(&configuration_data_region_index, ESPERANTO_FLASH_REGION_ID_CONFIGURATION_DATA)) {
            fprintf(stderr, "Error in create_partition: failed to allocate the configuration data region!\n");
            return -1;
        }
        regions_map[configuration_data_region_index].region_offset = next_free_offset;
        next_free_offset++;
        regions_map[configuration_data_region_index].region_reserved_size = 1;
    }

    // process all regions specified in the template
    for (n = 0; n < partition_info->regions_count; n++) {
        if (0 != reserve_region_index(&region_index, partition_info->regions[n].id)) {
            fprintf(stderr, "Error in create_partition: failed to allocate the region 0x%x!\n", partition_info->regions[n].id);
            return -1;
        }

        switch(partition_info->regions[n].id) {
        case ESPERANTO_FLASH_REGION_ID_PRIORITY_DESIGNATOR:
        case ESPERANTO_FLASH_REGION_ID_BOOT_COUNTERS:
        case ESPERANTO_FLASH_REGION_ID_CONFIGURATION_DATA:
            region_size = 1;
            if (0 != partition_info->regions[n].size && FLASH_PAGE_SIZE != (1024 * partition_info->regions[n].size)) {
                fprintf(stderr, "Error in create_partition: invalid region 0x%x size (%u KB)!\n", partition_info->regions[n].id, partition_info->regions[n].size);
                return -1;
            }
            break;
        default:
            if (0 != (partition_info->regions[n].size % 4)) {
                fprintf(stderr, "Error in create_partition: Region 0x%x size is not a multiple of 4 KB (%u)!\n", partition_info->regions[n].id, partition_info->regions[n].size);
                return -1;
            }
            region_size = partition_info->regions[n].size / 4;
            break;
        }

        if (NULL == partition_info->regions[n].file_path && 0 == region_size) {
            fprintf(stderr, "Error in create_partition: Neither region size nor fle content specified!\n");
            return -1;
        }

        if (NULL != partition_info->regions[n].file_path) {
            if (0 != load_file(partition_info->regions[n].file_path, &file_data, &file_size)) {
                fprintf(stderr, "Error in create_partition: Failed to open/read file '%s'!\n", partition_info->regions[n].file_path);
                return -1;
            }
            if (region_size > 0) {
                if (file_size > (FLASH_PAGE_SIZE * region_size)) {
                    fprintf(stderr, "Error in create_partition: file '%s' size (%u) exceeds the region size (%u)!\n", partition_info->regions[n].file_path, (uint32_t)file_size, region_size * FLASH_PAGE_SIZE);
                    free(file_data);
                    return -1;
                }
            } else {
            	uint32_t required_size = (uint32_t)(file_size + sizeof(file_info));
                region_size = (required_size + (FLASH_PAGE_SIZE - 1)) & (uint32_t)(~(FLASH_PAGE_SIZE - 1));
                region_size = region_size / FLASH_PAGE_SIZE;
            }

            printf("  region %u: '%s'\n", n, partition_info->regions[n].file_path);
            
            switch(partition_info->regions[n].id) {
            case ESPERANTO_FLASH_REGION_ID_PRIORITY_DESIGNATOR:
            case ESPERANTO_FLASH_REGION_ID_BOOT_COUNTERS:
            case ESPERANTO_FLASH_REGION_ID_CONFIGURATION_DATA:
                memcpy(partition_data + FLASH_PAGE_SIZE * next_free_offset, file_data, file_size);
                break;
            default:
                file_info.file_header_tag = ESPERANTO_FILE_TAG;
                file_info.file_header_size = sizeof(file_info);
                file_info.file_size = (uint32_t)file_size;
                file_info.file_header_crc = crc32_sum(&file_info, offsetof(ESPERANATO_FILE_INFO_t, file_header_crc));
                memcpy(partition_data + FLASH_PAGE_SIZE * next_free_offset, &file_info, sizeof(file_info));
                memcpy(partition_data + FLASH_PAGE_SIZE * next_free_offset + sizeof(file_info), file_data, file_size);
                break;
            }
            free(file_data);
        }

        switch(partition_info->regions[n].id) {
        case ESPERANTO_FLASH_REGION_ID_PRIORITY_DESIGNATOR:
            priority_designator_region_index = n;
            break;
        case ESPERANTO_FLASH_REGION_ID_BOOT_COUNTERS:
            boot_counters_region_index = n;
            break;
        case ESPERANTO_FLASH_REGION_ID_CONFIGURATION_DATA:
            configuration_data_region_index = n;
            break;
        default:
            break;
        }
        
        regions_map[region_index].region_offset = next_free_offset;
        next_free_offset += region_size;
        regions_map[region_index].region_reserved_size = region_size;
    }

    if (partition_info->priority > 0) {
        set_counter_field(partition_data + regions_map[priority_designator_region_index].region_offset * FLASH_PAGE_SIZE, FLASH_PAGE_SIZE, partition_info->priority);
    }
    if (partition_info->attempted_boot_counter > 0) {
        set_counter_field(partition_data + regions_map[boot_counters_region_index].region_offset * FLASH_PAGE_SIZE, FLASH_PAGE_SIZE/2, partition_info->attempted_boot_counter);
    }
    if (partition_info->completed_boot_counter > 0) {
        set_counter_field(partition_data + regions_map[boot_counters_region_index].region_offset * FLASH_PAGE_SIZE + FLASH_PAGE_SIZE/2, FLASH_PAGE_SIZE/2, partition_info->completed_boot_counter);
    }

    partition_header.partition_tag = ESPERANTO_PARTITION_TAG;
    partition_header.partition_header_size = sizeof(partition_header);
    partition_header.partition_size = partition_size / FLASH_PAGE_SIZE;
    partition_header.partition_image_version = 0;
    partition_header.region_info_size = sizeof(ESPERANATO_REGION_INFO_t);
    partition_header.regions_count = get_regions_count();
    partition_header.reserved = 0xFFFFFFFF;
    partition_header.partition_header_checksum = crc32_sum(&partition_header, offsetof(ESPERANTO_FLASH_PARTITION_HEADER_t, partition_header_checksum));

    for (n = 0; n < partition_header.regions_count; n++) {
        regions_map[n].region_info_checksum = crc32_sum(&regions_map[n], offsetof(ESPERANATO_REGION_INFO_t, region_info_checksum));
    }

    memcpy(partition_data, &partition_header, sizeof(partition_header));
    memcpy(partition_data + sizeof(partition_header), regions_map, partition_header.regions_count * sizeof(ESPERANATO_REGION_INFO_t));

    return 0;
}

int create_image(const ARGUMENTS_t * arguments) {
    int rv;
    TEMPLATE_INFO_t template;
    uint32_t partition_size;
    uint32_t image_size;
    uint8_t * image_data = NULL;
    uint32_t n;

    if (!arguments->silent) {
        printf("Command: CREATE\n");
        printf("%s path: '%s'\n", arguments->partition_mode ? "Partition" : "Image", arguments->args[CREATE_IMAGE_ARGS_PARTITION_FILE_PATH]);
        printf("Template path: '%s'\n", arguments->args[CREATE_IMAGE_ARGS_TEMPLATE_PATH]);
    }

    if (0 != parse_template_file(arguments->args[CREATE_IMAGE_ARGS_TEMPLATE_PATH], &template)) {
        fprintf(stderr, "Error in create_image: parse_template_file() failed!\n");
        return -1;
    }

    if (template.image_type) {
        if (0 != template.image->image_size) {
            image_size = template.image->image_size;
        } else {
            if (0 == template.image->partitions[0].partition_size || 0 == template.image->partitions[1].partition_size) {
                fprintf(stderr, "Error in create_image: neither image size nor both partition sizes are specified!\n");
                rv = -1;
                goto DONE;
            }

            if (template.image->partitions[0].partition_size != template.image->partitions[1].partition_size) {
                fprintf(stderr, "Error in create_image: partition sizes are not equal!\n");
                rv = -1;
                goto DONE;
            }

            image_size = template.image->partitions[0].partition_size + template.image->partitions[1].partition_size;
        }
        partition_size = image_size / 2;

        if (0 != template.image->partitions[0].partition_size && partition_size != template.image->partitions[0].partition_size) {
            fprintf(stderr, "Error in create_image: partition 0 size (%u) is not equal to half image size (%u/2=%u)!\n", template.image->partitions[0].partition_size, image_size, partition_size);
            rv = -1;
            goto DONE;
        }

        if (0 != template.image->partitions[1].partition_size && partition_size != template.image->partitions[1].partition_size) {
            fprintf(stderr, "Error in create_image: partition 1 size (%u) is not equal to half image size (%u/2=%u)!\n", template.image->partitions[1].partition_size, image_size, partition_size);
            rv = -1;
            goto DONE;
        }
    } else {
        if (0 == template.partition->partition_size) {
            fprintf(stderr, "Error in create_image: partition size is not specified!\n");
            rv = -1;
            goto DONE;
        }
        partition_size = template.partition->partition_size;
    }

    if (0 != (partition_size % 4)) {
        fprintf(stderr, "Error in create_image: partition size (%u) is not a multiple of 4 KB!\n", partition_size);
        rv = -1;
        goto DONE;
    }
    
    partition_size *= 1024;
    image_size *= 1024;

    image_data = (uint8_t*)malloc(template.image_type ? image_size : partition_size);
    if (NULL == image_data) {
        fprintf(stderr, "Error in create_image: failed to allocate memory for image/partition data!\n");
        rv = -1;
        goto DONE;
    }
    memset(image_data, 0xFF, template.image_type ? image_size : partition_size);

    if (template.image_type) {
        for (n = 0; n < 2; n++) {
            if (0 != create_partition(&(template.image->partitions[n]), image_data + n * partition_size, partition_size)) {
                fprintf(stderr, "Error in create_image: create_partition() failed to create partition %u!\n", n);
                rv = -1;
                goto DONE;
            }
        }
    } else {
        if (0 != create_partition(template.partition, image_data, partition_size)) {
            fprintf(stderr, "Error in create_image: create_partition() failed!\n");
            rv = -1;
            goto DONE;
        }
    }

    if (0 != view_image_or_partition(image_data, template.image_type ? image_size : partition_size, arguments->silent, arguments->verbose))
    {
        fprintf(stderr, "Error in create_image: view_image_or_partition() failed!\n");
        rv = -1;
        goto DONE;
    }

    if(verify_flash_image(&template, image_data, template.image_type ? image_size : partition_size) != 0)
    {
        fprintf(stderr, "Error in create_image: verify_flash_image() failed!\n");
        rv = -1;
        goto DONE;
    }

    if (0 != save_image(arguments->args[CREATE_IMAGE_ARGS_PARTITION_FILE_PATH], image_data, template.image_type ? image_size : partition_size)) {
        fprintf(stderr, "Error in create_image: save_image() failed!\n");
        rv = -1;
        goto DONE;
    }

    if (!arguments->silent) {
    	printf("Saved image to file: %s\n", arguments->args[CREATE_IMAGE_ARGS_PARTITION_FILE_PATH]);
    }

    rv = 0;

DONE:
    if (NULL != image_data) {
        free(image_data);
    }
    free_template_info(&template);
    return rv;
}

static int verify_flash_image(TEMPLATE_INFO_t *template, const uint8_t * image_data, uint32_t image_file_size) {

    const ESPERANTO_FLASH_PARTITION_HEADER_t * partition_header;
    const ESPERANATO_REGION_INFO_t * regions;
    const ESPERANATO_FILE_INFO_t * file_info;
    const PARTITION_INFO_t * partition_info;

    uint8_t partition_count = 0;
    const uint8_t * partition_data = NULL;
    uint32_t partition_size_in_blocks, next_offset, region_start_offset, image_size, partition_size=0;
    uint32_t total_image_size=0;

    partition_header = (const ESPERANTO_FLASH_PARTITION_HEADER_t *)(image_data);
    if (image_file_size == (FLASH_PAGE_SIZE * partition_header->partition_size)) {
        partition_count=1;
    } else if (image_file_size == (2 * FLASH_PAGE_SIZE * partition_header->partition_size)) {
        partition_count=2;
    } else {
        fprintf(stderr,"Error verify_flash_image: Invalid image size");
        return -1;
    }

    for (uint8_t p_idx=0; p_idx < partition_count; p_idx++)
    {
        partition_header = (const ESPERANTO_FLASH_PARTITION_HEADER_t*)(image_data + (p_idx * (image_file_size / 2)));
        partition_data = (const uint8_t*)partition_header;
        regions = (const ESPERANATO_REGION_INFO_t*)(partition_header + 1);

        if (0 != verify_partition_header(partition_header)) {
            fprintf(stderr, "Error verify_flash_image: verify_partition_header() failed!\n");
            return -1;
        }

        partition_size_in_blocks = (uint32_t)(image_file_size / 2) / FLASH_PAGE_SIZE;
        if (partition_size_in_blocks != partition_header->partition_size) {
            fprintf(stderr, "Error verify_flash_image: Partition size mismatch (header value %u, actual size %u)\n", partition_header->partition_size, partition_size_in_blocks);
            return -1;
        }

        if (template->image_type) {
            partition_info = &(template->image->partitions[p_idx]);
            if (0 != template->image->image_size) {
                image_size = template->image->image_size;
            } else {
                image_size = template->image->partitions[0].partition_size + template->image->partitions[1].partition_size;
            }
        } else {
            partition_info = template->partition;
            image_size = template->partition->partition_size;
        }

        for (uint32_t n = 0; n < partition_header->regions_count; n++) {
            
            const ESPERANATO_REGION_INFO_t*region = (const ESPERANATO_REGION_INFO_t*)(regions + n);

            if (region->region_id <= ESPERANTO_FLASH_REGION_ID_INVALID || region->region_id > ESPERANTO_FLASH_REGION_ID_AFTER_LAST_SUPPORTED_VALUE)
            {
                fprintf(stderr, "Error verify_flash_image: Invalid region[%d] ID %X\n",n, region->region_id );
                return -1;
            }
            
            /* Check region size alignment */
            if ((region->region_reserved_size * FLASH_PAGE_SIZE) % 4 != 0)
            {
                fprintf(stderr, "Error verify_flash_image: Region size if not multiple of 4KB\n");
                return -1;
            }

            /* Verify region offset*/
            if ( n > 0 &&  (region->region_offset != next_offset))
            {
                fprintf(stderr, "Error verify_flash_image: Region[%d] offset is not contigeous\n", n);
                return -1;
            }

            /* Calculate next region offset*/
            next_offset = region->region_offset + region->region_reserved_size;

            /* Calculate partition size*/
            partition_size += region->region_reserved_size * FLASH_PAGE_SIZE;

            switch (region->region_id) {
                case ESPERANTO_FLASH_REGION_ID_PRIORITY_DESIGNATOR:
                case ESPERANTO_FLASH_REGION_ID_BOOT_COUNTERS:
                case ESPERANTO_FLASH_REGION_ID_CONFIGURATION_DATA:
                    if (region->region_reserved_size != 1) {
                        fprintf(stderr, "Error verify_flash_image:  invalid region reserved size! Expected 1, got %u\n", region->region_reserved_size);
                        return -1;
                    }
                    break;
                default:
                    region_start_offset = region->region_offset * FLASH_PAGE_SIZE;
                    file_info = (const ESPERANATO_FILE_INFO_t*)(partition_data + region_start_offset);
                    
                    if (ESPERANTO_FILE_TAG != file_info->file_header_tag) {
                        fprintf(stderr, "Error verify_flash_image: invalid file header tag!\n");
                        return -1;
                    }

                    for(uint8_t r_idx=0; r_idx < partition_info->regions_count; r_idx++)
                    {
                        if (partition_info->regions[r_idx].id == region->region_id)
                        {
                            if (NULL != partition_info->regions[r_idx].file_path) {
                                if (get_filesize(partition_info->regions[r_idx].file_path) != file_info->file_size)
                                {
                                    fprintf(stderr, "Error verify_flash_image: Invalid region[%d] file size\n", n);
                                    return -1;
                                }
                            }
                            break;
                        }
                    }
                    break;
            }
        }
        if (partition_size > (partition_size_in_blocks * FLASH_PAGE_SIZE))
        {
            fprintf(stderr, "Error verify_flash_image: Invalid total image size\n");
            return -1;
        }
        total_image_size += partition_size;
        partition_size = 0;
    }

    if (total_image_size > (image_size * 1024))
    {
        fprintf(stderr, "Error verify_flash_image: Invalid total image size\n");
        return -1;
    }

    printf("Image verification complete. \n");

    return 0;
}