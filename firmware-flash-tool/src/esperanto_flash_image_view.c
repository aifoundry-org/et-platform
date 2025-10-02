#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stddef.h>

#include "esperanto_flash_image.h"
#include "esperanto_flash_tool.h"
#include "parse_template_file.h"
#include "esperanto_flash_image_view.h"
#include "esperanto_flash_image_util.h"

static int get_counter_value(const uint8_t * counter_data, uint32_t counter_data_size, uint32_t * counter_value) {
    uint32_t n, m;
    uint32_t value = 0;
    uint8_t byte;
    bool got_1 = false;
    int corrupted = 0;
    
    for (n = 0; n < counter_data_size; n++) {
        byte = counter_data[n];
        for (m = 0; m < 8; m++) {
            if (0 == (byte & (1 << m))) {
                value++;
                if (got_1) {
                    corrupted = -1;
                }
            } else {
                got_1 = true;
            }
        }
    }

    *counter_value = value;
    return corrupted;
}

int verify_partition_header(const ESPERANTO_FLASH_PARTITION_HEADER_t * partition_header) {
    uint32_t crc;

    if (ESPERANTO_PARTITION_TAG != partition_header->partition_tag) {
        fprintf(stderr, "Error in verify_partition_header: not a valid partition header!\n");
        return -1;
    }

    if (sizeof(ESPERANTO_FLASH_PARTITION_HEADER_t) != partition_header->partition_header_size) {
        fprintf(stderr, "Error in verify_partition_header: invalid or not supported partition header size!\n");
        return -1;
    }

    crc = crc32_sum(partition_header, offsetof(ESPERANTO_FLASH_PARTITION_HEADER_t, partition_header_checksum));
    if (crc != partition_header->partition_header_checksum) {
        fprintf(stderr, "Error in verify_partition_header: partition header checksum mismatch!\n");
        return -1;
    }

    if (sizeof(ESPERANATO_REGION_INFO_t) != partition_header->region_info_size) {
        fprintf(stderr, "Error in verify_partition_header: invalid or not supported region info size!\n");
        return -1;
    }

    if (0xFFFFFFFF != partition_header->reserved) {
        fprintf(stderr, "Error in verify_partition_header: the reserved field value is not 0xFFFFFFFF!\n");
        return -1;
    }

    return 0;
}

static int view_region(uint32_t region_index, const ESPERANATO_REGION_INFO_t * region, const uint8_t * partition_data, uint32_t partition_data_size, bool silent, bool verbose) {
    uint32_t crc;
    const char * region_name;
    uint32_t region_start_offset, region_end_offset, region_size;
    uint32_t counter;
    int counter_corrupted;
    const ESPERANATO_FILE_INFO_t * file_info;
    const uint8_t * file_data;

    if (!silent) {
        printf("  Region %u\n", region_index);
    }

    crc = crc32_sum(region, offsetof(ESPERANATO_REGION_INFO_t, region_info_checksum));
    if (crc != region->region_info_checksum) {
        fprintf(stderr, "Error in view_region: CRC checksum mismatch!\n");
        return -1;
    }

    if (ESPERANTO_FLASH_REGION_ID_INVALID == region->region_id) {
        fprintf(stderr, "Error in view_region: Invalid region id!\n");
        return -1;
    }
    region_name = region_id_to_name(region->region_id);
    if (!silent) {
        printf("    Region id: 0x%x (%s)\n", region->region_id, region_name ? region_name : "UNKNOWN");
    }

    region_size = region->region_reserved_size * FLASH_PAGE_SIZE;
    if (!silent) {
        printf("    Reserved size: %u blocks (%u bytes)\n", region->region_reserved_size, region_size);
    }
    region_start_offset = region->region_offset * FLASH_PAGE_SIZE;
    if (!silent) {
        printf("    Start offset: %u (%u bytes)\n", region->region_offset, region_start_offset);
    }
    region_end_offset = region_start_offset + region_size;
//    if (!silent) {
//        printf("    End offset:   %u (%u bytes)\n", region->region_offset + region->region_reserved_size, region_end_offset);
//    }
    if (region_start_offset < FLASH_PAGE_SIZE || partition_data_size < region_end_offset) {
        fprintf(stderr, "Error in view_region: invalid region start offset or size!\n");
        return -1;
    }

    switch (region->region_id) {
    case ESPERANTO_FLASH_REGION_ID_PRIORITY_DESIGNATOR:
    case ESPERANTO_FLASH_REGION_ID_BOOT_COUNTERS:
    case ESPERANTO_FLASH_REGION_ID_CONFIGURATION_DATA:
        if (1 != region->region_reserved_size) {
            fprintf(stderr, "Error in view_region:  invalid region reserved size! Expected 1, got %u\n", region->region_reserved_size);
            return -1;
        }
        break;
    default:
        break;
    }

    switch (region->region_id) {
    case ESPERANTO_FLASH_REGION_ID_PRIORITY_DESIGNATOR:
        counter_corrupted = get_counter_value(partition_data + region_start_offset, region_size, &counter);
        if (!silent) {
            printf("    PRIORITY DESIGNATOR COUNTER VALUE: %u%s\n", counter, counter_corrupted ? " (CORRUPTED!)" : "");
        }
        break;
    case ESPERANTO_FLASH_REGION_ID_BOOT_COUNTERS:
        counter_corrupted = get_counter_value(partition_data + region_start_offset, region_size / 2, &counter);
        if (!silent) {
            printf("    ATTEMPTED BOOT COUNTER VALUE: %u%s\n", counter, counter_corrupted ? " (CORRUPTED!)" : "");
        }
        counter_corrupted = get_counter_value(partition_data + region_start_offset + region_size / 2, region_size / 2, &counter);
        if (!silent) {
            printf("    COMPLETED BOOT COUNTER VALUE: %u%s\n", counter, counter_corrupted ? " (CORRUPTED!)" : "");
        }
        break;
    case ESPERANTO_FLASH_REGION_ID_CONFIGURATION_DATA:
        if (verbose) {
            printf("    CONFIGURATION DATA:\n");
            dumphex(partition_data + region_start_offset, region_size, 32);
        }
        break;
    default:
        file_info = (const ESPERANATO_FILE_INFO_t*)(partition_data + region_start_offset);
        file_data = (const uint8_t*)(file_info + 1);
        if (ESPERANTO_FILE_TAG != file_info->file_header_tag) {
            fprintf(stderr, "Error in view_region: invalid file header tag!\n");
            return -1;
        }
        if (sizeof(ESPERANATO_FILE_INFO_t) != file_info->file_header_size) {
            fprintf(stderr, "Error in view_region: invalid file header size!\n");
            return -1;
        }
        crc = crc32_sum(file_info, offsetof(ESPERANATO_FILE_INFO_t, file_header_crc));
        if (crc != file_info->file_header_crc) {
            fprintf(stderr, "Error in view_region: file header CRC checksum mismatch!\n");
            return -1;
        }
        if (!silent) {
            printf("    File size: %u\n", file_info->file_size);
        }
        if (file_info->file_size > (region_size - sizeof(ESPERANATO_FILE_INFO_t))) {
            fprintf(stderr, "Error in view_region: file size too large!\n");
            return -1;
        }
        if (verbose) {
            crc = crc32_sum(file_data, file_info->file_size);
            printf("    File CRC32: 0x%08x\n", crc);
        }
    }

    return 0;
}

static int view_partition(uint32_t partition_index, const ESPERANTO_FLASH_PARTITION_HEADER_t * partition_header, uint32_t data_size, bool silent, bool verbose) {
    uint32_t n;
    const ESPERANATO_REGION_INFO_t * regions;
    const uint8_t * partition_data = (const uint8_t*)partition_header;
    uint32_t partition_size_in_blocks = data_size / FLASH_PAGE_SIZE;
    if (0xFFFFFFFF != partition_index) {
        if (!silent) {
            printf("Partition %u start\n", partition_index);
        }

        if (!silent) {
            printf("  Partition size: %u blocks (%u KB)\n", partition_header->partition_size, partition_header->partition_size * FLASH_PAGE_SIZE / 1024);
        }
        if (partition_size_in_blocks != partition_header->partition_size) {
            fprintf(stderr, "Error in view_partition: Partition size mismatch (header value %u, actual size %u)\n", partition_header->partition_size, partition_size_in_blocks);
            return -1;
        }

        if (!silent) {
            printf("  Partition image version: %u (0x%x)\n", partition_header->partition_image_version, partition_header->partition_image_version);
            printf("  Regions count: %u\n", partition_header->regions_count);
        }

        regions = (const ESPERANATO_REGION_INFO_t*)(partition_header + 1);
        for (n = 0; n < partition_header->regions_count; n++) {
            if (0 != view_region(n, regions + n, partition_data, data_size, silent, verbose)) {
                fprintf(stderr, "Error in view_partition: view_region(%u) failed!\n", n);
                return -1;
            }
        }

        if (!silent) {
            printf("Partition %u end\n", partition_index);
        }
    }

    return 0;
}

int view_image_or_partition(const uint8_t * file_data, uint32_t file_size, bool silent, bool verbose) {
    const ESPERANTO_FLASH_PARTITION_HEADER_t * partition_header;
    if (0 != (file_size & (FLASH_PAGE_SIZE - 1))) {
        fprintf(stderr, "Error in view_image: file size is not a multiple of 4KB!\n");
        return -1;
    }
    partition_header = (const ESPERANTO_FLASH_PARTITION_HEADER_t*)file_data;
    if (0 != verify_partition_header(partition_header)) {
        fprintf(stderr, "Error in view_image: verify_partition_header() failed!\n");
        return -1;
    }

    if (file_size == (FLASH_PAGE_SIZE * partition_header->partition_size)) {
        if (!silent) {
            printf("*** Partition start ***\n");
        }
        if (0 != view_partition(0xFFFFFFFF, partition_header, file_size / 2, silent, verbose)) {
            fprintf(stderr, "Error in view_image: view_partition() failed!\n");
            return -1;
        }
        if (!silent) {
            printf("***  Partition end  ***\n");
        }
    } else if (file_size == (2 * FLASH_PAGE_SIZE * partition_header->partition_size)) {
        if (!silent) {
            printf("*** Image start ***\n");
        }
        if (0 != view_partition(0, partition_header, file_size / 2, silent, verbose)) {
            fprintf(stderr, "Error in view_image: view_partition() failed on 1st partition!\n");
            return -1;
        }
        partition_header = (const ESPERANTO_FLASH_PARTITION_HEADER_t*)(file_data + (file_size / 2));
        if (0 != verify_partition_header(partition_header)) {
            fprintf(stderr, "Error in view_image: verify_partition_header() failed on 2nd partition!\n");
            return -1;
        }
        if (0 != view_partition(1, partition_header, file_size / 2, silent, verbose)) {
            fprintf(stderr, "Error in view_image: view_partition() failed on 2nd partition!\n");
            return -1;
        }
        if (!silent) {
            printf("***  Image end  ***\n");
        }
    } else {
        fprintf(stderr, "Error in view_image: invalid file size!\n");
        return -1;
    }

    return 0;
}

int view_image(const ARGUMENTS_t * arguments) {
    int rv;
    size_t file_size;
    uint8_t * file_data = NULL;

    if (!arguments->silent) {
        printf("Command: VIEW\n");
        printf("Image/partition path: '%s'\n", arguments->args[VIEW_IMAGE_ARGS_IMAGE_FILE_PATH]);
    }

    if (0 != load_file(arguments->args[VIEW_IMAGE_ARGS_IMAGE_FILE_PATH], (char**)&file_data, &file_size)) {
        fprintf(stderr, "Error in view_image: Failed to open/read file '%s'!\n", arguments->args[VIEW_IMAGE_ARGS_IMAGE_FILE_PATH]);
        rv = -1;
        goto DONE;
    }

    rv = view_image_or_partition(file_data, (uint32_t)file_size, arguments->silent, arguments->verbose); 

DONE:
    if (NULL != file_data) {
        free(file_data);
    }
    return rv;
}

