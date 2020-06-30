#include "pcie_dma.h"

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#include "cacheops.h"
#include "io.h"
#include "etsoc_hal/inc/DWC_pcie_dbi_cpcie_usp_4x8.h"
#include "layout.h"
#include "pcie_device.h"
#include "pcie_dma_ll.h"
#include "printf.h"

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

//Always save 1 element for terminator
#define MAX_TRANSFER_LIST_SIZE (DMA_LL_SIZE / sizeof(transfer_list_elem_t) - 1)

int dma_configure_read(dma_chan_t chan)
{
    if (chan > DMA_CHAN_READ_3) {
        printf("Invalid DMA read channel %d\r\n", chan);
        return -EINVAL;
    }
    uint32_t chan_mask = 1U << (uint32_t)chan;

    uint32_t dma_read_engine_en;
    dma_read_engine_en = ioread32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_ENGINE_EN_OFF_ADDRESS);

    //Global enable for DMA reads
    dma_read_engine_en = PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_ENGINE_EN_OFF_DMA_READ_ENGINE_MODIFY(dma_read_engine_en, 1);

    //Enable handshake mode. 1 bit per channel, beginning at offset 16
    dma_read_engine_en |= chan_mask << PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_ENGINE_EN_OFF_DMA_READ_ENGINE_EN_HSHAKE_CH0_LSB;

    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_ENGINE_EN_OFF_ADDRESS, dma_read_engine_en);

    //Don't mask any DMA read interrupts
    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_INT_MASK_OFF_ADDRESS, 0);

    //Set Linked-List Remote Abort Interrupt Enable (LLRAIE) for DMA channel
    uint32_t tmp;
    tmp = ioread32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_LINKED_LIST_ERR_EN_OFF_ADDRESS);
    tmp = PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_LINKED_LIST_ERR_EN_OFF_RD_CHANNEL_LLRAIE_SET(chan_mask);
    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_LINKED_LIST_ERR_EN_OFF_ADDRESS, tmp);

    //Use linked list mode. All of the control 1 registers have the same layout.
    uint32_t control1 =
        PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_0_CCS_SET(1)           |
        PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_0_LLE_SET(1)           |
        PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_0_DMA_MEM_TYPE_SET(1);

    uint32_t transfer_list_low_addr = (uint32_t)((uint64_t)transfer_lists[chan] & 0xFFFFFFFF);
    uint32_t transfer_list_high_addr = (uint32_t)((uint64_t)transfer_lists[chan] >> 32);

    if (chan == DMA_CHAN_READ_0) {
        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_0_ADDRESS, control1);

        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_LOW_OFF_RDCH_0_ADDRESS, transfer_list_low_addr);
        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_HIGH_OFF_RDCH_0_ADDRESS, transfer_list_high_addr);
    }
    else if (chan == DMA_CHAN_READ_1) {
        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_1_ADDRESS, control1);

        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_LOW_OFF_RDCH_1_ADDRESS, transfer_list_low_addr);
        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_HIGH_OFF_RDCH_1_ADDRESS, transfer_list_high_addr);
    }
    else if (chan == DMA_CHAN_READ_2) {
        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_2_ADDRESS, control1);

        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_LOW_OFF_RDCH_2_ADDRESS, transfer_list_low_addr);
        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_HIGH_OFF_RDCH_2_ADDRESS, transfer_list_high_addr);
    }
    else if (chan == DMA_CHAN_READ_3) {
        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_3_ADDRESS, control1);

        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_LOW_OFF_RDCH_3_ADDRESS, transfer_list_low_addr);
        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_HIGH_OFF_RDCH_3_ADDRESS, transfer_list_high_addr);
    }
    else {
        printf("Invalid DMA read channel %d\r\n", chan);
        return -EINVAL;
    }

    //Ring the doorbell. This prefetches the first transfer list element and enables
    //DMA to begin very quickly once started.
    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_DOORBELL_OFF_ADDRESS, (uint32_t)chan);

    return 0;
}

int dma_configure_write(dma_chan_t chan)
{
    if (chan < DMA_CHAN_WRITE_0 || chan > DMA_CHAN_WRITE_3) {
        printf("Invalid DMA write channel %d\r\n", chan);
        return -EINVAL;
    }
    uint32_t wr_chan_num = chan - DMA_CHAN_WRITE_0;
    uint32_t chan_mask = 1U << wr_chan_num;

    uint32_t write_engine_en;
    write_engine_en = ioread32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_ENGINE_EN_OFF_ADDRESS);

    //Global enable for DMA reads
    write_engine_en = PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_ENGINE_EN_OFF_DMA_WRITE_ENGINE_MODIFY(write_engine_en, 1);

    //Enable handshake mode. 1 bit per channel, beginning at offset 16
    write_engine_en |= chan_mask << PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_ENGINE_EN_OFF_DMA_WRITE_ENGINE_EN_HSHAKE_CH0_LSB;

    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_ENGINE_EN_OFF_ADDRESS, write_engine_en);

    //Don't mask any DMA write interrupts
    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_INT_MASK_OFF_ADDRESS, 0);

    //Set Linked-List Remote Abort Interrupt Enable (LLRAIE) for DMA channel
    uint32_t tmp;
    tmp = ioread32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_LINKED_LIST_ERR_EN_OFF_ADDRESS);
    tmp = PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_LINKED_LIST_ERR_EN_OFF_WR_CHANNEL_LLRAIE_SET(chan_mask);
    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_LINKED_LIST_ERR_EN_OFF_ADDRESS, tmp);


    //Use linked list mode. All of the control 1 registers have the same layout.
    uint32_t control1 =
        PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_0_CCS_SET(1)           |
        PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_0_LLE_SET(1)           |
        PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_0_DMA_MEM_TYPE_SET(1);

    uint32_t transfer_list_low_addr = (uint32_t)((uint64_t)transfer_lists[chan] & 0xFFFFFFFF);
    uint32_t transfer_list_high_addr = (uint32_t)((uint64_t)transfer_lists[chan] >> 32);

    if (chan == DMA_CHAN_WRITE_0) {
        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_0_ADDRESS, control1);

        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_LOW_OFF_WRCH_0_ADDRESS, transfer_list_low_addr);
        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_HIGH_OFF_WRCH_0_ADDRESS, transfer_list_high_addr);
    }
    else if (chan == DMA_CHAN_WRITE_1) {
        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_1_ADDRESS, control1);

        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_LOW_OFF_WRCH_1_ADDRESS, transfer_list_low_addr);
        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_HIGH_OFF_WRCH_1_ADDRESS, transfer_list_high_addr);
    }
    else if (chan == DMA_CHAN_WRITE_2) {
        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_2_ADDRESS, control1);

        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_LOW_OFF_WRCH_2_ADDRESS, transfer_list_low_addr);
        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_HIGH_OFF_WRCH_2_ADDRESS, transfer_list_high_addr);
    }
    else if (chan == DMA_CHAN_WRITE_3) {
        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_3_ADDRESS, control1);

        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_LOW_OFF_WRCH_3_ADDRESS, transfer_list_low_addr);
        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_LLP_HIGH_OFF_WRCH_3_ADDRESS, transfer_list_high_addr);
    }
    else {
        printf("Invalid DMA write channel %d\r\n", chan);
        return -EINVAL;
    }

    //Ring the doorbell. This prefetches the first transfer list element and enables
    //DMA to begin very quickly once started.
    iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_DOORBELL_OFF_ADDRESS, wr_chan_num);

    return 0;
}

int dma_start(dma_chan_t chan)
{
    if (chan <= DMA_CHAN_READ_3) {
        iowrite32(PCIE_USRESR + PSHIRE_USR0_DMA_RD_XFER_ADDRESS, PSHIRE_USR0_DMA_RD_XFER_CHNL_GO_SET(1U << chan));
    }
    else if (chan >= DMA_CHAN_WRITE_0 && chan <= DMA_CHAN_WRITE_3) {
        iowrite32(PCIE_USRESR + PSHIRE_USR0_DMA_WR_XFER_ADDRESS, PSHIRE_USR0_DMA_WR_XFER_CHNL_GO_SET(1U << (chan - DMA_CHAN_WRITE_0)));
    }
    else {
        printf("Invalid DMA channel %d\r\n", chan);
        return -EINVAL;
    }

    return 0;
}

bool dma_check_done(dma_chan_t chan)
{
    if (chan <= DMA_CHAN_READ_3) {
        uint32_t dma_read_int_status = ioread32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_INT_STATUS_OFF_ADDRESS);
        uint32_t done_status = PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_INT_STATUS_OFF_RD_DONE_INT_STATUS_GET(dma_read_int_status);

        //TODO: this relies on int bit being set. Use the following instead?
        //PCIE0->PF0_DMA_CAP.DMA_CH_CONTROL1_OFF_RDCH_0.B.CS;

        return (done_status & (1U << chan)) != 0;
    }
    else if (chan >= DMA_CHAN_WRITE_0 && chan <= DMA_CHAN_WRITE_3) {
        uint32_t dma_write_int_status = ioread32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_INT_STATUS_OFF_ADDRESS);
        uint32_t done_status = PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_INT_STATUS_OFF_WR_DONE_INT_STATUS_GET(dma_write_int_status);

        return (done_status & (1U << (chan - DMA_CHAN_WRITE_0))) != 0;
    }
    else {
        printf("Invalid DMA channel %d\r\n", chan);
        return false;
    }
    return true;
}

void dma_clear_done(dma_chan_t chan)
{
    uint32_t chanU32 = (uint32_t)chan;

    if (chanU32 <= DMA_CHAN_READ_3) {
        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_INT_CLEAR_OFF_ADDRESS,
            PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_READ_INT_CLEAR_OFF_RD_DONE_INT_CLEAR_SET((1U << chan) & 0xF)
        );
    }
    else if (chanU32 >= DMA_CHAN_WRITE_0 && chanU32 <= DMA_CHAN_WRITE_3) {
        iowrite32(PCIE0 + PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_INT_CLEAR_OFF_ADDRESS,
            PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_DMA_CAP_DMA_WRITE_INT_CLEAR_OFF_WR_DONE_INT_CLEAR_SET((1U << (chan - DMA_CHAN_WRITE_0)) & 0xF)
        );
    }
    else {
        printf("Invalid DMA channel %d\r\n", chanU32);
    }
}
