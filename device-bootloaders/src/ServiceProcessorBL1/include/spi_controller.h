#ifndef __SPI_CONTROLLER_H__
#define __SPI_CONTROLLER_H__

#include <stdint.h>
#include <stdbool.h>

typedef enum SPI_CONTROLLER_ID {
    SPI_CONTROLLER_ID_INVALID = 0,
    SPI_CONTROLLER_ID_SPI_0,
    SPI_CONTROLLER_ID_SPI_1
} SPI_CONTROLLER_ID_t;

typedef struct SPI_COMMAND {
    uint8_t cmd;
    bool include_address;
    bool include_dummy;
    bool data_receive;
    uint32_t address;
    uint32_t data_size;
    uint8_t *data_buffer;
} SPI_COMMAND_t;

int spi_controller_init(SPI_CONTROLLER_ID_t id);
int spi_controller_command(SPI_CONTROLLER_ID_t id, uint8_t slave_index, SPI_COMMAND_t *command);

#endif
