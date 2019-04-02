#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <argp.h>

#include "esperanto_flash_image.h"
#include "esperanto_flash_tool.h"
#include "esperanto_flash_image_create.h"
#include "esperanto_flash_image_extract.h"
#include "esperanto_flash_image_view.h"
#include "esperanto_flash_image_replace.h"
#include "esperanto_flash_image_extract_all.h"
#include "esperanto_flash_image_util.h"

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
    { "verbose",    'v', NULL,                  0,                      "Produce verbose output",                                           0 },
    { "quiet",      'q', NULL,                  0,                      "Don't produce any output",                                         0 },
    { "silent",     's', NULL,                  OPTION_ALIAS,           NULL,                                                               0 },
    { "partition",  'P', "partition_index",     OPTION_ARG_OPTIONAL,    "Create partition instead of image, or\nUse specified partition",   0 },
    { "id",         'I', NULL,                  0,                      "Use region IDs instead of indexes",                                0 },
    { "output",     'O', "output_path",         0,                      "Write the image to specified path (REPLACE command only)",         0 },
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
    case 'I':
        arguments->use_region_ids = true;
        break;
    case 'P':
        arguments->partition_mode = true;
        arguments->partition_index = arg;
        break;
    case 'O':
        arguments->output_path = arg;
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
            case ESPERANTO_FLASH_TOOL_COMMAND_EXTRACT_ALL_FILES:
                if (state->arg_num > EXTRACT_ALL_FILES_ARGS_TOTAL_COUNT) {
                    // too many arguments
                    argp_usage(state);
                }
                break;
            case ESPERANTO_FLASH_TOOL_COMMAND_EXTRACT_FILE:
            case ESPERANTO_FLASH_TOOL_COMMAND_REPLACE:
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
        arguments->args_count++;
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
            if ((state->arg_num < (EXTRACT_FILE_ARGS_BASE_COUNT + 2)) || (0 != (state->arg_num % 2))) {
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
            if ((state->arg_num < (REPLACE_FILE_ARGS_BASE_COUNT + 2)) || (0 != (state->arg_num % 2))) {
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

// argp data
static struct argp argp = { options, parse_opt, args_doc, doc, NULL, NULL, NULL };

// arguments
ARGUMENTS_t g_arguments;

int main(int argc, char ** argv) {
//    json_object * private_key_jobj;

	g_arguments.args_max_count = 1;
	g_arguments.args_count = 0;
	g_arguments.args = (char**)malloc(sizeof(char*));
    g_arguments.silent = false;
    g_arguments.verbose = false;
    g_arguments.command = ESPERANTO_FLASH_TOOL_COMMAND_INVALID;
    g_arguments.partition_index = 0;
    g_arguments.partition_mode = false;
    g_arguments.use_region_ids = false;
    g_arguments.output_path = NULL;


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

