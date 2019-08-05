#include "pcie_dma.h"

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#include "cacheops.h"
#include "DWC_pcie_dbi_cpcie_usp_4x8.h"
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

    PE0_DWC_pcie_ctl_DBI_Slave_PF0_DMA_CAP_DMA_READ_ENGINE_EN_OFF_t dma_read_engine_en;
    dma_read_engine_en.R = PCIE0->PF0_DMA_CAP.DMA_READ_ENGINE_EN_OFF.R;
    
    //Global enable for DMA reads
    dma_read_engine_en.B.DMA_READ_ENGINE = 1;
    
    //Enable handshake mode. 1 bit per channel, beginning at offset 16
    dma_read_engine_en.R |= chan_mask << 16;

    PCIE0->PF0_DMA_CAP.DMA_READ_ENGINE_EN_OFF.R = dma_read_engine_en.R;

    //Don't mask any DMA read interrupts
    PCIE0->PF0_DMA_CAP.DMA_READ_INT_MASK_OFF.R = 0;

    //Set Linked-List Remote Abort Interrupt Enable (LLRAIE) for DMA channel
    PCIE0->PF0_DMA_CAP.DMA_READ_LINKED_LIST_ERR_EN_OFF.R |= chan_mask;

    //Use linked list mode. All of the control 1 registers have the same layout.
    PE0_DWC_pcie_ctl_DBI_Slave_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_0_t control1 = { .B = {
        .CCS = 1,
        .LLE = 1,
        .DMA_MEM_TYPE = 1
    }};

    uint32_t transfer_list_low_addr = (uint32_t)((uint64_t)transfer_lists[chan] & 0xFFFFFFFF);
    uint32_t transfer_list_high_addr = (uint32_t)((uint64_t)transfer_lists[chan] >> 32);

    if (chan == DMA_CHAN_READ_0) {
        PCIE0->PF0_DMA_CAP.DMA_CH_CONTROL1_OFF_RDCH_0.R = control1.R;
        
        PCIE0->PF0_DMA_CAP.DMA_LLP_LOW_OFF_RDCH_0.R = transfer_list_low_addr;
        PCIE0->PF0_DMA_CAP.DMA_LLP_HIGH_OFF_RDCH_0.R = transfer_list_high_addr;
    }
    else if (chan == DMA_CHAN_READ_1) {
        PCIE0->PF0_DMA_CAP.DMA_CH_CONTROL1_OFF_RDCH_1.R = control1.R;

        PCIE0->PF0_DMA_CAP.DMA_LLP_LOW_OFF_RDCH_1.R = transfer_list_low_addr;
        PCIE0->PF0_DMA_CAP.DMA_LLP_HIGH_OFF_RDCH_1.R = transfer_list_high_addr;
    }
    else if (chan == DMA_CHAN_READ_2) {
        PCIE0->PF0_DMA_CAP.DMA_CH_CONTROL1_OFF_RDCH_2.R = control1.R;

        PCIE0->PF0_DMA_CAP.DMA_LLP_LOW_OFF_RDCH_2.R = transfer_list_low_addr;
        PCIE0->PF0_DMA_CAP.DMA_LLP_HIGH_OFF_RDCH_2.R = transfer_list_high_addr;
    }
    else if (chan == DMA_CHAN_READ_3) {
        PCIE0->PF0_DMA_CAP.DMA_CH_CONTROL1_OFF_RDCH_3.R = control1.R;

        PCIE0->PF0_DMA_CAP.DMA_LLP_LOW_OFF_RDCH_3.R = transfer_list_low_addr;
        PCIE0->PF0_DMA_CAP.DMA_LLP_HIGH_OFF_RDCH_3.R = transfer_list_high_addr;
    }
    else {
        printf("Invalid DMA read channel %d\r\n", chan);
        return -EINVAL;
    }

    //Ring the doorbell. This prefetches the first transfer list element and enables
    //DMA to begin very quickly once started.
    PCIE0->PF0_DMA_CAP.DMA_READ_DOORBELL_OFF.R = (uint32_t)chan;

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

    PE0_DWC_pcie_ctl_DBI_Slave_PF0_DMA_CAP_DMA_WRITE_ENGINE_EN_OFF_t write_engine_en;
    write_engine_en.R = PCIE0->PF0_DMA_CAP.DMA_WRITE_ENGINE_EN_OFF.R;
    
    //Global enable for DMA reads
    write_engine_en.B.DMA_WRITE_ENGINE = 1;
    
    //Enable handshake mode. 1 bit per channel, beginning at offset 16
    write_engine_en.R |= chan_mask << 16;

    PCIE0->PF0_DMA_CAP.DMA_WRITE_ENGINE_EN_OFF.R = write_engine_en.R;

    //Don't mask any DMA write interrupts
    PCIE0->PF0_DMA_CAP.DMA_WRITE_INT_MASK_OFF.R = 0;

    //Set Linked-List Remote Abort Interrupt Enable (LLRAIE) for DMA channel
    PCIE0->PF0_DMA_CAP.DMA_WRITE_LINKED_LIST_ERR_EN_OFF.R |= chan_mask;

    //Use linked list mode. All of the control 1 registers have the same layout.
    PE0_DWC_pcie_ctl_DBI_Slave_PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_0_t control1 = { .B = {
        .CCS = 1,
        .LLE = 1,
        .DMA_MEM_TYPE = 1
    }};
    
    uint32_t transfer_list_low_addr = (uint32_t)((uint64_t)transfer_lists[chan] & 0xFFFFFFFF);
    uint32_t transfer_list_high_addr = (uint32_t)((uint64_t)transfer_lists[chan] >> 32);

    if (chan == DMA_CHAN_WRITE_0) {
        PCIE0->PF0_DMA_CAP.DMA_CH_CONTROL1_OFF_WRCH_0.R = control1.R;

        PCIE0->PF0_DMA_CAP.DMA_LLP_LOW_OFF_WRCH_0.R = transfer_list_low_addr;
        PCIE0->PF0_DMA_CAP.DMA_LLP_HIGH_OFF_WRCH_0.R = transfer_list_high_addr;
    }
    else if (chan == DMA_CHAN_WRITE_1) {
        PCIE0->PF0_DMA_CAP.DMA_CH_CONTROL1_OFF_WRCH_1.R = control1.R;

        PCIE0->PF0_DMA_CAP.DMA_LLP_LOW_OFF_WRCH_1.R = transfer_list_low_addr;
        PCIE0->PF0_DMA_CAP.DMA_LLP_HIGH_OFF_WRCH_1.R = transfer_list_high_addr;
    }
    else if (chan == DMA_CHAN_WRITE_2) {
        PCIE0->PF0_DMA_CAP.DMA_CH_CONTROL1_OFF_WRCH_2.R = control1.R;

        PCIE0->PF0_DMA_CAP.DMA_LLP_LOW_OFF_WRCH_2.R = transfer_list_low_addr;
        PCIE0->PF0_DMA_CAP.DMA_LLP_HIGH_OFF_WRCH_2.R = transfer_list_high_addr;
    }
    else if (chan == DMA_CHAN_WRITE_3) {
        PCIE0->PF0_DMA_CAP.DMA_CH_CONTROL1_OFF_WRCH_3.R = control1.R;

        PCIE0->PF0_DMA_CAP.DMA_LLP_LOW_OFF_WRCH_3.R = transfer_list_low_addr;
        PCIE0->PF0_DMA_CAP.DMA_LLP_HIGH_OFF_WRCH_3.R = transfer_list_high_addr;
    }
    else {
        printf("Invalid DMA write channel %d\r\n", chan);
        return -EINVAL;
    }

    //Ring the doorbell. This prefetches the first transfer list element and enables
    //DMA to begin very quickly once started.
    PCIE0->PF0_DMA_CAP.DMA_WRITE_DOORBELL_OFF.R = wr_chan_num;
   
    return 0;
}

int dma_start(dma_chan_t chan)
{
    if (chan <= DMA_CHAN_READ_3) {
        PCIE_USRESR->dma_rd_xfer.R = 1U << chan;
    }
    else if (chan >= DMA_CHAN_WRITE_0 && chan <= DMA_CHAN_WRITE_3) {
        PCIE_USRESR->dma_wr_xfer.R = 1U << (chan - DMA_CHAN_WRITE_0);
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
        PE0_DWC_pcie_ctl_DBI_Slave_PF0_DMA_CAP_DMA_READ_INT_STATUS_OFF_t dma_read_int_status;
        dma_read_int_status.R = PCIE0->PF0_DMA_CAP.DMA_READ_INT_STATUS_OFF.R;

        uint32_t done_status = dma_read_int_status.B.RD_DONE_INT_STATUS;

        //TODO: this relies on int bit being set. Use the following instead?
        //PCIE0->PF0_DMA_CAP.DMA_CH_CONTROL1_OFF_RDCH_0.B.CS;

        return (done_status & (1U << chan)) != 0;
    }
    else if (chan >= DMA_CHAN_WRITE_0 && chan <= DMA_CHAN_WRITE_3) {
        PE0_DWC_pcie_ctl_DBI_Slave_PF0_DMA_CAP_DMA_WRITE_INT_STATUS_OFF_t dma_write_int_status;
        dma_write_int_status.R = PCIE0->PF0_DMA_CAP.DMA_WRITE_INT_STATUS_OFF.R;

        uint32_t done_status = dma_write_int_status.B.WR_DONE_INT_STATUS;

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
        PCIE0->PF0_DMA_CAP.DMA_READ_INT_CLEAR_OFF.R = (PE0_DWC_pcie_ctl_DBI_Slave_PF0_DMA_CAP_DMA_READ_INT_CLEAR_OFF_t){ .B = {
            .RD_DONE_INT_CLEAR = ((1U << chan) & 0xF)
        }}.R;
    }
    else if (chanU32 >= DMA_CHAN_WRITE_0 && chanU32 <= DMA_CHAN_WRITE_3) {
        PCIE0->PF0_DMA_CAP.DMA_WRITE_INT_CLEAR_OFF.R = (PE0_DWC_pcie_ctl_DBI_Slave_PF0_DMA_CAP_DMA_WRITE_INT_CLEAR_OFF_t){ .B = {
            .WR_DONE_INT_CLEAR = ((1U << (chan - DMA_CHAN_WRITE_0)) & 0xF)
        }}.R;
    }
    else {
        printf("Invalid DMA channel %d\r\n", chanU32);
    }
}
