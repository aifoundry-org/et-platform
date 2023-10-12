#ifndef __ESPERANTO_FLASH_IMAGE_UTIL_H__
#define __ESPERANTO_FLASH_IMAGE_UTIL_H__

#include <stdint.h>
#include <stdlib.h>

uint32_t crc32_sum(const void *data, size_t size);
int load_file(const char * file_path, char ** buffer, size_t * buffer_size);
int save_image(const char * filename, const uint8_t * image_data, uint32_t image_size);
void dumphex(const void * data, uint32_t data_size, uint32_t max_line_length);
size_t get_filesize(const char *file_path);
#endif
