#ifndef PCIE_DMA_H
#define PCIE_DMA_H

#include <stdint.h>
#include <stdbool.h>
#include <esperanto/device-api/device_api_rpc_types_privileged.h>

#define PCIE_DMA_RD_CHANNEL_COUNT   4
#define PCIE_DMA_WRT_CHANNEL_COUNT  4
#define PCIE_DMA_CHANNEL_COUNT      (PCIE_DMA_RD_CHANNEL_COUNT+PCIE_DMA_WRT_CHANNEL_COUNT)


typedef enum { DMA_HOST_TO_DEVICE = 0, DMA_DEVICE_TO_HOST } DMA_TYPE_e;

typedef enum {
    DMA_ERROR_INVALID_PARAM = -5,
    DMA_ERROR_CHANNEL_NOT_AVAILABLE = -4,
    DMA_ERROR_INVALID_ADDRESS = -3,
    DMA_ERROR_OUT_OF_BOUNDS = -2,
    DMA_OPERATION_NOT_SUCCESS = -1,
    DMA_OPERATION_SUCCESS = 0
} DMA_STATUS_e;

/// @brief This function is used to trigger DMA read/write from the device.
DMA_STATUS_e dma_trigger_transfer(DMA_TYPE_e type, uint64_t src_addr, uint64_t dest_addr,
                                  uint64_t size);

DMA_STATUS_e dma_trigger_transfer2(DMA_TYPE_e type, uint64_t src_addr, uint64_t dest_addr,
                                  uint64_t size, et_dma_chan_id_e chan);


/// @brief Configure a DMA engine to issue PCIe memory reads to the x86 host, pulling data to the SoC.
int dma_configure_read(et_dma_chan_id_e chan);

/// @brief Configure a DMA engine to issue PCIe memory writes to the x86 host, pushing data from the SoC.
int dma_configure_write(et_dma_chan_id_e chan);

/// @brief Starts a DMA transfer for the specified channel.
int dma_start(et_dma_chan_id_e chan);

/// @brief Checks if channel is done with a DMA transfer.
bool dma_check_done(et_dma_chan_id_e chan);

/// @brief Clears the DMA transfer flag for the specified channel.
void dma_clear_done(et_dma_chan_id_e chan);

DMA_STATUS_e dma_chan_find_idle(DMA_TYPE_e type, et_dma_chan_id_e *chan);

#endif
