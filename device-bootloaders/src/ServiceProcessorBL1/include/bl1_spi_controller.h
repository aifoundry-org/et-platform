/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------
*/

#ifndef __BL1_SPI_CONTROLLER_H__
#define __BL1_SPI_CONTROLLER_H__

#include <stdint.h>
#include <stdbool.h>

/*! \def SPI_MAX_TX_FREQUENCY
    \brief Max SPI tx frequency in MHz, from silicon testing
*/
#define SPI_MAX_TX_FREQUENCY 36

/*! \def SPI_MAX_RX_FREQUENCY
    \brief Max SPI rx frequency in MHz, from silicon testing
*/
#define SPI_MAX_RX_FREQUENCY 36

/*! \def SPI_USE_DEFAULT_FREQUENCY
    \brief Special value that tells driver not to change divider.
*/
#define SPI_USE_DEFAULT_FREQUENCY 0

typedef enum SPI_CONTROLLER_ID
{
    SPI_CONTROLLER_ID_INVALID = 0,
    SPI_CONTROLLER_ID_SPI_0,
    SPI_CONTROLLER_ID_SPI_1
} SPI_CONTROLLER_ID_t;

typedef struct SPI_COMMAND
{
    uint8_t cmd;
    bool include_address;
    uint32_t dummy_bytes;
    bool data_receive;
    uint32_t address;
    uint32_t data_size;
    uint8_t *data_buffer;
} SPI_COMMAND_t;

int spi_controller_init(SPI_CONTROLLER_ID_t id, uint32_t rx_frequency, uint32_t tx_frequency);
int spi_controller_command(SPI_CONTROLLER_ID_t id, uint8_t slave_index, SPI_COMMAND_t *command);
void check_spi_dividers_otp_override_values(void);

#endif
