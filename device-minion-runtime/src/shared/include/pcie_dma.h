#ifndef PCIE_DMA_H
#define PCIE_DMA_H

#include <stdint.h>
#include <stdbool.h>

#define PCIE_DMA_RD_CHANNEL_COUNT   4
#define PCIE_DMA_WRT_CHANNEL_COUNT  4
#define PCIE_DMA_CHANNEL_COUNT      (PCIE_DMA_RD_CHANNEL_COUNT + PCIE_DMA_WRT_CHANNEL_COUNT)

/*! \enum dma_chan_id_e
    \brief Enum that provides the ID for a DMA channel
*/
typedef enum {
    DMA_CHAN_ID_READ_0 = 0,
    DMA_CHAN_ID_READ_1 = 1,
    DMA_CHAN_ID_READ_2 = 2,
    DMA_CHAN_ID_READ_3 = 3,
    DMA_CHAN_ID_WRITE_0 = 4,
    DMA_CHAN_ID_WRITE_1 = 5,
    DMA_CHAN_ID_WRITE_2 = 6,
    DMA_CHAN_ID_WRITE_3 = 7
} dma_chan_id_e;

/*! \enum DMA_STATUS_e
    \brief Enum that provides the status of DMA APIs
*/
typedef enum {
    DMA_ERROR_INVALID_PARAM = -5,
    DMA_ERROR_CHANNEL_NOT_AVAILABLE = -4,
    DMA_ERROR_INVALID_ADDRESS = -3,
    DMA_ERROR_OUT_OF_BOUNDS = -2,
    DMA_OPERATION_NOT_SUCCESS = -1,
    DMA_OPERATION_SUCCESS = 0
} DMA_STATUS_e;

/// @brief Triggers a DMA read/write transaction based on channel ID.
int dma_trigger_transfer(uint64_t src_addr, uint64_t dest_addr,
    uint64_t size, dma_chan_id_e chan);

/// @brief Configure a DMA engine to issue PCIe memory reads to the x86 host, pulling data to the SoC.
int dma_configure_read(dma_chan_id_e chan);

/// @brief Configure a DMA engine to issue PCIe memory writes to the x86 host, pushing data from the SoC.
int dma_configure_write(dma_chan_id_e chan);

/// @brief Starts a DMA transfer for the specified channel.
int dma_start(dma_chan_id_e chan);

/// @brief Checks if channel is done with a DMA transfer.
bool dma_check_done(dma_chan_id_e chan);

/// @brief Clears the DMA transfer flag for the specified channel.
void dma_clear_done(dma_chan_id_e chan);

#endif
