#ifndef PCIE_DMA_H
#define PCIE_DMA_H

#include <stdint.h>
#include <stdbool.h>
#include "io.h"
#include "etsoc_hal/inc/DWC_pcie_dbi_cpcie_usp_4x8.h"
#include "pcie_device.h"

#define PCIE_DMA_RD_CHANNEL_COUNT   4
#define PCIE_DMA_WRT_CHANNEL_COUNT  4
#define PCIE_DMA_CHANNEL_COUNT      (PCIE_DMA_RD_CHANNEL_COUNT + PCIE_DMA_WRT_CHANNEL_COUNT)


/*! \enum dma_read_flag_e
    \brief Enum that provides DMA flag to set a specific DMA action.
*/
typedef enum {
    DMA_NORMAL = 0,
    DMA_SOC_NO_BOUNDS_CHECK = 1,
} dma_flags_e;

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
    DMA_ERROR_CHANNEL_NOT_RUNNING = -6,
    DMA_ERROR_INVALID_PARAM = -5,
    DMA_ERROR_CHANNEL_NOT_AVAILABLE = -4,
    DMA_ERROR_INVALID_ADDRESS = -3,
    DMA_ERROR_OUT_OF_BOUNDS = -2,
    DMA_OPERATION_NOT_SUCCESS = -1,
    DMA_OPERATION_SUCCESS = 0
} DMA_STATUS_e;

/// @brief Triggers a DMA read/write transaction based on channel ID.
int dma_trigger_transfer(uint64_t src_addr, uint64_t dest_addr,
    uint64_t size, dma_chan_id_e chan, dma_flags_e dma_flags);

/// @brief Configure a DMA engine to issue PCIe memory reads to the x86 host, pulling data to the SoC.
int dma_configure_read(dma_chan_id_e chan);

/// @brief Configure a DMA engine to issue PCIe memory writes to the x86 host, pushing data from the SoC.
int dma_configure_write(dma_chan_id_e chan);

/// @brief Starts a DMA transfer for the specified channel.
int dma_start(dma_chan_id_e chan);

/// @brief Clears the DMA transfer flag for the specified channel.
void dma_clear_done(dma_chan_id_e chan);

/// @brief Clears the DMA abort flag for the specified read channel.
DMA_STATUS_e dma_clear_read_abort(dma_chan_id_e chan);

/// @brief Clears the DMA abort flag for the specified write channel.
DMA_STATUS_e dma_clear_write_abort(dma_chan_id_e chan);

/// @brief Abort DMA for the specified read channel.
DMA_STATUS_e dma_abort_read(dma_chan_id_e chan);

/// @brief Abort DMA for the specified write channel.
DMA_STATUS_e dma_abort_write(dma_chan_id_e chan);

static inline uint32_t dma_get_read_int_status(void)
{
    return ioread32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_INT_STATUS_OFF_ADDRESS);
}

static inline uint32_t dma_get_write_int_status(void)
{
    return ioread32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_INT_STATUS_OFF_ADDRESS);
}

static inline bool dma_check_read_done(dma_chan_id_e chan, uint32_t read_int_status)
{
    uint32_t done_status =
            PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_INT_STATUS_OFF_RD_DONE_INT_STATUS_GET(
                read_int_status);
    return (done_status & (1U << chan)) != 0;
}

static inline bool dma_check_write_done(dma_chan_id_e chan, uint32_t write_int_status)
{
    uint32_t done_status =
            PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_INT_STATUS_OFF_WR_DONE_INT_STATUS_GET(
                write_int_status);
    return (done_status & (1U << chan)) != 0;
}

static inline bool dma_check_read_abort(dma_chan_id_e chan, uint32_t read_int_status)
{
    uint32_t abort_status =
            PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_INT_STATUS_OFF_RD_ABORT_INT_STATUS_GET(
                read_int_status);
    return (abort_status & (1U << chan)) != 0;
}

static inline bool dma_check_write_abort(dma_chan_id_e chan, uint32_t write_int_status)
{
    uint32_t abort_status =
            PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_INT_STATUS_OFF_WR_ABORT_INT_STATUS_GET(
                write_int_status);
    return (abort_status & (1U << chan )) != 0;
}

#endif
