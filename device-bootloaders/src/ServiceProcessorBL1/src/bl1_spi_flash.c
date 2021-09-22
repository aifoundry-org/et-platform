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

#include "etsoc/drivers/serial/serial.h"
#include "string.h"

#include "printx.h"
#include "bl1_spi_controller.h"
#include "bl1_spi_flash.h"

#include "bl1_main.h"

#pragma GCC push_options
#pragma GCC diagnostic ignored "-Wswitch-enum"

#define SPI_FLASH_CMD_RDSR         0x05
#define SPI_FLASH_CMD_WREN         0x06
#define SPI_FLASH_CMD_RDID         0x9F
#define SPI_FLASH_CMD_RDSFDP       0x5A
#define SPI_FLASH_CMD_NORMAL_READ  0x03
#define SPI_FLASH_CMD_FAST_READ    0x0B
#define SPI_FLASH_CMD_PAGE_PROGRAM 0x02

static int get_flash_controller_and_slave_ids(SPI_FLASH_ID_t id, SPI_CONTROLLER_ID_t *controller_id,
                                              uint8_t *slave_index)
{
    switch (id) {
    case SPI_FLASH_ON_PACKAGE:
        *controller_id = SPI_CONTROLLER_ID_SPI_0;
        *slave_index = 0;
        return 0;
    case SPI_FLASH_OFF_PACKAGE:
        *controller_id = SPI_CONTROLLER_ID_SPI_1;
        *slave_index = 0;
        return 0;
    default:
        return -1;
    }
}

int spi_flash_init(SPI_FLASH_ID_t flash_id)
{
    SPI_CONTROLLER_ID_t controller_id;
    uint8_t slave_index;

    if (0 != get_flash_controller_and_slave_ids(flash_id, &controller_id, &slave_index)) {
        MESSAGE_ERROR("get_flash_controller_and_slave_ids() failed!\n");
        return -1;
    }

    return spi_controller_init(controller_id);
}

int spi_flash_wren(SPI_FLASH_ID_t flash_id)
{
    SPI_COMMAND_t command;
    SPI_CONTROLLER_ID_t controller_id;
    uint8_t slave_index;

    if (0 != get_flash_controller_and_slave_ids(flash_id, &controller_id, &slave_index)) {
        return -1;
    }

    command.cmd = SPI_FLASH_CMD_WREN;
    command.include_address = false;
    command.dummy_bytes = 0;
    command.data_receive = false;
    command.address = 0;
    command.data_size = 0;
    command.data_buffer = NULL;

    if (0 != spi_controller_command(controller_id, slave_index, &command)) {
        MESSAGE_ERROR("spi_controller_command(RDSR) failed!\n");
        return -1;
    }

    return 0;
}

int spi_flash_rdsr(SPI_FLASH_ID_t flash_id, uint8_t *status)
{
    SPI_COMMAND_t command;
    SPI_CONTROLLER_ID_t controller_id;
    uint8_t slave_index;
    uint8_t data;

    if (0 != get_flash_controller_and_slave_ids(flash_id, &controller_id, &slave_index)) {
        return -1;
    }

    command.cmd = SPI_FLASH_CMD_RDSR;
    command.include_address = false;
    command.dummy_bytes = 0;
    command.data_receive = true;
    command.address = 0;
    command.data_size = sizeof(data);
    command.data_buffer = &data;

    if (0 != spi_controller_command(controller_id, slave_index, &command)) {
        MESSAGE_ERROR("spi_controller_command(RDSR) failed!\n");
        return -1;
    }

    *status = data;
    return 0;
}

int spi_flash_rdid(SPI_FLASH_ID_t flash_id, uint8_t *manufacturer_id, uint8_t device_id[2])
{
    SPI_COMMAND_t command;
    SPI_CONTROLLER_ID_t controller_id;
    uint8_t slave_index;
    uint8_t data[3];

    if (0 != get_flash_controller_and_slave_ids(flash_id, &controller_id, &slave_index)) {
        return -1;
    }

    command.cmd = SPI_FLASH_CMD_RDID;
    command.include_address = false;
    command.dummy_bytes = 0;
    command.data_receive = true;
    command.address = 0;
    command.data_size = sizeof(data);
    command.data_buffer = data;

    if (0 != spi_controller_command(controller_id, slave_index, &command)) {
        MESSAGE_ERROR("spi_controller_command(RDID) failed!\n");
        return -1;
    }

    *manufacturer_id = data[0];
    device_id[0] = data[1];
    device_id[1] = data[2];
    return 0;
}

int spi_flash_rdsfdp(SPI_FLASH_ID_t flash_id, uint32_t address, uint8_t *data_buffer,
                     uint32_t data_buffer_size)
{
    SPI_COMMAND_t command;
    SPI_CONTROLLER_ID_t controller_id;
    uint8_t slave_index;

    if (0 != get_flash_controller_and_slave_ids(flash_id, &controller_id, &slave_index)) {
        return -1;
    }

    command.cmd = SPI_FLASH_CMD_RDSFDP;
    command.include_address = true;
    command.dummy_bytes = 1;
    command.data_receive = true;
    command.address = address;
    command.data_size = data_buffer_size;
    command.data_buffer = data_buffer;

    if (0 != spi_controller_command(controller_id, slave_index, &command)) {
        MESSAGE_ERROR("spi_controller_command(RDSFDP) failed!\n");
        return -1;
    }

    return 0;
}

#define MAXIMUM_READ_SIZE 256
int spi_flash_normal_read(SPI_FLASH_ID_t flash_id, uint32_t address, uint8_t *data_buffer,
                          uint32_t data_buffer_size)
{
    SPI_COMMAND_t command;
    SPI_CONTROLLER_ID_t controller_id;
    uint8_t slave_index;
    uint32_t read_size;

    if (0 != get_flash_controller_and_slave_ids(flash_id, &controller_id, &slave_index)) {
        return -1;
    }

    while (data_buffer_size > 0) {
        read_size = data_buffer_size;
        if (read_size > MAXIMUM_READ_SIZE) {
            read_size = MAXIMUM_READ_SIZE;
        }

        command.cmd = SPI_FLASH_CMD_NORMAL_READ;
        command.include_address = true;
        command.dummy_bytes = 0;
        command.data_receive = true;
        command.address = address;
        command.data_size = read_size;
        command.data_buffer = data_buffer;

        if (0 != spi_controller_command(controller_id, slave_index, &command)) {
            MESSAGE_ERROR("spi_controller_command(RD) failed!\n");
            return -1;
        }

        address += read_size;
        data_buffer += read_size;
        data_buffer_size -= read_size;
    }

    return 0;
}

int spi_flash_fast_read(SPI_FLASH_ID_t flash_id, uint32_t address, uint8_t *data_buffer,
                        uint32_t data_buffer_size)
{
    SPI_COMMAND_t command;
    SPI_CONTROLLER_ID_t controller_id;
    uint8_t slave_index;
    uint32_t read_size;

    if (0 != get_flash_controller_and_slave_ids(flash_id, &controller_id, &slave_index)) {
        return -1;
    }

    while (data_buffer_size > 0) {
        read_size = data_buffer_size;
        if (read_size > MAXIMUM_READ_SIZE) {
            read_size = MAXIMUM_READ_SIZE;
        }

        command.cmd = SPI_FLASH_CMD_FAST_READ;
        command.include_address = true;
        command.dummy_bytes = 1;
        command.data_receive = true;
        command.address = address;
        command.data_size = data_buffer_size;
        command.data_buffer = data_buffer;

        if (0 != spi_controller_command(controller_id, slave_index, &command)) {
            MESSAGE_ERROR("spi_controller_command(RDHS) failed!\n");
            return -1;
        }

        address += read_size;
        data_buffer += read_size;
        data_buffer_size -= read_size;
    }

    return 0;
}

int spi_flash_program(SPI_FLASH_ID_t flash_id, uint32_t address, const uint8_t *data_buffer,
                      uint32_t data_buffer_size)
{
    SPI_COMMAND_t command;
    SPI_CONTROLLER_ID_t controller_id;
    uint8_t slave_index;

    if (0 != get_flash_controller_and_slave_ids(flash_id, &controller_id, &slave_index)) {
        return -1;
    }

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

    if (0 != spi_controller_command(controller_id, slave_index, &command)) {
        MESSAGE_ERROR("spi_controller_command(PP) failed!\n");
        return -1;
    }

    return 0;
}

#pragma GCC pop_options
