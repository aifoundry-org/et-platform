#include "drivers/pcie_dma.h"

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#include "cacheops.h"
#include "services/log.h"
#include "layout.h"
#include "pcie_dma_ll.h"
#include "printf.h"
#include "atomic.h"

//Control Register Bitfields:

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

#define DATA_CTRL     0
#define DATA_SIZE     4
#define DATA_SAR_LOW  8
#define DATA_SAR_HIGH 12
#define DATA_DAR_LOW  16
#define DATA_DAR_HIGH 20

#define LINK_CTRL     0
#define LINK_PTR_LOW  8
#define LINK_PTR_HIGH 12

#define XFER_NODE_BYTES    24
#define DMA_CHANNELS_COUNT 8
#define DMA_MEM_REGIONS    2

//Always save 1 element for terminator
#define MAX_TRANSFER_LIST_SIZE (DMA_LL_SIZE / sizeof(transfer_list_elem_t) - 1)

struct dma_mem_region {
    uint64_t begin;
    uint64_t end;
};

volatile transfer_list_elem_t *transfer_lists[] = {
    (volatile transfer_list_elem_t *)DMA_CHAN_READ_0_LL_BASE,
    (volatile transfer_list_elem_t *)DMA_CHAN_READ_1_LL_BASE,
    (volatile transfer_list_elem_t *)DMA_CHAN_READ_2_LL_BASE,
    (volatile transfer_list_elem_t *)DMA_CHAN_READ_3_LL_BASE,
    (volatile transfer_list_elem_t *)DMA_CHAN_WRITE_0_LL_BASE,
    (volatile transfer_list_elem_t *)DMA_CHAN_WRITE_1_LL_BASE,
    (volatile transfer_list_elem_t *)DMA_CHAN_WRITE_2_LL_BASE,
    (volatile transfer_list_elem_t *)DMA_CHAN_WRITE_3_LL_BASE
};

static const struct dma_mem_region valid_dma_targets[DMA_MEM_REGIONS] = {
    //L3, but beginning reserved for minion stacks, end reserved for DMA config
    { .begin = HOST_MANAGED_DRAM_START, .end = (HOST_MANAGED_DRAM_END - 1) },
    //Shire-cache scratch pads. Limit to first 4MB * 33 shires.
    { .begin = 0x80000000, .end = 0x883FFFFF }
};

/*
 * Writes one data element of a DMA tranfser list. This structure is used
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
 * Writes one data element of a DMA tranfser list. This structure is used
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
    uint64_t end_addr = soc_addr + size - 1;

    // Check for uint64_t overflow
    if (end_addr > soc_addr) {
        for (int i = 0; i < DMA_MEM_REGIONS; ++i) {
            if (soc_addr >= valid_dma_targets[i].begin && end_addr <= valid_dma_targets[i].end) {
                return DMA_OPERATION_SUCCESS;
            }
        }
    }
    return DMA_ERROR_INVALID_ADDRESS;
}

int dma_trigger_transfer(uint64_t src_addr, uint64_t dest_addr,
    uint64_t size, dma_chan_id_e chan)
{
    /* Reads data from Host to Device memory */
    int status = DMA_OPERATION_NOT_SUCCESS;

    /* Validate the params */
    if ((src_addr == 0U) || (dest_addr == 0U) || (size == 0U) ||
        !(chan >= DMA_CHAN_ID_READ_0 && chan <= DMA_CHAN_ID_WRITE_3))
    {
        status = DMA_ERROR_INVALID_PARAM;
    }
    else
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
        status = dma_config_buff(src_addr, dest_addr, (uint32_t)size, chan);

        if (status == DMA_OPERATION_SUCCESS)
        {
            if (chan <= DMA_CHAN_ID_READ_3)
            {
                /* Configure the read channel */
                status = dma_configure_read(chan);
            }
            else
            {
                /* Configure the write channel */
                status = dma_configure_write(chan);
            }

            if (status == DMA_OPERATION_SUCCESS)
            {
                dma_start(chan);
            }
        }
    }

    return status;
}

int dma_configure_read(dma_chan_id_e chan)
{
    if (chan > DMA_CHAN_ID_READ_3) {
        Log_Write(LOG_LEVEL_CRITICAL, "Invalid DMA read channel %d\r\n", chan);
        return -EINVAL;
    }
    uint32_t chan_mask = 1U << (uint32_t)chan;

    uint32_t dma_read_engine_en;
    dma_read_engine_en =
        ioread32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_ENGINE_EN_OFF_ADDRESS);

    //Global enable for DMA reads
    dma_read_engine_en =
        PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_ENGINE_EN_OFF_DMA_READ_ENGINE_MODIFY(
            dma_read_engine_en, 1);

    //Enable handshake mode. 1 bit per channel, beginning at offset 16
    dma_read_engine_en |=
        chan_mask
        << PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_ENGINE_EN_OFF_DMA_READ_ENGINE_EN_HSHAKE_CH0_LSB;

    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_ENGINE_EN_OFF_ADDRESS,
              dma_read_engine_en);

    //Don't mask any DMA read interrupts
    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_INT_MASK_OFF_ADDRESS, 0);

    //Set Linked-List Remote Abort Interrupt Enable (LLRAIE) for DMA channel
    uint32_t tmp;
    tmp = ioread32(PCIE0 +
                   PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_LINKED_LIST_ERR_EN_OFF_ADDRESS);
    tmp =
        PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_LINKED_LIST_ERR_EN_OFF_RD_CHANNEL_LLRAIE_SET(
            chan_mask);
    iowrite32(PCIE0 +
                  PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_LINKED_LIST_ERR_EN_OFF_ADDRESS,
              tmp);

    //Use linked list mode. All of the control 1 registers have the same layout.
    uint32_t control1 =
        PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_0_CCS_SET(1) |
        PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_0_LLE_SET(1) |
        PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_0_DMA_MEM_TYPE_SET(1);

    uint32_t transfer_list_low_addr = (uint32_t)((uint64_t)transfer_lists[chan] & 0xFFFFFFFF);
    uint32_t transfer_list_high_addr = (uint32_t)((uint64_t)transfer_lists[chan] >> 32);

    if (chan == DMA_CHAN_ID_READ_0) {
        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_0_ADDRESS,
                  control1);

        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_LOW_OFF_RDCH_0_ADDRESS,
                  transfer_list_low_addr);
        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_HIGH_OFF_RDCH_0_ADDRESS,
                  transfer_list_high_addr);
    } else if (chan == DMA_CHAN_ID_READ_1) {
        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_1_ADDRESS,
                  control1);

        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_LOW_OFF_RDCH_1_ADDRESS,
                  transfer_list_low_addr);
        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_HIGH_OFF_RDCH_1_ADDRESS,
                  transfer_list_high_addr);
    } else if (chan == DMA_CHAN_ID_READ_2) {
        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_2_ADDRESS,
                  control1);

        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_LOW_OFF_RDCH_2_ADDRESS,
                  transfer_list_low_addr);
        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_HIGH_OFF_RDCH_2_ADDRESS,
                  transfer_list_high_addr);
    } else if (chan == DMA_CHAN_ID_READ_3) {
        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_3_ADDRESS,
                  control1);

        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_LOW_OFF_RDCH_3_ADDRESS,
                  transfer_list_low_addr);
        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_HIGH_OFF_RDCH_3_ADDRESS,
                  transfer_list_high_addr);
    } else {
        Log_Write(LOG_LEVEL_CRITICAL, "Invalid DMA read channel %d\r\n", chan);
        return -EINVAL;
    }

    //Ring the doorbell. This prefetches the first transfer list element and enables
    //DMA to begin very quickly once started.
    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_DOORBELL_OFF_ADDRESS,
              (uint32_t)chan);

    return 0;
}

int dma_configure_write(dma_chan_id_e chan)
{
    if (chan < DMA_CHAN_ID_WRITE_0 || chan > DMA_CHAN_ID_WRITE_3) {
        Log_Write(LOG_LEVEL_CRITICAL, "Invalid DMA write channel %d\r\n", chan);
        return -EINVAL;
    }
    uint32_t wr_chan_num = chan - DMA_CHAN_ID_WRITE_0;
    uint32_t chan_mask = 1U << wr_chan_num;

    uint32_t write_engine_en;
    write_engine_en =
        ioread32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_ENGINE_EN_OFF_ADDRESS);

    //Global enable for DMA reads
    write_engine_en =
        PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_ENGINE_EN_OFF_DMA_WRITE_ENGINE_MODIFY(
            write_engine_en, 1);

    //Enable handshake mode. 1 bit per channel, beginning at offset 16
    write_engine_en |=
        chan_mask
        << PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_ENGINE_EN_OFF_DMA_WRITE_ENGINE_EN_HSHAKE_CH0_LSB;

    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_ENGINE_EN_OFF_ADDRESS,
              write_engine_en);

    //Don't mask any DMA write interrupts
    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_INT_MASK_OFF_ADDRESS, 0);

    //Set Linked-List Remote Abort Interrupt Enable (LLRAIE) for DMA channel
    uint32_t tmp;
    tmp = ioread32(PCIE0 +
                   PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_LINKED_LIST_ERR_EN_OFF_ADDRESS);
    tmp =
        PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_LINKED_LIST_ERR_EN_OFF_WR_CHANNEL_LLRAIE_SET(
            chan_mask);
    iowrite32(PCIE0 +
                  PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_LINKED_LIST_ERR_EN_OFF_ADDRESS,
              tmp);

    //Use linked list mode. All of the control 1 registers have the same layout.
    uint32_t control1 =
        PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_0_CCS_SET(1) |
        PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_0_LLE_SET(1) |
        PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_0_DMA_MEM_TYPE_SET(1);

    uint32_t transfer_list_low_addr = (uint32_t)((uint64_t)transfer_lists[chan] & 0xFFFFFFFF);
    uint32_t transfer_list_high_addr = (uint32_t)((uint64_t)transfer_lists[chan] >> 32);

    if (chan == DMA_CHAN_ID_WRITE_0) {
        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_0_ADDRESS,
                  control1);

        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_LOW_OFF_WRCH_0_ADDRESS,
                  transfer_list_low_addr);
        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_HIGH_OFF_WRCH_0_ADDRESS,
                  transfer_list_high_addr);
    } else if (chan == DMA_CHAN_ID_WRITE_1) {
        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_1_ADDRESS,
                  control1);

        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_LOW_OFF_WRCH_1_ADDRESS,
                  transfer_list_low_addr);
        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_HIGH_OFF_WRCH_1_ADDRESS,
                  transfer_list_high_addr);
    } else if (chan == DMA_CHAN_ID_WRITE_2) {
        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_2_ADDRESS,
                  control1);

        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_LOW_OFF_WRCH_2_ADDRESS,
                  transfer_list_low_addr);
        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_HIGH_OFF_WRCH_2_ADDRESS,
                  transfer_list_high_addr);
    } else if (chan == DMA_CHAN_ID_WRITE_3) {
        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_3_ADDRESS,
                  control1);

        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_LOW_OFF_WRCH_3_ADDRESS,
                  transfer_list_low_addr);
        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_HIGH_OFF_WRCH_3_ADDRESS,
                  transfer_list_high_addr);
    } else {
        Log_Write(LOG_LEVEL_CRITICAL, "Invalid DMA write channel %d\r\n", chan);
        return -EINVAL;
    }

    //Ring the doorbell. This prefetches the first transfer list element and enables
    //DMA to begin very quickly once started.
    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_DOORBELL_OFF_ADDRESS,
              wr_chan_num);

    return 0;
}

int dma_start(dma_chan_id_e chan)
{
    if (chan <= DMA_CHAN_ID_READ_3) {
        iowrite32(PCIE_USRESR + PSHIRE_USR0_DMA_RD_XFER_ADDRESS,
                  PSHIRE_USR0_DMA_RD_XFER_CHNL_GO_SET(1U << chan));
    } else if (chan >= DMA_CHAN_ID_WRITE_0 && chan <= DMA_CHAN_ID_WRITE_3) {
        iowrite32(PCIE_USRESR + PSHIRE_USR0_DMA_WR_XFER_ADDRESS,
                  PSHIRE_USR0_DMA_WR_XFER_CHNL_GO_SET(1U << (chan - DMA_CHAN_ID_WRITE_0)));
    } else {
        Log_Write(LOG_LEVEL_CRITICAL, "Invalid DMA channel %d\r\n", chan);
        return -EINVAL;
    }

    return 0;
}

void dma_clear_done(dma_chan_id_e chan)
{
    uint32_t chanU32 = (uint32_t)chan;

    if (chanU32 <= DMA_CHAN_ID_READ_3) {
        iowrite32(
            PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_INT_CLEAR_OFF_ADDRESS,
            PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_INT_CLEAR_OFF_RD_DONE_INT_CLEAR_SET(
                (1U << chan) & 0xF));
    } else if (chanU32 >= DMA_CHAN_ID_WRITE_0 && chanU32 <= DMA_CHAN_ID_WRITE_3) {
        iowrite32(
            PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_INT_CLEAR_OFF_ADDRESS,
            PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_INT_CLEAR_OFF_WR_DONE_INT_CLEAR_SET(
                (1U << (chan - DMA_CHAN_ID_WRITE_0)) & 0xF));
    } else {
        Log_Write(LOG_LEVEL_CRITICAL, "Invalid DMA channel %d\r\n", chanU32);
    }
}

DMA_STATUS_e dma_clear_read_abort(dma_chan_id_e chan)
{
    if (chan <= DMA_CHAN_ID_READ_3) {
        iowrite32(
            PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_INT_CLEAR_OFF_ADDRESS,
            PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_INT_CLEAR_OFF_RD_ABORT_INT_CLEAR_SET(
                (1U << chan) & 0xF));
    } else {
        Log_Write(LOG_LEVEL_CRITICAL, "Invalid DMA read channel %d\r\n", chan);
        return DMA_ERROR_CHANNEL_NOT_AVAILABLE;
    }

    return DMA_OPERATION_SUCCESS;
}

DMA_STATUS_e dma_clear_write_abort(dma_chan_id_e chan)
{
    if (chan >= DMA_CHAN_ID_WRITE_0 && chan <= DMA_CHAN_ID_WRITE_3) {
        iowrite32(
            PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_INT_CLEAR_OFF_ADDRESS,
            PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_INT_CLEAR_OFF_WR_ABORT_INT_CLEAR_SET(
                (1U << (chan - DMA_CHAN_ID_WRITE_0)) & 0xF));
    } else {
        Log_Write(LOG_LEVEL_CRITICAL, "Invalid DMA write channel %d\r\n", chan);
        return DMA_ERROR_CHANNEL_NOT_AVAILABLE;
    }

    return DMA_OPERATION_SUCCESS;
}

DMA_STATUS_e dma_abort_read(dma_chan_id_e chan)
{
    uint32_t control1;

    /* Get the control1 register of the respective channel */
    if (chan == DMA_CHAN_ID_READ_0) {
        control1 =
            ioread32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_0_ADDRESS);
    } else if (chan == DMA_CHAN_ID_READ_1) {
        control1 =
            ioread32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_1_ADDRESS);
    } else if (chan == DMA_CHAN_ID_READ_2) {
        control1 =
            ioread32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_2_ADDRESS);
    } else if (chan == DMA_CHAN_ID_READ_3) {
        control1 =
            ioread32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_3_ADDRESS);
    } else {
        Log_Write(LOG_LEVEL_CRITICAL, "Invalid DMA read channel %d\r\n", chan);
        return DMA_ERROR_CHANNEL_NOT_AVAILABLE;
    }

    // If the respective channel is running, abort it
    if (1 == PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_0_CS_GET(control1)) {
        uint32_t dma_abort = (uint32_t)chan |
            PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_DOORBELL_OFF_RD_STOP_FIELD_MASK;
        iowrite32(
            PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_DOORBELL_OFF_ADDRESS, dma_abort);
    } else {
        Log_Write(LOG_LEVEL_DEBUG, "Channel %d is not running\r\n", chan);
        return DMA_ERROR_CHANNEL_NOT_RUNNING;
    }

    return DMA_OPERATION_SUCCESS;
}

DMA_STATUS_e dma_abort_write(dma_chan_id_e chan)
{
    uint32_t control1;

    /* Get the control1 register of the respective channel */
    if (chan == DMA_CHAN_ID_WRITE_0) {
        control1 =
            ioread32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_0_ADDRESS);
    } else if (chan == DMA_CHAN_ID_WRITE_1) {
        control1 =
            ioread32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_1_ADDRESS);
    } else if (chan == DMA_CHAN_ID_WRITE_2) {
        control1 =
            ioread32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_2_ADDRESS);
    } else if (chan == DMA_CHAN_ID_WRITE_3) {
        control1 =
            ioread32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_3_ADDRESS);
    } else {
        Log_Write(LOG_LEVEL_CRITICAL, "Invalid DMA write channel %d\r\n", chan);
        return DMA_ERROR_CHANNEL_NOT_AVAILABLE;
    }

    // If the respective channel is running, abort it
    if (1 == PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_0_CS_GET(control1)) {
        uint32_t dma_abort = (uint32_t)(chan - DMA_CHAN_ID_WRITE_0) |
            PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_DOORBELL_OFF_WR_STOP_FIELD_MASK;
        iowrite32(
            PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_DOORBELL_OFF_ADDRESS, dma_abort);
    } else {
        Log_Write(LOG_LEVEL_DEBUG, "Channel %d is not running\r\n", chan);
        return DMA_ERROR_CHANNEL_NOT_RUNNING;
    }

    return DMA_OPERATION_SUCCESS;
}
