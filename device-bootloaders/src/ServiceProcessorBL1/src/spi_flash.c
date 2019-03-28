#include "et_cru.h"
#include "serial.h"

#include "printx.h"
#include "spi_controller.h"
#include "spi_flash.h"

#define SPI_FLASH_CMD_RDID      0x9F
#define SPI_FLASH_CMD_RDSR      0x05
#define SPI_FLASH_CMD_RDSFDP    0x5A

static int get_flash_controller_and_slave_ids(SPI_FLASH_ID_t id, SPI_CONTROLLER_ID_t * controller_id, uint8_t * slave_index) {
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

int spi_flash_init(SPI_FLASH_ID_t flash_id) {
    SPI_CONTROLLER_ID_t controller_id;
    uint8_t slave_index;

    if (0 != get_flash_controller_and_slave_ids(flash_id, &controller_id, &slave_index)) {
        return -1;
    }

    return spi_controller_init(controller_id);
}

int spi_flash_rdsr(SPI_FLASH_ID_t flash_id, uint8_t * status) {
    SPI_COMMAND_t command;
    SPI_CONTROLLER_ID_t controller_id;
    uint8_t slave_index;
    uint8_t data;

    if (0 != get_flash_controller_and_slave_ids(flash_id, &controller_id, &slave_index)) {
        return -1;
    }

    command.cmd = SPI_FLASH_CMD_RDSR;
    command.include_address = false;
    command.include_dummy = false;
    command.data_receive = true;
    command.address = 0;
    command.data_size = sizeof(data);
    command.data_buffer = &data;

    if (0 != spi_controller_command(controller_id, slave_index, &command)) {
        printx("spi_controller_command(RDSR) failed!\n");
        return -1;
    }

    *status = data;
    return 0;
}

int spi_flash_rdid(SPI_FLASH_ID_t flash_id, uint8_t * manufacturer_id, uint16_t * device_id) {
    SPI_COMMAND_t command;
    SPI_CONTROLLER_ID_t controller_id;
    uint8_t slave_index;
    uint8_t data[3];

    if (0 != get_flash_controller_and_slave_ids(flash_id, &controller_id, &slave_index)) {
        return -1;
    }

    command.cmd = SPI_FLASH_CMD_RDID;
    command.include_address = false;
    command.include_dummy = false;
    command.data_receive = true;
    command.address = 0;
    command.data_size = sizeof(data);
    command.data_buffer = data;

    if (0 != spi_controller_command(controller_id, slave_index, &command)) {
        printx("spi_controller_command(RDID) failed!\n");
        return -1;
    }

    *manufacturer_id = data[0];
    *device_id = (uint16_t)((data[1] << 8) | (data[2]));
    return 0;
}

int spi_flash_rdsfdp(SPI_FLASH_ID_t flash_id, uint32_t address, uint8_t * data_buffer, uint32_t data_buffer_size) {
    SPI_COMMAND_t command;
    SPI_CONTROLLER_ID_t controller_id;
    uint8_t slave_index;

    if (0 != get_flash_controller_and_slave_ids(flash_id, &controller_id, &slave_index)) {
        return -1;
    }

    command.cmd = SPI_FLASH_CMD_RDID;
    command.include_address = true;
    command.include_dummy = false;
    command.data_receive = true;
    command.address = address;
    command.data_size = data_buffer_size;
    command.data_buffer = data_buffer;

    if (0 != spi_controller_command(controller_id, slave_index, &command)) {
        printx("spi_controller_command(RDSFDP) failed!\n");
        return -1;
    }

    return 0;
}
