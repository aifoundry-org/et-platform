#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stddef.h>

#include "esperanto_flash_image.h"
#include "esperanto_flash_tool.h"
#include "parse_template_file.h"
#include "esperanto_flash_image_replace.h"
#include "esperanto_flash_image_extract.h"
#include "esperanto_flash_image_util.h"
#include "esperanto_flash_image_view.h"

static int replace_file_in_region(const ESPERANATO_REGION_INFO_t * region, uint8_t * partition_data, uint32_t partition_size_in_bytes, const char * replacement_file_path) {
    int rv;
    ESPERANATO_FILE_INFO_t * file_info;
    uint32_t region_size, region_end_offset;
    char * file_data = NULL;
    size_t file_size;

    if (region->region_info_checksum != crc32_sum(region, offsetof(ESPERANATO_REGION_INFO_t, region_info_checksum))) {
        fprintf(stderr, "Error in replace_file_in_region: region header CRC checksum is invalid!\n");
        rv = -1;
        goto DONE;
    }
    region_size = FLASH_PAGE_SIZE * region->region_reserved_size;
    region_end_offset = region->region_offset + region_size;
    if (partition_size_in_bytes < region_end_offset) {
        fprintf(stderr, "Error in replace_file_in_region: region size is invalid!\n");
        rv = -1;
        goto DONE;
    }

    if (0 != load_file(replacement_file_path, &file_data, &file_size)) {
        fprintf(stderr, "Error in replace_file_in_region: region size is invalid!\n");
        rv = -1;
        goto DONE;
    }

    switch (region->region_id) {
    case ESPERANTO_FLASH_REGION_ID_INVALID:
        rv = -1;
        goto DONE;

    case ESPERANTO_FLASH_REGION_ID_PRIORITY_DESIGNATOR:
    case ESPERANTO_FLASH_REGION_ID_BOOT_COUNTERS:
    case ESPERANTO_FLASH_REGION_ID_CONFIGURATION_DATA:
        if (FLASH_PAGE_SIZE != file_size) {
            fprintf(stderr, "Error in replace_file_in_region: invalid file size!\n");
            rv = -1;
            goto DONE;
        }
        memcpy(partition_data + (region->region_offset * FLASH_PAGE_SIZE), file_data, file_size);
        break;

    default:
        file_info = (ESPERANATO_FILE_INFO_t*)(partition_data + (region->region_offset * FLASH_PAGE_SIZE));
        if (file_info->file_header_crc != crc32_sum(file_info, offsetof(ESPERANATO_FILE_INFO_t, file_header_crc))) {
            fprintf(stderr, "Error in replace_file_in_region: file header CRC checksum is invalid!\n");
            rv = -1;
            goto DONE;
        }
        if (file_info->file_size > (region_size - sizeof(ESPERANATO_FILE_INFO_t))) {
            fprintf(stderr, "Error in replace_file_in_region: original file size too large!\n");
            rv = -1;
            goto DONE;
        }
        if (file_size > (region_size - sizeof(ESPERANATO_FILE_INFO_t))) {
            fprintf(stderr, "Error in replace_file_in_region: replacement file size too large!\n");
            rv = -1;
            goto DONE;
        }
        memset(file_info + 1, 0, (region_size - sizeof(ESPERANATO_FILE_INFO_t)));
        memcpy(file_info + 1, file_data, file_size);
        file_info->file_size = (uint32_t)file_size;
        file_info->file_header_crc = crc32_sum(file_info, offsetof(ESPERANATO_FILE_INFO_t, file_header_crc));
        break;
    }
    rv = 0;

DONE:
    if (NULL != file_data) {
        free(file_data);
    }
    return rv;
}

static int replace_file_in_partition(ESPERANTO_FLASH_PARTITION_HEADER_t * partition_header, ESPERANTO_FLASH_REGION_ID_t region_id, uint32_t region_index, const char * replacement_file_path) {
    uint32_t n;
    const ESPERANATO_REGION_INFO_t * regions;
    uint8_t * partition_data = (uint8_t*)partition_header;
    uint32_t partition_size_in_bytes = partition_header->partition_size * FLASH_PAGE_SIZE;

    regions = (const ESPERANATO_REGION_INFO_t*)(partition_header + 1);
    if (ESPERANTO_FLASH_REGION_ID_INVALID == region_id) {
        if (region_index < partition_header->regions_count) {
            return replace_file_in_region(&regions[region_index], partition_data, partition_size_in_bytes, replacement_file_path);
        } else {
            fprintf(stderr, "Error in replace_file_in_partition: region_index (%u) is out of range of valid regions (0..%u)!\n", region_index, partition_header->regions_count - 1);
            return -1;
        }
    }
    for (n = 0; n < partition_header->regions_count; n++) {
        if (regions[n].region_id == region_id) {
            return replace_file_in_region(&regions[n], partition_data, partition_size_in_bytes, replacement_file_path);
        }
    }

    fprintf(stderr, "Error in replace_file_in_partition: region_d (0x%x) not found!\n", region_id);
    return -1;
}

static int replace_files_in_partition(ESPERANTO_FLASH_PARTITION_HEADER_t * partition_header, bool use_region_ids, uint32_t args_count, char ** args, bool silent, bool verbose) {
    uint32_t n;
    const char * file_path;
	ESPERANTO_FLASH_REGION_ID_t region_id;
	const char * region_name;
	uint32_t region_index;
	unsigned long int ul;
	char * endptr;

    (void)verbose;

    for (n = REPLACE_FILE_ARGS_BASE_COUNT; n < args_count; n += 2) {
        file_path = args[n + 1];
        region_id = region_name_to_id(args[n]);
        if (ESPERANTO_FLASH_REGION_ID_INVALID != region_id) {
            region_name = region_id_to_name(region_id);
        } else {
            ul = strtoul(args[n], &endptr, 0);
            if (0 != *endptr) {
                fprintf(stderr, "Error in replace_files_in_partition: invalid region id or index value '%s'!\n", args[n]);
                return -1;
            }
            if (use_region_ids) {
                region_id = (ESPERANTO_FLASH_REGION_ID_t)ul;
                region_name = region_id_to_name(region_id);
            } else {
                region_id = ESPERANTO_FLASH_REGION_ID_INVALID;
                region_index = (uint32_t)ul;
            }
        }

        if (0 != replace_file_in_partition(partition_header, region_id, region_index, file_path)) {
            fprintf(stderr, "Error in replace_files_in_partition: replace_file_in_partition() failed!\n");
            return -1;
        } else {
            if (!silent) {
                if (ESPERANTO_FLASH_REGION_ID_INVALID != region_id) {
                    printf("Replace region 0x%x (%s) with file '%s'\n", region_id, region_name, file_path);
                } else {
                    printf("Repalced region %u with file '%s'\n", region_index, file_path);
                }
            }
        }
    }

    return 0;
}

int replace_files(const ARGUMENTS_t * arguments) {
    int rv;
	uint32_t n;
	ESPERANTO_FLASH_REGION_ID_t region_id;
	const char * region_name;
	uint32_t region_index;
	unsigned long int ul;
	char * endptr;
    ESPERANTO_FLASH_PARTITION_HEADER_t * partition_header;
    size_t file_size;
    uint8_t * file_data = NULL;
    uint32_t partition_index = 0;
	
    if (NULL != arguments->partition_index) {
        ul = strtoul(arguments->partition_index, &endptr, 0);
        if (0 != *endptr) {
            fprintf(stderr, "Error in replace_files: invalid partition index value '%s'!\n", arguments->partition_index);
            rv = -1;
            goto DONE;
        }
        partition_index = (uint32_t)ul;
        if (partition_index < 1 || partition_index > 2) {
            fprintf(stderr, "Error in replace_files: partition index (%u) not in valid range (1 <= index <= 2)!\n", partition_index);
            rv = -1;
            goto DONE;
        }
    }

    if (!arguments->silent) {
        printf("Command: REPLACE\n");
        printf("Image/partition path: '%s'\n", arguments->args[REPLACE_FILE_ARGS_IMAGE_FILE_PATH]);
        if (0 != partition_index) {
            printf("Partiton index: %u\n", partition_index);
        }
        for (n = REPLACE_FILE_ARGS_BASE_COUNT; n < arguments->args_count; n += 2) {
        	region_id = region_name_to_id(arguments->args[n]);
        	if (ESPERANTO_FLASH_REGION_ID_INVALID != region_id) {
        		region_name = region_id_to_name(region_id);
        		printf("region id: 0x%x (%s), file path: '%s'\n", region_id, region_name ? region_name : "unknown", arguments->args[n + 1]);
        	} else {
        		ul = strtoul(arguments->args[n], &endptr, 0);
        		if (0 != *endptr) {
        			fprintf(stderr, "Error in replace_files: invalid region id or index value '%s'!\n", arguments->args[n]);
        			return -1;
        		}
            	if (arguments->use_region_ids) {
            		region_id = (ESPERANTO_FLASH_REGION_ID_t)ul;
            		region_name = region_id_to_name(region_id);
            		printf("region id: 0x%x (%s), file path: '%s'\n", region_id, region_name ? region_name : "unknown", arguments->args[n + 1]);
            	} else {
            		region_index = (uint32_t)ul;
            		printf("region index: %u, file path: '%s'\n", region_index, arguments->args[n + 1]);
            	}
        	}
        }
    }

    if (0 != load_file(arguments->args[EXTRACT_FILE_ARGS_IMAGE_FILE_PATH], (char**)&file_data, &file_size)) {
        fprintf(stderr, "Error in replace_files: Failed to open/read file '%s'!\n", arguments->args[EXTRACT_FILE_ARGS_IMAGE_FILE_PATH]);
        rv = -1;
        goto DONE;
    }

    if (0 != (file_size & (FLASH_PAGE_SIZE - 1))) {
        fprintf(stderr, "Error in replace_files: file size is not a multiple of 4KB!\n");
        rv = -1;
        goto DONE;
    }
    partition_header = (ESPERANTO_FLASH_PARTITION_HEADER_t*)file_data;
    if (0 != verify_partition_header(partition_header)) {
        fprintf(stderr, "Error in replace_files: verify_partition_header() failed!\n");
        rv = -1;
        goto DONE;
    }

    if (file_size == (FLASH_PAGE_SIZE * partition_header->partition_size)) {
        if (2 == partition_index) {
            fprintf(stderr, "Error in replace_files: non-zero partition index (%u) was specified, but the image contains only one partition!\n", partition_index);
            rv = -1;
            goto DONE;
        }
        if (0 != replace_files_in_partition(partition_header, arguments->use_region_ids, arguments->args_count, arguments->args, arguments->silent, arguments->verbose)) {
            fprintf(stderr, "Error in replace_files: replace_files_in_partition() failed!\n");
            rv = -1;
            goto DONE;
        }
    } else if (file_size == (2 * FLASH_PAGE_SIZE * partition_header->partition_size)) {
        if (0 == partition_index) {
            if (!arguments->silent) {
                printf("Partition index was not specified!\n");
                rv = -1;
                goto DONE;
            }
        }
        if (1 == partition_index) {
            if (0 != replace_files_in_partition(partition_header, arguments->use_region_ids, arguments->args_count, arguments->args, arguments->silent, arguments->verbose)) {
                fprintf(stderr, "Error in replace_files: replace_files_in_partition() failed!\n");
                rv = -1;
                goto DONE;
            }
        } else {
            partition_header = (ESPERANTO_FLASH_PARTITION_HEADER_t*)(file_data + (file_size / 2));
            if (0 != verify_partition_header(partition_header)) {
                fprintf(stderr, "Error in replace_files: verify_partition_header() failed on 2nd partition!\n");
                return -1;
            }
            if (0 != replace_files_in_partition(partition_header, arguments->use_region_ids, arguments->args_count, arguments->args, arguments->silent, arguments->verbose)) {
                fprintf(stderr, "Error in replace_files: replace_files_in_partition() failed!\n");
                rv = -1;
                goto DONE;
            }
        }
    } else {
        fprintf(stderr, "Error in replace_files: invalid image file size!\n");
        return -1;
    }

    if (0 != save_image(arguments->output_path ? arguments->output_path : arguments->args[REPLACE_FILE_ARGS_IMAGE_FILE_PATH], file_data, (uint32_t)file_size)) {
        fprintf(stderr, "Error in create_image: save_image() failed!\n");
        rv = -1;
        goto DONE;
    }

    if (!arguments->silent) {
    	printf("Saved updated image to file '%s'.\n", arguments->output_path ? arguments->output_path : arguments->args[REPLACE_FILE_ARGS_IMAGE_FILE_PATH]);
    }
    rv = 0;

DONE:
    if (NULL != file_data) {
        free(file_data);
    }
    return rv;
}

