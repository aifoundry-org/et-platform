#include "et_cru.h"
#include "serial.h"

#include "printx.h"
#include "spi_controller.h"
#include "synopsys_spi.h"

#define SPIO_SPI0_BASE_ADDR                 0x52021000
#define SPIO_SPI1_BASE_ADDR                 0x54051000

#define SLAVE_MASK ((1u << SPI_SSI_NUM_SLAVES) - 1)
#define TX_TIMEOUT 0x100
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
    SPI_BAUDR_t br;
    volatile SPI_t * spi_regs = get_spi_registers(id);
    if (NULL == spi_regs) {
        return -1;
    }

    printx("SSI_VERSION_ID: 0x%08x\n", spi_regs->SSI_VERSION_ID);

    spi_regs->SSIENR.R = (SPI_SSIENR_t){ .B = { .SSI_EN = 0 }}.R;

    spi_regs->CTRLR0.R = (SPI_CTRLR0_t){ .B = { .DFS = SPI_CTRLR0_FRAME_08BITS,
                                              .FRF = SPI_CTRLR0_FRF_MOTOROLA_SPI,
                                              .SCPH = SPI_CTRLR0_SCPH_MIDDLE,
                                              .SCPOL = SPI_CTRLR0_SCPOL_SCLK_LOW,
                                              .TMOD = SPI_CTRLR0_TMOD_TX_AND_RX,
                                              .SRL = SPI_CTRLR0_SRL_NORMAL_MODE,
                                              .DFS_32 = SPI_CTRLR0_FRAME_08BITS,
                                              .SPI_FRF = SPI_CGTRLR0_SPI_PERF_STD,
                                              .SSTE = 0 }}.R;

    spi_regs->CTRLR1.R = (SPI_CTRLR1_t){ .B = { .NDF = 0 }}.R;

    printx("SSIENR: 0x%x\n", spi_regs->SSIENR.R);
    spi_regs->BAUDR.R = (SPI_BAUDR_t){ .B = { .SCKDV = 4 }}.R;
    br = (SPI_BAUDR_t){ .B = { .SCKDV = 4 }};
    printx("BR: 0x%x, 0x%x\n", spi_regs->BAUDR.R, br.R);

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
    int rv;
    SPI_TXFLR_t txflr;
    uint32_t timeout;

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
}

static int spi_controller_rx_data(volatile SPI_t * spi_regs, const uint8_t * spi_command, uint32_t spi_command_length, uint8_t * rx_data, uint32_t rx_data_size) {
    int rv;
    SPI_SR_t sr;
    SPI_TXFLR_t txflr;
    SPI_RXFLR_t rxflr;
    uint32_t timeout;
    uint32_t dummy_tx_count = rx_data_size;
    uint32_t skip_receive_data = spi_command_length;
    rx_data_size += spi_command_length;

    // transmit the command, address and dummy bits
    while (spi_command_length > 0) {
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
                spi_regs->DRx[0] = 0;
                dummy_tx_count--;
            }
        }
        rxflr.R = spi_regs->RXFLR.R;
        if (rxflr.B.RXTFL > 0) {
            if (skip_receive_data > 0) {
                skip_receive_data--;
            } else {
                *rx_data = spi_regs->DRx[0] & 0xFF;
                rx_data++;
                rx_data_size--;
            }
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

int spi_controller_command(SPI_CONTROLLER_ID_t id, uint8_t slave_index, SPI_COMMAND_t * command) {
    uint8_t spi_command[1 + 3 + 1];
    uint32_t spi_command_length;
    volatile SPI_t * spi_regs = get_spi_registers(id);
    if (NULL == spi_regs) {
        return -1;
    }
    if (slave_index >= SPI_SSI_NUM_SLAVES) {
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
    if (command->include_dummy) {
        spi_command[spi_command_length] = 0;
        spi_command_length++;
    }

    spi_regs->SSIENR.R = (SPI_SSIENR_t){ .B = { .SSI_EN = 0 }}.R;
    spi_regs->SER.R = (SPI_SER_t){ .B = { .SER = (1u << slave_index) & SLAVE_MASK }}.R;

    if (command->data_receive) {
        spi_regs->CTRLR0.R = (SPI_CTRLR0_t){ .B = { .DFS = SPI_CTRLR0_FRAME_08BITS,
                                                .FRF = SPI_CTRLR0_FRF_MOTOROLA_SPI,
                                                .SCPH = SPI_CTRLR0_SCPH_MIDDLE,
                                                .SCPOL = SPI_CTRLR0_SCPOL_SCLK_LOW,
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
                                                .SCPH = SPI_CTRLR0_SCPH_MIDDLE,
                                                .SCPOL = SPI_CTRLR0_SCPOL_SCLK_LOW,
                                                .TMOD = SPI_CTRLR0_TMOD_TX_ONLY,
                                                .SRL = SPI_CTRLR0_SRL_NORMAL_MODE,
                                                .DFS_32 = SPI_CTRLR0_FRAME_08BITS,
                                                .SPI_FRF = SPI_CGTRLR0_SPI_PERF_STD,
                                                .SSTE = 0 }}.R;
        return spi_controller_tx_data(spi_regs, spi_command, spi_command_length, command->data_buffer, command->data_size);
    }
}
