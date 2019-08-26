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

#include "serial.h"

#include "printx.h"
#include "bl1_spi_controller.h"
#include "DW_apb_ssi.h"
#include "spio_DW_apb_ssi_config.h"
#include "hal_device.h"
#include "bl1_main.h"

#pragma GCC push_options
//#pragma GCC optimize ("O3")
#pragma GCC diagnostic ignored "-Wswitch-enum"

#define DUMMY_TX_BYTE_VALUE 0xFF
//#define USE_TX_ONLY_MODE
#define WRITES_USE_32_BIT_FRAMES
#define READS_USE_32_BIT_FRAMES

#define MAX_RX_TX_FIFO_SIZE 256

#if 1
#define SCPOL_VALUE SSI_CTRLR0_SCPOL_SCPOL_SCLK_LOW
#else
#define SCPOL_VALUE SSI_CTRLR0_SCPOL_SCPOL_SCLK_HIGH
#endif

#if 1
#define SCPH_VALUE SSI_CTRLR0_SCPH_SCPH_SCPH_MIDDLE
#else
#define SCPH_VALUE SSI_CTRLR0_SCPH_SCPH_SCPH_START
#endif

#define SLAVE_MASK ((1u << SPI_SSI_NUM_SLAVES) - 1)
#define TX_TIMEOUT 0x1000
#define RX_TIMEOUT 0x1000

static volatile Ssi_t * get_spi_registers(SPI_CONTROLLER_ID_t id) {
    switch (id) {
    case SPI_CONTROLLER_ID_SPI_0:
        return (volatile Ssi_t *)R_SP_SPI0_BASEADDR;
    case SPI_CONTROLLER_ID_SPI_1:
        return (volatile Ssi_t *)R_SP_SPI1_BASEADDR;
    default:
        return NULL;
    }
}

int spi_controller_init(SPI_CONTROLLER_ID_t id) {
    volatile Ssi_t * spi_regs = get_spi_registers(id);
    if (NULL == spi_regs) {
        return -1;
    }

    // printx("SSI_VERSION_ID: 0x%08x\n", spi_regs->SSI_VERSION_ID);

    spi_regs->SSIENR.R = (Ssi_SSIENR_t){ .B = { .SSI_EN = 0 }}.R;

    spi_regs->CTRLR0.R = (Ssi_CTRLR0_t){ .B = { .DFS = SSI_CTRLR0_DFS_DFS_FRAME_08BITS,
                                              .FRF = SSI_CTRLR0_FRF_FRF_MOTOROLA_SPI,
                                              .SCPH = SCPH_VALUE,
                                              .SCPOL = SCPOL_VALUE,
                                              .TMOD = SSI_CTRLR0_TMOD_TMOD_TX_AND_RX,
                                              .SRL = SSI_CTRLR0_SRL_SRL_NORMAL_MODE,
                                              .DFS_32 = SSI_CTRLR0_DFS_32_DFS_32_FRAME_08BITS,
                                              .SPI_FRF = SSI_CTRLR0_SPI_FRF_SPI_FRF_STD_SPI_FRF,
                                              .SSTE = 0 }}.R;

    spi_regs->CTRLR1.R = (Ssi_CTRLR1_t){ .B = { .NDF = 0 }}.R;

    spi_regs->TXFTLR.R = (Ssi_TXFTLR_t){ .B = { .TFT = SPI_TX_FIFO_MAX_DEPTH }}.R;
    spi_regs->RXFTLR.R = (Ssi_RXFTLR_t){ .B = { .RFT = SPI_RX_FIFO_MAX_DEPTH }}.R;

    spi_regs->IMR.R = (Ssi_IMR_t){ .B = { .TXEIM = 0,
                                        .TXOIM = 0,
                                        .RXUIM = 0,
                                        .RXOIM = 0,
                                        .RXFIM = 0,
                                        .MSTIM = 0 }}.R;

    spi_regs->SSIENR.R = (Ssi_SSIENR_t){ .B = { .SSI_EN = 1 }}.R;

    return 0;
}

#if defined(WRITES_USE_32_BIT_FRAMES) || defined(READS_USE_32_BIT_FRAMES)
static inline uint32_t reverse_endian(const uint32_t v) {
    return (v << 24) | (0x00FF0000 & (v << 8)) | (0x0000FF00 & (v >> 8)) | (0xFF & (v >> 24));
}
#endif

#ifdef WRITES_USE_32_BIT_FRAMES
static int spi_controller_tx32_data(volatile Ssi_t * spi_regs, const uint8_t * spi_command, uint32_t spi_command_length, uint8_t * tx_data, uint32_t tx_data_size, uint32_t slave_en_mask) {
    int rv;
    //Ssi_SR_t sr;
    //Ssi_TXFLR_t txflr;
    Ssi_RXFLR_t rxflr;
    uint32_t timeout;
    uint32_t rx32_count;
    uint32_t dummy_value;
    uint32_t n;
    const uint32_t * tx_data_32;
    const uint32_t * tx_data_32_end;
    const uint32_t * spi_command_32;

    //printx("tx32\n");
    // printx("SPI_TX32: cmd=%02x, len=%u, tx_data=0x%x, tx_size=%u\n", spi_command[0], spi_command_length, tx_data, tx_data_size);
    // for (n = 1; n < spi_command_length; n++) {
    //     printx(" %02x", spi_command[n]);
    // }
    // if (spi_command_length > 1) {
    //     printx("\n");
    // }
    // if (tx_data_size > 0) {
    //     for (n = 0; n < tx_data_size; n++) {
    //         printx(" %02x", tx_data[n]);
    //     }
    //     printx("\n");
    // }

    if (4 != spi_command_length) {
        printx("spi_controller_tx32_data: command_length (%u) is not 4!\n", spi_command_length);
        return -1;
    }

    if (0 != (tx_data_size & 0x3)) {
        printx("spi_controller_tx32_data: tx_data_size (%u) is not a multiple of 4!\n", tx_data_size);
        return -1;
    }

    if (0 != (((const size_t)spi_command) & 0x3)) {
        printx("spi_controller_tx32_data: command is not 32-bit aligned!\n");
        return -1;
    }

    if (0 != (((size_t)tx_data) & 0x3)) {
        printx("spi_controller_tx32_data: tx_data is not 32-bit aligned!\n");
        return -1;
    }

    rx32_count = (spi_command_length + tx_data_size) / 4;

    if (rx32_count > MAX_RX_TX_FIFO_SIZE) {
        printx("spi_controller_tx32_data: tx_data size exceeds RX/TX FIFO size!\n");
        return -1;
    }

    spi_command_32 = (const uint32_t*)(const void*)spi_command;
    tx_data_32 = (const uint32_t*)(const void*)tx_data;
    tx_data_32_end = (const uint32_t*)(const void*)(tx_data + tx_data_size);

    // transmit the command (and address)
    spi_regs->DR0.B.DR = reverse_endian(*spi_command_32);
    // transmit the data
    while (tx_data_32 < tx_data_32_end) {
        spi_regs->DR0.B.DR = reverse_endian(*tx_data_32);
        tx_data_32++;
    }

    // set SLAVE ENABLE REGISTER to start the transfer
    spi_regs->SER.R = slave_en_mask;

    // read the data from the RX FIFO to complete the transfer
    timeout = 0;
    while (rx32_count > 0) {
        rxflr.R = spi_regs->RXFLR.R;
        if (rxflr.B.RXTFL > 0) {
            for (n = 0; n < rxflr.B.RXTFL; n++) {
                dummy_value = spi_regs->DR0.B.DR;
                (void)dummy_value;
            }
            rx32_count -= rxflr.B.RXTFL;
            timeout = 0;
        } else {
            timeout++;
            if (timeout > TX_TIMEOUT) {
                rv = -1;
                printx("TX ERR: SR=0x%x, TXTFL=0x%x, RXTFL=0x%x\n", spi_regs->SR.R, spi_regs->TXFLR.R, rxflr.R);
                printx("        CTRLR0=0x%x, SSIENR=0x%x, SER=0x%x\n", spi_regs->CTRLR0.R, spi_regs->SSIENR.R, spi_regs->SER.R);
                printx("        BAUDR=0x%x, TXFTLR=0x%x, RXFTLR=0x%x\n", spi_regs->BAUDR.R, spi_regs->TXFTLR.R, spi_regs->RXFTLR.R);
                //printx("SR! 0x%x TXFLR! 0x%x\n", sr.R, txflr.R);
                goto DONE;
            }
        }
    }

    rv = 0;
DONE:
    spi_regs->SSIENR.R = (Ssi_SSIENR_t){ .B = { .SSI_EN = 0 }}.R;
    spi_regs->SER.R = (Ssi_SER_t){ .B = { .SER = 0 }}.R;
    return rv;
}
#endif

static int spi_controller_tx_data(volatile Ssi_t * spi_regs, const uint8_t * spi_command, uint32_t spi_command_length, uint8_t * tx_data, uint32_t tx_data_size, uint32_t slave_en_mask) {
#ifdef USE_TX_ONLY_MODE
    int rv;
    Ssi_TXFLR_t txflr;
    uint32_t timeout;
    uint32_t n;
    uint32_t combined_size = spi_command_length + tx_data_size;

    printx("SPI_TX: cmd=%02x, len=%u, tx_data=0x%x, tx_size=%u\n", spi_command[0], spi_command_length, tx_data, tx_data_size);
    for (n = 1; n < spi_command_length; n++) {
        printx(" %02x", spi_command[n]);
    }
    if (spi_command_length > 1) {
        printx("\n");
    }
    if (tx_data_size > 0) {
        for (n = 0; n < tx_data_size; n++) {
            printx(" %02x", tx_data[n]);
        }
        printx("\n");
    }

    if (combined_size > 256) {
        printx("spi_controller_tx_data: TX size (%u + %u = %u)too large!\n", spi_command_length, tx_data_size, combined_size);
        return -1;
    }
    
    // transmit the command, address and dummy bits
    while (spi_command_length > 0) {
        spi_regs->DR0.B.DR = (uint32_t)*spi_command;
        spi_command++;
        spi_command_length--;
    }

    // transmit the (optional) data
    while (tx_data_size > 0) {
        spi_regs->DR0.B.DR = (uint32_t)*tx_data;
        tx_data++;
        tx_data_size--;
    }

    // wait for all the command/data bytes to be sent
    timeout = 0;
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
    spi_regs->SSIENR.R = (Ssi_SSIENR_t){ .B = { .SSI_EN = 0 }}.R;
    spi_regs->SER.R = (Ssi_SER_t){ .B = { .SER = 0 }}.R;
    return rv;
#else
    int rv;
    //Ssi_SR_t sr;
    //Ssi_TXFLR_t txflr;
    Ssi_RXFLR_t rxflr;
    uint32_t timeout;
    uint32_t rx_data_count;
    uint32_t dummy_value;
    uint32_t n;
    const uint8_t * tx_data_end;
    const uint8_t * spi_command_end;

    //printx("tx\n");
    // printx("SPI_TX: cmd=%02x, len=%u, tx_data=0x%x, tx_size=%u\n", spi_command[0], spi_command_length, tx_data, tx_data_size);
    // for (n = 1; n < spi_command_length; n++) {
    //     printx(" %02x", spi_command[n]);
    // }
    // if (spi_command_length > 1) {
    //     printx("\n");
    // }
    // if (tx_data_size > 0) {
    //     for (n = 0; n < tx_data_size; n++) {
    //         printx(" %02x", tx_data[n]);
    //     }
    //     printx("\n");
    // }
    rx_data_count = spi_command_length + tx_data_size;

    if (rx_data_count > MAX_RX_TX_FIFO_SIZE) {
        printx("spi_controller_tx32_data: tx_data size exceeds RX/TX FIFO size!\n");
        return -1;
    }

    spi_command_end = spi_command + spi_command_length;
    tx_data_end = tx_data + tx_data_size;

    // transmit the command, address and dummy bits
    while (spi_command < spi_command_end) {
        spi_regs->DR0.B.DR = (uint32_t)*spi_command;
        spi_command++;
    }
    // transmit the data
    while (tx_data < tx_data_end) {
        spi_regs->DR0.B.DR = (uint32_t)*tx_data;
        tx_data++;
    }

    // set SLAVE ENABLE REGISTER to start the transfer
    spi_regs->SER.R = slave_en_mask;

    timeout = 0;
    while (rx_data_count > 0) {
        rxflr.R = spi_regs->RXFLR.R;
        if (rxflr.B.RXTFL > 0) {
            for (n = 0; n < rxflr.B.RXTFL; n++) {
                dummy_value = spi_regs->DR0.B.DR;
                (void)dummy_value;
            }
            rx_data_count -= rxflr.B.RXTFL;
            timeout = 0;
        } else {
            timeout++;
            if (timeout > TX_TIMEOUT) {
                rv = -1;
                printx("TX ERR: SR=0x%x, TXTFL=0x%x, RXTFL=0x%x\n", spi_regs->SR.R, spi_regs->TXFLR.R, rxflr.R);
                printx("        CTRLR0=0x%x, SSIENR=0x%x, SER=0x%x\n", spi_regs->CTRLR0.R, spi_regs->SSIENR.R, spi_regs->SER.R);
                printx("        BAUDR=0x%x, TXFTLR=0x%x, RXFTLR=0x%x\n", spi_regs->BAUDR.R, spi_regs->TXFTLR.R, spi_regs->RXFTLR.R);
                //printx("SR! 0x%x TXFLR! 0x%x\n", sr.R, txflr.R);
                goto DONE;
            }
        }
    }

    rv = 0;
DONE:
    spi_regs->SSIENR.R = (Ssi_SSIENR_t){ .B = { .SSI_EN = 0 }}.R;
    spi_regs->SER.R = (Ssi_SER_t){ .B = { .SER = 0 }}.R;
    return rv;
#endif
}

#ifdef READS_USE_32_BIT_FRAMES
static int spi_controller_rx32_data(volatile Ssi_t * spi_regs, const uint8_t * spi_command, uint32_t spi_command_length, uint32_t read_frames, uint32_t skip_read_size, uint8_t * rx_data, uint32_t rx_data_size) {
    int rv;
    //Ssi_SR_t sr;
    //Ssi_TXFLR_t txflr;
    Ssi_RXFLR_t rxflr;
    uint32_t timeout;
    union {
        uint8_t u8[4];
        uint32_t u32;        
    } read_value;
    uint32_t n;
    const uint32_t * spi_command_32;
    const uint8_t * data;
    uint32_t data_size;

    //printx("rx32\n");
    // printx("SPI_RX32: cmd=%02x, len=%u, rf=%u, srs=%u, rx_data=0x%x, rx_size=%u\n", spi_command[0], spi_command_length, read_frames, skip_read_size, rx_data, rx_data_size);
    // for (n = 1; n < spi_command_length; n++) {
    //     printx(" %02x", spi_command[n]);
    // }
    // if (spi_command_length > 1) {
    //     printx("\n");
    // }

    if (4 != spi_command_length) {
        printx("spi_controller_rx32_data: command_length is not 4!\n");
        return -1;
    }

    if (0 != (rx_data_size & 0x3)) {
        printx("spi_controller_rx32_data: rx_data_size is not a multiple of 32-bit!\n");
        return -1;
    }

    if (0 != (((const size_t)spi_command) & 0x3)) {
        printx("spi_controller_rx32_data: command is not 32-bit aligned!\n");
        return -1;
    }

    if (0 != (((const size_t)rx_data) & 0x3)) {
        printx("spi_controller_rx32_data: rx_data is not 32-bit aligned!\n");
        return -1;
    }

    spi_command_32 = (const uint32_t*)(const void*)spi_command;

    // transmit the command (and address)
    spi_regs->DR0.B.DR = reverse_endian(*spi_command_32);

    // read the data from the RX FIFO
    timeout = 0;
    while (read_frames > 0) {
        rxflr.R = spi_regs->RXFLR.R;
        if (rxflr.B.RXTFL > 0) {
            for (n = 0; n < rxflr.R; n++) {
                read_value.u32 = reverse_endian(spi_regs->DR0.B.DR);
                if (0 == skip_read_size) {
                    data = read_value.u8;
                    data_size = 4;
                } else if (1 == skip_read_size) {
                    data = read_value.u8 + 1;
                    data_size = 3;
                    skip_read_size -= 1;
                    //printx("S1");
                } else if (2 == skip_read_size) {
                    data = read_value.u8 + 2;
                    data_size = 2;
                    skip_read_size -= 2;
                    //printx("S2");
                } else if (3 == skip_read_size) {
                    data = read_value.u8 + 3;
                    data_size = 1;
                    skip_read_size -= 3;
                    //printx("S3");
                } else { // if (skip_read_size >= 4)
                    data_size = 0;
                    skip_read_size -= 4;
                }
                //printx("R%u", data_size);

                while (data_size > 0 && rx_data_size > 0) {
                    *rx_data = *data;
                    rx_data++;
                    data++;
                    data_size--;
                    rx_data_size--;
                }

                read_frames--;
                timeout = 0;
            }
        } else {
            timeout++;
            if (timeout > RX_TIMEOUT) {
                rv = -1;
                printx("RX ERR: SR=0x%x, TXTFL=0x%x, RXTFL=0x%x\n", spi_regs->SR.R, spi_regs->TXFLR.R, rxflr.R);
                printx("        CTRLR0=0x%x, SSIENR=0x%x, SER=0x%x\n", spi_regs->CTRLR0.R, spi_regs->SSIENR.R, spi_regs->SER.R);
                printx("        BAUDR=0x%x, TXFTLR=0x%x, RXFTLR=0x%x\n", spi_regs->BAUDR.R, spi_regs->TXFTLR.R, spi_regs->RXFTLR.R);
                //printx("SR! 0x%x\n", sr.R);
                goto DONE;
            }
        }
    }
    //printx("\n");

    rv = 0;
DONE:
    spi_regs->SSIENR.R = (Ssi_SSIENR_t){ .B = { .SSI_EN = 0 }}.R;
    spi_regs->SER.R = (Ssi_SER_t){ .B = { .SER = 0 }}.R;
    return rv;
}
#endif

static int spi_controller_rx_data(volatile Ssi_t * spi_regs, const uint8_t * spi_command, uint32_t spi_command_length, uint8_t * rx_data, uint32_t rx_data_size) {
    int rv;
    Ssi_SR_t sr;
    //Ssi_TXFLR_t txflr;
    Ssi_RXFLR_t rxflr;
    uint32_t timeout;
    uint8_t byte_value;
    //uint32_t n;

    //printx("rx\n");
    // printx("SPI_RX: cmd=%02x, len=%u, rx_data=0x%x, rx_size=%u\n", spi_command[0], spi_command_length, rx_data, rx_data_size);
    // for (n = 1; n < spi_command_length; n++) {
    //     printx(" %02x", spi_command[n]);
    // }
    // if (spi_command_length > 1) {
    //     printx("\n");
    // }

    // transmit the command, address and dummy bits
    while (spi_command_length > 0) {
        //printx("{tx: %02x} ", *spi_command);
        spi_regs->DR0.B.DR = (uint32_t)*spi_command;
        spi_command++;
        spi_command_length--;
    }

    //txflr.R = 0;
    sr.R = spi_regs->SR.R;
    timeout = 0;
    while (rx_data_size > 0) {
        rxflr.R = spi_regs->RXFLR.R;
        if (rxflr.B.RXTFL > 0) {
            byte_value = spi_regs->DR0.B.DR & 0xFF;
            //printx("{rx: %02x} ", byte_value);
            *rx_data = byte_value;
            rx_data++;
            rx_data_size--;
            timeout = 0;
        } else {
            timeout++;
            if (timeout > RX_TIMEOUT) {
                rv = -1;
                printx("RX ERR: SR=0x%x, TXTFL=0x%x, RXTFL=0x%x\n", spi_regs->SR.R, spi_regs->TXFLR.R, rxflr.R);
                printx("        CTRLR0=0x%x, SSIENR=0x%x, SER=0x%x\n", spi_regs->CTRLR0.R, spi_regs->SSIENR.R, spi_regs->SER.R);
                printx("        BAUDR=0x%x, TXFTLR=0x%x, RXFTLR=0x%x\n", spi_regs->BAUDR.R, spi_regs->TXFTLR.R, spi_regs->RXFTLR.R);
                printx("SR! 0x%x\n", sr.R);
                goto DONE;
            }
        }
    }

    rv = 0;
DONE:
    spi_regs->SSIENR.R = (Ssi_SSIENR_t){ .B = { .SSI_EN = 0 }}.R;
    spi_regs->SER.R = (Ssi_SER_t){ .B = { .SER = 0 }}.R;
    return rv;
}

#define MAX_DUMMY_BYTES 8
int spi_controller_command(SPI_CONTROLLER_ID_t id, uint8_t slave_index, SPI_COMMAND_t * command) {
    int rv;
    uint8_t spi_command[1 + 3 + MAX_DUMMY_BYTES] __attribute__ ((aligned (4)));
    uint32_t spi_command_length;
    volatile Ssi_t * spi_regs = get_spi_registers(id);
    uint32_t dfs32_frame_size;
    Ssi_BAUDR_t baud_rate;
    SERVICE_PROCESSOR_BL1_DATA_t * bl1_data = get_service_processor_bl1_data();
    uint32_t slave_en_mask;

#if defined(WRITES_USE_32_BIT_FRAMES) || defined(READS_USE_32_BIT_FRAMES)
    bool use_32bit_frames = false;
#endif

#ifdef WRITES_USE_32_BIT_FRAMES
    uint32_t true_write_size;
#endif

#ifdef READS_USE_32_BIT_FRAMES
    uint32_t true_read_size = 0;
    uint32_t skip_read_size = 0;
    uint32_t read_frames;
#else
    uint32_t n;
#endif

    if (NULL == spi_regs) {
        return -1;
    }
    if (slave_index >= SPI_SSI_NUM_SLAVES) {
        return -1;
    }
    if (command->dummy_bytes > MAX_DUMMY_BYTES) {
        return -1;
    }
    if (command->data_receive) {
        if (0 == command->data_size || command->data_size > 0x10000u) {
            return -1;
        }
    }

    spi_command[0] = command->cmd;
    spi_command_length = 1;
    if (command->include_address) {
        spi_command[spi_command_length + 0] = (command->address >> 16u) & 0xFF;
        spi_command[spi_command_length + 1] = (command->address >> 8u) & 0xFF;
        spi_command[spi_command_length + 2] = (command->address >> 0u) & 0xFF;
        spi_command_length += 3;
    }

    if (0 == command->data_receive) {
        // we are transmitting data
#ifdef WRITES_USE_32_BIT_FRAMES
        true_write_size = spi_command_length + command->data_size;

        if (0 != command->dummy_bytes) {
            printx("spi_controller_command: support for tx with dummy bytes not implemented!\n");
            return -1;
        }
        if (0 == (true_write_size & 0x3) && 4 == spi_command_length) {
            // we will use 32-bit frames
            use_32bit_frames = true;
        } else if (1 == true_write_size) {
            // we will use 8-bit frames
        } else {
            printx("spi_controller_command: tx command_length (%u) and data_size (%u) not supported!\n", spi_command_length, command->data_size);
            return -1;
        }
#endif
    } else {
        // we are receiving data
#ifdef READS_USE_32_BIT_FRAMES
        true_read_size = command->dummy_bytes + command->data_size;
        skip_read_size = command->dummy_bytes;
        if (4 == spi_command_length) {
            // command length is 32-bit, we will handle dummy bytes as part of the read
            true_read_size = (true_read_size + 3) & 0xFFFFFFFC; // round up to the next 4 bytes
            // we will use 32-bit frames
            use_32bit_frames = true;
        } else if (1 == spi_command_length) {
            // command length is 8-bit, we will handle dummy bytes as part of the read
            // we will use 8-bit frames
        } else {
            printx("spi_controller_command: rx command_length (%u) not supported!\n", spi_command_length);
            return -1;
        }
#else
        // force all reads to use 8-bit frames
        for (n = 0; n < command->dummy_bytes; n++) {
            spi_command[spi_command_length] = 0;
            spi_command_length++;
        }
#endif
    }

#if defined(WRITES_USE_32_BIT_FRAMES) || defined(READS_USE_32_BIT_FRAMES)
    if (use_32bit_frames) {
        dfs32_frame_size = SSI_CTRLR0_DFS_32_DFS_32_FRAME_32BITS;
    } else {
        dfs32_frame_size = SSI_CTRLR0_DFS_32_DFS_32_FRAME_08BITS;
    }
#else
    dfs32_frame_size = SSI_CTRLR0_DFS_32_DFS_32_FRAME_08BITS;
#endif

    spi_regs->SSIENR.R = (Ssi_SSIENR_t){ .B = { .SSI_EN = 0 }}.R;
    slave_en_mask = (Ssi_SER_t){ .B = { .SER = (1u << slave_index) & SLAVE_MASK }}.R;

    if (command->data_receive) {
        baud_rate.R = (Ssi_BAUDR_t){ .B = { .SCKDV = bl1_data->spi_controller_rx_baudrate_divider }}.R;
        spi_regs->BAUDR.R = baud_rate.R;
        spi_regs->CTRLR0.R = (Ssi_CTRLR0_t){ .B = { 
                                                // .DFS = 0,
                                                .FRF = SSI_CTRLR0_FRF_FRF_MOTOROLA_SPI,
                                                .SCPH = SCPH_VALUE,
                                                .SCPOL = SCPOL_VALUE,
                                                .TMOD = SSI_CTRLR0_TMOD_TMOD_EEPROM_READ,
                                                .SRL = SSI_CTRLR0_SRL_SRL_NORMAL_MODE,
                                                .DFS_32 = dfs32_frame_size & 0x1Fu,
                                                .SPI_FRF = SSI_CTRLR0_SPI_FRF_SPI_FRF_STD_SPI_FRF,
                                                .SSTE = 0 }}.R;
        
#ifdef READS_USE_32_BIT_FRAMES
        if (use_32bit_frames) {
            read_frames = true_read_size / 4;
            //printx("RX frames: %u\n", read_frames);
            spi_regs->CTRLR1.R = (Ssi_CTRLR1_t){ .B = { .NDF = (uint16_t)(read_frames - 1u) }}.R;
        } else {
            //printx("RX bytes: %u\n", true_read_size);
            spi_regs->CTRLR1.R = (Ssi_CTRLR1_t){ .B = { .NDF = (uint16_t)(true_read_size - 1u) }}.R;
        }
#else
        spi_regs->CTRLR1.R = (Ssi_CTRLR1_t){ .B = { .NDF = (uint16_t)(command->data_size - 1u) }}.R;
#endif

        spi_regs->SSIENR.R = (Ssi_SSIENR_t){ .B = { .SSI_EN = 1 }}.R;
        spi_regs->SER.R = slave_en_mask;

#ifdef READS_USE_32_BIT_FRAMES
        if (use_32bit_frames) {
            rv = spi_controller_rx32_data(spi_regs, spi_command, spi_command_length, read_frames, skip_read_size, command->data_buffer, command->data_size);
        } else {
            rv = spi_controller_rx_data(spi_regs, spi_command, spi_command_length, command->data_buffer, command->data_size);
        }
#else
        rv = spi_controller_rx_data(spi_regs, spi_command, spi_command_length, command->data_buffer, command->data_size);
#endif
    } else {
        baud_rate.R = (Ssi_BAUDR_t){ .B = { .SCKDV = bl1_data->spi_controller_tx_baudrate_divider }}.R;
        spi_regs->BAUDR.R = baud_rate.R;
        spi_regs->CTRLR0.R = (Ssi_CTRLR0_t){ .B = { 
                                                // .DFS = 0,
                                                .FRF = SSI_CTRLR0_FRF_FRF_MOTOROLA_SPI,
                                                .SCPH = SCPH_VALUE,
                                                .SCPOL = SCPOL_VALUE,
#ifdef USE_TX_ONLY_MODE
                                                .TMOD = SSI_CTRLR0_TMOD_TMOD_TX_ONLY,
#else
                                                .TMOD = SSI_CTRLR0_TMOD_TMOD_TX_AND_RX,
#endif
                                                .SRL = SSI_CTRLR0_SRL_SRL_NORMAL_MODE,
                                                .DFS_32 = dfs32_frame_size & 0x1Fu,
                                                .SPI_FRF = SSI_CTRLR0_SPI_FRF_SPI_FRF_STD_SPI_FRF,
                                                .SSTE = 0 }}.R;
        spi_regs->SSIENR.R = (Ssi_SSIENR_t){ .B = { .SSI_EN = 1 }}.R;
#ifdef WRITES_USE_32_BIT_FRAMES
    if (use_32bit_frames) {
        rv = spi_controller_tx32_data(spi_regs, spi_command, spi_command_length, command->data_buffer, command->data_size, slave_en_mask);
    } else {
        rv = spi_controller_tx_data(spi_regs, spi_command, spi_command_length, command->data_buffer, command->data_size, slave_en_mask);
    }
#else
    rv = spi_controller_tx_data(spi_regs, spi_command, spi_command_length, command->data_buffer, command->data_size);
#endif
    }

    // force the CS# to go high at the end of the command
    spi_regs->SSIENR.R = (Ssi_SSIENR_t){ .B = { .SSI_EN = 0 }}.R;
    spi_regs->SER.R = (Ssi_SER_t){ .B = { .SER = 0 }}.R;
    spi_regs->SSIENR.R = (Ssi_SSIENR_t){ .B = { .SSI_EN = 1 }}.R;
    spi_regs->SSIENR.R = (Ssi_SSIENR_t){ .B = { .SSI_EN = 0 }}.R;

    return rv;
}

#pragma GCC pop_options
