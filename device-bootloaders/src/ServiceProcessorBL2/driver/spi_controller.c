/***********************************************************************
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
************************************************************************/
/*! \file spi_controller.c
    \brief A C module that implements SPI controller's functionality.
    It provides functions to read/write data to SPI controller

    Public interfaces:
        spi_controller_init
        spi_controller_command
*/
/***********************************************************************/

#include "etsoc/drivers/serial/serial.h"
#include <stdio.h>
#include "log.h"
#include "etsoc/isa/mem-access/io.h"
#include "bl2_spi_controller.h"
#include "spio_DW_apb_ssi_config.h"
#include "bl2_main.h"

#include "hwinc/sp_spi0.h"
#include "hwinc/hal_device.h"

#pragma GCC push_options

/* #pragma GCC optimize ("O3") */
#pragma GCC diagnostic ignored "-Wswitch-enum"

/*! \def SPI_RX_VERBOSITY
    \brief Verbose level of SPI Rx operation
*/
#define SPI_RX_VERBOSITY 0

/*! \def SPI_TX_VERBOSITY
    \brief Verbose level of SPI Tx operation
*/
#define SPI_TX_VERBOSITY 0

/*! \def DUMMY_TX_BYTE_VALUE
    \brief Dummy byte for Tx
*/
#define DUMMY_TX_BYTE_VALUE 0xFF

/*! \def MAX_RX_TX_FIFO_SIZE
    \brief FIFO sizes of Rx and Tx
*/
#define MAX_RX_TX_FIFO_SIZE 256

/*! \def RX_BAUD_RATE_DIVIDER_100_MHZ_VALUE
    \brief used when the PLL_1 is turned off, will result in SCLK_OUT frequency of 50 MHz
*/
#define RX_BAUD_RATE_DIVIDER_100_MHZ_VALUE     2

/*! \def RX_BAUD_RATE_DIVIDER_250_MHZ_VALUE
    \brief used when the PLL_1 is set to 1000 MHz, will result in SCLK_OUT frequency of 41.7 MHz
*/
#define RX_BAUD_RATE_DIVIDER_250_MHZ_VALUE     6

/*! \def RX_BAUD_RATE_DIVIDER_375_MHZ_VALUE
    \brief used when the PLL_1 is set to 1500 MHz, will result in SCLK_OUT frequency of 46.9 MHz
*/
#define RX_BAUD_RATE_DIVIDER_375_MHZ_VALUE     8

/*! \def RX_BAUD_RATE_DIVIDER_500_MHZ_VALUE
    \brief used when the PLL_1 is set to 2000 MHz, will result in SCLK_OUT frequency of 50.0 MHz
*/
#define RX_BAUD_RATE_DIVIDER_500_MHZ_VALUE     10

/*! \def TX_BAUD_RATE_DIVIDER_100_MHZ_VALUE
    \brief used when the PLL_1 is turned off, will result in SCLK_OUT frequency of 50 MHz
*/
#define TX_BAUD_RATE_DIVIDER_100_MHZ_VALUE     2

/*! \def TX_BAUD_RATE_DIVIDER_250_MHZ_VALUE
    \brief used when the PLL_1 is set to 1000 MHz, will result in SCLK_OUT frequency of 41.7 MHz
*/
#define TX_BAUD_RATE_DIVIDER_250_MHZ_VALUE     6

/*! \def TX_BAUD_RATE_DIVIDER_375_MHZ_VALUE
    \brief used when the PLL_1 is set to 1500 MHz, will result in SCLK_OUT frequency of 46.9 MHz
*/
#define TX_BAUD_RATE_DIVIDER_375_MHZ_VALUE     8

/*! \def TX_BAUD_RATE_DIVIDER_500_MHZ_VALUE
    \brief used when the PLL_1 is set to 2000 MHz, will result in SCLK_OUT frequency of 50.0 MHz
*/
#define TX_BAUD_RATE_DIVIDER_500_MHZ_VALUE     10

/*! \def RX_BAUD_RATE_DIVIDER_VALUE
    \brief Rx baud divider values
*/
#define RX_BAUD_RATE_DIVIDER_VALUE RX_BAUD_RATE_DIVIDER_500_MHZ_VALUE

/*! \def TX_BAUD_RATE_DIVIDER_VALUE
    \brief Tx baud divider values
*/
#define TX_BAUD_RATE_DIVIDER_VALUE TX_BAUD_RATE_DIVIDER_500_MHZ_VALUE

#if 1
/*! \def SCPOL_VALUE
    \brief
*/
#define SCPOL_VALUE SSI_CTRLR0_SCPOL_SCPOL_SCLK_LOW
#else
#define SCPOL_VALUE SSI_CTRLR0_SCPOL_SCPOL_SCLK_HIGH
#endif

#if 1
/*! \def SCPH_VALUE
    \brief
*/
#define SCPH_VALUE SSI_CTRLR0_SCPH_SCPH_SCPH_MIDDLE
#else
#define SCPH_VALUE SSI_CTRLR0_SCPH_SCPH_SCPH_START
#endif

/*! \def SLAVE_MASK
    \brief slave device bit mask
*/
#define SLAVE_MASK ((1u << SPI_SSI_NUM_SLAVES) - 1)

/*! \def TX_TIMEOUT
    \brief Tx timeout value
*/
#define TX_TIMEOUT 0x1000

/*! \def RX_TIMEOUT
    \brief Rx timeout value
*/
#define RX_TIMEOUT 0x1000

static uintptr_t get_spi_registers(SPI_CONTROLLER_ID_t id)
{
    switch (id)
    {
        case SPI_CONTROLLER_ID_SPI_0:
            return R_SP_SPI0_BASEADDR;
        case SPI_CONTROLLER_ID_SPI_1:
            return R_SP_SPI1_BASEADDR;
        default:
            return 0;
    }
}

int spi_controller_init(SPI_CONTROLLER_ID_t id)
{
    uintptr_t spi_regs = get_spi_registers(id);
    if (0 == spi_regs)
    {
        return -1;
    }

    iowrite32(spi_regs + SSI_SSIENR_ADDRESS, SSI_SSIENR_SSI_EN_SET(0));

    iowrite32(spi_regs + SSI_CTRLR0_ADDRESS,
              SSI_CTRLR0_DFS_SET(SSI_CTRLR0_DFS_DFS_FRAME_08BITS) |
                  SSI_CTRLR0_FRF_SET(SSI_CTRLR0_FRF_FRF_MOTOROLA_SPI) |
                  SSI_CTRLR0_SCPH_SET(SCPH_VALUE) | SSI_CTRLR0_SCPOL_SET(SCPOL_VALUE) |
                  SSI_CTRLR0_TMOD_SET(SSI_CTRLR0_TMOD_TMOD_TX_AND_RX) |
                  SSI_CTRLR0_SRL_SET(SSI_CTRLR0_SRL_SRL_NORMAL_MODE) |
                  SSI_CTRLR0_DFS_32_SET(SSI_CTRLR0_DFS_32_DFS_32_FRAME_08BITS) |
                  SSI_CTRLR0_SPI_FRF_SET(SSI_CTRLR0_SPI_FRF_SPI_FRF_STD_SPI_FRF) |
                  SSI_CTRLR0_SSTE_SET(0));

    iowrite32(spi_regs + SSI_CTRLR1_ADDRESS, SSI_CTRLR1_NDF_SET(0));

    iowrite32(spi_regs + SSI_TXFTLR_ADDRESS, SSI_TXFTLR_TFT_SET(SPI_TX_FIFO_MAX_DEPTH));
    iowrite32(spi_regs + SSI_RXFTLR_ADDRESS, SSI_RXFTLR_RFT_SET(SPI_RX_FIFO_MAX_DEPTH));

    iowrite32(spi_regs + SSI_IMR_ADDRESS, SSI_IMR_TXEIM_SET(0) | SSI_IMR_TXOIM_SET(0) |
                                              SSI_IMR_RXUIM_SET(0) | SSI_IMR_RXOIM_SET(0) |
                                              SSI_IMR_RXFIM_SET(0) | SSI_IMR_MSTIM_SET(0));

    iowrite32(spi_regs + SSI_SSIENR_ADDRESS, SSI_SSIENR_SSI_EN_SET(1));

    return 0;
}

static inline uint32_t reverse_endian(const uint32_t v)
{
    return (v << 24) | (0x00FF0000 & (v << 8)) | (0x0000FF00 & (v >> 8)) | (0xFF & (v >> 24));
}

static int spi_controller_tx32_data(uintptr_t spi_regs, const uint8_t *spi_command,
                                    uint32_t spi_command_length, uint8_t *tx_data,
                                    uint32_t tx_data_size, uint32_t slave_en_mask)
{
    int rv;
    uint32_t timeout;
    uint32_t rx32_count;
    const uint32_t *tx_data_32;
    const uint32_t *tx_data_32_end;
    const uint32_t *spi_command_32;
    uint32_t empty_flag;

#if SPI_TX_VERBOSITY >= 2
    uint32_t n;
    MESSAGE_INFO_DEBUG("SPI_TX32: cmd=%02x, len=%u, tx_data=0x%x, tx_size=%u\n", spi_command[0],
                       spi_command_length, tx_data, tx_data_size);
    for (n = 1; n < spi_command_length; n++)
    {
        MESSAGE_INFO_DEBUG(" %02x", spi_command[n]);
    }
    if (spi_command_length > 1)
    {
        MESSAGE_INFO_DEBUG("\n");
    }
    if (tx_data_size > 0)
    {
        for (n = 0; n < tx_data_size; n++)
        {
            MESSAGE_INFO_DEBUG(" %02x", tx_data[n]);
        }
        MESSAGE_INFO_DEBUG("\n");
    }
#endif

    if (4 != spi_command_length)
    {
        MESSAGE_ERROR("spi_controller_tx32_data: command_length (%u) is not 4!\n",
                      spi_command_length);
        return -1;
    }

    if (0 != (tx_data_size & 0x3))
    {
        MESSAGE_ERROR("spi_controller_tx32_data: tx_data_size (%u) is not a multiple of 4!\n",
                      tx_data_size);
        return -1;
    }

    if (0 != (((const size_t)spi_command) & 0x3))
    {
        MESSAGE_ERROR("spi_controller_tx32_data: command is not 32-bit aligned!\n");
        return -1;
    }

    if (0 != (((size_t)tx_data) & 0x3))
    {
        MESSAGE_ERROR("spi_controller_tx32_data: tx_data is not 32-bit aligned!\n");
        return -1;
    }

    rx32_count = (spi_command_length + tx_data_size) / 4;

    if (rx32_count > MAX_RX_TX_FIFO_SIZE)
    {
        MESSAGE_ERROR("spi_controller_tx32_data: tx_data size exceeds RX/TX FIFO size!\n");
        return -1;
    }

    spi_command_32 = (const uint32_t *)(const void *)spi_command;
    tx_data_32 = (const uint32_t *)(const void *)tx_data;
    tx_data_32_end = (const uint32_t *)(const void *)(tx_data + tx_data_size);

    /* transmit the command (and address) */
    iowrite32(spi_regs + SSI_DR0_ADDRESS, reverse_endian(*spi_command_32));
    /* transmit the data */
    while (tx_data_32 < tx_data_32_end)
    {
        iowrite32(spi_regs + SSI_DR0_ADDRESS, reverse_endian(*tx_data_32));
        tx_data_32++;
    }

    /* set SLAVE ENABLE REGISTER to start the transfer */
    iowrite32(spi_regs + SSI_SER_ADDRESS, slave_en_mask);

    /* wait for all the command/data bytes to be sent by polling on FIFO Empty and Busy bit */
    timeout = 0;
    empty_flag = ioread32(spi_regs + SSI_SR_ADDRESS) & 0x5; /*Bit[2]- FIFO Empty, Bit[0]- Busy  */
    while (empty_flag != 0x4)
    {
        timeout++;
        if (timeout > TX_TIMEOUT)
        {
            rv = -1;
            MESSAGE_INFO_DEBUG("TX ERR 2: SR=0x%x\n", ioread32(spi_regs + SSI_SR_ADDRESS));
            goto DONE;
        }
        empty_flag = ioread32(spi_regs + SSI_SR_ADDRESS) & 0x5;
    }

    rv = 0;
DONE:
    iowrite32(spi_regs + SSI_SSIENR_ADDRESS, SSI_SSIENR_SSI_EN_SET(0));
    iowrite32(spi_regs + SSI_SER_ADDRESS, SSI_SER_SER_SET(0));
    return rv;
}

static int spi_controller_tx_data(uintptr_t spi_regs, const uint8_t *spi_command,
                                  uint32_t spi_command_length, uint8_t *tx_data,
                                  uint32_t tx_data_size, uint32_t slave_en_mask)
{
    int rv;
    uint32_t empty_flag;
    uint32_t timeout;
    const uint8_t *tx_data_end;
    const uint8_t *spi_command_end;
    uint32_t total_tx_count;

#if SPI_TX_VERBOSITY > 2
    uint32_t n;
    MESSAGE_INFO_DEBUG("SPI_TX: cmd=%02x, len=%u, tx_data=0x%x, tx_size=%u\n", spi_command[0],
                       spi_command_length, tx_data, tx_data_size);
    for (n = 1; n < spi_command_length; n++)
    {
        MESSAGE_INFO_DEBUG(" %02x", spi_command[n]);
    }
    if (spi_command_length > 1)
    {
        MESSAGE_INFO_DEBUG("\n");
    }
    if (tx_data_size > 0)
    {
        for (n = 0; n < tx_data_size; n++)
        {
            MESSAGE_INFO_DEBUG(" %02x", tx_data[n]);
        }
        MESSAGE_INFO_DEBUG("\n");
    }
#endif

    total_tx_count = spi_command_length + tx_data_size;

    if (total_tx_count > MAX_RX_TX_FIFO_SIZE)
    {
        MESSAGE_ERROR("spi_controller_tx32_data: tx_data size exceeds RX/TX FIFO size!\n");
        return -1;
    }

    spi_command_end = spi_command + spi_command_length;
    tx_data_end = tx_data + tx_data_size;

    /* transmit the command, address and dummy bits */
    while (spi_command < spi_command_end)
    {
        iowrite32(spi_regs + SSI_DR0_ADDRESS, (uint32_t)*spi_command);
        spi_command++;
    }
    /* transmit the data */
    while (tx_data < tx_data_end)
    {
        iowrite32(spi_regs + SSI_DR0_ADDRESS, (uint32_t)*tx_data);
        tx_data++;
    }

    /* set SLAVE ENABLE REGISTER to start the transfer */
    iowrite32(spi_regs + SSI_SER_ADDRESS, slave_en_mask);

    /* wait for all the command/data bytes to be sent by polling on FIFO Empty and Busy bit */
    timeout = 0;
    empty_flag = ioread32(spi_regs + SSI_SR_ADDRESS) & 0x5; /*Bit[2]- FIFO Empty, Bit[0]- Busy */
    while (empty_flag != 0x4)
    {
        timeout++;
        if (timeout > TX_TIMEOUT)
        {
            rv = -1;
            MESSAGE_INFO_DEBUG("TX ERR 2: SR=0x%x\n", ioread32(spi_regs + SSI_SR_ADDRESS));
            goto DONE;
        }
        empty_flag = ioread32(spi_regs + SSI_SR_ADDRESS) & 0x5;
    }

    rv = 0;
DONE:
    iowrite32(spi_regs + SSI_SSIENR_ADDRESS, SSI_SSIENR_SSI_EN_SET(0));
    iowrite32(spi_regs + SSI_SER_ADDRESS, SSI_SER_SER_SET(0));
    return rv;
}

#define CHECK_ERROR_CONDITION(cmd_len, data_size, cmd, data)                                    \
    if (4 != cmd_len)                                                                           \
    {                                                                                           \
        MESSAGE_ERROR("spi_controller_rx32_data: command_length is not 4!\n");                  \
        return -1;                                                                              \
    }                                                                                           \
    if (0 != (data_size & 0x3))                                                                 \
    {                                                                                           \
        MESSAGE_ERROR("spi_controller_rx32_data: rx_data_size is not a multiple of 32-bit!\n"); \
        return -1;                                                                              \
    }                                                                                           \
    if (0 != (cmd & 0x3))                                                                       \
    {                                                                                           \
        MESSAGE_ERROR("spi_controller_rx32_data: command is not 32-bit aligned!\n");            \
        return -1;                                                                              \
    }                                                                                           \
    if (0 != (data & 0x3))                                                                      \
    {                                                                                           \
        MESSAGE_ERROR("spi_controller_rx32_data: rx_data is not 32-bit aligned!\n");            \
        return -1;                                                                              \
    }                                                                                           \

#define ALIGN_START_ADDRESS(data, data_size, read_value, bytes_to_align) \
        switch(bytes_to_align) {                                         \
             case 0:                                                     \
                    data = read_value;                                   \
                    data_size = 4;                                       \
                    break;                                               \
             case 1:                                                     \
                    data = read_value + 1;                               \
                    data_size = 3;                                       \
                    bytes_to_align -= 1;                                 \
                    break;                                               \
             case 2:                                                     \
                    data = read_value + 2;                               \
                    data_size = 2;                                       \
                    bytes_to_align -= 2;                                 \
                    break;                                               \
             case 3:                                                     \
                    data = read_value + 3;                               \
                    data_size = 1;                                       \
                    bytes_to_align -= 3;                                 \
                    break;                                               \
             default:                                                    \
                    data_size = 0;                                       \
                    bytes_to_align -= 4;                                 \
           }                                                             \

#define MEM_COPY_DATA(src_ptr, src_size, dest_ptr, dest_size) \
                while (src_size > 0 && dest_size > 0)         \
                {                                             \
                    *dest_ptr = *src_ptr;                     \
                    dest_ptr++;                               \
                    src_ptr++;                                \
                    src_size--;                               \
                    dest_size--;                              \
                }                                             \

static int spi_controller_rx32_data(uintptr_t spi_regs, const uint8_t *spi_command,
                                    uint32_t spi_command_length, uint32_t read_frames,
                                    uint32_t skip_read_size, uint8_t *rx_data,
                                    uint32_t rx_data_size)
{
    int rv;
    uint32_t rxflr;
    uint32_t timeout;
    union
    {
        uint8_t u8[4];
        uint32_t u32;
    } read_value;
    const uint32_t *spi_command_32;
    const uint8_t *data;
    uint32_t data_size;


#if SPI_RX_VERBOSITY > 2
    MESSAGE_INFO_DEBUG("SPI_RX32: cmd=%02x, len=%u, rf=%u, srs=%u, rx_data=0x%x, rx_size=%u\n",
                       spi_command[0], spi_command_length, read_frames, skip_read_size, rx_data,
                       rx_data_size);
    for (n = 1; n < spi_command_length; n++)
    {
        MESSAGE_INFO_DEBUG(" %02x", spi_command[n]);
    }
    if (spi_command_length > 1)
    {
        MESSAGE_INFO_DEBUG("\n");
    }
#endif

    CHECK_ERROR_CONDITION(spi_command_length, rx_data_size, (const size_t)spi_command, (const size_t)rx_data)

    spi_command_32 = (const uint32_t *)(const void *)spi_command;

    /* transmit the command (and address) */
    iowrite32(spi_regs + SSI_DR0_ADDRESS, reverse_endian(*spi_command_32));

    /* read the data from the RX FIFO */
    timeout = 0;
    while (read_frames > 0)
    {
        rxflr = ioread32(spi_regs + SSI_RXFLR_ADDRESS);
        if (SSI_RXFLR_RXTFL_GET(rxflr) > 0)
        {
            for (uint32_t n = 0; n < rxflr; n++)
            {
                read_value.u32 = reverse_endian(ioread32(spi_regs + SSI_DR0_ADDRESS));

                ALIGN_START_ADDRESS(data, data_size, read_value.u8, skip_read_size)

                MEM_COPY_DATA(data, data_size, rx_data, rx_data_size)

                read_frames--;
                timeout = 0;
            }
        }
        else
        {
            timeout++;
            if (timeout > RX_TIMEOUT)
            {
                rv = -1;
                MESSAGE_ERROR("RX ERR: SR=0x%x, TXTFL=0x%x, RXTFL=0x%x\n",
                              ioread32(spi_regs + SSI_SR_ADDRESS),
                              ioread32(spi_regs + SSI_TXFLR_ADDRESS), rxflr);
                MESSAGE_ERROR("        CTRLR0=0x%x, SSIENR=0x%x, SER=0x%x\n",
                              ioread32(spi_regs + SSI_CTRLR0_ADDRESS),
                              ioread32(spi_regs + SSI_SSIENR_ADDRESS),
                              ioread32(spi_regs + SSI_SER_ADDRESS));
                MESSAGE_ERROR("        BAUDR=0x%x, TXFTLR=0x%x, RXFTLR=0x%x\n",
                              ioread32(spi_regs + SSI_BAUDR_ADDRESS),
                              ioread32(spi_regs + SSI_TXFTLR_ADDRESS),
                              ioread32(spi_regs + SSI_RXFTLR_ADDRESS));
                goto DONE;
            }
        }
    }

    rv = 0;
DONE:
    iowrite32(spi_regs + SSI_SSIENR_ADDRESS, SSI_SSIENR_SSI_EN_SET(0));
    iowrite32(spi_regs + SSI_SER_ADDRESS, SSI_SER_SER_SET(0));
    return rv;
}

static int spi_controller_rx_data(uintptr_t spi_regs, const uint8_t *spi_command,
                                  uint32_t spi_command_length, uint8_t *rx_data,
                                  uint32_t rx_data_size)
{
    int rv;
    uint32_t sr;
    uint32_t rxflr;
    uint32_t timeout;
    uint8_t byte_value;

    (void)sr;

#if SPI_RX_VERBOSITY > 2
    uint32_t n;
    MESSAGE_INFO_DEBUG("SPI_RX: cmd=%02x, len=%u, rx_data=0x%x, rx_size=%u\n", spi_command[0],
                       spi_command_length, rx_data, rx_data_size);
    for (n = 1; n < spi_command_length; n++)
    {
        MESSAGE_INFO_DEBUG(" %02x", spi_command[n]);
    }
    if (spi_command_length > 1)
    {
        MESSAGE_INFO_DEBUG("\n");
    }
#endif

    /* transmit the command, address and dummy bits */
    while (spi_command_length > 0)
    {
        iowrite32(spi_regs + SSI_DR0_ADDRESS, (uint32_t)*spi_command);
        spi_command++;
        spi_command_length--;
    }

    sr = ioread32(spi_regs + SSI_SR_ADDRESS);
    timeout = 0;
    while (rx_data_size > 0)
    {
        rxflr = ioread32(spi_regs + SSI_RXFLR_ADDRESS);
        if (SSI_RXFLR_RXTFL_GET(rxflr) > 0)
        {
            byte_value = ioread32(spi_regs + SSI_DR0_ADDRESS) & 0xFF;
            *rx_data = byte_value;
            rx_data++;
            rx_data_size--;
            timeout = 0;
        }
        else
        {
            timeout++;
            if (timeout > RX_TIMEOUT)
            {
                rv = -1;
                MESSAGE_ERROR("RX ERR: SR=0x%x, TXTFL=0x%x, RXTFL=0x%x\n",
                              ioread32(spi_regs + SSI_SR_ADDRESS),
                              ioread32(spi_regs + SSI_TXFLR_ADDRESS), rxflr);
                MESSAGE_ERROR("        CTRLR0=0x%x, SSIENR=0x%x, SER=0x%x\n",
                              ioread32(spi_regs + SSI_CTRLR0_ADDRESS),
                              ioread32(spi_regs + SSI_SSIENR_ADDRESS),
                              ioread32(spi_regs + SSI_SER_ADDRESS));
                MESSAGE_ERROR("        BAUDR=0x%x, TXFTLR=0x%x, RXFTLR=0x%x\n",
                              ioread32(spi_regs + SSI_BAUDR_ADDRESS),
                              ioread32(spi_regs + SSI_TXFTLR_ADDRESS),
                              ioread32(spi_regs + SSI_RXFTLR_ADDRESS));
                MESSAGE_ERROR("SR! 0x%x\n", sr);
                goto DONE;
            }
        }
    }

    rv = 0;
DONE:
    iowrite32(spi_regs + SSI_SSIENR_ADDRESS, SSI_SSIENR_SSI_EN_SET(0));
    iowrite32(spi_regs + SSI_SER_ADDRESS, SSI_SER_SER_SET(0));
    return rv;
}

#define MAX_DUMMY_BYTES 8
int spi_controller_command(SPI_CONTROLLER_ID_t id, uint8_t slave_index, SPI_COMMAND_t *command)
{
    int rv;
    uint8_t spi_command[1 + 3 + MAX_DUMMY_BYTES] __attribute__((aligned(4)));
    uint32_t spi_command_length;
    uintptr_t spi_regs = get_spi_registers(id);
    uint32_t dfs32_frame_size;
    uint32_t slave_en_mask;
    bool use_32bit_frames = false;
    uint32_t true_write_size;
    uint32_t true_read_size = 0;
    uint32_t skip_read_size = 0;
    uint32_t read_frames;

    if (0 == spi_regs)
    {
        return -1;
    }
    if (slave_index >= SPI_SSI_NUM_SLAVES)
    {
        return -1;
    }
    if (command->dummy_bytes > MAX_DUMMY_BYTES)
    {
        return -1;
    }
    if (command->data_receive)
    {
        if (0 == command->data_size || command->data_size > 0x10000u)
        {
            return -1;
        }
    }

    spi_command[0] = command->cmd;
    spi_command_length = 1;
    if (command->include_address)
    {
        spi_command[spi_command_length + 0] = (command->address >> 16u) & 0xFF;
        spi_command[spi_command_length + 1] = (command->address >> 8u) & 0xFF;
        spi_command[spi_command_length + 2] = (command->address >> 0u) & 0xFF;
        spi_command_length += 3;
    }

    if (0 == command->data_receive)
    {
        /* we are transmitting data */
        true_write_size = spi_command_length + command->data_size;

        if (0 != command->dummy_bytes)
        {
            MESSAGE_ERROR(
                "spi_controller_command: support for tx with dummy bytes not implemented!\n");
            return -1;
        }
        if (0 == (true_write_size & 0x3) && 4 == spi_command_length)
        {
            /* we will use 32-bit frames */
            use_32bit_frames = true;
        }
        else if (1 == true_write_size)
        {
            /* we will use 8-bit frames */
        }
        else
        {
            MESSAGE_ERROR(
                "spi_controller_command: tx command_length (%u) and data_size (%u) not supported!\n",
                spi_command_length, command->data_size);
            return -1;
        }
    }
    else
    {
        /* we are receiving data */
        true_read_size = command->dummy_bytes + command->data_size;
        skip_read_size = command->dummy_bytes;
        if (4 == spi_command_length)
        {
            /* command length is 32-bit, we will handle dummy bytes as part of the read */
            true_read_size = (true_read_size + 3) & 0xFFFFFFFC; /* round up to the next 4 bytes */
            /* we will use 32-bit frames */
            use_32bit_frames = true;
        }
        else if (1 == spi_command_length)
        {
            /* command length is 8-bit, we will handle dummy bytes as part of the read */
            /* we will use 8-bit frames */
        }
        else
        {
            MESSAGE_ERROR("spi_controller_command: rx command_length (%u) not supported!\n",
                          spi_command_length);
            return -1;
        }
    }

    if (use_32bit_frames)
    {
        dfs32_frame_size = SSI_CTRLR0_DFS_32_DFS_32_FRAME_32BITS;
    }
    else
    {
        dfs32_frame_size = SSI_CTRLR0_DFS_32_DFS_32_FRAME_08BITS;
    }

    iowrite32(spi_regs + SSI_SSIENR_ADDRESS, SSI_SSIENR_SSI_EN_SET(0));
    slave_en_mask = SSI_SER_SER_SET((1u << slave_index) & SLAVE_MASK);

    if (command->data_receive)
    {
        iowrite32(spi_regs + SSI_BAUDR_ADDRESS, SSI_BAUDR_SCKDV_SET(RX_BAUD_RATE_DIVIDER_VALUE));
        iowrite32(spi_regs + SSI_CTRLR0_ADDRESS,
                  (uint32_t)(
                      /* SSI_CTRLR0_DFS_SET(0) | */
                      SSI_CTRLR0_FRF_SET(SSI_CTRLR0_FRF_FRF_MOTOROLA_SPI) |
                      SSI_CTRLR0_SCPH_SET(SCPH_VALUE) | SSI_CTRLR0_SCPOL_SET(SCPOL_VALUE) |
                      SSI_CTRLR0_TMOD_SET(SSI_CTRLR0_TMOD_TMOD_EEPROM_READ) |
                      SSI_CTRLR0_SRL_SET(SSI_CTRLR0_SRL_SRL_NORMAL_MODE) |
                      SSI_CTRLR0_DFS_32_SET(dfs32_frame_size & 0x1Fu) |
                      SSI_CTRLR0_SPI_FRF_SET(SSI_CTRLR0_SPI_FRF_SPI_FRF_STD_SPI_FRF) |
                      SSI_CTRLR0_SSTE_SET(0)));

        if (use_32bit_frames)
        {
            read_frames = true_read_size / 4;
            iowrite32(spi_regs + SSI_CTRLR1_ADDRESS,
                      SSI_CTRLR1_NDF_SET((uint16_t)(read_frames - 1u)));
        }
        else
        {
            iowrite32(spi_regs + SSI_CTRLR1_ADDRESS,
                      SSI_CTRLR1_NDF_SET((uint16_t)(true_read_size - 1u)));
        }
        iowrite32(spi_regs + SSI_SSIENR_ADDRESS, SSI_SSIENR_SSI_EN_SET(1));
        iowrite32(spi_regs + SSI_SER_ADDRESS, slave_en_mask);

        if (use_32bit_frames)
        {
            rv = spi_controller_rx32_data(spi_regs, spi_command, spi_command_length, read_frames,
                                        skip_read_size, command->data_buffer, command->data_size);
        }
        else
        {
            rv = spi_controller_rx_data(spi_regs, spi_command, spi_command_length,
                                        command->data_buffer, command->data_size);
        }
    }
    else
    {
        iowrite32(spi_regs + SSI_BAUDR_ADDRESS, SSI_BAUDR_SCKDV_SET(TX_BAUD_RATE_DIVIDER_VALUE));
        iowrite32(spi_regs + SSI_CTRLR0_ADDRESS,
                  (uint32_t)(
                      /* SSI_CTRLR0_DFS_SET(0)                                       | */
                      SSI_CTRLR0_FRF_SET(SSI_CTRLR0_FRF_FRF_MOTOROLA_SPI) |
                      SSI_CTRLR0_SCPH_SET(SCPH_VALUE) | SSI_CTRLR0_SCPOL_SET(SCPOL_VALUE) |
                      SSI_CTRLR0_TMOD_SET(SSI_CTRLR0_TMOD_TMOD_TX_ONLY) |
                      SSI_CTRLR0_SRL_SET(SSI_CTRLR0_SRL_SRL_NORMAL_MODE) |
                      SSI_CTRLR0_DFS_32_SET(dfs32_frame_size & 0x1Fu) |
                      SSI_CTRLR0_SPI_FRF_SET(SSI_CTRLR0_SPI_FRF_SPI_FRF_STD_SPI_FRF) |
                      SSI_CTRLR0_SSTE_SET(0)));
        iowrite32(spi_regs + SSI_SSIENR_ADDRESS, SSI_SSIENR_SSI_EN_SET(1));
        if (use_32bit_frames)
        {
            rv = spi_controller_tx32_data(spi_regs, spi_command, spi_command_length,
                                          command->data_buffer, command->data_size, slave_en_mask);
        }
        else
        {
            rv = spi_controller_tx_data(spi_regs, spi_command, spi_command_length,
                                        command->data_buffer, command->data_size, slave_en_mask);
        }
    }

    /* force the CS# to go high at the end of the command */
    iowrite32(spi_regs + SSI_SSIENR_ADDRESS, SSI_SSIENR_SSI_EN_SET(0));
    iowrite32(spi_regs + SSI_SER_ADDRESS, SSI_SER_SER_SET(0));
    iowrite32(spi_regs + SSI_SSIENR_ADDRESS, SSI_SSIENR_SSI_EN_SET(1));
    iowrite32(spi_regs + SSI_SSIENR_ADDRESS, SSI_SSIENR_SSI_EN_SET(0));

    return rv;
}

#pragma GCC pop_options
