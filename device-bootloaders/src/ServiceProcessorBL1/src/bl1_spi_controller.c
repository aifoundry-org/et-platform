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

#include "et_cru.h"
#include "serial.h"

#include "printx.h"
#include "bl1_spi_controller.h"
#include "synopsys_spi.h"

#pragma GCC push_options
//#pragma GCC optimize ("O3")
#pragma GCC diagnostic ignored "-Wswitch-enum"

#define DUMMY_TX_BYTE_VALUE 0xFF
//#define US_TX_ONLY_MODE

#if 0
#define BAUD_RATE_DIVIDER_VALUE 4
#else
#define BAUD_RATE_DIVIDER_VALUE 64
#endif

#if 1
#define SCPOL_VALUE SPI_CTRLR0_SCPOL_SCLK_LOW
#else
#define SCPOL_VALUE SPI_CTRLR0_SCPOL_SCLK_HIGH
#endif

#if 1
#define SCPH_VALUE SPI_CTRLR0_SCPH_MIDDLE
#else
#define SCPH_VALUE SPI_CTRLR0_SCPH_START
#endif

#define SPIO_SPI0_BASE_ADDR                 0x52021000
#define SPIO_SPI1_BASE_ADDR                 0x54051000

#define SLAVE_MASK ((1u << SPI_SSI_NUM_SLAVES) - 1)
#define TX_TIMEOUT 0x1000
#define RX_TIMEOUT 0x1000

static volatile SPI_t * get_spi_registers(SPI_CONTROLLER_ID_t id) {
    switch (id) {
    case SPI_CONTROLLER_ID_SPI_0:
        return (volatile SPI_t *)SPIO_SPI0_BASE_ADDR;
    case SPI_CONTROLLER_ID_SPI_1:
        return (volatile SPI_t *)SPIO_SPI1_BASE_ADDR;
    default:
        return NULL;
    }
}

int spi_controller_init(SPI_CONTROLLER_ID_t id) {
    volatile SPI_t * spi_regs = get_spi_registers(id);
    if (NULL == spi_regs) {
        return -1;
    }

    // printx("SSI_VERSION_ID: 0x%08x\n", spi_regs->SSI_VERSION_ID);

    spi_regs->SSIENR.R = (SPI_SSIENR_t){ .B = { .SSI_EN = 0 }}.R;

    spi_regs->CTRLR0.R = (SPI_CTRLR0_t){ .B = { .DFS = SPI_CTRLR0_FRAME_08BITS,
                                              .FRF = SPI_CTRLR0_FRF_MOTOROLA_SPI,
                                              .SCPH = SCPH_VALUE,
                                              .SCPOL = SCPOL_VALUE,
                                              .TMOD = SPI_CTRLR0_TMOD_TX_AND_RX,
                                              .SRL = SPI_CTRLR0_SRL_NORMAL_MODE,
                                              .DFS_32 = SPI_CTRLR0_FRAME_08BITS,
                                              .SPI_FRF = SPI_CGTRLR0_SPI_PERF_STD,
                                              .SSTE = 0 }}.R;

    spi_regs->CTRLR1.R = (SPI_CTRLR1_t){ .B = { .NDF = 0 }}.R;

    spi_regs->BAUDR.R = (SPI_BAUDR_t){ .B = { .SCKDV = BAUD_RATE_DIVIDER_VALUE }}.R;

    spi_regs->TXFTLR.R = (SPI_TXFTLR_t){ .B = { .TFT = SPI_TX_FIFO_MAX_DEPTH }}.R;
    spi_regs->RXFTLR.R = (SPI_RXFTLR_t){ .B = { .RFT = SPI_RX_FIFO_MAX_DEPTH }}.R;

    spi_regs->IMR.R = (SPI_IMR_t){ .B = { .TXEIM = 0,
                                        .TXOIM = 0,
                                        .RXUIM = 0,
                                        .RXOIM = 0,
                                        .RXFIM = 0,
                                        .MSTIM = 0 }}.R;

    spi_regs->SSIENR.R = (SPI_SSIENR_t){ .B = { .SSI_EN = 1 }}.R;

    return 0;
}

static int spi_controller_tx_data(volatile SPI_t * spi_regs, const uint8_t * spi_command, uint32_t spi_command_length, uint8_t * tx_data, uint32_t tx_data_size) {
#ifdef US_TX_ONLY_MODE
    int rv;
    SPI_TXFLR_t txflr;
    uint32_t timeout;

    printx("spi_controller_tx_data: spi_command=0x%08x, spi_command_length=%u, tx_data=0x%x, tx_data_size=%u\n", spi_command, spi_command_length, tx_data, tx_data_size);
    
    // transmit the command, address and dummy bits
    while (spi_command_length > 0) {
        spi_regs->DRx[0] = (uint32_t)*spi_command;
        spi_command++;
        spi_command_length--;
    }

    // transmit the (optional) data
    timeout = 0;
    while (tx_data_size > 0) {
        txflr.R = spi_regs->TXFLR.R;
        if (txflr.B.TXTFL < SPI_TX_FIFO_MAX_DEPTH) {
            spi_regs->DRx[0] = (uint32_t)*tx_data;
            tx_data++;
            tx_data_size--;
            timeout = 0;
        } else {
            timeout++;
            if (timeout > TX_TIMEOUT) {
                rv = -1;
                printx("TX ERR 1: SR=0x%x, TXTFL=0x%x\n", spi_regs->SR.R, txflr.R);
                goto DONE;
            }
        }
    }

    // wait for all the command/data bytes to be sent
    txflr.R = spi_regs->TXFLR.R;
    while (txflr.B.TXTFL > 0) {
        timeout++;
        if (timeout > TX_TIMEOUT) {
            rv = -1;
            printx("TX ERR 2: SR=0x%x, TXTFL=0x%x\n", spi_regs->SR.R, txflr.R);
            goto DONE;
        }
        txflr.R = spi_regs->TXFLR.R;
    }

    rv = 0;
DONE:
    spi_regs->SSIENR.R = (SPI_SSIENR_t){ .B = { .SSI_EN = 0 }}.R;
    spi_regs->SER.R = (SPI_SER_t){ .B = { .SER = 0 }}.R;
    return rv;
#else
    int rv;
    SPI_SR_t sr;
    SPI_TXFLR_t txflr;
    SPI_RXFLR_t rxflr;
    uint32_t timeout;
    uint32_t rx_data_size;
    uint8_t byte_value;

    //printx("\nSPI TX: cmd=%02x, cmd_len=%u, data_size=%u\n", *spi_command, spi_command_length, tx_data_size);

    rx_data_size = spi_command_length + tx_data_size;

    // transmit the command, address and dummy bits
    while (spi_command_length > 0) {
        //printx("[tx:%02x] ", *spi_command);
        spi_regs->DRx[0] = (uint32_t)*spi_command;
        spi_command++;
        spi_command_length--;
    }
    sr.R = spi_regs->SR.R;
    timeout = 0;
    while (rx_data_size > 0) {
        if (tx_data_size > 0) {
            txflr.R = spi_regs->TXFLR.R;
            if (txflr.B.TXTFL < SPI_TX_FIFO_MAX_DEPTH) {
                //printx("[tx: %02x] ", *tx_data);
                spi_regs->DRx[0] = *tx_data;
                tx_data_size--;
                tx_data++;
            }
        }
        rxflr.R = spi_regs->RXFLR.R;
        if (rxflr.B.RXTFL > 0) {
            byte_value = spi_regs->DRx[0] & 0xFF;
            (void)byte_value;
            //printx("[rx: %02x] ", byte_value);
            rx_data_size--;
            timeout = 0;
        } else {
            timeout++;
            if (timeout > RX_TIMEOUT) {
                rv = -1;
                printx("RX ERR: SR=0x%x, TXTFL=0x%x, RXTFL=0x%x\n", spi_regs->SR.R, spi_regs->TXFLR.R, rxflr.R);
                printx("        CTRLR0=0x%x, SSIENR=0x%x, SER=0x%x\n", spi_regs->CTRLR0.R, spi_regs->SSIENR.R, spi_regs->SER.R);
                printx("        BAUDR=0x%x, TXFTLR=0x%x, RXFTLR=0x%x\n", spi_regs->BAUDR.R, spi_regs->TXFTLR.R, spi_regs->RXFTLR.R);
                printx("SR! 0x%x TXFLR! 0x%x\n", sr.R, txflr.R);
                goto DONE;
            }
        }
    }

    rv = 0;
DONE:
    spi_regs->SSIENR.R = (SPI_SSIENR_t){ .B = { .SSI_EN = 0 }}.R;
    spi_regs->SER.R = (SPI_SER_t){ .B = { .SER = 0 }}.R;
    return rv;
#endif
}

static int spi_controller_rx_data(volatile SPI_t * spi_regs, const uint8_t * spi_command, uint32_t spi_command_length, uint8_t * rx_data, uint32_t rx_data_size) {
    int rv;
    SPI_SR_t sr;
    SPI_TXFLR_t txflr;
    SPI_RXFLR_t rxflr;
    uint32_t timeout;
    uint32_t dummy_tx_count = rx_data_size;
    uint32_t skip_receive_data = spi_command_length;
    uint8_t byte_value;
    static bool print_stack = true;

    if (print_stack) {
        print_stack = false;
        printx("spi_controller_data: SP=0x%x\n", &sr);
    }

    //printx("\nSPI RX: cmd=%02x, cmd_len=%u, data_size=%u\n", *spi_command, spi_command_length, rx_data_size);

    rx_data_size += spi_command_length;

    // transmit the command, address and dummy bits
    while (spi_command_length > 0) {
        //printx("{tx: %02x} ", *spi_command);
        spi_regs->DRx[0] = (uint32_t)*spi_command;
        spi_command++;
        spi_command_length--;
    }
    sr.R = spi_regs->SR.R;
    timeout = 0;
    while (rx_data_size > 0) {
        if (dummy_tx_count > 0) {
            txflr.R = spi_regs->TXFLR.R;
            if (txflr.B.TXTFL < SPI_TX_FIFO_MAX_DEPTH) {
                //printx("{tx: %02x} ", DUMMY_TX_BYTE_VALUE);
                spi_regs->DRx[0] = DUMMY_TX_BYTE_VALUE;
                dummy_tx_count--;
            }
        }
        rxflr.R = spi_regs->RXFLR.R;
        if (rxflr.B.RXTFL > 0) {
            byte_value = spi_regs->DRx[0] & 0xFF;
            //printx("{rx: %02x} ", byte_value);
            if (skip_receive_data > 0) {
                skip_receive_data--;
            } else {
                *rx_data = byte_value;
                rx_data++;
            }
            rx_data_size--;
            timeout = 0;
        } else {
            timeout++;
            if (timeout > RX_TIMEOUT) {
                rv = -1;
                printx("RX ERR: SR=0x%x, TXTFL=0x%x, RXTFL=0x%x\n", spi_regs->SR.R, spi_regs->TXFLR.R, rxflr.R);
                printx("        CTRLR0=0x%x, SSIENR=0x%x, SER=0x%x\n", spi_regs->CTRLR0.R, spi_regs->SSIENR.R, spi_regs->SER.R);
                printx("        BAUDR=0x%x, TXFTLR=0x%x, RXFTLR=0x%x\n", spi_regs->BAUDR.R, spi_regs->TXFTLR.R, spi_regs->RXFTLR.R);
                printx("SR! 0x%x TXFLR! 0x%x\n", sr.R, txflr.R);
                goto DONE;
            }
        }
    }

    rv = 0;
DONE:
    spi_regs->SSIENR.R = (SPI_SSIENR_t){ .B = { .SSI_EN = 0 }}.R;
    spi_regs->SER.R = (SPI_SER_t){ .B = { .SER = 0 }}.R;
    return rv;
}

#define MAX_DUMMY_BYTES 8
int spi_controller_command(SPI_CONTROLLER_ID_t id, uint8_t slave_index, SPI_COMMAND_t * command) {
    uint8_t spi_command[1 + 3 + MAX_DUMMY_BYTES];
    uint32_t spi_command_length;
    uint32_t d;
    volatile SPI_t * spi_regs = get_spi_registers(id);
    if (NULL == spi_regs) {
        return -1;
    }
    if (slave_index >= SPI_SSI_NUM_SLAVES) {
        return -1;
    }
    if (command->dummy_bytes > MAX_DUMMY_BYTES) {
        return -1;
    }

    spi_command[0] = command->cmd;
    spi_command_length = 1;
    if (command->include_address) {
        spi_command[spi_command_length + 0] = (command->address >> 16u) & 0xFF;
        spi_command[spi_command_length + 1] = (command->address >> 8u) & 0xFF;
        spi_command[spi_command_length + 2] = (command->address >> 0u) & 0xFF;
        spi_command_length += 3;
    }
    for (d = 0; d < command->dummy_bytes; d++) {
        spi_command[spi_command_length] = 0;
        spi_command_length++;
    }

    spi_regs->SSIENR.R = (SPI_SSIENR_t){ .B = { .SSI_EN = 0 }}.R;
    spi_regs->SER.R = (SPI_SER_t){ .B = { .SER = (1u << slave_index) & SLAVE_MASK }}.R;

    if (command->data_receive) {
        spi_regs->CTRLR0.R = (SPI_CTRLR0_t){ .B = { .DFS = SPI_CTRLR0_FRAME_08BITS,
                                                .FRF = SPI_CTRLR0_FRF_MOTOROLA_SPI,
                                                .SCPH = SCPH_VALUE,
                                                .SCPOL = SCPOL_VALUE,
                                                .TMOD = SPI_CTRLR0_TMOD_TX_AND_RX,
                                                .SRL = SPI_CTRLR0_SRL_NORMAL_MODE,
                                                .DFS_32 = SPI_CTRLR0_FRAME_08BITS,
                                                .SPI_FRF = SPI_CGTRLR0_SPI_PERF_STD,
                                                .SSTE = 0 }}.R;
        spi_regs->SSIENR.R = (SPI_SSIENR_t){ .B = { .SSI_EN = 1 }}.R;
        return spi_controller_rx_data(spi_regs, spi_command, spi_command_length, command->data_buffer, command->data_size);
    } else {
        spi_regs->CTRLR0.R = (SPI_CTRLR0_t){ .B = { .DFS = SPI_CTRLR0_FRAME_08BITS,
                                                .FRF = SPI_CTRLR0_FRF_MOTOROLA_SPI,
                                                .SCPH = SCPH_VALUE,
                                                .SCPOL = SCPOL_VALUE,
#ifdef US_TX_ONLY_MODE
                                                .TMOD = SPI_CTRLR0_TMOD_TX_ONLY,
#else
                                                .TMOD = SPI_CTRLR0_TMOD_TX_AND_RX,
#endif
                                                .SRL = SPI_CTRLR0_SRL_NORMAL_MODE,
                                                .DFS_32 = SPI_CTRLR0_FRAME_08BITS,
                                                .SPI_FRF = SPI_CGTRLR0_SPI_PERF_STD,
                                                .SSTE = 0 }}.R;
        spi_regs->SSIENR.R = (SPI_SSIENR_t){ .B = { .SSI_EN = 1 }}.R;
        return spi_controller_tx_data(spi_regs, spi_command, spi_command_length, command->data_buffer, command->data_size);
    }
}

#pragma GCC pop_options
