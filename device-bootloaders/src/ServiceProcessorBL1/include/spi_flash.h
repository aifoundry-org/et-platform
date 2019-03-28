#ifndef __SPI_FLASH_H__
#define __SPI_FLASH_H__

#include <stdint.h>
#include <stdbool.h>

typedef enum SPI_FLASH_ID {
    SPI_FLASH_INVALID,
    SPI_FLASH_ON_PACKAGE,
    SPI_FLASH_OFF_PACKAGE
} SPI_FLASH_ID_t;

int spi_flash_init(SPI_FLASH_ID_t flash_id);
int spi_flash_rdsr(SPI_FLASH_ID_t flash_id, uint8_t * status);
int spi_flash_rdid(SPI_FLASH_ID_t flash_id, uint8_t * manufacturer_id, uint16_t * device_id);
int spi_flash_rdsfdp(SPI_FLASH_ID_t flash_id, uint32_t address, uint8_t * data_buffer, uint32_t data_buffer_size);

#endif
