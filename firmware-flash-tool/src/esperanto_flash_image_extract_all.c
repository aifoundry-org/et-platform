#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <argp.h>

#include "esperanto_flash_image.h"
#include "esperanto_flash_tool.h"
#include "parse_template_file.h"
#include "esperanto_flash_image_extract_all.h"
#include "esperanto_flash_image_util.h"

int extract_all_files(const ARGUMENTS_t * arguments) {
    if (!arguments->silent) {
        printf("Command: EXTRACT ALL\n");
        printf("Image/partition path: '%s'\n", arguments->args[EXTRACT_ALL_FILES_ARGS_IMAGE_FILE_PATH]);
        printf("Folder path: '%s'\n", arguments->args[EXTRACT_ALL_FILES_ARGS_EXTRACTED_FILES_FOLDER_PATH]);
    }
    return 0;
}
