/***********************************************************************
*
* Copyright (C) 2022 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************

************************************************************************
*
*   DESCRIPTION
*
*       This file consists of tests related to DMAW.
*
*   FUNCTIONS
*
*       DMAW_Tests
*
***********************************************************************/
#include <stddef.h>
#include <stdint.h>

#include "services/log.h"
#include "workers/dmaw.h"
#include "error_codes.h"

#include "dmaw_tests.h"

typedef struct dma_info_ {
    uint64_t src_addr;
    uint64_t dst_addr;
    uint32_t size;
} dma_info_t;

/* Local functions */
int32_t dma_test_read(
    dma_read_chan_id_e read_chan, uint8_t dma_nodes, dma_info_t *dma_trans, bool test_abort);
int32_t dma_test_write(
    dma_write_chan_id_e write_chan, uint8_t dma_nodes, dma_info_t *dma_trans, bool test_abort);

void DMAW_Tests(uint32_t hart_id)
{
    /* Add some delay */
    int64_t delay = 999999999;
    while (delay)
    {
        delay--;
    }

    Log_Write(LOG_LEVEL_CRITICAL, "DMAW_Tests:H%d\r\n", hart_id);

    uint8_t dma_nodes = 4;
    uint32_t index = 0;
    dma_info_t dma_trans[dma_nodes];
    uint64_t src_base_addr = 0x8100000000;
    uint64_t dst_base_addr = 0x8200000000;
    uint32_t size = 0x8000000; /* 128 MB */
    dma_read_chan_id_e read_chan = 0;
    dma_write_chan_id_e write_chan = 0;

    Log_Write(LOG_LEVEL_CRITICAL, "DMAW_Tests:Creating test transactions\r\n");

    /* Fill in the test DMA transactions */
    for (index = 0; index < dma_nodes; index++)
    {
        dma_trans[index].src_addr = src_base_addr + (index * size);
        dma_trans[index].dst_addr = dst_base_addr + (index * size);
        dma_trans[index].size = size;
    }

    /***********************
    * DMA Read Abort Tests *
    ************************/

    for (index = 0; index < 10; index++)
    {
        Log_Write(LOG_LEVEL_CRITICAL, "DMAW_Tests:DMA_Read:Testing Abort\r\n");

        /* Doing DMA reads with abort */
        dma_test_read(read_chan, dma_nodes, dma_trans, true);
    }

    Log_Write(LOG_LEVEL_CRITICAL, "DMAW_Tests:Verifying the state of DMA read channel!\r\n");

    /* Doing DMA reads without abort */
    dma_test_read(read_chan, dma_nodes, dma_trans, false);

    Log_Write(LOG_LEVEL_CRITICAL, "DMAW_Tests:DMA Read tests Completed!\r\n");

    /***********************
    * DMA Write Abort Tests *
    ************************/

    for (index = 0; index < 10; index++)
    {
        Log_Write(LOG_LEVEL_CRITICAL, "DMAW_Tests:DMA_Write:Testing Abort\r\n");

        /* Doing DMA write with abort */
        dma_test_write(write_chan, dma_nodes, dma_trans, true);
    }

    Log_Write(LOG_LEVEL_CRITICAL, "DMAW_Tests:Verifying the state of DMA write channel!\r\n");

    /* Doing DMA write without abort */
    dma_test_write(write_chan, dma_nodes, dma_trans, false);

    Log_Write(LOG_LEVEL_CRITICAL, "DMAW_Tests:DMA Write tests Completed!\r\n");

    while (1)
    {
        asm volatile("wfi");
    }
}

int32_t dma_test_read(
    dma_read_chan_id_e read_chan, uint8_t dma_nodes, dma_info_t *dma_trans, bool test_abort)
{
    int32_t status = STATUS_SUCCESS;
    uint32_t dma_read_status;
    bool dma_read_done = false;
    bool dma_read_aborted = false;
    uint8_t last_i = (uint8_t)(dma_nodes - 1);

    for (uint8_t xfer_index = 0; (xfer_index <= last_i) && (status == STATUS_SUCCESS); ++xfer_index)
    {
        /* Add DMA list data node for current transfer in the list. Enable completion interrupt for last transfer node. */
        status = dma_config_read_add_data_node(dma_trans[xfer_index].src_addr,
            dma_trans[xfer_index].dst_addr, dma_trans[xfer_index].size, read_chan, xfer_index,
            (xfer_index == last_i ? true : false));
    }

    if (status == STATUS_SUCCESS)
    {
        /* Add DMA list link node at the end ot transfer list. */
        status = dma_config_read_add_link_node(read_chan, dma_nodes);

        if (status == STATUS_SUCCESS)
        {
            /* Start the DMA channel */
            status = dma_start_read(read_chan);

            if (status == STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_CRITICAL, "DMAW_Tests:DMA read chan started!\r\n");
            }
        }
        else
        {
            Log_Write(LOG_LEVEL_CRITICAL, "DMAW_Tests:DMA Read link node config failed!\r\n");
        }
    }
    else
    {
        Log_Write(LOG_LEVEL_CRITICAL, "DMAW_Tests:DMA Read data node config failed!\r\n");
    }

    bool break_loop = false;
    bool aborted = false;
    while (!break_loop)
    {
        Log_Write(LOG_LEVEL_DEBUG, "DMAW_Tests:DMA Read active!\r\n");

        dma_read_status = dma_get_read_int_status();
        dma_read_done = dma_check_read_done(read_chan, dma_read_status);
        if (!dma_read_done)
        {
            dma_read_aborted = dma_check_read_abort(read_chan, dma_read_status);
        }

        if (dma_read_done || dma_read_aborted)
        {
            if (dma_read_done)
            {
                /* DMA transfer complete, clear interrupt status */
                dma_clear_read_done(read_chan);
                Log_Write(LOG_LEVEL_CRITICAL, "DMAW_Tests:Read Transfer Completed\r\n");
            }
            else
            {
                /* DMA transfer aborted, clear interrupt status */
                dma_clear_read_abort(read_chan);
                dma_configure_read(read_chan);
                Log_Write(LOG_LEVEL_ERROR, "DMAW_Tests:Read Transfer Error Aborted\r\n");
                status = ~STATUS_SUCCESS;
            }
            break_loop = true;
            continue;
        }

        if (test_abort)
        {
            int64_t delay = 9999999;
            while (delay)
            {
                delay--;
            }
            int32_t dma_status;

            /* Abort the channel */
            dma_status = dma_abort_read(read_chan);

            if (dma_status == STATUS_SUCCESS)
            {
                /* DMA transfer aborted, clear interrupt status */
                dma_status = dma_clear_read_abort(read_chan);
                dma_status |= dma_configure_read(read_chan);
                Log_Write(LOG_LEVEL_CRITICAL, "DMAW_Tests:Read Transfer Aborted\r\n");
            }

            if (dma_status != STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_CRITICAL, "DMAW_Tests:Read Transfer Abort failed\r\n");
                status = ~STATUS_SUCCESS;
            }
            aborted = true;
            break_loop = true;
        }
    }

    if (test_abort && !aborted)
    {
        Log_Write(LOG_LEVEL_CRITICAL, "DMAW_Tests:Read chan not aborted\r\n");
    }

    return status;
}

int32_t dma_test_write(
    dma_write_chan_id_e write_chan, uint8_t dma_nodes, dma_info_t *dma_trans, bool test_abort)
{
    int32_t status = STATUS_SUCCESS;
    uint32_t dma_write_status;
    bool dma_write_done = false;
    bool dma_write_aborted = false;
    uint8_t last_i = (uint8_t)(dma_nodes - 1);

    for (uint8_t xfer_index = 0; (xfer_index <= last_i) && (status == STATUS_SUCCESS); ++xfer_index)
    {
        /* Add DMA list data node for current transfer in the list. Enable completion interrupt for last transfer node. */
        status = dma_config_write_add_data_node(dma_trans[xfer_index].src_addr,
            dma_trans[xfer_index].dst_addr, dma_trans[xfer_index].size, write_chan, xfer_index,
            DMA_NORMAL, (xfer_index == last_i ? true : false));
    }

    if (status == STATUS_SUCCESS)
    {
        /* Add DMA list link node at the end ot transfer list. */
        status = dma_config_write_add_link_node(write_chan, dma_nodes);

        if (status == STATUS_SUCCESS)
        {
            /* Start the DMA channel */
            status = dma_start_write(write_chan);

            if (status == STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_CRITICAL, "DMAW_Tests:DMA write chan started!\r\n");
            }
        }
        else
        {
            Log_Write(LOG_LEVEL_CRITICAL, "DMAW_Tests:DMA write link node config failed!\r\n");
        }
    }
    else
    {
        Log_Write(LOG_LEVEL_CRITICAL, "DMAW_Tests:DMA write data node config failed!\r\n");
    }

    bool break_loop = false;
    bool aborted = false;
    while (!break_loop)
    {
        Log_Write(LOG_LEVEL_DEBUG, "DMAW_Tests:DMA write active!\r\n");

        dma_write_status = dma_get_write_int_status();
        dma_write_done = dma_check_write_done(write_chan, dma_write_status);
        if (!dma_write_done)
        {
            dma_write_aborted = dma_check_write_abort(write_chan, dma_write_status);
        }

        if (dma_write_done || dma_write_aborted)
        {
            if (dma_write_done)
            {
                /* DMA transfer complete, clear interrupt status */
                dma_clear_write_done(write_chan);
                Log_Write(LOG_LEVEL_CRITICAL, "DMAW_Tests:write Transfer Completed\r\n");
            }
            else
            {
                /* DMA transfer aborted, clear interrupt status */
                dma_clear_write_abort(write_chan);
                dma_configure_write(write_chan);
                Log_Write(LOG_LEVEL_ERROR, "DMAW_Tests:write Transfer Error Aborted\r\n");
                status = ~STATUS_SUCCESS;
            }
            break_loop = true;
            continue;
        }

        if (test_abort)
        {
            int64_t delay = 9999999;
            while (delay)
            {
                delay--;
            }
            int32_t dma_status;

            /* Abort the channel */
            dma_status = dma_abort_write(write_chan);

            if (dma_status == STATUS_SUCCESS)
            {
                /* DMA transfer aborted, clear interrupt status */
                dma_status = dma_clear_write_abort(write_chan);
                dma_status |= dma_configure_write(write_chan);
                Log_Write(LOG_LEVEL_CRITICAL, "DMAW_Tests:write Transfer Aborted\r\n");
            }

            if (dma_status != STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_CRITICAL, "DMAW_Tests:write Transfer Abort failed\r\n");
                status = ~STATUS_SUCCESS;
            }
            aborted = true;
            break_loop = true;
        }
    }

    if (test_abort && !aborted)
    {
        Log_Write(LOG_LEVEL_CRITICAL, "DMAW_Tests:write chan not aborted\r\n");
    }

    return status;
}
