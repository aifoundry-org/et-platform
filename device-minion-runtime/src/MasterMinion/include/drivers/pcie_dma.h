/***********************************************************************
*
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
************************************************************************/
/*! \file pcie_dma.h
    \brief A C header that defines the DMA Driver's public interfaces
*/
/***********************************************************************/

#ifndef PCIE_DMA_H
#define PCIE_DMA_H

#include <stdint.h>
#include <stdbool.h>

/* mm_rt_svcs */
#include <etsoc/isa/io.h>
#include <etsoc/drivers/pcie/pcie_device.h>

/* etsoc_hal */
#include "hwinc/pcie0_dbi_slv.h"

#define PCIE_DMA_RD_CHANNEL_COUNT   4
#define PCIE_DMA_WRT_CHANNEL_COUNT  4
#define PCIE_DMA_CHANNEL_COUNT      (PCIE_DMA_RD_CHANNEL_COUNT + PCIE_DMA_WRT_CHANNEL_COUNT)


/*! \enum dma_flags_e
    \brief Enum that provides DMA flag to set a specific DMA action.
*/
typedef enum {
    DMA_NORMAL = 0,
    DMA_SOC_NO_BOUNDS_CHECK = 1,
} dma_flags_e;

/*! \enum dma_read_chan_id
    \brief Enum that provides the ID for a DMA read channels
*/
enum dma_read_chan_id {
    DMA_CHAN_ID_READ_0       = 0,
    DMA_CHAN_ID_READ_1       = 1,
    DMA_CHAN_ID_READ_2       = 2,
    DMA_CHAN_ID_READ_3       = 3,
    DMA_CHAN_ID_READ_INVALID = 4
};

/*! \typedef dma_read_chan_id_e
    \brief It that provides the ID for a DMA read channels
           NOTE: It can take value of enum dma_read_chan_id
*/
typedef uint8_t dma_read_chan_id_e;

/*! \enum dma_write_chan_id
    \brief Enum that provides the ID for a DMA write channels
*/
enum  dma_write_chan_id {
    DMA_CHAN_ID_WRITE_0       = 0,
    DMA_CHAN_ID_WRITE_1       = 1,
    DMA_CHAN_ID_WRITE_2       = 2,
    DMA_CHAN_ID_WRITE_3       = 3,
    DMA_CHAN_ID_WRITE_INVALID = 4
};

/*! \typedef dma_write_chan_id_e
    \brief It that provides the ID for a DMA write channels
           NOTE: It can take value of enum dma_write_chan_id
*/
typedef uint8_t dma_write_chan_id_e;

/*! \enum DMA_STATUS_e
    \brief Enum that provides the status of DMA APIs
*/
typedef enum {
    DMA_ERROR_INVALID_XFER_COUNT = -7,
    DMA_ERROR_CHANNEL_NOT_RUNNING = -6,
    DMA_ERROR_INVALID_PARAM = -5,
    DMA_ERROR_CHANNEL_NOT_AVAILABLE = -4,
    DMA_ERROR_INVALID_ADDRESS = -3,
    DMA_ERROR_OUT_OF_BOUNDS = -2,
    DMA_OPERATION_NOT_SUCCESS = -1,
    DMA_OPERATION_SUCCESS = 0
} DMA_STATUS_e;

/*! \fn int8_t dma_configure_read(dma_read_chan_id_e chan)
    \brief This function configures a DMA engine to issue PCIe memory reads to
           the x86 host, pulling data to the SoC.
    \param chan DMA channel ID
    \return Status success or error
*/
int8_t dma_configure_read(dma_read_chan_id_e chan);

/*! \fn int8_t dma_configure_write(dma_write_chan_id_e chan)
    \brief This function configures a DMA engine to issue PCIe memory writes to
           the x86 host, pushing data from the SoC.
    \param chan DMA channel ID
    \return Status success or error
*/
int8_t dma_configure_write(dma_write_chan_id_e chan);

/*! \fn int8_t dma_config_read_add_data_node(uint64_t src_addr, uint64_t dest_addr,
    uint32_t size, dma_read_chan_id_e chan, uint32_t index, bool local_interrupt)
    \brief This function adds a new data node in DMA read transfer list. After adding
           all data nodes user must to add a link node as well.
    \param src_addr Source address
    \param dest_addr Pointer to command buffer
    \param size SQW ID
    \param chan DMA channel ID
    \param index Index of current data node
    \param local_interrupt Enable/Disable DMA local interrupt.
    \return Status success or error
*/
int8_t dma_config_read_add_data_node(uint64_t src_addr, uint64_t dest_addr,
    uint32_t size, dma_read_chan_id_e chan, uint32_t index, bool local_interrupt);

/*! \fn int8_t dma_config_write_add_data_node(uint64_t src_addr, uint64_t dest_addr,
    uint32_t size, dma_write_chan_id_e chan, uint32_t index, dma_flags_e dma_flags,
    bool local_interrupt)
    \brief This function adds a new data node in DMA write transfer list. After adding
           all data nodes user must to add a link node as well.
    \param src_addr Source address
    \param dest_addr Pointer to command buffer
    \param size SQW ID
    \param chan DMA channel ID
    \param index Index of current data node
    \param dma_flags DMA flag to set a specific DMA action.
    \param local_interrupt Enable/Disable DMA local interrupt.
    \return Status success or error
*/
int8_t dma_config_write_add_data_node(uint64_t src_addr, uint64_t dest_addr,
    uint32_t size, dma_write_chan_id_e chan, uint32_t index, dma_flags_e dma_flags,
    bool local_interrupt);

/*! \fn int8_t dma_config_read_add_link_node(dma_read_chan_id_e chan, uint32_t index)
    \brief This function adds a new link node in DMA transfer list.
    \param chan DMA channel ID
    \param index Index of current data node
    \return Status success or error
*/
int8_t dma_config_read_add_link_node(dma_read_chan_id_e chan, uint32_t index);

/*! \fn int8_t dma_config_write_add_link_node(dma_write_chan_id_e chan, uint32_t index)
    \brief This function adds a new link node in DMA transfer list.
    \param chan DMA channel ID
    \param index Index of current data node
    \return Status success or error
*/
int8_t dma_config_write_add_link_node(dma_write_chan_id_e chan, uint32_t index);

/*! \fn int8_t dma_start_read(dma_read_chan_id_e chan)
    \brief This function triggers DMA read for specified channel.
    \param chan DMA channel ID
    \return Status success or error
*/
int8_t dma_start_read(dma_read_chan_id_e chan);

/*! \fn int8_t dma_start_write(dma_write_chan_id_e chan)
    \brief This function triggers DMA write for specified channel.
    \param chan DMA channel ID
    \return Status success or error
*/
int8_t dma_start_write(dma_write_chan_id_e chan);

/*! \fn void dma_clear_read_done(dma_read_chan_id_e chan)
    \brief This function clears the DMA transfer flag for specified channel.
    \param chan DMA channel ID
    \return Status success or error
*/
void dma_clear_read_done(dma_read_chan_id_e chan);

/*! \fn void dma_clear_write_done(dma_write_chan_id_e chan)
    \brief This function clears the DMA transfer flag for specified channel.
    \param chan DMA channel ID
    \return None
*/
void dma_clear_write_done(dma_write_chan_id_e chan);

/*! \fn int8_t dma_clear_read_abort(dma_read_chan_id_e chan)
    \brief This function clears the DMA abort flag for specified read channel.
    \param chan DMA channel ID
    \return Status success or error
*/
int8_t dma_clear_read_abort(dma_read_chan_id_e chan);

/*! \fn int8_t dma_clear_write_abort(dma_write_chan_id_e chan)
    \brief This function clears the DMA abort flag for specified write channel.
    \param chan DMA channel ID
    \return Status success or error
*/
int8_t dma_clear_write_abort(dma_write_chan_id_e chan);

/*! \fn int8_t dma_abort_read(dma_read_chan_id_e chan)
    \brief This function aborts DMA for the specified read channel.
    \param chan DMA channel ID
    \return Status success or error
*/
int8_t dma_abort_read(dma_read_chan_id_e chan);

/*! \fn int8_t dma_abort_write(dma_write_chan_id_e chan)
    \brief This function aborts DMA for the specified write channel.
    \param chan DMA channel ID
    \return Status success or error
*/
int8_t dma_abort_write(dma_write_chan_id_e chan);

/*! \fn static inline uint32_t dma_get_read_int_status(void)
    \brief This function returns status of specified DMA read channel.
    \return Read status
*/
static inline uint32_t dma_get_read_int_status(void)
{
    return ioread32(PCIE0 + PE0_DWC_EP_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_INT_STATUS_OFF_ADDRESS);
}

/*! \fn static inline uint32_t dma_get_write_int_status(void)
    \brief This function returns status of specified DMA write channel.
    \return Write status
*/
static inline uint32_t dma_get_write_int_status(void)
{
    return ioread32(PCIE0 + PE0_DWC_EP_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_INT_STATUS_OFF_ADDRESS);
}

/*! \fn static inline bool dma_check_read_done(dma_read_chan_id_e chan, uint32_t read_int_status)
    \brief This function checks completion status of specified DMA read channel.
    \param chan DMA channel ID
    \param read_int_status Interrupt status of specified DMA channel.
    \return Read done status
*/
static inline bool dma_check_read_done(dma_read_chan_id_e chan, uint32_t read_int_status)
{
    uint32_t done_status =
            PE0_DWC_EP_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_INT_STATUS_OFF_RD_DONE_INT_STATUS_GET(
                read_int_status);
    return (done_status & (1U << chan)) != 0;
}

/*! \fn static inline bool dma_check_write_done(dma_write_chan_id_e chan, uint32_t write_int_status)
    \brief This function checks completion status of specified DMA write channel.
    \param chan DMA channel ID
    \param write_int_status Interrupt status of specified DMA channel.
    \return Write done status
*/
static inline bool dma_check_write_done(dma_write_chan_id_e chan, uint32_t write_int_status)
{
    uint32_t done_status =
            PE0_DWC_EP_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_INT_STATUS_OFF_WR_DONE_INT_STATUS_GET(
                write_int_status);
    return (done_status & (1U << chan)) != 0;
}

/*! \fn static inline bool dma_check_read_abort(dma_read_chan_id_e chan, uint32_t read_int_status)
    \brief This function checks abort status of specified DMA read channel.
    \param chan DMA channel ID
    \param read_int_status Interrupt status of specified DMA channel.
    \return Read abort status
*/
static inline bool dma_check_read_abort(dma_read_chan_id_e chan, uint32_t read_int_status)
{
    uint32_t abort_status =
            PE0_DWC_EP_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_INT_STATUS_OFF_RD_ABORT_INT_STATUS_GET(
                read_int_status);
    return (abort_status & (1U << chan)) != 0;
}

/*! \fn static inline bool dma_check_write_abort(dma_write_chan_id_e chan, uint32_t write_int_status)
    \brief This function checks abort status of specified DMA write channel.
    \param chan DMA channel ID
    \param write_int_status Interrupt status of specified DMA channel.
    \return Write abort status
*/
static inline bool dma_check_write_abort(dma_write_chan_id_e chan, uint32_t write_int_status)
{
    uint32_t abort_status =
            PE0_DWC_EP_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_INT_STATUS_OFF_WR_ABORT_INT_STATUS_GET(
                write_int_status);
    return (abort_status & (1U << chan )) != 0;
}

#endif
