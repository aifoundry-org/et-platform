#include "etsoc_memory.h"
#include "drivers/pcie_dma.h"
#include "drivers/pcie_dma_ll.h"
#include "layout.h"
#include "services/log.h"
#include "services/trace.h"

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

/* Cycle Bit */
#define CTRL_CB 0x01
/* Toggle Cycle Bit */
#define CTRL_TCB 0x02
/* Load Link Pointer */
#define CTRL_LLP 0x04
/* Local Interrupt Enable */
#define CTRL_LIE 0x08
/* Remote Interrupt Enable */
#define CTRL_RIE 0x10

/*! \def WR_DMA_REG_CHANNEL_STRIDE
    \brief DMA registers are replicated per Channel. This macro to calculates
           Strides between write Channels
*/
#define WR_DMA_REG_CHANNEL_STRIDE                                                \
    (PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_1_ADDRESS - \
     PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_0_ADDRESS)

/*! \def RD_DMA_REG_CHANNEL_STRIDE
    \brief DMA registers are replicated per Channel. THis macro to calculates
           Strides between read Channels
*/
#define RD_DMA_REG_CHANNEL_STRIDE                                                \
    (PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_1_ADDRESS - \
     PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_0_ADDRESS)

/*! \def DMA_CHAN_GET_LL_BASE
    \brief It returns DMA link list base address for specified channel.
*/
#define DMA_CHAN_GET_LL_BASE(chan) (DMA_CHAN_READ_0_LL_BASE + (chan * DMA_LL_SIZE))

/*! \struct dma_mem_region
    \brief Structure for a memory region, used to verify DMA bounds check.
*/
struct dma_mem_region {
    uint64_t begin;
    uint64_t end;
};

/*!
    Valid DMA memory range upon which DMA can perform Read/Write.
*/
static const struct dma_mem_region valid_dma_targets[] = {
    /* L3, but beginning reserved for minion stacks, end reserved for DMA config */
    { .begin = HOST_MANAGED_DRAM_START, .end = (HOST_MANAGED_DRAM_END - 1) },
    /* Shire-cache scratch pads. Limit to first 4MB * 33 shires */
    { .begin = 0x80000000, .end = 0x883FFFFF }
};

/************************************************************************
*
*   FUNCTION
*
*       write_xfer_list_data
*
*   DESCRIPTION
*
*       Writes one data element of a DMA transfer list. This structure is used
*       to configure the DMA engine.
*
*   INPUTS
*
*       chan       DMA channel ID
*       index      Index of current data node
*
*   OUTPUTS
*
*       int8_t     Status success or error
*
***********************************************************************/
static inline void write_xfer_list_data(dma_chan_id_e id, uint32_t index, uint64_t sar,
                                        uint64_t dar, uint32_t size, bool lie)
{
    transfer_list_elem_t *transfer = (transfer_list_elem_t*)DMA_CHAN_GET_LL_BASE(id);

    uint32_t ctrl_dword = CTRL_CB;
    if (lie)
        ctrl_dword |= CTRL_LIE;

    /* Populate DMA list data node's data. */
    transfer[index].data.ctrl.R = ctrl_dword;
    transfer[index].data.size = size;
    transfer[index].data.sar = sar;
    transfer[index].data.dar = dar;
}

/************************************************************************
*
*   FUNCTION
*
*       write_xfer_list_link
*
*   DESCRIPTION
*
*       Writes one data element of a DMA transfer list. This structure is used
*       to configure the DMA engine.
*
*   INPUTS
*
*       chan       DMA channel ID
*       index      Index of current link node
*
*   OUTPUTS
*
*       int8_t     Status success or error
*
***********************************************************************/
static inline void write_xfer_list_link(dma_chan_id_e id, uint32_t index, uint64_t ptr)
{
    transfer_list_elem_t *transfer = (transfer_list_elem_t*)DMA_CHAN_GET_LL_BASE(id);

    /* Populate DMA list link node's data. */
    transfer[index].link.ctrl.R = CTRL_TCB | CTRL_LLP;
    transfer[index].link.ptr = ptr;

    /* This is last element for a single DMA list transfer. Evict whole list here.
       Total number of elements is calculated by index of this link element,
       starting from zero. */
    ETSOC_MEM_EVICT(transfer, sizeof(transfer_list_elem_t)*(index+1), to_L3)
}

/************************************************************************
*
*   FUNCTION
*
*       dma_bounds_check
*
*   DESCRIPTION
*
*       This function check bounds of given address and data size.
*
*   INPUTS
*
*       soc_addr     Address on SoC memory.
*       size         Size of data block.
*
*   OUTPUTS
*
*       int8_t     Status success or error
*
***********************************************************************/
static inline int8_t dma_bounds_check(uint64_t soc_addr, uint64_t size)
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
*       int8_t     status success or error
*
***********************************************************************/
int8_t dma_config_add_data_node(uint64_t src_addr, uint64_t dest_addr,
    uint32_t size, dma_chan_id_e chan, uint32_t index, dma_flags_e dma_flags,
    bool local_interrupt)
{
    /* Reads data from Host to Device memory */
    int8_t status = DMA_OPERATION_SUCCESS;

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
*       int8_t     status success or error
*
***********************************************************************/
int8_t dma_config_add_link_node(dma_chan_id_e chan, uint32_t index)
{
    /* Add a link node in xfer list */
    write_xfer_list_link(chan, index, (uint64_t)(DMA_CHAN_GET_LL_BASE(chan)) /* circular link */);

    return DMA_OPERATION_SUCCESS;
}

/************************************************************************
*
*   FUNCTION
*
*       dma_configure_read
*
*   DESCRIPTION
*
*       This function configures a DMA engine to issue PCIe memory reads to
*       the x86 host, pulling data to the SoC.
*
*   INPUTS
*
*       chan    DMA channel ID
*
*   OUTPUTS
*
*       int8_t     Status success or error
*
***********************************************************************/
int8_t dma_configure_read(dma_chan_id_e chan)
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

    transfer_list_low_addr = (uint32_t)((uint64_t)DMA_CHAN_GET_LL_BASE(chan) & 0xFFFFFFFF);
    transfer_list_high_addr = (uint32_t)((uint64_t)DMA_CHAN_GET_LL_BASE(chan) >> 32);

    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_LOW_OFF_RDCH_0_ADDRESS
                    + (chan * RD_DMA_REG_CHANNEL_STRIDE), transfer_list_low_addr);
    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_HIGH_OFF_RDCH_0_ADDRESS
                    + (chan * RD_DMA_REG_CHANNEL_STRIDE), transfer_list_high_addr);

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       dma_configure_write
*
*   DESCRIPTION
*
*       This function configures a DMA engine to issue PCIe memory writes to
*       the x86 host, pushing data from the SoC.
*
*   INPUTS
*
*       chan    DMA channel ID
*
*   OUTPUTS
*
*       int8_t     Status success or error
*
***********************************************************************/
int8_t dma_configure_write(dma_chan_id_e chan)
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

    transfer_list_low_addr = (uint32_t)((uint64_t)DMA_CHAN_GET_LL_BASE(chan) & 0xFFFFFFFF);
    transfer_list_high_addr = (uint32_t)((uint64_t)DMA_CHAN_GET_LL_BASE(chan) >> 32);

    wr_chan_num = chan - DMA_CHAN_ID_WRITE_0;
    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_LOW_OFF_WRCH_0_ADDRESS
                    + (wr_chan_num * WR_DMA_REG_CHANNEL_STRIDE), transfer_list_low_addr);
    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_HIGH_OFF_WRCH_0_ADDRESS
                    + (wr_chan_num * WR_DMA_REG_CHANNEL_STRIDE), transfer_list_high_addr);

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       dma_start_read
*
*   DESCRIPTION
*
*       This function triggers DMA read for specified channel.
*
*   INPUTS
*
*       chan    DMA channel ID
*
*   OUTPUTS
*
*       int8_t  Status success or error
*
***********************************************************************/
int8_t dma_start_read(dma_chan_id_e chan)
{
    uint32_t control1;

    if (chan >= DMA_CHAN_ID_READ_0 && chan <= DMA_CHAN_ID_READ_3)
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
    else
    {
        Log_Write(LOG_LEVEL_ERROR, "Invalid DMA Read channel %d\r\n", chan);
        return -EINVAL;
    }

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       dma_start_write
*
*   DESCRIPTION
*
*       This function triggers DMA write for specified channel.
*
*   INPUTS
*
*       chan    DMA channel ID
*
*   OUTPUTS
*
*       int8_t     Status success or error
*
***********************************************************************/
int8_t dma_start_write(dma_chan_id_e chan)
{
    uint32_t control1;
    uint32_t wr_chan_num;

    if (chan >= DMA_CHAN_ID_WRITE_0 && chan <= DMA_CHAN_ID_WRITE_3)
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
    }
    else
    {
        Log_Write(LOG_LEVEL_ERROR, "Invalid DMA Write channel %d\r\n", chan);
        return -EINVAL;
    }

    return 0;
}

/************************************************************************
*
*   FUNCTION
*
*       dma_clear_read_done
*
*   DESCRIPTION
*
*       This function clears the DMA transfer flag for specified channel.
*
*   INPUTS
*
*       chan    DMA channel ID
*
*   OUTPUTS
*
*       int8_t     Status success or error
*
***********************************************************************/
void dma_clear_read_done(dma_chan_id_e chan)
{
    if (chan >= DMA_CHAN_ID_READ_0 && chan <= DMA_CHAN_ID_READ_3)
    {
        iowrite32(
            PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_INT_CLEAR_OFF_ADDRESS,
            PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_INT_CLEAR_OFF_RD_DONE_INT_CLEAR_SET(
                (1U << chan) & 0xF));
    }
    else
    {
        Log_Write(LOG_LEVEL_ERROR, "Invalid DMA read channel %d\r\n", chan);
    }
}

/************************************************************************
*
*   FUNCTION
*
*       dma_clear_write_done
*
*   DESCRIPTION
*
*       This function clears the DMA transfer flag for specified channel.
*
*   INPUTS
*
*       chan    DMA channel ID
*
*   OUTPUTS
*
*       int8_t     Status success or error
*
***********************************************************************/
void dma_clear_write_done(dma_chan_id_e chan)
{
    if (chan >= DMA_CHAN_ID_WRITE_0 && chan <= DMA_CHAN_ID_WRITE_3)
    {
        iowrite32(
            PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_INT_CLEAR_OFF_ADDRESS,
            PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_INT_CLEAR_OFF_WR_DONE_INT_CLEAR_SET(
                (1U << (chan - DMA_CHAN_ID_WRITE_0)) & 0xF));
    }
    else
    {
        Log_Write(LOG_LEVEL_ERROR, "Invalid DMA write channel %d\r\n", chan);
    }
}

/************************************************************************
*
*   FUNCTION
*
*       dma_clear_read_abort
*
*   DESCRIPTION
*
*       This function clears the DMA abort flag for specified read channel.
*
*   INPUTS
*
*       chan    DMA channel ID
*
*   OUTPUTS
*
*       int8_t     Status success or error
*
***********************************************************************/
int8_t dma_clear_read_abort(dma_chan_id_e chan)
{
    int8_t status = DMA_OPERATION_SUCCESS;

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

/************************************************************************
*
*   FUNCTION
*
*       dma_clear_write_abort
*
*   DESCRIPTION
*
*       This function clears the DMA abort flag for specified write channel.
*
*   INPUTS
*
*       chan    DMA channel ID
*
*   OUTPUTS
*
*       int8_t     Status success or error
*
***********************************************************************/
int8_t dma_clear_write_abort(dma_chan_id_e chan)
{
    int8_t status = DMA_OPERATION_SUCCESS;

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

/************************************************************************
*
*   FUNCTION
*
*       dma_abort_read
*
*   DESCRIPTION
*
*       This function aborts DMA for the specified read channel.
*
*   INPUTS
*
*       chan    DMA channel ID
*
*   OUTPUTS
*
*       int8_t     Status success or error
*
***********************************************************************/
int8_t dma_abort_read(dma_chan_id_e chan)
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

/************************************************************************
*
*   FUNCTION
*
*       dma_abort_write
*
*   DESCRIPTION
*
*       This function aborts DMA for the specified write channel.
*
*   INPUTS
*
*       chan    DMA channel ID
*
*   OUTPUTS
*
*       int8_t     Status success or error
*
***********************************************************************/
int8_t dma_abort_write(dma_chan_id_e chan)
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
