#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <argp.h>

#include "esperanto_flash_image.h"
#include "esperanto_flash_tool.h"
#include "parse_template_file.h"

#define offsetof(st, m) ((size_t)&(((st *)0)->m))

// version string
const char *argp_program_version = "esperanto_flash_tool 1.0";

// bug address
const char *argp_program_bug_address = "problems@esperantotech.com";

// short program documentation
static char doc[] = "Generates, views, updates or extracts files from flash image or partition";

// arguments description
static char args_doc[] = "create      <image_file> <template_file>\n" \
                         "view        <image_file>\n" \
                         "extract     <image_file> <region_index> <extracted_file_path> [<region_index_2> <extracted_file_2_path> ... [<region_index_n> <extracted_file_n_path>]]\n" \
                         "extract_all <image_file> <extracted_files_folder_path>\n" \
                         "replace     <image_file> <region_index> <file_path> [<region_index_2> <file_2_path> ... [<region_index_n> <file_n_path>]]\n";
// options
static struct argp_option options[] = {
    { "verbose",    'v', NULL,  0,                      "Produce verbose output",                                           0 },
    { "quiet",      'q', NULL,  0,                      "Don't produce any output",                                         0 },
    { "silent",     's', 0,     OPTION_ALIAS,           NULL,                                                               0 },
    { "partition",  'P', 0,     OPTION_ARG_OPTIONAL,    "Create partition instead of image, or\nUse specified partition",   0 },
    { "id",         'I', 0,     0,                      "Use region ID instead of region index",                            0 },
    { 0 }
};

// options parser
static error_t parse_opt(int key, char * arg, struct argp_state * state) {
    ARGUMENTS_t * arguments = state->input;

    switch (key) {
    case 'q':
    case 's':
        arguments->silent = true;
        break;
    case 'v':
        arguments->verbose = true;
        break;
    
    case ARGP_KEY_ARG:
        if (0 == state->arg_num) {
            if (0 == strcasecmp(arg, "create")) {
                arguments->command = ESPERANTO_FLASH_TOOL_COMMAND_CREATE;
            } else if (0 == strcasecmp(arg, "view")) {
                arguments->command = ESPERANTO_FLASH_TOOL_COMMAND_VIEW;
            } else if (0 == strcasecmp(arg, "extract")) {
                arguments->command = ESPERANTO_FLASH_TOOL_COMMAND_EXTRACT_FILE;
            } else if (0 == strcasecmp(arg, "extract_all")) {
                arguments->command = ESPERANTO_FLASH_TOOL_COMMAND_EXTRACT_ALL_FILES;
            } else if (0 == strcasecmp(arg, "replace")) {
                arguments->command = ESPERANTO_FLASH_TOOL_COMMAND_REPLACE;
            } else {
                fprintf(stderr, "Invalid command '%s'!\n", arg);
                argp_usage(state);
            }
        } else {
            switch (arguments->command) {
            case ESPERANTO_FLASH_TOOL_COMMAND_CREATE:
                if (state->arg_num > CREATE_IMAGE_ARGS_TOTAL_COUNT) {
                    // too many arguments
                    argp_usage(state);
                }
                break;
            case ESPERANTO_FLASH_TOOL_COMMAND_VIEW:
                if (state->arg_num > VIEW_IMAGE_ARGS_TOTAL_COUNT) {
                    // too many arguments
                    argp_usage(state);
                }
                break;
            case ESPERANTO_FLASH_TOOL_COMMAND_EXTRACT_FILE:
                if (state->arg_num > EXTRACT_FILE_ARGS_BASE_COUNT) {
                    // too many arguments
                    argp_usage(state);
                }
                break;
            case ESPERANTO_FLASH_TOOL_COMMAND_EXTRACT_ALL_FILES:
                if (state->arg_num > EXTRACT_ALL_FILES_ARGS_TOTAL_COUNT) {
                    // too many arguments
                    argp_usage(state);
                }
                break;
            case ESPERANTO_FLASH_TOOL_COMMAND_REPLACE:
                if (state->arg_num > REPLACE_FILE_ARGS_BASE_COUNT) {
                    // too many arguments
                    argp_usage(state);
                }
                break;
            default:
                // we should never get here
                argp_usage(state);
                break;
            }
        }

        if (g_arguments.args_max_count <= state->arg_num) {
        	g_arguments.args_max_count = g_arguments.args_max_count * 2;
        	g_arguments.args = (char**)realloc(g_arguments.args, g_arguments.args_max_count * sizeof(char*));
        	if (NULL == g_arguments.args) {
        		fprintf(stderr, "realloc() failed!\n");
        		exit(-1);
        	}
        }
        arguments->args[state->arg_num] = arg;
        break;    
    
    case ARGP_KEY_END:
        if (state->arg_num < 2) {
            // not enough arguments
            argp_usage(state);
        }
        switch (arguments->command) {
        case ESPERANTO_FLASH_TOOL_COMMAND_CREATE:
            if (state->arg_num < CREATE_IMAGE_ARGS_TOTAL_COUNT) {
                // not enough arguments
                argp_usage(state);
            }
            break;
        case ESPERANTO_FLASH_TOOL_COMMAND_VIEW:
            if (state->arg_num < VIEW_IMAGE_ARGS_TOTAL_COUNT) {
                // not enough arguments
                argp_usage(state);
            }
            break;
        case ESPERANTO_FLASH_TOOL_COMMAND_EXTRACT_FILE:
            if (state->arg_num < EXTRACT_FILE_ARGS_BASE_COUNT) {
                // not enough arguments
                argp_usage(state);
            }
            break;
        case ESPERANTO_FLASH_TOOL_COMMAND_EXTRACT_ALL_FILES:
            if (state->arg_num < EXTRACT_ALL_FILES_ARGS_TOTAL_COUNT) {
                // not enough arguments
                argp_usage(state);
            }
            break;
        case ESPERANTO_FLASH_TOOL_COMMAND_REPLACE:
            if (state->arg_num < REPLACE_FILE_ARGS_BASE_COUNT) {
                // not enough arguments
                argp_usage(state);
            }
            break;
        default:
            // we should never get here
            argp_usage(state);
            break;
        }
        break;
    
    default:
        return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

static const uint32_t crc32Table[256] = {
		0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
		0xe963a535, 0x9e6495a3,	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
		0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
		0xf3b97148, 0x84be41de,	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
		0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,	0x14015c4f, 0x63066cd9,
		0xfa0f3d63, 0x8d080df5,	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
		0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,	0x35b5a8fa, 0x42b2986c,
		0xdbbbc9d6, 0xacbcf940,	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
		0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
		0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
		0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,	0x76dc4190, 0x01db7106,
		0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
		0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
		0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
		0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
		0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
		0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
		0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
		0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
		0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
		0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
		0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
		0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
		0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
		0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
		0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
		0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
		0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
		0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
		0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
		0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
		0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
		0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
		0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
		0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
		0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
		0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
		0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
		0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
		0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
		0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
		0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
		0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};
static uint32_t crc32_sum(const void *data, size_t size) {
	const uint8_t *p = (const uint8_t*)data;
    uint32_t crc = ~0U;

	while (size) {
		crc = crc32Table[(crc ^ *p++) & 0xff] ^ (crc >> 8);
        size--;
    }

	return crc ^ ~0U;
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
                region_size = ((uint32_t)file_size + (FLASH_PAGE_SIZE - 1)) & (uint32_t)(~(FLASH_PAGE_SIZE - 1));
                region_size = region_size / FLASH_PAGE_SIZE;
            }

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
        set_counter_field(partition_data + regions_map[priority_designator_region_index].region_offset, FLASH_PAGE_SIZE, partition_info->priority);
    }
    if (partition_info->attempted_boot_counter > 0) {
        set_counter_field(partition_data + regions_map[boot_counters_region_index].region_offset, FLASH_PAGE_SIZE/2, partition_info->priority);
    }
    if (partition_info->completed_boot_counter > 0) {
        set_counter_field(partition_data + regions_map[boot_counters_region_index].region_offset + FLASH_PAGE_SIZE/2, FLASH_PAGE_SIZE/2, partition_info->priority);
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

static int save_image(const char * filename, const uint8_t * image_data, uint32_t image_size) {
    int rv;
    FILE * f = fopen(filename, "wb");
    if (NULL == f) {
        fprintf(stderr, "Error in save_image: failed to open file '%s' for writing!\n", filename);
        return -1;
    }

    if (1 != fwrite(image_data, image_size, 1, f)) {
        fprintf(stderr, "Error in save_image: failed to write image to file!\n");
        rv = -1;
        goto DONE;
    }
    rv = 0;

DONE:
    fclose(f);
    return rv;
}

static int verify_partition_header(const ESPERANTO_FLASH_PARTITION_HEADER_t * partition_header) {
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

static void dumphex(const void * data, uint32_t data_size, uint32_t max_line_length) {
    uint32_t col = 0;
    const uint8_t * ps = (const uint8_t *)data;
    const uint8_t * pe = ps + data_size;
    while (ps < pe) {
        printf(" %02x", *ps);
        ps++;
        col++;
        if (col == max_line_length) {
            printf("\n");
            col = 0;
        }
    }
    if (0 != col && 0 != max_line_length) {
        printf("\n");
    }
}

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

static int view_image_or_partition(const uint8_t * file_data, uint32_t file_size, bool silent, bool verbose) {
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

static int create_image(const ARGUMENTS_t * arguments) {
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

    view_image_or_partition(image_data, template.image_type ? image_size : partition_size, arguments->silent, arguments->verbose);

    if (0 != save_image(arguments->args[CREATE_IMAGE_ARGS_PARTITION_FILE_PATH], image_data, template.image_type ? image_size : partition_size)) {
        fprintf(stderr, "Error in create_image: save_image() failed!\n");
        rv = -1;
        goto DONE;
    }

    if (!arguments->silent) {
    	printf("Saved image to file.\n");
    }
    rv = 0;

DONE:
    if (NULL != image_data) {
        free(image_data);
    }
    free_template_info(&template);
    return rv;
}

static int view_image(const ARGUMENTS_t * arguments) {
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

static int extract_file(const ARGUMENTS_t * arguments) {
    if (!arguments->silent) {
        printf("Command: EXTRACT\n");
        printf("Image/partition path: '%s'\n", arguments->args[EXTRACT_FILE_ARGS_IMAGE_FILE_PATH]);
    }
    return 0;
}

static int extract_all_files(const ARGUMENTS_t * arguments) {
    if (!arguments->silent) {
        printf("Command: EXTRACT ALL\n");
        printf("Image/partition path: '%s'\n", arguments->args[EXTRACT_ALL_FILES_ARGS_IMAGE_FILE_PATH]);
        printf("Folder path: '%s'\n", arguments->args[EXTRACT_ALL_FILES_ARGS_EXTRACTED_FILES_FOLDER_PATH]);
    }
    return 0;
}

static int replace_files(const ARGUMENTS_t * arguments) {
    if (!arguments->silent) {
        printf("Command: REPLACE\n");
        printf("Image/partition path: '%s'\n", arguments->args[REPLACE_FILE_ARGS_IMAGE_FILE_PATH]);
    }
    return 0;
}

// argp data
static struct argp argp = { options, parse_opt, args_doc, doc, NULL, NULL, NULL };

// arguments
ARGUMENTS_t g_arguments;

int main(int argc, char ** argv) {
//    json_object * private_key_jobj;

	g_arguments.args_max_count = 1;
	g_arguments.args = (char**)malloc(sizeof(char*));
    g_arguments.silent = false;
    g_arguments.verbose = false;
    g_arguments.command = ESPERANTO_FLASH_TOOL_COMMAND_INVALID;
    g_arguments.partition_index = 0;
    g_arguments.partition_mode = false;
    g_arguments.use_region_ids = false;


    argp_parse(&argp, argc, argv, 0, 0, &g_arguments);

    switch (g_arguments.command) {
    case ESPERANTO_FLASH_TOOL_COMMAND_CREATE:
        return create_image(&g_arguments);
    case ESPERANTO_FLASH_TOOL_COMMAND_VIEW:
        return view_image(&g_arguments);
    case ESPERANTO_FLASH_TOOL_COMMAND_EXTRACT_FILE:
        return extract_file(&g_arguments);
    case ESPERANTO_FLASH_TOOL_COMMAND_EXTRACT_ALL_FILES:
        return extract_all_files(&g_arguments);
    case ESPERANTO_FLASH_TOOL_COMMAND_REPLACE:
        return replace_files(&g_arguments);
    default:
        // we should never get here
        return -1;
    }
}
