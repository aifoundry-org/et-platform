#ifndef PCIE_DMA_H
#define PCIE_DMA_H

#include <stdint.h>
#include <stdbool.h>

#include "mailbox_id.h"
#include "scatter_gather.h"

typedef enum {
    DMA_CHAN_READ_0 = 0,
    DMA_CHAN_READ_1,
    DMA_CHAN_READ_2,
    DMA_CHAN_READ_3,
    DMA_CHAN_WRITE_0,
    DMA_CHAN_WRITE_1,
    DMA_CHAN_WRITE_2,
    DMA_CHAN_WRITE_3
} dma_chan_t;

typedef struct {
    mbox_message_id_t message_id;
    uint32_t chan;
} dma_run_to_done_message_t;

typedef struct {
    mbox_message_id_t message_id;
    uint32_t chan;
    int32_t status;
} dma_done_message_t;

/* 
 * Configure a DMA engine to issue PCIe memory reads to the x86 host, pulling data to the SoC.
 * 
 * Assumes the host will MMIO the transfer configuration list to memory before this is called.
 */
int dma_configure_read(dma_chan_t chan);

/*
 * Configure a DMA engine to issue PCIe memory writes to the x86 host, pushing data from the SoC.
 * 
 * Assumes the host will MMIO the transfer configuration list to memory before this is called.
 */
int dma_configure_write(dma_chan_t chan);

/*
 * Starts a DMA transfer.
 * 
 * Clients should call dma_configure_*, then dma_start().
 */
int dma_start(dma_chan_t chan);

/* 
 * Checks if @chan is done with a DMA transfer.
 */
bool dma_check_done(dma_chan_t chan);

void dma_clear_done(dma_chan_t chan);

#endif