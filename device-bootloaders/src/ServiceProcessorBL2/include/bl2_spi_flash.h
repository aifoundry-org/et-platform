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

#ifndef __BL2_SPI_FLASH_H__
#define __BL2_SPI_FLASH_H__

#include <stdint.h>
#include <stdbool.h>

#include "service_processor_BL2_data.h"

// PAGE:           : 256 B
// BLOCK (256 * Page): 64 KB
#define SPI_FLASH_PAGE_SIZE   256
#define SPI_FLASH_BLOCK_SIZE  0x10000U
#define SPI_FLASH_BLOCK_MASK  0xFFFFul
#define SPI_FLASH_SECTOR_MASK 0x0FFFul

/*! \fn int SPI_Flash_Initialize (uint32_t type)
    \brief This function initialize flash fs according to given type
    \param type - type of flash fs to be initialized
    \return The function call status, pass/fail.
*/
int SPI_Flash_Initialize (uint32_t type);

/*! \fn int SPI_Flash_Read_Page(SPI_FLASH_ID_t flash_id, uint32_t address, uint32_t* data_buffer, uint32_t size)
    \brief This function reads the data from the file of the particular region.
    \param flash_id flash device id
    \param address - memory address
    \param data_buffer - buffer to store data read from the address
    \param size - size of data to read
    \return The function call status, pass/fail.
*/
int SPI_Flash_Read_Page(SPI_FLASH_ID_t flash_id, uint32_t address, uint32_t* data_buffer, uint32_t size);

/*! \fn int SPI_Flash_Write_Page(SPI_FLASH_ID_t flash_id, uint32_t address, uint32_t *data, uint32_t size) 
    \brief This function reads the data from the file of the particular region.
    \param flash_id flash device id
    \param address - memory address
    \param data - data to be written on to memory address
    \param size - size of data
    \return The function call status, pass/fail.
*/
int SPI_Flash_Write_Page(SPI_FLASH_ID_t flash_id, uint32_t address, uint32_t *data, uint32_t size);

/*! \fn int spi_flash_wren(SPI_FLASH_ID_t flash_id)
    \brief This function performs write enable of flash memory.
    \param flash_id - flash id (SPI_FLASH_ON_PACKAGE or SPI_FLASH_OFF_PACKAGE)
    \return The function call status, pass/fail.
*/
int spi_flash_wren(SPI_FLASH_ID_t flash_id);

/*! \fn int spi_flash_rdsr(SPI_FLASH_ID_t flash_id, uint8_t *status)
    \brief This function performs read of status register of flash memory.
    \param flash_id - flash id (SPI_FLASH_ON_PACKAGE or SPI_FLASH_OFF_PACKAGE)
    \param status - return value of the status register of flash memory
    \return The function call status, pass/fail.
*/
int spi_flash_rdsr(SPI_FLASH_ID_t flash_id, uint8_t *status);

/*! \fn int spi_flash_rdid(SPI_FLASH_ID_t flash_id, uint8_t *manufacturer_id, uint8_t device_id[2])
    \brief This function performs read of manufacturer and device id of flash memory.
    \param flash_id - flash id (SPI_FLASH_ON_PACKAGE or SPI_FLASH_OFF_PACKAGE)
    \param manufacturer_id - manufacturer id of flash memory
    \param device_id - device id of flash memory
    \return The function call status, pass/fail.
*/
int spi_flash_rdid(SPI_FLASH_ID_t flash_id, uint8_t *manufacturer_id, uint8_t device_id[2]);

/*! \fn int spi_flash_rdsfdp(SPI_FLASH_ID_t flash_id, uint32_t address, uint8_t *data_buffer, 
*                            uint32_t data_buffer_size)
    \brief This function performs read of SFDP (Serial Flash Discoverable Parameter) area.
    \param flash_id - flash id (SPI_FLASH_ON_PACKAGE or SPI_FLASH_OFF_PACKAGE)
    \param address - address inside SFDP area to start read from
    \param data_buffer - readed bytes
    \param data_buffer_size - number of bytes to be read
    \return The function call status, pass/fail.
*/
int spi_flash_rdsfdp(SPI_FLASH_ID_t flash_id, uint32_t address, uint8_t *data_buffer,
                     uint32_t data_buffer_size);

/*! \fn int spi_flash_normal_read(SPI_FLASH_ID_t flash_id, uint32_t address, uint8_t *data_buffer, 
*                                 uint32_t data_buffer_size)
    \brief This function performs normal read from the memory.
    \param flash_id - flash id (SPI_FLASH_ON_PACKAGE or SPI_FLASH_OFF_PACKAGE)
    \param address - address to start read from
    \param data_buffer - readed bytes
    \param data_buffer_size - number of bytes to be read
    \return The function call status, pass/fail.
*/
int spi_flash_normal_read(SPI_FLASH_ID_t flash_id, uint32_t address, uint8_t *data_buffer,
                          uint32_t data_buffer_size);

/*! \fn int spi_flash_fast_read(SPI_FLASH_ID_t flash_id, uint32_t address, uint8_t *data_buffer, 
*                               uint32_t data_buffer_size)
    \brief This function performs fast read from the memory.
    \param flash_id - flash id (SPI_FLASH_ON_PACKAGE or SPI_FLASH_OFF_PACKAGE)
    \param address - address to start read from
    \param data_buffer - readed bytes
    \param data_buffer_size - number of bytes to be read
    \return The function call status, pass/fail.
*/
int spi_flash_fast_read(SPI_FLASH_ID_t flash_id, uint32_t address, uint8_t *data_buffer,
                        uint32_t data_buffer_size);

/*! \fn int spi_flash_page_program(SPI_FLASH_ID_t flash_id, uint32_t address, uint8_t *data_buffer, 
*                                  uint32_t data_buffer_size)
    \brief This function performs write of up to 256 bytes to the memory.
    \param flash_id - flash id (SPI_FLASH_ON_PACKAGE or SPI_FLASH_OFF_PACKAGE)
    \param address - address to start writing from
    \param data_buffer - bytes to be written
    \param data_buffer_size - number of bytes to be written (must be multiple of 4)
    \return The function call status, pass/fail.
*/
int spi_flash_page_program(SPI_FLASH_ID_t flash_id, uint32_t address, const uint8_t *data_buffer,
                           uint32_t data_buffer_size);

/*! \fn int spi_flash_block_erase(SPI_FLASH_ID_t flash_id, uint32_t address)
    \brief This function performs erase of one block (64KB) of the memory.
    \param flash_id - flash id (SPI_FLASH_ON_PACKAGE or SPI_FLASH_OFF_PACKAGE)
    \param address - address of the block to be erased
    \return The function call status, pass/fail.
*/
int spi_flash_block_erase(SPI_FLASH_ID_t flash_id, uint32_t address);

/*! \fn int spi_flaspi_flash_sector_erasesh_block_erase(SPI_FLASH_ID_t flash_id, uint32_t address)
    \brief This function performs erase of one sector (4KB) of the memory.
    \param flash_id - flash id (SPI_FLASH_ON_PACKAGE or SPI_FLASH_OFF_PACKAGE)
    \param address - address of the sector to be erased
    \return The function call status, pass/fail.
*/
int spi_flash_sector_erase(SPI_FLASH_ID_t flash_id, uint32_t address);

#endif
