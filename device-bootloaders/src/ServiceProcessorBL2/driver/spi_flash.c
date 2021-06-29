/***********************************************************************
*
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
#include <stdio.h>
#include "serial.h"
#include "string.h"
#include "bl2_spi_controller.h"
#include "bl2_spi_flash.h"
#include "bl2_main.h"
#include "bl_error_code.h"

#pragma GCC push_options
#pragma GCC diagnostic ignored "-Wswitch-enum"

#define SPI_FLASH_CMD_RDSR         0x05
#define SPI_FLASH_CMD_WREN         0x06
#define SPI_FLASH_CMD_RDID         0x9F
#define SPI_FLASH_CMD_RDSFDP       0x5A
#define SPI_FLASH_CMD_NORMAL_READ  0x03
#define SPI_FLASH_CMD_FAST_READ    0x0B
#define SPI_FLASH_CMD_PAGE_PROGRAM 0x02
#define SPI_FLASH_CMD_BLOCK_ERASE  0xD8 /* 16KB block erase */
#define SPI_FLASH_CMD_SECTOR_ERASE 0x20 /* 4KB sector erase */

/* SPI TX FIFO Depth: 256 entries x 32 bits each */
#define MAX_SPI_TX_FIFO_DEPTH 256
#define MAX_SPI_TX_FIFO_SIZE  (MAX_SPI_TX_FIFO_DEPTH * 4)

/* Polling for mem busy loop, num_reads - number of status reads before timeout error */
#define SPI_MEM_BUSY_WAIT_LOOP(flash_id, spi_status, num_reads)            \
    int k = 0;                                                             \
    if (0 != spi_flash_rdsr(flash_id, &spi_status))                        \
    {                                                                      \
        MESSAGE_ERROR("spi_flash_rdsr() failed!\n");                       \
        return ERROR_SPI_FLASH_RDSR_FAILED;                                \
    }                                                                      \
    while (1 & spi_status)                                                 \
    {                                                                      \
        k++;                                                               \
        if (k > num_reads)                                                 \
        {                                                                  \
            MESSAGE_ERROR("timeout waiting for write/erase to finish!\n"); \
            return ERROR_SPI_FLASH_TIMEOUT_WAITING_MEM_READY;              \
        }                                                                  \
        if (0 != spi_flash_rdsr(flash_id, &spi_status))                    \
        {                                                                  \
            MESSAGE_ERROR("spi_flash_rdsr() failed!\n");                   \
            return ERROR_SPI_FLASH_RDSR_FAILED;                            \
        }                                                                  \
    }

/* Check memory readiness for erase/write command */
#define SPI_MEM_PREPARE_FOR_ERASE_WRITE(flash_id, spi_status) \
    if (0 != spi_flash_rdsr(flash_id, &spi_status))           \
    {                                                         \
        MESSAGE_ERROR("spi_flash_rdsr() failed!\n");          \
        return ERROR_SPI_FLASH_RDSR_FAILED;                   \
    }                                                         \
    if (0 != spi_status)                                      \
    {                                                         \
        MESSAGE_ERROR("flash device not ready!\n");           \
        return ERROR_SPI_FLASH_MEM_NOT_READY;                 \
    }                                                         \
    if (0 != spi_flash_wren(flash_id))                        \
    {                                                         \
        MESSAGE_ERROR("spi_flash_wren() failed!\n");          \
        return ERROR_SPI_FLASH_WREN_FAILED;                   \
    }

/************************************************************************
*
*   FUNCTION
*
*       get_flash_controller_and_slave_ids
*
*   DESCRIPTION
*
*       This function returns spi controller and slave id.
*
*   INPUTS
*
*       id                flash id (SPI_FLASH_ON_PACKAGE or SPI_FLASH_OFF_PACKAGE)
*
*   OUTPUTS
*
*       controller_id     spi controller id
*       slave_index       spi slave id
*
***********************************************************************/

static int get_flash_controller_and_slave_ids(SPI_FLASH_ID_t id, 
                                               SPI_CONTROLLER_ID_t *controller_id,
                                               uint8_t *slave_index)
{
    switch (id) 
    {
        case SPI_FLASH_ON_PACKAGE:
            *controller_id = SPI_CONTROLLER_ID_SPI_0;
            *slave_index = 0;
            return 0;
        case SPI_FLASH_OFF_PACKAGE:
            *controller_id = SPI_CONTROLLER_ID_SPI_1;
            *slave_index = 0;
            return 0;
        default:
            return ERROR_SPI_FLASH_INVALID_ARGUMENTS;
    }
}

/************************************************************************
*
*   FUNCTION
*
*       SPI_Flash_Initialize
*
*   DESCRIPTION
*
*       This function initialize spi controller.
*
*   INPUTS
*
*       flash_id           flash id (SPI_FLASH_ON_PACKAGE or SPI_FLASH_OFF_PACKAGE)
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

int SPI_Flash_Initialize(SPI_FLASH_ID_t flash_id)
{
    SPI_CONTROLLER_ID_t controller_id;
    uint8_t slave_index;

    if (0 != get_flash_controller_and_slave_ids(flash_id, &controller_id, &slave_index))
    {
        MESSAGE_ERROR("get_flash_controller_and_slave_ids() failed!\n");
        return ERROR_SPI_FLASH_INVALID_ARGUMENTS;
    }

    return spi_controller_init(controller_id);
}

/************************************************************************
*
*   FUNCTION
*
*       spi_flash_wren
*
*   DESCRIPTION
*
*       This function performs write enable of flash memory.
*
*   INPUTS
*
*       flash_id           flash id (SPI_FLASH_ON_PACKAGE or SPI_FLASH_OFF_PACKAGE)
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

int spi_flash_wren(SPI_FLASH_ID_t flash_id)
{
    SPI_COMMAND_t command;
    SPI_CONTROLLER_ID_t controller_id;
    uint8_t slave_index;

    if (0 != get_flash_controller_and_slave_ids(flash_id, &controller_id, &slave_index)) 
    {
        return ERROR_SPI_FLASH_INVALID_ARGUMENTS;
    }

    command.cmd = SPI_FLASH_CMD_WREN;
    command.include_address = false;
    command.dummy_bytes = 0;
    command.data_receive = false;
    command.address = 0;
    command.data_size = 0;
    command.data_buffer = NULL;

    if (0 != spi_controller_command(controller_id, slave_index, &command)) 
    {
        MESSAGE_ERROR("spi_controller_command(WREN) failed!\n");
        return ERROR_SPI_FLASH_WREN_FAILED;
    }

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       spi_flash_rdsr
*
*   DESCRIPTION
*
*       This function performs read of status register of flash memory.
*
*   INPUTS
*
*       flash_id           flash id (SPI_FLASH_ON_PACKAGE or SPI_FLASH_OFF_PACKAGE)
*
*   OUTPUTS
*
*       status             value of the status register of flash memory
*
***********************************************************************/

int spi_flash_rdsr(SPI_FLASH_ID_t flash_id, uint8_t *status)
{
    SPI_COMMAND_t command;
    SPI_CONTROLLER_ID_t controller_id;
    uint8_t slave_index;
    uint8_t data;

    if (0 != get_flash_controller_and_slave_ids(flash_id, &controller_id, &slave_index)) 
    {
        return ERROR_SPI_FLASH_INVALID_ARGUMENTS;
    }

    command.cmd = SPI_FLASH_CMD_RDSR;
    command.include_address = false;
    command.dummy_bytes = 0;
    command.data_receive = true;
    command.address = 0;
    command.data_size = sizeof(data);
    command.data_buffer = &data;

    if (0 != spi_controller_command(controller_id, slave_index, &command)) 
    {
        MESSAGE_ERROR("spi_controller_command(RDSR) failed!\n");
        return ERROR_SPI_FLASH_RDSR_FAILED;
    }

    *status = data;
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       spi_flash_rdid
*
*   DESCRIPTION
*
*       This function performs read of manufacturer and device id of flash memory.
*
*   INPUTS
*
*       flash_id           flash id (SPI_FLASH_ON_PACKAGE or SPI_FLASH_OFF_PACKAGE)
*
*   OUTPUTS
*
*       manufacturer_id    manufacturer id of flash memory
*       device_id          device id of flash memory
*
***********************************************************************/

int spi_flash_rdid(SPI_FLASH_ID_t flash_id, uint8_t *manufacturer_id, uint8_t device_id[2])
{
    SPI_COMMAND_t command;
    SPI_CONTROLLER_ID_t controller_id;
    uint8_t slave_index;
    uint8_t data[3];

    if (0 != get_flash_controller_and_slave_ids(flash_id, &controller_id, &slave_index)) 
    {
        return ERROR_SPI_FLASH_INVALID_ARGUMENTS;
    }

    command.cmd = SPI_FLASH_CMD_RDID;
    command.include_address = false;
    command.dummy_bytes = 0;
    command.data_receive = true;
    command.address = 0;
    command.data_size = sizeof(data);
    command.data_buffer = data;

    if (0 != spi_controller_command(controller_id, slave_index, &command)) 
    {
        MESSAGE_ERROR("spi_controller_command(RDID) failed!\n");
        return ERROR_SPI_FLASH_RDID_FAILED;
    }

    *manufacturer_id = data[0];
    device_id[0] = data[1];
    device_id[1] = data[2];
    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       spi_flash_rdsfdp
*
*   DESCRIPTION
*
*       This function performs read of SFDP (Serial Flash Discoverable Parameter) area.
*
*   INPUTS
*
*       flash_id           flash id (SPI_FLASH_ON_PACKAGE or SPI_FLASH_OFF_PACKAGE)
*       address            address inside SFDP area to start read from
*       data_buffer_size   number of bytes to be read
*
*   OUTPUTS
*
*       data_buffer        readed bytes
*
***********************************************************************/

int spi_flash_rdsfdp(SPI_FLASH_ID_t flash_id, uint32_t address, uint8_t *data_buffer,
                     uint32_t data_buffer_size)
{
    SPI_COMMAND_t command;
    SPI_CONTROLLER_ID_t controller_id;
    uint8_t slave_index;

    if (0 != get_flash_controller_and_slave_ids(flash_id, &controller_id, &slave_index)) 
    {
        return ERROR_SPI_FLASH_INVALID_ARGUMENTS;
    }

    command.cmd = SPI_FLASH_CMD_RDSFDP;
    command.include_address = true;
    command.dummy_bytes = 1;
    command.data_receive = true;
    command.address = address;
    command.data_size = data_buffer_size;
    command.data_buffer = data_buffer;

    if (0 != spi_controller_command(controller_id, slave_index, &command)) 
    {
        MESSAGE_ERROR("spi_controller_command(RDSFDP) failed!\n");
        return ERROR_SPI_FLASH_RDSFDP_FAILED;
    }

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       spi_flash_normal_read
*
*   DESCRIPTION
*
*       This function performs normal read from the memory.
*
*   INPUTS
*
*       flash_id           flash id (SPI_FLASH_ON_PACKAGE or SPI_FLASH_OFF_PACKAGE)
*       address            address to start read from
*       data_buffer_size   number of bytes to be read
*
*   OUTPUTS
*
*       data_buffer        readed bytes
*
***********************************************************************/

#define MAXIMUM_READ_SIZE 256
int spi_flash_normal_read(SPI_FLASH_ID_t flash_id, uint32_t address, uint8_t *data_buffer,
                          uint32_t data_buffer_size)
{
    SPI_COMMAND_t command;
    SPI_CONTROLLER_ID_t controller_id;
    uint8_t slave_index;
    uint32_t read_size;

    if (0 != get_flash_controller_and_slave_ids(flash_id, &controller_id, &slave_index)) 
    {
        return ERROR_SPI_FLASH_INVALID_ARGUMENTS;
    }

    while (data_buffer_size > 0) 
    {
        read_size = data_buffer_size;
        if (read_size > MAXIMUM_READ_SIZE) 
        {
            read_size = MAXIMUM_READ_SIZE;
        }

        command.cmd = SPI_FLASH_CMD_NORMAL_READ;
        command.include_address = true;
        command.dummy_bytes = 0;
        command.data_receive = true;
        command.address = address;
        command.data_size = read_size;
        command.data_buffer = data_buffer;

        if (0 != spi_controller_command(controller_id, slave_index, &command)) 
        {
            MESSAGE_ERROR("spi_controller_command(RD) failed!\n");
            return ERROR_SPI_FLASH_NORMAL_RD_FAILED;
        }

        address += read_size;
        data_buffer += read_size;
        data_buffer_size -= read_size;
    }

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       spi_flash_fast_read
*
*   DESCRIPTION
*
*       This function performs fast read from the memory.
*
*   INPUTS
*
*       flash_id           flash id (SPI_FLASH_ON_PACKAGE or SPI_FLASH_OFF_PACKAGE)
*       address            address to start read from
*       data_buffer_size   number of bytes to be read
*
*   OUTPUTS
*
*       data_buffer        readed bytes
*
***********************************************************************/

int spi_flash_fast_read(SPI_FLASH_ID_t flash_id, uint32_t address, uint8_t *data_buffer,
                        uint32_t data_buffer_size)
{
    SPI_COMMAND_t command;
    SPI_CONTROLLER_ID_t controller_id;
    uint8_t slave_index;
    uint32_t read_size;

    if (0 != get_flash_controller_and_slave_ids(flash_id, &controller_id, &slave_index)) 
    {
        return ERROR_SPI_FLASH_INVALID_ARGUMENTS;
    }

    while (data_buffer_size > 0) 
    {
        read_size = data_buffer_size;
        if (read_size > MAXIMUM_READ_SIZE) 
        {
            read_size = MAXIMUM_READ_SIZE;
        }

        command.cmd = SPI_FLASH_CMD_FAST_READ;
        command.include_address = true;
        command.dummy_bytes = 1;
        command.data_receive = true;
        command.address = address;
        command.data_size = data_buffer_size;
        command.data_buffer = data_buffer;

        if (0 != spi_controller_command(controller_id, slave_index, &command)) 
        {
            MESSAGE_ERROR("spi_controller_command(RDHS) failed!\n");
            return ERROR_SPI_FLASH_FAST_RD_FAILED;
        }

        address += read_size;
        data_buffer += read_size;
        data_buffer_size -= read_size;
    }

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       spi_flash_page_program
*
*   DESCRIPTION
*
*       This function performs write of up to 256 bytes to the memory.
*
*   INPUTS
*
*       flash_id           flash id (SPI_FLASH_ON_PACKAGE or SPI_FLASH_OFF_PACKAGE)
*       address            address to start writing from
*       data_buffer_size   number of bytes to be written (must be multiple of 4)
*       data_buffer        bytes to be written
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

int spi_flash_page_program(SPI_FLASH_ID_t flash_id, uint32_t address, const uint8_t *data_buffer,
                           uint32_t data_buffer_size)
{
    SPI_COMMAND_t command;
    SPI_CONTROLLER_ID_t controller_id;
    uint8_t slave_index;
    uint8_t spi_status;

    if (0 != get_flash_controller_and_slave_ids(flash_id, &controller_id, &slave_index)) 
    {
        MESSAGE_ERROR("spi_flash_page_program: get_flash_controller_and_slave_ids() failed!\n");
        return ERROR_SPI_FLASH_INVALID_ARGUMENTS;
    }

    /* Max supported size will include 4 bytes of command for 8bits access */
    if (0 == (data_buffer_size & 0x3)) 
    {
        if (data_buffer_size > SPI_FLASH_PAGE_SIZE) 
        {
            MESSAGE_ERROR("spi_flash_page_program failed! Max size = %d\n", SPI_FLASH_PAGE_SIZE);
            return ERROR_SPI_FLASH_INVALID_ARGUMENTS;
        }
    } 
    else 
    {
        if (data_buffer_size > (MAX_SPI_TX_FIFO_DEPTH - 4)) 
        {
            MESSAGE_ERROR("spi_flash_page_program failed! Max size = %d\n",
                          (MAX_SPI_TX_FIFO_DEPTH - 4));
            return ERROR_SPI_FLASH_INVALID_ARGUMENTS;
        }
    }

    SPI_MEM_PREPARE_FOR_ERASE_WRITE(flash_id, spi_status)

    command.cmd = SPI_FLASH_CMD_PAGE_PROGRAM;
    command.include_address = true;
    command.dummy_bytes = 0;
    command.data_receive = false;
    command.address = address;
    command.data_size = data_buffer_size;

#pragma GCC push_options
#pragma GCC diagnostic ignored "-Wcast-qual"
    command.data_buffer = (uint8_t *)data_buffer;
#pragma GCC pop_options

    if (0 != spi_controller_command(controller_id, slave_index, &command)) 
    {
        MESSAGE_ERROR("spi_flash_page_program failed!\n");
        return ERROR_SPI_FLASH_PP_FAILED;
    }

    SPI_MEM_BUSY_WAIT_LOOP(flash_id, spi_status, 40000)

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       spi_flash_block_erase
*
*   DESCRIPTION
*
*       This function performs erase of one block (64KB) of the memory.
*
*   INPUTS
*
*       flash_id           flash id (SPI_FLASH_ON_PACKAGE or SPI_FLASH_OFF_PACKAGE)
*       address            address of the block to be erased
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

int spi_flash_block_erase(SPI_FLASH_ID_t flash_id, uint32_t address)
{
    SPI_COMMAND_t command;
    SPI_CONTROLLER_ID_t controller_id;
    uint8_t slave_index;
    uint8_t spi_status;

    if (0 != get_flash_controller_and_slave_ids(flash_id, &controller_id, &slave_index)) 
    {
        MESSAGE_ERROR("spi_flash_block_erase: get_flash_controller_and_slave_ids() failed!\n");
        return ERROR_SPI_FLASH_INVALID_ARGUMENTS;
    }

    if (0 != (address & SPI_FLASH_BLOCK_MASK)) 
    {
        MESSAGE_ERROR("spi_flash_sector_erase: address must be 64kB alligned!\n");
        return ERROR_SPI_FLASH_INVALID_ARGUMENTS;
    }

    SPI_MEM_PREPARE_FOR_ERASE_WRITE(flash_id, spi_status)

    command.cmd = SPI_FLASH_CMD_BLOCK_ERASE;
    command.include_address = true;
    command.dummy_bytes = 0;
    command.data_receive = false;
    command.address = address;
    command.data_size = 0;
    command.data_buffer = NULL;

    if (0 != spi_controller_command(controller_id, slave_index, &command)) 
    {
        MESSAGE_ERROR("spi_flash_block_erase: spi_controller_command(BE) failed!\n");
        return ERROR_SPI_FLASH_BE_FAILED;
    }

    SPI_MEM_BUSY_WAIT_LOOP(flash_id, spi_status, 40000)

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       spi_flash_sector_erase
*
*   DESCRIPTION
*
*       This function performs erase of one sector (4KB) of the memory.
*
*   INPUTS
*
*       flash_id           flash id (SPI_FLASH_ON_PACKAGE or SPI_FLASH_OFF_PACKAGE)
*       address            address of the sector to be erased
*
*   OUTPUTS
*
*       none
*
***********************************************************************/

int spi_flash_sector_erase(SPI_FLASH_ID_t flash_id, uint32_t address)
{
    SPI_COMMAND_t command;
    SPI_CONTROLLER_ID_t controller_id;
    uint8_t slave_index;
    uint8_t spi_status;

    if (0 != get_flash_controller_and_slave_ids(flash_id, &controller_id, &slave_index)) 
    {
        MESSAGE_ERROR("spi_flash_sector_erase: get_flash_controller_and_slave_ids() failed!\n");
        return ERROR_SPI_FLASH_INVALID_ARGUMENTS;
    }

    if (0 != (address & SPI_FLASH_SECTOR_MASK)) 
    {
        MESSAGE_ERROR("spi_flash_sector_erase: address must be 4kB alligned!\n");
        return ERROR_SPI_FLASH_INVALID_ARGUMENTS;
    }

    SPI_MEM_PREPARE_FOR_ERASE_WRITE(flash_id, spi_status)

    command.cmd = SPI_FLASH_CMD_SECTOR_ERASE;
    command.include_address = true;
    command.dummy_bytes = 0;
    command.data_receive = false;
    command.address = address;
    command.data_size = 0;
    command.data_buffer = NULL;

    if (0 != spi_controller_command(controller_id, slave_index, &command)) 
    {
        MESSAGE_ERROR("spi_flash_sector_erase: spi_controller_command(BE) failed!\n");
        return ERROR_SPI_FLASH_SE_FAILED;
    }

    SPI_MEM_BUSY_WAIT_LOOP(flash_id, spi_status, 40000)

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       SPI_Flash_Read_Word
*
*   DESCRIPTION
*
*       This function performs read operation on SPI flash memory.
*
*   INPUTS
*
*       flash_id           flash id (SPI_FLASH_ON_PACKAGE or SPI_FLASH_OFF_PACKAGE)
*       address            memory address to read from
*       size               size of data to read
*
*   OUTPUTS
*
*       data_buffer        data read from the address
*
***********************************************************************/
int SPI_Flash_Read_Page(SPI_FLASH_ID_t flash_id, uint32_t address, uint32_t* data_buffer, uint32_t size)
{
    if (0 != spi_flash_normal_read(flash_id, address, (uint8_t *)data_buffer, size)) 
    {
        return ERROR_SPI_FLASH_NORMAL_RD_FAILED;
    }

    return (0);
}

/************************************************************************
*
*   FUNCTION
*
*       SPI_Flash_Read_Word
*
*   DESCRIPTION
*
*       This function performs read operation on SPI flash memory.
*
*   INPUTS
*
*       flash_id           flash id (SPI_FLASH_ON_PACKAGE or SPI_FLASH_OFF_PACKAGE)
*       address            memory address to read from
*       data_buffer        data to wrote onto memorty location
*       size               size of data to be written
*
*   OUTPUTS
*
*       none
*
***********************************************************************/
int SPI_Flash_Write_Page(SPI_FLASH_ID_t flash_id, uint32_t address, uint32_t *data, uint32_t size) 
{
    if (0 != spi_flash_page_program(flash_id, address, (uint8_t *)data, size)) 
    {
        return ERROR_SPI_FLASH_PP_FAILED;
    }

    return 0;
}
#pragma GCC pop_options
