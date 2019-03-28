#ifndef __SPI_FLASH_FS_H__
#define __SPI_FLASH_FS_H__

#include "spi_flash.h"

int flash_fs_init(SPI_FLASH_ID_t flash_id);
int flash_fs_get_flash_size(SPI_FLASH_ID_t flash_id, uint32_t * size);

#endif
