#ifndef __ESPERANTO_FLASH_IMAGE_VIEW_H__
#define __ESPERANTO_FLASH_IMAGE_VIEW_H__

#include "esperanto_flash_tool.h"

int verify_partition_header(const ESPERANTO_FLASH_PARTITION_HEADER_t * partition_header);
int view_image_or_partition(const uint8_t * file_data, uint32_t file_size, bool silent, bool verbose);
int view_image(const ARGUMENTS_t * arguments);

#endif
