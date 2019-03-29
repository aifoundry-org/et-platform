#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <argp.h>

#include "esperanto_flash_image.h"
#include "esperanto_flash_tool.h"
#include "parse_template_file.h"
#include "esperanto_flash_image_replace.h"
#include "esperanto_flash_image_util.h"

int replace_files(const ARGUMENTS_t * arguments) {
    if (!arguments->partition_mode || NULL == arguments->partition_index) {
        fprintf(stderr, "The replace file option requires the partition option and partition index!\n");
        return -1;
    }
    if (!arguments->silent) {
        printf("Command: REPLACE\n");
        printf("Image/partition path: '%s'\n", arguments->args[REPLACE_FILE_ARGS_IMAGE_FILE_PATH]);
    }
    return 0;
}

