/*-------------------------------------------------------------------------
* Copyright (C) 2018, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

#ifndef __BL1_SPI_FLASH_H__
#define __BL1_SPI_FLASH_H__

#include <stdint.h>
#include <stdbool.h>

#include "service_processor_BL1_data.h"

int spi_flash_init(SPI_FLASH_ID_t flash_id);
int spi_flash_wren(SPI_FLASH_ID_t flash_id);
int spi_flash_rdsr(SPI_FLASH_ID_t flash_id, uint8_t *status);
int spi_flash_rdid(SPI_FLASH_ID_t flash_id, uint8_t *manufacturer_id, uint8_t device_id[2]);
int spi_flash_rdsfdp(SPI_FLASH_ID_t flash_id, uint32_t address, uint8_t *data_buffer,
                     uint32_t data_buffer_size);
int spi_flash_normal_read(SPI_FLASH_ID_t flash_id, uint32_t address, uint8_t *data_buffer,
                          uint32_t data_buffer_size);
int spi_flash_fast_read(SPI_FLASH_ID_t flash_id, uint32_t address, uint8_t *data_buffer,
                        uint32_t data_buffer_size);
int spi_flash_program(SPI_FLASH_ID_t flash_id, uint32_t address, const uint8_t *data_buffer,
                      uint32_t data_buffer_size);

#endif
