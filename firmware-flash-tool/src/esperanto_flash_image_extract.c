#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stddef.h>

#include "esperanto_flash_image.h"
#include "esperanto_flash_tool.h"
#include "parse_template_file.h"
#include "esperanto_flash_image_extract.h"
#include "esperanto_flash_image_util.h"
#include "esperanto_flash_image_view.h"

static int extract_file_from_region(const ESPERANATO_REGION_INFO_t * region, const uint8_t * partition_data, uint32_t partition_size_in_bytes, const char * extracted_file_path) {
    const ESPERANATO_FILE_INFO_t * file_info;
    uint32_t region_size, region_end_offset;
    const uint8_t * file_data;

    if (region->region_info_checksum != crc32_sum(region, offsetof(ESPERANATO_REGION_INFO_t, region_info_checksum))) {
        fprintf(stderr, "Error in extract_file_from_region: region header CRC checksum is invalid!\n");
        return -1;
    }
    region_size = FLASH_PAGE_SIZE * region->region_reserved_size;
    region_end_offset = region->region_offset + region_size;
    if (partition_size_in_bytes < region_end_offset) {
        fprintf(stderr, "Error in extract_file_from_region: region size is invalid!\n");
        return -1;
    }

    switch (region->region_id) {
    case ESPERANTO_FLASH_REGION_ID_INVALID:
        return -1;

    case ESPERANTO_FLASH_REGION_ID_PRIORITY_DESIGNATOR:
    case ESPERANTO_FLASH_REGION_ID_BOOT_COUNTERS:
    case ESPERANTO_FLASH_REGION_ID_CONFIGURATION_DATA:
        if (0 != save_image(extracted_file_path, partition_data + (region->region_offset * FLASH_PAGE_SIZE), FLASH_PAGE_SIZE)) {
            fprintf(stderr, "Error in extract_file_from_region: save_image() failed!\n");
            return -1;
        }
        return 0;

    default:
        file_info = (const ESPERANATO_FILE_INFO_t*)(partition_data + (region->region_offset * FLASH_PAGE_SIZE));
        if (file_info->file_header_crc != crc32_sum(file_info, offsetof(ESPERANATO_FILE_INFO_t, file_header_crc))) {
            fprintf(stderr, "Error in extract_file_from_region: file header CRC checksum is invalid!\n");
            return -1;
        }
        if (file_info->file_size > (region_size - sizeof(ESPERANATO_FILE_INFO_t))) {
            fprintf(stderr, "Error in extract_file_from_region: file size too large!\n");
            return -1;
        }
        file_data = (const uint8_t*)(file_info + 1);
        if (0 != save_image(extracted_file_path, file_data, file_info->file_size)) {
            fprintf(stderr, "Error in extract_file_from_region: save_image() failed!\n");
            return -1;
        }
        return 0;
    }
}

int extract_file_from_partition(const ESPERANTO_FLASH_PARTITION_HEADER_t * partition_header, ESPERANTO_FLASH_REGION_ID_t region_id, uint32_t region_index, const char * extracted_file_path) {
    uint32_t n;
    const ESPERANATO_REGION_INFO_t * regions;
    const uint8_t * partition_data = (const uint8_t*)partition_header;
    uint32_t partition_size_in_bytes = partition_header->partition_size * FLASH_PAGE_SIZE;

    regions = (const ESPERANATO_REGION_INFO_t*)(partition_header + 1);
    if (ESPERANTO_FLASH_REGION_ID_INVALID == region_id) {
        if (region_index < partition_header->regions_count) {
            return extract_file_from_region(&regions[region_index], partition_data, partition_size_in_bytes, extracted_file_path);
        } else {
            fprintf(stderr, "Error in extract_file_from_partition: region_index (%u) is out of range of valid regions (0..%u)!\n", region_index, partition_header->regions_count - 1);
            return -1;
        }
    }
    for (n = 0; n < partition_header->regions_count; n++) {
        if (regions[n].region_id == region_id) {
            return extract_file_from_region(&regions[n], partition_data, partition_size_in_bytes, extracted_file_path);
        }
    }

    fprintf(stderr, "Error in extract_file_from_partition: region_d (0x%x) not found!\n", region_id);
    return -1;
}

static int extract_files_from_partition(const ESPERANTO_FLASH_PARTITION_HEADER_t * partition_header, bool use_region_ids, uint32_t args_count, char ** args, bool silent, bool verbose) {
    uint32_t n;
	ESPERANTO_FLASH_REGION_ID_t region_id;
    const char * file_path;
	const char * region_name;
	uint32_t region_index;
	unsigned long int ul;
	char * endptr;

    (void)verbose;

    for (n = EXTRACT_FILE_ARGS_BASE_COUNT; n < args_count; n += 2) {
        file_path = args[n + 1];
        region_id = region_name_to_id(args[n]);
        if (ESPERANTO_FLASH_REGION_ID_INVALID != region_id) {
            region_name = region_id_to_name(region_id);
        } else {
            ul = strtoul(args[n], &endptr, 0);
            if (0 != *endptr) {
                fprintf(stderr, "Error in extract_files_from_partition: invalid region id or index value '%s'!\n", args[n]);
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

        if (0 != extract_file_from_partition(partition_header, region_id, region_index, file_path)) {
            fprintf(stderr, "Error in extract_files_from_partition: extract_file_from_partition() failed!\n");
            return -1;
        } else {
            if (!silent) {
                if (ESPERANTO_FLASH_REGION_ID_INVALID != region_id) {
                    printf("Extracted region 0x%x (%s) to file '%s'\n", region_id, region_name, file_path);
                } else {
                    printf("Extracted region %u to file '%s'\n", region_index, file_path);
                }
            }
        }
    }

    return 0;
}

int extract_file(const ARGUMENTS_t * arguments) {
    int rv;
	uint32_t n;
	ESPERANTO_FLASH_REGION_ID_t region_id;
	const char * region_name;
	uint32_t region_index;
	unsigned long int ul;
	char * endptr;
    const ESPERANTO_FLASH_PARTITION_HEADER_t * partition_header;
    size_t file_size;
    uint8_t * file_data = NULL;
    uint32_t partition_index = 0;
	
    if (NULL != arguments->partition_index) {
        ul = strtoul(arguments->partition_index, &endptr, 0);
        if (0 != *endptr) {
            fprintf(stderr, "Error in extract_file: invalid partition index value '%s'!\n", arguments->partition_index);
            rv = -1;
            goto DONE;
        }
        partition_index = (uint32_t)ul;
        if (partition_index < 1 || partition_index > 2) {
            fprintf(stderr, "Error in extract_file: partition index (%u) not in valid range (1 <= index <= 2)!\n", partition_index);
            rv = -1;
            goto DONE;
        }
    }

    if (!arguments->silent) {
        printf("Command: EXTRACT\n");
        printf("Image/partition path: '%s'\n", arguments->args[EXTRACT_FILE_ARGS_IMAGE_FILE_PATH]);
        if (0 != partition_index) {
            printf("Partiton index: %u\n", partition_index);
        }
        for (n = EXTRACT_FILE_ARGS_BASE_COUNT; n < arguments->args_count; n += 2) {
        	region_id = region_name_to_id(arguments->args[n]);
        	if (ESPERANTO_FLASH_REGION_ID_INVALID != region_id) {
        		region_name = region_id_to_name(region_id);
        		printf("region id: 0x%x (%s), file path: '%s'\n", region_id, region_name ? region_name : "unknown", arguments->args[n + 1]);
        	} else {
        		ul = strtoul(arguments->args[n], &endptr, 0);
        		if (0 != *endptr) {
        			fprintf(stderr, "Error in extract_file: invalid region id or index value '%s'!\n", arguments->args[n]);
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
        fprintf(stderr, "Error in extract_file: Failed to open/read file '%s'!\n", arguments->args[EXTRACT_FILE_ARGS_IMAGE_FILE_PATH]);
        rv = -1;
        goto DONE;
    }

    if (0 != (file_size & (FLASH_PAGE_SIZE - 1))) {
        fprintf(stderr, "Error in extract_file: file size is not a multiple of 4KB!\n");
        rv = -1;
        goto DONE;
    }
    partition_header = (const ESPERANTO_FLASH_PARTITION_HEADER_t*)file_data;
    if (0 != verify_partition_header(partition_header)) {
        fprintf(stderr, "Error in extract_file: verify_partition_header() failed!\n");
        rv = -1;
        goto DONE;
    }

    if (file_size == (FLASH_PAGE_SIZE * partition_header->partition_size)) {
        if (2 == partition_index) {
            fprintf(stderr, "Error in extract_file: non-zero partition index (%u) was specified, but the image contains only one partition!\n", partition_index);
            rv = -1;
            goto DONE;
        }
        if (0 != extract_files_from_partition(partition_header, arguments->use_region_ids, arguments->args_count, arguments->args, arguments->silent, arguments->verbose)) {
            fprintf(stderr, "Error in extract_file: extract_files_from_partition() failed!\n");
            rv = -1;
            goto DONE;
        }
    } else if (file_size == (2 * FLASH_PAGE_SIZE * partition_header->partition_size)) {
        if (0 == partition_index) {
            if (!arguments->silent) {
                printf("Partition index was not specified! Assuming first partition.\n");
                partition_index = 1;
            }
        }
        if (1 == partition_index) {
            if (0 != extract_files_from_partition(partition_header, arguments->use_region_ids, arguments->args_count, arguments->args, arguments->silent, arguments->verbose)) {
                fprintf(stderr, "Error in extract_file: extract_files_from_partition() failed!\n");
                rv = -1;
                goto DONE;
            }
        } else {
            partition_header = (const ESPERANTO_FLASH_PARTITION_HEADER_t*)(file_data + (file_size / 2));
            if (0 != verify_partition_header(partition_header)) {
                fprintf(stderr, "Error in extract_file: verify_partition_header() failed on 2nd partition!\n");
                return -1;
            }
            if (0 != extract_files_from_partition(partition_header, arguments->use_region_ids, arguments->args_count, arguments->args, arguments->silent, arguments->verbose)) {
                fprintf(stderr, "Error in extract_file: extract_files_from_partition() failed!\n");
                rv = -1;
                goto DONE;
            }
        }
    } else {
        fprintf(stderr, "Error in extract_file: invalid file size!\n");
        return -1;
    }

    rv = 0; 

DONE:
    if (NULL != file_data) {
        free(file_data);
    }
    return rv;
}

