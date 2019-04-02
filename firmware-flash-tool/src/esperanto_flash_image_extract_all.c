#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <argp.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#include "esperanto_flash_image.h"
#include "esperanto_flash_tool.h"
#include "parse_template_file.h"
#include "esperanto_flash_image_view.h"
#include "esperanto_flash_image_extract.h"
#include "esperanto_flash_image_extract_all.h"
#include "esperanto_flash_image_util.h"

static int ensure_folder_exists(const char * folder_path) {
    struct stat st = {0};

    if (0 == stat(folder_path, &st)) {
        if (S_ISDIR(st.st_mode)) {
            return 0;
        }
        //path exists but is not a directory
        return -1;
    }

    if (0 != mkdir(folder_path, 0755)) {
        return -1;
    }

    return 0;
}

static int extract_all_files_from_partition(const ESPERANTO_FLASH_PARTITION_HEADER_t * partition_header, char * folder_path, uint32_t partition_index, bool silent, bool verbose) {
    uint32_t n;
    const ESPERANATO_REGION_INFO_t * regions;
    char file_path[PATH_MAX];
    uint32_t folder_path_length;
    char * base_folder_end;
    const char * region_name;
    char generic_region_name[16];

    (void)verbose;
    
    folder_path_length = (uint32_t)strlen(folder_path);
    base_folder_end = file_path + folder_path_length;
    memcpy(file_path, folder_path, folder_path_length);

    if ('/' != file_path[folder_path_length-1]) {
        file_path[folder_path_length] = '/';
        folder_path_length++;
        base_folder_end++;
        file_path[folder_path_length] = 0;
    }

    switch (partition_index) {
    case 1:
        memcpy(base_folder_end, "1/", 3);
        base_folder_end += 2;
        break;
    case 2:
        memcpy(base_folder_end, "2/", 3);
        base_folder_end += 2;
        break;
    default:
        break;
    }

    if (0 != ensure_folder_exists(file_path)) {
        fprintf(stderr, "Error in extract_all_files_from_partition: failed to create folder '%s'!\n", file_path);
        return -1;
    }

    regions = (const ESPERANATO_REGION_INFO_t*)(partition_header + 1);
    for (n = 0; n < partition_header->regions_count; n++) {
        region_name = region_id_to_name(regions[n].region_id);
        if (NULL == region_name) {
            sprintf(generic_region_name, "REGION_%X", regions[n].region_id);
            region_name = generic_region_name;
        }
        strcpy(base_folder_end, region_name);

        if (0 != extract_file_from_partition(partition_header, ESPERANTO_FLASH_REGION_ID_INVALID, n, file_path)) {
            fprintf(stderr, "Error in extract_all_files_from_partition: extract_file_from_region() failed!\n");
            return -1;
        }

        if (!silent) {
            printf("  Extracted file '%s'\n", file_path);
        }
    }

    return 0;
}

int extract_all_files(const ARGUMENTS_t * arguments) {
    int rv;
	unsigned long int ul;
	char * endptr;
    const ESPERANTO_FLASH_PARTITION_HEADER_t * partition_header;
    const ESPERANTO_FLASH_PARTITION_HEADER_t * partition_header2;
    size_t file_size;
    uint8_t * file_data = NULL;
    uint32_t partition_index = 0;
	
    if (NULL != arguments->partition_index) {
        ul = strtoul(arguments->partition_index, &endptr, 0);
        if (0 != *endptr) {
            fprintf(stderr, "Error in extract_all_files: invalid partition index value '%s'!\n", arguments->partition_index);
            rv = -1;
            goto DONE;
        }
        partition_index = (uint32_t)ul;
        if (partition_index < 1 || partition_index > 2) {
            fprintf(stderr, "Error in extract_all_files: partition index (%u) not in valid range (1 <= index <= 2)!\n", partition_index);
            rv = -1;
            goto DONE;
        }
    }

    if (!arguments->silent) {
        printf("Command: EXTRACT ALL\n");
        printf("Image/partition path: '%s'\n", arguments->args[EXTRACT_ALL_FILES_ARGS_IMAGE_FILE_PATH]);
        printf("Folder path: '%s'\n", arguments->args[EXTRACT_ALL_FILES_ARGS_EXTRACTED_FILES_FOLDER_PATH]);
    }

    if (0 != load_file(arguments->args[EXTRACT_FILE_ARGS_IMAGE_FILE_PATH], (char**)&file_data, &file_size)) {
        fprintf(stderr, "Error in extract_all_files: Failed to open/read file '%s'!\n", arguments->args[EXTRACT_FILE_ARGS_IMAGE_FILE_PATH]);
        rv = -1;
        goto DONE;
    }

    if (0 != (file_size & (FLASH_PAGE_SIZE - 1))) {
        fprintf(stderr, "Error in extract_all_files: file size is not a multiple of 4KB!\n");
        rv = -1;
        goto DONE;
    }
    partition_header = (const ESPERANTO_FLASH_PARTITION_HEADER_t*)file_data;
    if (0 != verify_partition_header(partition_header)) {
        fprintf(stderr, "Error in extract_all_files: verify_partition_header() failed!\n");
        rv = -1;
        goto DONE;
    }

    if (file_size == (FLASH_PAGE_SIZE * partition_header->partition_size)) {
        if (2 == partition_index) {
            fprintf(stderr, "Error in extract_all_files: non-zero partition index (%u) was specified, but the image contains only one partition!\n", partition_index);
            rv = -1;
            goto DONE;
        }
        if (0 != extract_all_files_from_partition(partition_header, arguments->args[EXTRACT_ALL_FILES_ARGS_EXTRACTED_FILES_FOLDER_PATH], 0, arguments->silent, arguments->verbose)) {
            fprintf(stderr, "Error in extract_all_files: extract_all_files_from_partition() failed!\n");
            rv = -1;
            goto DONE;
        }
    } else if (file_size == (2 * FLASH_PAGE_SIZE * partition_header->partition_size)) {
        if (1 == partition_index) {
            if (0 != extract_all_files_from_partition(partition_header, arguments->args[EXTRACT_ALL_FILES_ARGS_EXTRACTED_FILES_FOLDER_PATH], 0, arguments->silent, arguments->verbose)) {
                fprintf(stderr, "Error in extract_all_files: extract_all_files_from_partition() failed!\n");
                rv = -1;
                goto DONE;
            }
        } else if (2 == partition_index) {
            partition_header = (const ESPERANTO_FLASH_PARTITION_HEADER_t*)(file_data + (file_size / 2));
            if (0 != verify_partition_header(partition_header)) {
                fprintf(stderr, "Error in extract_all_files: verify_partition_header() failed on 2nd partition!\n");
                return -1;
            }
            if (0 != extract_all_files_from_partition(partition_header, arguments->args[EXTRACT_ALL_FILES_ARGS_EXTRACTED_FILES_FOLDER_PATH], 0, arguments->silent, arguments->verbose)) {
                fprintf(stderr, "Error in extract_all_files: extract_all_files_from_partition() failed!\n");
                rv = -1;
                goto DONE;
            }
        } else {
            partition_header2 = (const ESPERANTO_FLASH_PARTITION_HEADER_t*)(file_data + (file_size / 2));
            if (0 != verify_partition_header(partition_header2)) {
                fprintf(stderr, "Error in extract_all_files: verify_partition_header() failed on 2nd partition!\n");
                return -1;
            }
            if (0 != extract_all_files_from_partition(partition_header, arguments->args[EXTRACT_ALL_FILES_ARGS_EXTRACTED_FILES_FOLDER_PATH], 1, arguments->silent, arguments->verbose)) {
                fprintf(stderr, "Error in extract_all_files: extract_all_files_from_partition(1) failed!\n");
                rv = -1;
                goto DONE;
            }
            if (0 != extract_all_files_from_partition(partition_header2, arguments->args[EXTRACT_ALL_FILES_ARGS_EXTRACTED_FILES_FOLDER_PATH], 2, arguments->silent, arguments->verbose)) {
                fprintf(stderr, "Error in extract_all_files: extract_all_files_from_partition(2) failed!\n");
                rv = -1;
                goto DONE;
            }
        }
    } else {
        fprintf(stderr, "Error in extract_all_files: invalid file size!\n");
        return -1;
    }

    rv = 0; 

DONE:
    if (NULL != file_data) {
        free(file_data);
    }
    return rv;
}
