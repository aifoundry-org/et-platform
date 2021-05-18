#include "device-common/atomic.h"
#include "drivers/pcie_dma.h"
#include "drivers/pcie_dma_ll.h"
#include "layout.h"
#include "services/log.h"

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

//Cycle Bit
#define CTRL_CB 0x01
//Toggle Cycle Bit
#define CTRL_TCB 0x02
//Load Link Pointer
#define CTRL_LLP 0x04
//Local Interrupt Enable
#define CTRL_LIE 0x08
//Remote Interrupt Enable
#define CTRL_RIE 0x10

/* DMA registers are replicated per Channel. Using macro to calculate Strides between Channels */
#define WR_DMA_REG_CHANNEL_STRIDE                                                \
    (PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_1_ADDRESS - \
     PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_0_ADDRESS)

#define RD_DMA_REG_CHANNEL_STRIDE                                                \
    (PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_1_ADDRESS - \
     PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_0_ADDRESS)

/* Always save 1 element for terminator */
#define MAX_TRANSFER_LIST_SIZE (DMA_LL_SIZE / sizeof(transfer_list_elem_t) - 1)

struct dma_mem_region {
    uint64_t begin;
    uint64_t end;
};

static volatile transfer_list_elem_t *transfer_lists[] = {
    (volatile transfer_list_elem_t *)DMA_CHAN_READ_0_LL_BASE,
    (volatile transfer_list_elem_t *)DMA_CHAN_READ_1_LL_BASE,
    (volatile transfer_list_elem_t *)DMA_CHAN_READ_2_LL_BASE,
    (volatile transfer_list_elem_t *)DMA_CHAN_READ_3_LL_BASE,
    (volatile transfer_list_elem_t *)DMA_CHAN_WRITE_0_LL_BASE,
    (volatile transfer_list_elem_t *)DMA_CHAN_WRITE_1_LL_BASE,
    (volatile transfer_list_elem_t *)DMA_CHAN_WRITE_2_LL_BASE,
    (volatile transfer_list_elem_t *)DMA_CHAN_WRITE_3_LL_BASE
};

static const struct dma_mem_region valid_dma_targets[] = {
    /* L3, but beginning reserved for minion stacks, end reserved for DMA config */
    { .begin = HOST_MANAGED_DRAM_START, .end = (HOST_MANAGED_DRAM_END - 1) },
    /* Shire-cache scratch pads. Limit to first 4MB * 33 shires */
    { .begin = 0x80000000, .end = 0x883FFFFF }
};

/*
 * Writes one data element of a DMA transfer list. This structure is used
 * to configure the DMA engine.
 */
static inline void write_xfer_list_data(dma_chan_id_e id, uint32_t i, uint64_t sar,
                                        uint64_t dar, uint32_t size, bool lie)
{
    // Calculate the offset to write
    volatile transfer_list_elem_t *transfer = &transfer_lists[id][i];

    uint32_t ctrl_dword = CTRL_CB;
    if (lie)
        ctrl_dword |= CTRL_LIE;

    atomic_store_global_32(&transfer->data.ctrl.R, ctrl_dword);
    atomic_store_global_32(&transfer->data.size, size);
    atomic_store_global_64(&transfer->data.sar, sar);
    atomic_store_global_64(&transfer->data.dar, dar);
}

/*
 * Writes one data element of a DMA transfer list. This structure is used
 * to configure the DMA engine.
 */
static inline void write_xfer_list_link(dma_chan_id_e id, uint32_t i, uint64_t ptr)
{
    // Calculate the offset to write
    volatile transfer_list_elem_t *transfer = &transfer_lists[id][i];

    atomic_store_global_32(&transfer->link.ctrl.R, CTRL_TCB | CTRL_LLP);
    atomic_store_global_64(&transfer->link.ptr, ptr);
}

static inline DMA_STATUS_e dma_config_buff(uint64_t src_addr, uint64_t dest_addr, uint32_t size,
                                           dma_chan_id_e id)
{
    // Write a single-data-element xfer list
    write_xfer_list_data(id, 0, src_addr, dest_addr, size, true /* interrupt */);
    write_xfer_list_link(id, 1, (uint64_t)(transfer_lists[id]) /* circular link */);

    // TODO: Need to implement proper scatter-gather

    return DMA_OPERATION_SUCCESS;
}

static inline DMA_STATUS_e dma_bounds_check(uint64_t soc_addr, uint64_t size)
{
    uint64_t end_addr = soc_addr + size - 1U;

    /* Check for uint64_t overflow */
    if (end_addr >= soc_addr)
    {
        for (uint16_t i = 0; i < sizeof(valid_dma_targets) / sizeof(*valid_dma_targets); ++i)
        {
            if ((soc_addr >= valid_dma_targets[i].begin) && (end_addr <= valid_dma_targets[i].end))
            {
                return DMA_OPERATION_SUCCESS;
            }
        }
    }

    return DMA_ERROR_INVALID_ADDRESS;
}

/************************************************************************
*
*   FUNCTION
*
*       dma_config_add_data_node
*
*   DESCRIPTION
*
*       This function adds a new data node in DMA transfer list. After adding
*       all data nodes user must to add a link node as well.
*
*   INPUTS
*
*       src_addr        Source address
*       dest_addr       Destination address
*       xfer_count      Number of transfer nodes in command.
*       size            Size of DMA transaction
*       chan            DMA channel ID
*       index           Index of current data node
*       dma_flags       DMA flag to set a specific DMA action.
*       local_interrupt Enable/Disable DMA local interrupt.
*
*   OUTPUTS
*
*       DMA_STATUS_e     status success or error
*
***********************************************************************/
DMA_STATUS_e dma_config_add_data_node(uint64_t src_addr, uint64_t dest_addr,
    uint32_t size, dma_chan_id_e chan, uint32_t index, dma_flags_e dma_flags,
    bool local_interrupt)
{
    /* Reads data from Host to Device memory */
    int status = DMA_OPERATION_SUCCESS;

    /* Validate the params */
    if (!(chan >= DMA_CHAN_ID_READ_0 && chan <= DMA_CHAN_ID_WRITE_3))
    {
        status = DMA_ERROR_INVALID_PARAM;
    }
    else if(!(dma_flags & DMA_SOC_NO_BOUNDS_CHECK))
    {
        if (chan <= DMA_CHAN_ID_READ_3)
        {
            /* Validate the bounds. Read: source is on host, dest is on SoC */
            status = dma_bounds_check(dest_addr, size);
        }
        else
        {
            /* Validate the bounds. Write: source is on SoC, dest is on host */
            status = dma_bounds_check(src_addr, size);
        }
    }

    if (status == DMA_OPERATION_SUCCESS)
    {
        /* Add a data node in xfer list */
        write_xfer_list_data(chan, index, src_addr, dest_addr, size, local_interrupt);
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       dma_config_add_link_node
*
*   DESCRIPTION
*
*       This function adds a new link node in DMA transfer list.
*
*   INPUTS
*
*       chan            DMA channel ID
*       index           Index of current data node
*
*   OUTPUTS
*
*       DMA_STATUS_e     status success or error
*
***********************************************************************/
DMA_STATUS_e dma_config_add_link_node(dma_chan_id_e chan, uint32_t index)
{
    /* Add a link node in xfer list */
    write_xfer_list_link(chan, index, (uint64_t)(transfer_lists[chan]) /* circular link */);

    return DMA_OPERATION_SUCCESS;
}

int dma_configure_read(dma_chan_id_e chan)
{
    uint32_t read_engine_en;
    uint32_t transfer_list_low_addr;
    uint32_t transfer_list_high_addr;

    if (chan > DMA_CHAN_ID_READ_3)
    {
        Log_Write(LOG_LEVEL_CRITICAL, "Invalid DMA read channel %d\r\n", chan);
        return -EINVAL;
    }

    read_engine_en =
        ioread32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_ENGINE_EN_OFF_ADDRESS);

    /* Global enable for DMA reads */
    read_engine_en =
        PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_ENGINE_EN_OFF_DMA_READ_ENGINE_MODIFY(
            read_engine_en, 1);

    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_ENGINE_EN_OFF_ADDRESS,
              read_engine_en);

    transfer_list_low_addr = (uint32_t)((uint64_t)transfer_lists[chan] & 0xFFFFFFFF);
    transfer_list_high_addr = (uint32_t)((uint64_t)transfer_lists[chan] >> 32);

    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_LOW_OFF_RDCH_0_ADDRESS
                    + (chan * RD_DMA_REG_CHANNEL_STRIDE), transfer_list_low_addr);
    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_HIGH_OFF_RDCH_0_ADDRESS
                    + (chan * RD_DMA_REG_CHANNEL_STRIDE), transfer_list_high_addr);

    return 0;
}

int dma_configure_write(dma_chan_id_e chan)
{
    uint32_t wr_chan_num;
    uint32_t write_engine_en;
    uint32_t transfer_list_low_addr;
    uint32_t transfer_list_high_addr;

    if (chan < DMA_CHAN_ID_WRITE_0 || chan > DMA_CHAN_ID_WRITE_3)
    {
        Log_Write(LOG_LEVEL_CRITICAL, "Invalid DMA write channel %d\r\n", chan);
        return -EINVAL;
    }

    write_engine_en =
        ioread32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_ENGINE_EN_OFF_ADDRESS);

    /* Global enable for DMA writes */
    write_engine_en =
        PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_ENGINE_EN_OFF_DMA_WRITE_ENGINE_MODIFY(
            write_engine_en, 1);

    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_ENGINE_EN_OFF_ADDRESS,
              write_engine_en);

    transfer_list_low_addr = (uint32_t)((uint64_t)transfer_lists[chan] & 0xFFFFFFFF);
    transfer_list_high_addr = (uint32_t)((uint64_t)transfer_lists[chan] >> 32);

    wr_chan_num = chan - DMA_CHAN_ID_WRITE_0;
    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_LOW_OFF_WRCH_0_ADDRESS
                    + (wr_chan_num * WR_DMA_REG_CHANNEL_STRIDE), transfer_list_low_addr);
    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_HIGH_OFF_WRCH_0_ADDRESS
                    + (wr_chan_num * WR_DMA_REG_CHANNEL_STRIDE), transfer_list_high_addr);

    return 0;
}

int dma_start(dma_chan_id_e chan)
{
    uint32_t control1;
    uint32_t wr_chan_num;

    if (chan <= DMA_CHAN_ID_READ_3)
    {
        /* Use linked list mode. All of the control 1 registers have the same layout. */
        control1 =
            PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_0_CCS_SET(1) |
            PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_0_LLE_SET(1) |
            PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_0_DMA_MEM_TYPE_SET(1);

        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_0_ADDRESS
                        + (chan * RD_DMA_REG_CHANNEL_STRIDE), control1);

        /* Ring the doorbell. This prefetches the first transfer list element and enables
        DMA to begin very quickly once started. */
        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_DOORBELL_OFF_ADDRESS,
                  chan);
    }
    else if (chan >= DMA_CHAN_ID_WRITE_0 && chan <= DMA_CHAN_ID_WRITE_3)
    {
        wr_chan_num = chan - DMA_CHAN_ID_WRITE_0;

        /* Use linked list mode. All of the control 1 registers have the same layout. */
        control1 =
            PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_0_CCS_SET(1) |
            PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_0_LLE_SET(1) |
            PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_0_DMA_MEM_TYPE_SET(1);

        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_0_ADDRESS
                        + (wr_chan_num * WR_DMA_REG_CHANNEL_STRIDE), control1);

        /* Ring the doorbell. This prefetches the first transfer list element and enables
        DMA to begin very quickly once started. */
        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_DOORBELL_OFF_ADDRESS,
                  wr_chan_num);
    } else {
        Log_Write(LOG_LEVEL_CRITICAL, "Invalid DMA channel %d\r\n", chan);
        return -EINVAL;
    }

    return 0;
}

void dma_clear_done(dma_chan_id_e chan)
{
    if (chan <= DMA_CHAN_ID_READ_3)
    {
        iowrite32(
            PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_INT_CLEAR_OFF_ADDRESS,
            PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_INT_CLEAR_OFF_RD_DONE_INT_CLEAR_SET(
                (1U << chan) & 0xF));
    }
    else if (chan >= DMA_CHAN_ID_WRITE_0 && chan <= DMA_CHAN_ID_WRITE_3)
    {
        iowrite32(
            PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_INT_CLEAR_OFF_ADDRESS,
            PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_INT_CLEAR_OFF_WR_DONE_INT_CLEAR_SET(
                (1U << (chan - DMA_CHAN_ID_WRITE_0)) & 0xF));
    }
    else
    {
        Log_Write(LOG_LEVEL_CRITICAL, "Invalid DMA channel %d\r\n", chan);
    }
}

DMA_STATUS_e dma_clear_read_abort(dma_chan_id_e chan)
{
    DMA_STATUS_e status = DMA_OPERATION_SUCCESS;

    if (chan <= DMA_CHAN_ID_READ_3)
    {
        iowrite32(
            PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_INT_CLEAR_OFF_ADDRESS,
            PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_INT_CLEAR_OFF_RD_ABORT_INT_CLEAR_SET(
                (1U << chan) & 0xF));
    }
    else
    {
        Log_Write(LOG_LEVEL_CRITICAL, "Invalid DMA read channel %d\r\n", chan);
        status = DMA_ERROR_CHANNEL_NOT_AVAILABLE;
    }

    return status;
}

DMA_STATUS_e dma_clear_write_abort(dma_chan_id_e chan)
{
    DMA_STATUS_e status = DMA_OPERATION_SUCCESS;

    if (chan >= DMA_CHAN_ID_WRITE_0 && chan <= DMA_CHAN_ID_WRITE_3)
    {
        iowrite32(
            PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_INT_CLEAR_OFF_ADDRESS,
            PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_INT_CLEAR_OFF_WR_ABORT_INT_CLEAR_SET(
                (1U << (chan - DMA_CHAN_ID_WRITE_0)) & 0xF));
    }
    else
    {
        Log_Write(LOG_LEVEL_CRITICAL, "Invalid DMA write channel %d\r\n", chan);
        status = DMA_ERROR_CHANNEL_NOT_AVAILABLE;
    }

    return status;
}

DMA_STATUS_e dma_abort_read(dma_chan_id_e chan)
{
    /* Verify the channel ID */
    if ((chan < DMA_CHAN_ID_READ_0) || (chan > DMA_CHAN_ID_READ_3))
    {
        Log_Write(LOG_LEVEL_ERROR, "Invalid DMA read channel %d\r\n", chan);
        return DMA_ERROR_CHANNEL_NOT_AVAILABLE;
    }

    /* Get the DMA error status DMA of respective channel */
    uint32_t dma_error_stat_low   = ioread32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_DMA_CAP_DMA_READ_ERR_STATUS_LOW_OFF_ADDRESS);
    uint32_t dma_error_stat_high  = ioread32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_DMA_CAP_DMA_READ_ERR_STATUS_HIGH_OFF_ADDRESS);
    Log_Write(LOG_LEVEL_ERROR, "DMA Read channel (%d) Error Status: %d(LOW) %d(HIGH)\r\n", chan, dma_error_stat_low, dma_error_stat_high);

    /* Get the control1 register of the respective channel */
    uint32_t control1 =
             ioread32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_0_ADDRESS
                            + (chan * RD_DMA_REG_CHANNEL_STRIDE));

    /* If the respective channel is running, abort it */
    if (1 == PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_0_CS_GET(control1))
    {
        uint32_t dma_abort = (uint32_t)chan |
            PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_DOORBELL_OFF_RD_STOP_FIELD_MASK;
        iowrite32(
            PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_DOORBELL_OFF_ADDRESS, dma_abort);
    }

    return DMA_OPERATION_SUCCESS;
}

DMA_STATUS_e dma_abort_write(dma_chan_id_e chan)
{
    /* Verify the channel ID */
    if ((chan < DMA_CHAN_ID_WRITE_0) || (chan > DMA_CHAN_ID_WRITE_3))
    {
        Log_Write(LOG_LEVEL_ERROR, "Invalid DMA write channel %d\r\n", chan);
        return DMA_ERROR_CHANNEL_NOT_AVAILABLE;
    }

    uint32_t wr_chan_num = chan - DMA_CHAN_ID_WRITE_0;

    /* Get the DMA error status DMA of respective channel */
    uint32_t dma_error_stat  = ioread32(PCIE0 + PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_DMA_CAP_DMA_WRITE_ERR_STATUS_OFF_ADDRESS);
    Log_Write(LOG_LEVEL_ERROR, "DMA Write channel (%d) Error Status: %d \r\n", chan, dma_error_stat);

    /* Get the control1 register of the respective channel */
    uint32_t control1 =
            ioread32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_0_ADDRESS
                           + (wr_chan_num * WR_DMA_REG_CHANNEL_STRIDE));

    /* If the respective channel is running, abort it */
    if (1 == PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_0_CS_GET(control1))
    {
        uint32_t dma_abort = (uint32_t)(chan - DMA_CHAN_ID_WRITE_0) |
            PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_DOORBELL_OFF_WR_STOP_FIELD_MASK;
        iowrite32(
            PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_DOORBELL_OFF_ADDRESS, dma_abort);
    }

    return DMA_OPERATION_SUCCESS;
}
