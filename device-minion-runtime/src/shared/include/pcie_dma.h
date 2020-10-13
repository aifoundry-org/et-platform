#ifndef PCIE_DMA_H
#define PCIE_DMA_H

#include <stdint.h>
#include <stdbool.h>
#include <esperanto/device-api/device_api_rpc_types_privileged.h>

/*
 * Configure a DMA engine to issue PCIe memory reads to the x86 host, pulling data to the SoC.
 *
 * Assumes the host will MMIO the transfer configuration list to memory before this is called.
 */
int dma_configure_read(et_dma_chan_id_e chan);

/*
 * Configure a DMA engine to issue PCIe memory writes to the x86 host, pushing data from the SoC.
 *
 * Assumes the host will MMIO the transfer configuration list to memory before this is called.
 */
int dma_configure_write(et_dma_chan_id_e chan);

/*
 * Starts a DMA transfer.
 *
 * Clients should call dma_configure_*, then dma_start().
 */
int dma_start(et_dma_chan_id_e chan);

/*
 * Checks if @chan is done with a DMA transfer.
 */
bool dma_check_done(et_dma_chan_id_e chan);

void dma_clear_done(et_dma_chan_id_e chan);

#endif
