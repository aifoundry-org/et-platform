#ifndef __ESPERANTO_FLASH_IMAGE_EXTRACT_H__
#define __ESPERANTO_FLASH_IMAGE_EXTRACT_H__

#include "esperanto_flash_tool.h"

int extract_file_from_partition(const ESPERANTO_FLASH_PARTITION_HEADER_t * partition_header, ESPERANTO_FLASH_REGION_ID_t region_id, uint32_t region_index, const char * extracted_file_path);
int extract_file(const ARGUMENTS_t * arguments);

#endif
