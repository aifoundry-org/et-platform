#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <argp.h>

#include "esperanto_flash_image.h"
#include "esperanto_flash_tool.h"
#include "parse_template_file.h"

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

static int create_image(const ARGUMENTS_t * arguments) {
    int rv;
    TEMPLATE_INFO_t template;
    uint32_t partition_size;
    uint32_t image_size;

    if (!arguments->silent) {
        printf("Command: CREATE\n");
        printf("Image/partition path: '%s'\n", arguments->args[CREATE_IMAGE_ARGS_PARTITION_FILE_PATH]);
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

DONE:
    free_template_info(&template);
    return 0;
}

static int view_image(const ARGUMENTS_t * arguments) {
    if (!arguments->silent) {
        printf("Command: VIEW\n");
        printf("Image/partition path: '%s'\n", arguments->args[VIEW_IMAGE_ARGS_IMAGE_FILE_PATH]);
    }
    return 0;
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
