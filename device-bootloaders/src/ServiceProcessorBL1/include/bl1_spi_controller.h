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
