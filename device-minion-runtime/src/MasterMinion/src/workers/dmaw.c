/***********************************************************************
*
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
/***********************************************************************/
/*! \file dmaw.c
    \brief A C module that implements the DMA Worker's public and private
    interfaces. Two DMAWs are used in the Master Minion SW architecture,
    one dedicated for processing data move commands that engage DMA write
    channels, and other DMAW for processing data move commands that engage
    DMA read channels.
    This module implements:
    1. DMAW_Launch - An infinite loop that polls for completion of DMA
    transactions on active DMA channels. On detecting completion of
    a an configured DMA transaction, a DMA complete response is
    constructed and transmitted to host.
    2. It implements, and exposes the below listed public interfaces to
    other master shire runtime components present in the system
    to facilitate DMAW management

    Public interfaces:
        DMAW_Init
        DMAW_Read_Find_Idle_Chan_And_Reserve
        DMAW_Write_Find_Idle_Chan_And_Reserve
        DMAW_Read_Trigger_Transfer
        DMAW_Write_Trigger_Transfer
        DMAW_Launch
        DMAW_Abort_All_Dispatched_Read_Channels
        DMAW_Abort_All_Dispatched_Write_Channels
*/
/***********************************************************************/
/* mm_rt_svcs */
#include <etsoc/drivers/pmu/pmu.h>
#include <etsoc/isa/sync.h>

/* mm specific headers */
#include "workers/dmaw.h"
#include "workers/sqw.h"
#include "workers/statw.h"
#include "services/log.h"
#include "services/host_iface.h"
#include "services/sp_iface.h"
#include "services/trace.h"

/* mm_rt_helpers */
#include "error_codes.h"

/*! \struct dmaw_read_cb_t
    \brief DMA Worker Read Control Block structure.
    Used to maintain DMA Worker Read related resources.
*/
typedef struct dmaw_read_cb {
    dma_channel_status_cb_t chan_status_cb[PCIE_DMA_RD_CHANNEL_COUNT];
} dmaw_read_cb_t;

/*! \struct dmaw_write_cb_t
    \brief DMA Worker Write Control Block structure.
    Used to maintain DMA Worker Write related resources.
*/
typedef struct dmaw_write_cb {
    dma_channel_status_cb_t chan_status_cb[PCIE_DMA_WRT_CHANNEL_COUNT];
} dmaw_write_cb_t;

/*! \var dmaw_read_cb_t DMAW_Read_CB
    \brief Global DMA Read Control Block
    \warning Not thread safe!, used by minions in
    master shire, use local atomics for access.
*/
static dmaw_read_cb_t DMAW_Read_CB __attribute__((aligned(64))) = { 0 };

/*! \var dmaw_write_cb_t DMAW_Write_CB
    \brief Global DMA Write Control Block
    \warning Not thread safe!, used by minions in
    master shire, use local atomics for access.
*/
static dmaw_write_cb_t DMAW_Write_CB __attribute__((aligned(64))) = { 0 };

/*! \def DMAW_BYTES_PER_CYCLE_TO_MBPS
    \brief A helper macro to convert DMA bandwidth to MB/Sec using following formulae
           Total DMA BW = (Total Transfer size (in bytes) / num of bytes in 1MB) / (DMA duration cycles / Minion Freq)
*/
#define DMAW_BYTES_PER_CYCLE_TO_MBPS(bytes, cycles) \
    ((bytes * STATW_MINION_FREQ) / (cycles * STATW_NUM_OF_BYTES_IN_1MB))

/************************************************************************
*
*   FUNCTION
*
*       DMAW_Init
*
*   DESCRIPTION
*
*       Initialize DMA Worker
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void DMAW_Init(void)
{
    dma_channel_status_t chan_status;

    /* Initialize the values */
    chan_status.tag_id = 0;
    chan_status.sqw_idx = 0;
    chan_status.channel_state = DMA_CHAN_STATE_IDLE;

    /* Initialize DMA Read channel status */
    for (int i = 0; i < PCIE_DMA_RD_CHANNEL_COUNT; i++)
    {
        /* Store tag id, channel state and sqw idx */
        atomic_store_local_64(&DMAW_Read_CB.chan_status_cb[i].status.raw_u64, chan_status.raw_u64);
    }

    /* Initialize DMA Write channel status */
    for (int i = 0; i < PCIE_DMA_WRT_CHANNEL_COUNT; i++)
    {
        /* Store tag id, channel state and sqw idx */
        atomic_store_local_64(&DMAW_Write_CB.chan_status_cb[i].status.raw_u64, chan_status.raw_u64);
    }

    return;
}

/************************************************************************
*
*   FUNCTION
*
*       DMAW_Read_Find_Idle_Chan_And_Reserve
*
*   DESCRIPTION
*
*       Finds an idle DMA read channel and reserves it. This
*       function returns the absolute DMA channel id.
*
*   INPUTS
*
*       chan_id    Pointer to DMA channel ID
*       sqw_idx    Submission queue index
*
*   OUTPUTS
*
*       int32_t     status success or error
*
***********************************************************************/
int32_t DMAW_Read_Find_Idle_Chan_And_Reserve(dma_read_chan_id_e *chan_id, uint8_t sqw_idx)
{
    int32_t status = STATUS_SUCCESS;
    bool read_chan_reserved = false;
    sqw_state_e sqw_state;

    /* Try to find idle channel until aborted */
    do
    {
        /* Find the idle channel and reserve it */
        for (uint8_t ch = 0; ch < PCIE_DMA_RD_CHANNEL_COUNT; ch++)
        {
            /* Compare for idle state and reserve */
            if (atomic_compare_and_exchange_local_32(
                    &DMAW_Read_CB.chan_status_cb[ch].status.channel_state, DMA_CHAN_STATE_IDLE,
                    DMA_CHAN_STATE_RESERVED) == DMA_CHAN_STATE_IDLE)
            {
                /* Return the DMA channel ID */
                *chan_id = ch;
                read_chan_reserved = true;
                break;
            }
        }

        /* Read the SQW state */
        sqw_state = SQW_Get_State(sqw_idx);
    } while (!read_chan_reserved && (sqw_state != SQW_STATE_ABORTED));

    /* Verify SQW state */
    if (sqw_state == SQW_STATE_ABORTED)
    {
        status = DMAW_ABORTED_IDLE_CHANNEL_SEARCH;
        Log_Write(LOG_LEVEL_ERROR, "DMAW:ABORTED:Idle read channel search\r\n");

        /* Unreserve the channel */
        if (read_chan_reserved)
        {
            atomic_store_local_32(
                &DMAW_Read_CB.chan_status_cb[*chan_id].status.channel_state, DMA_CHAN_STATE_IDLE);
        }
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       DMAW_Write_Find_Idle_Chan_And_Reserve
*
*   DESCRIPTION
*
*       Finds an idle DMA write channel and reserves it. This
*       function returns the absolute DMA channel id.
*
*   INPUTS
*
*       chan_id    Pointer to DMA channel ID
*       sqw_idx    Submission queue index
*
*   OUTPUTS
*
*       int32_t     status success or error
*
***********************************************************************/
int32_t DMAW_Write_Find_Idle_Chan_And_Reserve(dma_write_chan_id_e *chan_id, uint8_t sqw_idx)
{
    int32_t status = STATUS_SUCCESS;
    bool write_chan_reserved = false;
    sqw_state_e sqw_state;

    /* Try to find idle channel until aborted */
    do
    {
        /* Find the idle channel and reserve it */
        for (uint8_t ch = 0; ch < PCIE_DMA_WRT_CHANNEL_COUNT; ch++)
        {
            /* Compare for idle state and reserve */
            if (atomic_compare_and_exchange_local_32(
                    &DMAW_Write_CB.chan_status_cb[ch].status.channel_state, DMA_CHAN_STATE_IDLE,
                    DMA_CHAN_STATE_RESERVED) == DMA_CHAN_STATE_IDLE)
            {
                /* Return the DMA channel ID */
                *chan_id = ch;
                write_chan_reserved = true;
                break;
            }
        }
        /* Read the SQW state */
        sqw_state = SQW_Get_State(sqw_idx);
    } while (!write_chan_reserved && (sqw_state != SQW_STATE_ABORTED));

    /* Verify SQW state */
    if (sqw_state == SQW_STATE_ABORTED)
    {
        status = DMAW_ABORTED_IDLE_CHANNEL_SEARCH;
        Log_Write(LOG_LEVEL_ERROR, "DMAW:ABORTED:Idle write channel search\r\n");

        /* Unreserve the channel */
        if (write_chan_reserved)
        {
            atomic_store_local_32(
                &DMAW_Write_CB.chan_status_cb[*chan_id].status.channel_state, DMA_CHAN_STATE_IDLE);
        }
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       DMAW_Read_Trigger_Transfer
*
*   DESCRIPTION
*
*       This function is used to trigger a DMA Read transaction by calling
*       the PCIe device driver routine.
*
*   INPUTS
*
*       read_chan_id    DMA channel ID
*       cmd             Pointer to command buffer
*       xfer_count      Number of transfer nodes in command.
*       sqw_idx         SQW ID
*       cycles          Pointer to latency cycles struct
*
*   OUTPUTS
*
*       int32_t          status success or error
*
***********************************************************************/
int32_t DMAW_Read_Trigger_Transfer(dma_read_chan_id_e read_chan_id,
    const struct device_ops_dma_writelist_cmd_t *cmd, uint8_t xfer_count, uint8_t sqw_idx,
    const execution_cycles_t *cycles)
{
    uint64_t transfer_size = 0;
    int32_t status = STATUS_SUCCESS;
    dma_channel_status_t chan_status;

    /* Set tag ID, set channel state to active, set SQW Index */
    chan_status.tag_id = cmd->command_info.cmd_hdr.tag_id;
    chan_status.sqw_idx = sqw_idx;
    chan_status.channel_state = DMA_CHAN_STATE_IN_USE;

    /* TODO: SW-9022: To be removed */
    uint16_t t_msg_id = cmd->command_info.cmd_hdr.msg_id;
    atomic_store_local_16(&DMAW_Read_CB.chan_status_cb[read_chan_id].msg_id, t_msg_id);

    /* To start indexing from zero, decrement the count by one. */
    uint8_t last_i = (uint8_t)(xfer_count - 1);

    /* Configure DMA for all transfers one-by-one. */
    for (uint8_t xfer_index = 0; (xfer_index <= last_i) && (status == STATUS_SUCCESS); ++xfer_index)
    {
        /* Add DMA list data node for current transfer in the list.Enable completion interrupt for last transfer node. */
        status = dma_config_read_add_data_node(cmd->list[xfer_index].src_host_phy_addr,
            cmd->list[xfer_index].dst_device_phy_addr, cmd->list[xfer_index].size, read_chan_id,
            xfer_index, (xfer_index == last_i ? true : false));

        transfer_size += cmd->list[xfer_index].size;

        if (status == DMA_DRIVER_ERROR_INVALID_ADDRESS)
        {
            Log_Write(LOG_LEVEL_ERROR, "SQ[%d]:TID:%u:DMAW:Config:Invld dst Address 0x%lx\r\n",
                sqw_idx, cmd->command_info.cmd_hdr.tag_id,
                cmd->list[xfer_index].dst_device_phy_addr);

            status = DMAW_ERROR_DRIVER_INAVLID_DEV_ADDRESS;
        }
        else if (status != STATUS_SUCCESS)
        {
            Log_Write(LOG_LEVEL_ERROR,
                "SQ[%d]:TID:%u:DMAW_Read:Config:Add data node failed:Status:%d\r\n", sqw_idx,
                cmd->command_info.cmd_hdr.tag_id, status);

            status = DMAW_ERROR_DRIVER_DATA_CONFIG_FAILED;
        }

        Log_Write(LOG_LEVEL_DEBUG, "DMAW_Read:Config:Added read data node No:%u\r\n", xfer_index);
    }

    if (status == STATUS_SUCCESS)
    {
        /* Add DMA list link node at the end ot transfer list. */
        status = dma_config_read_add_link_node(read_chan_id, xfer_count);

        if (status != STATUS_SUCCESS)
        {
            Log_Write(LOG_LEVEL_ERROR,
                "SQ[%d]:TID:%u:DMAW_Read:Config: Add link node failed: Status: %d\r\n", sqw_idx,
                cmd->command_info.cmd_hdr.tag_id, status);
            status = DMAW_ERROR_DRIVER_LINK_CONFIG_FAILED;
        }

        Log_Write(LOG_LEVEL_DEBUG,
            "DMAW_Read:Config:Added DMA red Link node at the end of list transfer.\r\n");
    }

    if (status == STATUS_SUCCESS)
    {
        /* Start the DMA channel */
        status = dma_start_read(read_chan_id);

        if (status != STATUS_SUCCESS)
        {
            Log_Write(LOG_LEVEL_DEBUG, "SQ[%d] Failed to started DMA read chanel:Status:%d!\r\n",
                sqw_idx, status);
            status = DMAW_ERROR_DRIVER_CHAN_START_FAILED;
        }
    }

    if (status == STATUS_SUCCESS)
    {
        /* Log the command state in trace */
        TRACE_LOG_CMD_STATUS(cmd->command_info.cmd_hdr.msg_id, sqw_idx,
            cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_EXECUTING)

        /* Update cycles value into the Global Channel Status data structure */
        atomic_store_local_64(
            &DMAW_Read_CB.chan_status_cb[read_chan_id].dmaw_cycles.cmd_start_cycles,
            cycles->cmd_start_cycles);
        atomic_store_local_64(
            &DMAW_Read_CB.chan_status_cb[read_chan_id].dmaw_cycles.exec_start_cycles,
            cycles->exec_start_cycles);
        atomic_store_local_32(&DMAW_Read_CB.chan_status_cb[read_chan_id].dmaw_cycles.wait_cycles,
            cycles->wait_cycles);

        /* Update the global structure to make it visible to DMAW */
        atomic_store_local_64(
            &DMAW_Read_CB.chan_status_cb[read_chan_id].status.raw_u64, chan_status.raw_u64);
        atomic_store_local_64(
            &DMAW_Read_CB.chan_status_cb[read_chan_id].transfer_size, transfer_size);

        Log_Write(LOG_LEVEL_DEBUG, "SQ[%d] DMAW_Read_Trigger_Transfer:Success!\r\n", sqw_idx);

        status = STATUS_SUCCESS;
    }
    else
    {
        /* Release the DMA resources */
        chan_status.tag_id = 0;
        chan_status.sqw_idx = 0;
        chan_status.channel_state = DMA_CHAN_STATE_IDLE;

        atomic_store_local_64(
            &DMAW_Read_CB.chan_status_cb[read_chan_id].status.raw_u64, chan_status.raw_u64);

        Log_Write(LOG_LEVEL_ERROR, "SQ[%d]:TID:%u:DMAW Read Config Failed:%d!\r\n", sqw_idx,
            cmd->command_info.cmd_hdr.tag_id, status);

        SP_Iface_Report_Error(MM_RECOVERABLE_FW_MM_DMAW_ERROR, MM_DMA_WRITE_CONFIG_ERROR);
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       DMAW_Write_Trigger_Transfer
*
*   DESCRIPTION
*
*       This function is used to trigger a DMA write transaction by calling
*       the PCIe device driver routine.
*
*   INPUTS
*
*       write_chan_id   DMA channel ID
*       cmd             Pointer to command buffer
*       xfer_count      Number of transfer nodes in command.
*       sqw_idx         SQW ID
*       cycles          Pointer to latency cycles struct
*       flags           DMA flag to set a specific DMA action.
*
*   OUTPUTS
*
*       int32_t          status success or error
*
***********************************************************************/
int32_t DMAW_Write_Trigger_Transfer(dma_write_chan_id_e write_chan_id,
    const struct device_ops_dma_readlist_cmd_t *cmd, uint8_t xfer_count, uint8_t sqw_idx,
    const execution_cycles_t *cycles, dma_flags_e flags)
{
    uint64_t transfer_size = 0;
    int32_t status = STATUS_SUCCESS;
    dma_channel_status_t chan_status;

    /* Set tag ID, set channel state to active, set SQW Index */
    chan_status.tag_id = cmd->command_info.cmd_hdr.tag_id;
    chan_status.sqw_idx = sqw_idx;
    chan_status.channel_state = DMA_CHAN_STATE_IN_USE;

    /* TODO: SW-9022: To be removed */
    uint16_t t_msg_id = cmd->command_info.cmd_hdr.msg_id;
    atomic_store_local_16(&DMAW_Write_CB.chan_status_cb[write_chan_id].msg_id, t_msg_id);

    /* To start indexing from zero, decrement the count by one. */
    uint8_t last_i = (uint8_t)(xfer_count - 1);

    /* Configure DMA for all transfers one-by-one. */
    for (uint8_t xfer_index = 0; (xfer_index <= last_i) && (status == STATUS_SUCCESS); ++xfer_index)
    {
        /* Add DMA list data node for current transfer in the list. Enable completion interrupt for last transfer node.*/
        status = dma_config_write_add_data_node(cmd->list[xfer_index].src_device_phy_addr,
            cmd->list[xfer_index].dst_host_phy_addr, cmd->list[xfer_index].size, write_chan_id,
            xfer_index, flags, (xfer_index == last_i ? true : false));

        transfer_size += cmd->list[xfer_index].size;

        if (status == DMA_DRIVER_ERROR_INVALID_ADDRESS)
        {
            Log_Write(LOG_LEVEL_ERROR,
                "SQ[%d]:TID:%u:DMAW_Write:Config:Invld src Address 0x%lx\r\n", sqw_idx,
                cmd->command_info.cmd_hdr.tag_id, cmd->list[xfer_index].src_device_phy_addr);
            status = DMAW_ERROR_DRIVER_INAVLID_DEV_ADDRESS;
        }
        else if (status != STATUS_SUCCESS)
        {
            Log_Write(LOG_LEVEL_ERROR,
                "SQ[%d]:TID:%u:DMAW_Write:Config:Add data node failed:Status:%d\r\n", sqw_idx,
                cmd->command_info.cmd_hdr.tag_id, status);
            status = DMAW_ERROR_DRIVER_DATA_CONFIG_FAILED;
        }

        Log_Write(LOG_LEVEL_DEBUG, "DMAW_Write:Config:Added write data node No:%u\r\n", xfer_index);
    }

    if (status == STATUS_SUCCESS)
    {
        /* Add DMA list link node at the end ot transfer list. */
        status = dma_config_write_add_link_node(write_chan_id, xfer_count);

        if (status != STATUS_SUCCESS)
        {
            Log_Write(LOG_LEVEL_ERROR,
                "SQ[%d]:TID:%u:DMAW:Config: Add link node failed: Stauts: %d\r\n", sqw_idx,
                cmd->command_info.cmd_hdr.tag_id, status);
            status = DMAW_ERROR_DRIVER_LINK_CONFIG_FAILED;
        }

        Log_Write(LOG_LEVEL_DEBUG,
            "DMAW:Config:Added DMA write Link node at the end of list transfer.\r\n");
    }

    if (status == STATUS_SUCCESS)
    {
        /* Start the DMA channel */
        status = dma_start_write(write_chan_id);

        if (status != STATUS_SUCCESS)
        {
            Log_Write(LOG_LEVEL_DEBUG, "SQ[%d] Failed to started DMA write chanel:Status:%d!\r\n",
                sqw_idx, status);
            status = DMAW_ERROR_DRIVER_CHAN_START_FAILED;
        }
    }

    if (status == STATUS_SUCCESS)
    {
        /* Log the command state in trace */
        TRACE_LOG_CMD_STATUS(cmd->command_info.cmd_hdr.msg_id, sqw_idx,
            cmd->command_info.cmd_hdr.tag_id, CMD_STATUS_EXECUTING)

        /* Update cycles value into the Global Channel Status data structure */
        atomic_store_local_64(
            &DMAW_Write_CB.chan_status_cb[write_chan_id].dmaw_cycles.cmd_start_cycles,
            cycles->cmd_start_cycles);
        atomic_store_local_64(
            &DMAW_Write_CB.chan_status_cb[write_chan_id].dmaw_cycles.exec_start_cycles,
            cycles->exec_start_cycles);
        atomic_store_local_32(&DMAW_Write_CB.chan_status_cb[write_chan_id].dmaw_cycles.wait_cycles,
            cycles->wait_cycles);

        /* Update the global structure to make it visible to DMAW */
        atomic_store_local_64(
            &DMAW_Write_CB.chan_status_cb[write_chan_id].status.raw_u64, chan_status.raw_u64);
        atomic_store_local_64(
            &DMAW_Write_CB.chan_status_cb[write_chan_id].transfer_size, transfer_size);

        Log_Write(LOG_LEVEL_DEBUG, "SQ[%d]:DMAW_Write_Trigger_Transfer:Success!\r\n", sqw_idx);

        status = STATUS_SUCCESS;
    }
    else
    {
        /* Release the DMA resources */
        chan_status.tag_id = 0;
        chan_status.sqw_idx = 0;
        chan_status.channel_state = DMA_CHAN_STATE_IDLE;

        atomic_store_local_64(
            &DMAW_Write_CB.chan_status_cb[write_chan_id].status.raw_u64, chan_status.raw_u64);

        Log_Write(LOG_LEVEL_ERROR, "SQ[%d]:TID:%u:DMAW Write Config Failed:%d!\r\n", sqw_idx,
            cmd->command_info.cmd_hdr.tag_id, status);

        SP_Iface_Report_Error(MM_RECOVERABLE_FW_MM_DMAW_ERROR, MM_DMA_WRITE_CONFIG_ERROR);
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       process_dma_read_chan_in_use
*
*   DESCRIPTION
*
*       Helper function to process DMA read chan when it is busy. It polls status
*       of DMA channel if it has completed transfer, then this function sends
*       corresponding response in Host CQ
*
*   INPUTS
*
*       dma_read_chan_id_e           DMA read channel ID
*       device_ops_data_write_rsp_t  Pointer to buffer for DMA response.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static inline void process_dma_read_chan_in_use(
    dma_read_chan_id_e read_chan, struct device_ops_data_write_rsp_t *write_rsp)
{
    dma_channel_status_t read_chan_status;
    execution_cycles_t dma_rd_cycles;
    uint32_t dma_read_status;
    bool dma_read_done = false;
    bool dma_read_aborted = false;
    int32_t status = STATUS_SUCCESS;
    uint16_t msg_id; /* TODO: SW-9022: To be removed */

    dma_read_status = dma_get_read_int_status();
    dma_read_done = dma_check_read_done(read_chan, dma_read_status);
    if (!dma_read_done)
    {
        dma_read_aborted = dma_check_read_abort(read_chan, dma_read_status);
    }

    if (dma_read_done || dma_read_aborted)
    {
        /* Read the channel status from CB */
        read_chan_status.raw_u64 =
            atomic_load_local_64(&DMAW_Read_CB.chan_status_cb[read_chan].status.raw_u64);

        if (dma_read_done)
        {
            /* DMA transfer complete, clear interrupt status */
            dma_clear_read_done(read_chan);
            write_rsp->status = DEV_OPS_API_DMA_RESPONSE_COMPLETE;
            Log_Write(LOG_LEVEL_DEBUG, "DMAW: Read Transfer Completed\r\n");
        }
        else
        {
            /* DMA transfer aborted, clear interrupt status */
            dma_clear_read_abort(read_chan);
            dma_configure_read(read_chan);
            write_rsp->status = DEV_OPS_API_DMA_RESPONSE_ERROR_ABORTED;
            Log_Write(LOG_LEVEL_ERROR, "DMAW:Tag_ID=%u:Read Transfer Aborted\r\n",
                read_chan_status.tag_id);
        }

        /* TODO: SW-9022: To be removed */
        msg_id = atomic_load_local_16(&DMAW_Read_CB.chan_status_cb[read_chan].msg_id);

        Log_Write(LOG_LEVEL_DEBUG, "SQ[%d] DMAW: Read Tag ID:%d Chan ID:%d \r\n",
            read_chan_status.sqw_idx, read_chan_status.tag_id, read_chan);
        /* Obtain wait latency, start cycles measured
        for the command and obtain current cycles */
        dma_rd_cycles.cmd_start_cycles = atomic_load_local_64(
            &DMAW_Read_CB.chan_status_cb[read_chan].dmaw_cycles.cmd_start_cycles);
        dma_rd_cycles.exec_start_cycles = atomic_load_local_64(
            &DMAW_Read_CB.chan_status_cb[read_chan].dmaw_cycles.exec_start_cycles);
        dma_rd_cycles.wait_cycles =
            atomic_load_local_32(&DMAW_Read_CB.chan_status_cb[read_chan].dmaw_cycles.wait_cycles);

        /* Update global DMA channel status
        NOTE: Channel state must be made idle once all resources are read */
        atomic_store_local_32(
            &DMAW_Read_CB.chan_status_cb[read_chan].status.channel_state, DMA_CHAN_STATE_IDLE);

        /* Decrement the commands count being processed by the
        given SQW. Should be done after clearing channel state */
        SQW_Decrement_Command_Count(read_chan_status.sqw_idx);

        /* Create and transmit DMA command response */
        write_rsp->response_info.rsp_hdr.size =
            sizeof(struct device_ops_data_write_rsp_t) - sizeof(struct cmn_header_t);
        write_rsp->response_info.rsp_hdr.tag_id = read_chan_status.tag_id;
        write_rsp->response_info.rsp_hdr.msg_id = (msg_id_t)(msg_id + 1U);
        /* TODO: SW-9022 To be enabled back
        write_rsp->response_info.rsp_hdr.msg_id =
            DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_RSP */
        write_rsp->device_cmd_start_ts = dma_rd_cycles.cmd_start_cycles;
        write_rsp->device_cmd_wait_dur = dma_rd_cycles.wait_cycles;
        /* Compute command execution latency */
        write_rsp->device_cmd_execute_dur = PMC_GET_LATENCY(dma_rd_cycles.exec_start_cycles);

        Log_Write(LOG_LEVEL_DEBUG, "DMAW:Pushing:DATA_WRITE_CMD_RSP:tag_id=%x->Host_CQ\r\n",
            write_rsp->response_info.rsp_hdr.tag_id);

        status = Host_Iface_CQ_Push_Cmd(0, write_rsp, sizeof(struct device_ops_data_write_rsp_t));

        /* Total DMA BW = (Total Transfer size (in bytes) / num of bytes in 1MB) / (DMA duration cycles / Minion Freq) */
        STATW_Add_New_Sample_Atomically(STATW_RESOURCE_DMA_READ,
            DMAW_BYTES_PER_CYCLE_TO_MBPS(
                atomic_load_local_64(&DMAW_Read_CB.chan_status_cb[read_chan].transfer_size),
                write_rsp->device_cmd_execute_dur));

        if (status != STATUS_SUCCESS)
        {
            TRACE_LOG_CMD_STATUS(
                msg_id, read_chan_status.sqw_idx, read_chan_status.tag_id, CMD_STATUS_FAILED)
            Log_Write(LOG_LEVEL_ERROR, "DMAW:Tag_ID=%u:HostIface:Push:Failed\r\n",
                read_chan_status.tag_id);
            SP_Iface_Report_Error(MM_RECOVERABLE_FW_MM_DMAW_ERROR, MM_CQ_PUSH_ERROR);
        }
        else
        {
            /* Log command status to trace */
            if (write_rsp->status == DEV_OPS_API_DMA_RESPONSE_COMPLETE)
            {
                TRACE_LOG_CMD_STATUS(
                    msg_id, read_chan_status.sqw_idx, read_chan_status.tag_id, CMD_STATUS_SUCCEEDED)
            }
            else
            {
                TRACE_LOG_CMD_STATUS(
                    msg_id, read_chan_status.sqw_idx, read_chan_status.tag_id, CMD_STATUS_FAILED)
            }
        }

        /* Check for device API error */
        if (write_rsp->status != DEV_OPS_API_DMA_RESPONSE_COMPLETE)
        {
            /* Report device API error to SP */
            SP_Iface_Report_Error(MM_RECOVERABLE_OPS_API_DMA_WRITELIST, (int16_t)write_rsp->status);
        }
    }
}

/************************************************************************
*
*   FUNCTION
*
*       process_dma_read_chan_aborting
*
*   DESCRIPTION
*
*       Helper function to process DMA read channel when it is in aborting state.
*       It aborts the DMA channel and reconfigure it in read mode, then this
*       function sends corresponding response in Host CQ
*
*   INPUTS
*
*       dma_read_chan_id_e           DMA read channel ID
*       device_ops_data_write_rsp_t  Pointer to buffer for DMA response.
*       channel_aborted              Pointer to array containing channel
*                                    abort flags
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static inline void process_dma_read_chan_aborting(dma_read_chan_id_e read_chan,
    struct device_ops_data_write_rsp_t *write_rsp, bool *channel_aborted)
{
    dma_channel_status_t read_chan_status;
    execution_cycles_t dma_read_cycles;
    uint32_t dma_read_status;
    int32_t status = STATUS_SUCCESS;
    int32_t dma_status = STATUS_SUCCESS;
    uint16_t msg_id; /* TODO: SW-9022: To be removed */

    if (!channel_aborted[read_chan])
    {
        channel_aborted[read_chan] = true;

        /* Abort the channel */
        dma_abort_read(read_chan);
    }

    /* Verify the channel status after abort */
    dma_read_status = dma_get_read_int_status();

    if (dma_check_read_done(read_chan, dma_read_status))
    {
        /* DMA transfer already completed */
        dma_clear_read_done(read_chan);
    }
    else if (dma_check_read_abort(read_chan, dma_read_status))
    {
        Log_Write(LOG_LEVEL_INFO,
            "DMAW_Read:Clearing abort status and re-configuring channel %d\r\n", read_chan);

        /* DMA transfer aborted, clear interrupt status */
        dma_status = dma_clear_read_abort(read_chan);
        dma_status |= dma_configure_read(read_chan);
    }
    else
    {
        /* Abort or done status not set, return */
        return;
    }

    /* Clear the channel abort flag */
    channel_aborted[read_chan] = false;

    if (dma_status != STATUS_SUCCESS)
    {
        status = DMAW_ERROR_DRIVER_ABORT_FAILED;
    }

    /* TODO: SW-9022: To be removed */
    msg_id = atomic_load_local_16(&DMAW_Read_CB.chan_status_cb[read_chan].msg_id);

    /* Read the channel status from CB */
    read_chan_status.raw_u64 =
        atomic_load_local_64(&DMAW_Read_CB.chan_status_cb[read_chan].status.raw_u64);

    /* Obtain wait latency, start cycles measured for the command
    and obtain current cycles */
    dma_read_cycles.cmd_start_cycles =
        atomic_load_local_64(&DMAW_Read_CB.chan_status_cb[read_chan].dmaw_cycles.cmd_start_cycles);
    dma_read_cycles.exec_start_cycles =
        atomic_load_local_64(&DMAW_Read_CB.chan_status_cb[read_chan].dmaw_cycles.exec_start_cycles);
    dma_read_cycles.wait_cycles =
        atomic_load_local_32(&DMAW_Read_CB.chan_status_cb[read_chan].dmaw_cycles.wait_cycles);

    /* Update global DMA channel status
    NOTE: Channel state must be made idle once all resources are read */
    atomic_store_local_32(
        &DMAW_Read_CB.chan_status_cb[read_chan].status.channel_state, DMA_CHAN_STATE_IDLE);

    /* Decrement the commands count being processed by the
    given SQW. Should be done after clearing channel state */
    SQW_Decrement_Command_Count(read_chan_status.sqw_idx);

    if (status == DMAW_ERROR_DRIVER_ABORT_FAILED)
    {
        write_rsp->status = DEV_OPS_API_DMA_RESPONSE_DRIVER_ABORT_FAILED;
    }
    else
    {
        write_rsp->status = DEV_OPS_API_DMA_RESPONSE_HOST_ABORTED;
    }

    /* Create and transmit DMA command response */
    write_rsp->response_info.rsp_hdr.size =
        sizeof(struct device_ops_data_write_rsp_t) - sizeof(struct cmn_header_t);
    write_rsp->response_info.rsp_hdr.tag_id = read_chan_status.tag_id;
    write_rsp->response_info.rsp_hdr.msg_id = (msg_id_t)(msg_id + 1U);
    /* TODO: SW-9022 To be enabled back
    write_rsp->response_info.rsp_hdr.msg_id =
        DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_RSP */
    write_rsp->device_cmd_start_ts = dma_read_cycles.cmd_start_cycles;
    write_rsp->device_cmd_wait_dur = dma_read_cycles.wait_cycles;
    /* Compute command execution latency */
    write_rsp->device_cmd_execute_dur = PMC_GET_LATENCY(dma_read_cycles.exec_start_cycles);

    status = Host_Iface_CQ_Push_Cmd(0, write_rsp, sizeof(struct device_ops_data_write_rsp_t));

    if (status == STATUS_SUCCESS)
    {
        /* Log command status to trace */
        TRACE_LOG_CMD_STATUS(
            msg_id, read_chan_status.sqw_idx, read_chan_status.tag_id, CMD_STATUS_ABORTED)

        Log_Write(LOG_LEVEL_DEBUG, "DMAW:Pushed:DATA_WRITE_CMD_RSP->Host_CQ\r\n");
    }
    else
    {
        TRACE_LOG_CMD_STATUS(
            msg_id, read_chan_status.sqw_idx, read_chan_status.tag_id, CMD_STATUS_FAILED)
        Log_Write(
            LOG_LEVEL_ERROR, "DMAW:Tag_ID=%u:HostIface:Push:Failed\r\n", read_chan_status.tag_id);
        SP_Iface_Report_Error(MM_RECOVERABLE_FW_MM_DMAW_ERROR, MM_CQ_PUSH_ERROR);
    }

    /* Report device API error to SP */
    SP_Iface_Report_Error(MM_RECOVERABLE_OPS_API_DMA_WRITELIST, (int16_t)write_rsp->status);
}

/************************************************************************
*
*   FUNCTION
*
*       process_dma_write_chan_in_use
*
*   DESCRIPTION
*
*       Helper function to process DMA write channel when it is busy. It polls
*       status of DMA channel if it has completed transfer, then this function
*       sends corresponding response in Host CQ
*
*   INPUTS
*
*       dma_write_chan_id_e          DMA write channel ID
*       device_ops_data_write_rsp_t  Pointer to buffer for DMA response.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static inline void process_dma_write_chan_in_use(
    dma_write_chan_id_e write_chan, struct device_ops_data_read_rsp_t *read_rsp)
{
    uint32_t dma_write_status;
    bool dma_write_done = false;
    bool dma_write_aborted = false;
    execution_cycles_t dma_write_cycles;
    dma_channel_status_t write_chan_status;
    int32_t status = STATUS_SUCCESS;
    uint16_t msg_id; /* TODO: SW-9022: To be removed */

    dma_write_status = dma_get_write_int_status();
    dma_write_done = dma_check_write_done(write_chan, dma_write_status);
    if (!dma_write_done)
    {
        dma_write_aborted = dma_check_write_abort(write_chan, dma_write_status);
    }

    if (dma_write_done || dma_write_aborted)
    {
        /* Read the channel status from CB */
        write_chan_status.raw_u64 =
            atomic_load_local_64(&DMAW_Write_CB.chan_status_cb[write_chan].status.raw_u64);

        if (dma_write_done)
        {
            /* DMA transfer complete, clear interrupt status */
            dma_clear_write_done(write_chan);
            read_rsp->status = DEV_OPS_API_DMA_RESPONSE_COMPLETE;
            Log_Write(LOG_LEVEL_DEBUG, "DMAW: Write Transfer Completed\r\n");
        }
        else
        {
            /* DMA transfer aborted, clear interrupt status */
            dma_clear_write_abort(write_chan);
            dma_configure_write(write_chan);
            read_rsp->status = DEV_OPS_API_DMA_RESPONSE_ERROR_ABORTED;
            Log_Write(LOG_LEVEL_ERROR, "DMAW:Tag_ID=%u:Write Transfer Aborted\r\n",
                write_chan_status.tag_id);
        }

        /* TODO: SW-9022: To be removed */
        msg_id = atomic_load_local_16(&DMAW_Write_CB.chan_status_cb[write_chan].msg_id);

        Log_Write(LOG_LEVEL_DEBUG, "SQ[%d] DMAW: Write Tag ID:%d Chan ID:%d \r\n",
            write_chan_status.sqw_idx, write_chan_status.tag_id, write_chan);
        /* Obtain wait latency, start cycles measured
        for the command and obtain current cycles */
        dma_write_cycles.cmd_start_cycles = atomic_load_local_64(
            &DMAW_Write_CB.chan_status_cb[write_chan].dmaw_cycles.cmd_start_cycles);
        dma_write_cycles.exec_start_cycles = atomic_load_local_64(
            &DMAW_Write_CB.chan_status_cb[write_chan].dmaw_cycles.exec_start_cycles);
        dma_write_cycles.wait_cycles =
            atomic_load_local_32(&DMAW_Write_CB.chan_status_cb[write_chan].dmaw_cycles.wait_cycles);

        /* Update global DMA channel status
        NOTE: Channel state must be made idle once all resources are read */
        atomic_store_local_32(
            &DMAW_Write_CB.chan_status_cb[write_chan].status.channel_state, DMA_CHAN_STATE_IDLE);

        /* Decrement the commands count being processed by the
        given SQW. Should be done after clearing channel state */
        SQW_Decrement_Command_Count(write_chan_status.sqw_idx);

        /* Create and transmit DMA command response */
        read_rsp->response_info.rsp_hdr.size =
            sizeof(struct device_ops_data_read_rsp_t) - sizeof(struct cmn_header_t);
        read_rsp->response_info.rsp_hdr.tag_id = write_chan_status.tag_id;
        read_rsp->response_info.rsp_hdr.msg_id = (msg_id_t)(msg_id + 1U);
        /* TODO: SW-9022 To be enabled back
        read_rsp->response_info.rsp_hdr.msg_id =
            DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_RSP */
        read_rsp->device_cmd_start_ts = dma_write_cycles.cmd_start_cycles;
        read_rsp->device_cmd_wait_dur = dma_write_cycles.wait_cycles;
        /* Compute command execution latency */
        read_rsp->device_cmd_execute_dur = PMC_GET_LATENCY(dma_write_cycles.exec_start_cycles);

        status = Host_Iface_CQ_Push_Cmd(0, read_rsp, sizeof(struct device_ops_data_read_rsp_t));

        /* Total DMA BW = (Total Transfer size (in bytes) / num of bytes in 1MB) / (DMA duration cycles / Minion Freq) */
        STATW_Add_New_Sample_Atomically(STATW_RESOURCE_DMA_WRITE,
            DMAW_BYTES_PER_CYCLE_TO_MBPS(
                atomic_load_local_64(&DMAW_Write_CB.chan_status_cb[write_chan].transfer_size),
                read_rsp->device_cmd_execute_dur));

        if (status == STATUS_SUCCESS)
        {
            /* Log command status to trace */
            if (read_rsp->status == DEV_OPS_API_DMA_RESPONSE_COMPLETE)
            {
                TRACE_LOG_CMD_STATUS(msg_id, write_chan_status.sqw_idx, write_chan_status.tag_id,
                    CMD_STATUS_SUCCEEDED)
            }
            else
            {
                TRACE_LOG_CMD_STATUS(
                    msg_id, write_chan_status.sqw_idx, write_chan_status.tag_id, CMD_STATUS_FAILED)
            }

            Log_Write(LOG_LEVEL_DEBUG, "DMAW:Pushed:DATA_READ_CMD_RSP:tag_id=%x->Host_CQ\r\n",
                read_rsp->response_info.rsp_hdr.tag_id);
        }
        else
        {
            TRACE_LOG_CMD_STATUS(
                msg_id, write_chan_status.sqw_idx, write_chan_status.tag_id, CMD_STATUS_FAILED)
            Log_Write(LOG_LEVEL_ERROR, "DMAW:Tag_ID=%u:HostIface:Push:Failed\r\n",
                write_chan_status.tag_id);
            SP_Iface_Report_Error(MM_RECOVERABLE_FW_MM_DMAW_ERROR, MM_CQ_PUSH_ERROR);
        }

        /* Check for device API error */
        if (read_rsp->status != DEV_OPS_API_DMA_RESPONSE_COMPLETE)
        {
            /* Report device API error to SP */
            SP_Iface_Report_Error(MM_RECOVERABLE_OPS_API_DMA_READLIST, (int16_t)read_rsp->status);
        }
    }
}

/************************************************************************
*
*   FUNCTION
*
*       process_dma_write_chan_aborting
*
*   DESCRIPTION
*
*       Helper function to process DMA write channel when it is in aborting state.
*       It aborts the DMA channel and reconfigure it in write mode, then this
*       function sends corresponding response in Host CQ
*
*   INPUTS
*
*       dma_write_chan_id_e          DMA write channel ID
*       device_ops_data_write_rsp_t  Pointer to buffer for DMA response.
*       channel_aborted              Pointer to array containing channel
*                                    abort flags
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static inline void process_dma_write_chan_aborting(dma_write_chan_id_e write_chan,
    struct device_ops_data_read_rsp_t *read_rsp, bool *channel_aborted)
{
    dma_channel_status_t write_chan_status;
    execution_cycles_t dma_write_cycles;
    uint32_t dma_write_status;
    int32_t status = STATUS_SUCCESS;
    int32_t dma_status = STATUS_SUCCESS;
    uint16_t msg_id; /* TODO: SW-9022: To be removed */

    if (!channel_aborted[write_chan])
    {
        channel_aborted[write_chan] = true;

        /* Abort the channel */
        dma_abort_write(write_chan);
    }

    /* Verify the channel status after abort */
    dma_write_status = dma_get_write_int_status();

    if (dma_check_write_done(write_chan, dma_write_status))
    {
        /* DMA transfer already completed */
        dma_clear_write_done(write_chan);
    }
    else if (dma_check_write_abort(write_chan, dma_write_status))
    {
        Log_Write(LOG_LEVEL_INFO,
            "DMAW_Write:Clearing abort status and re-configuring channel %d\r\n", write_chan);

        /* DMA transfer aborted, clear interrupt status */
        dma_status = dma_clear_write_abort(write_chan);
        dma_status |= dma_configure_write(write_chan);
    }
    else
    {
        /* Abort or done status not set, return */
        return;
    }

    /* Clear the channel abort flag */
    channel_aborted[write_chan] = false;

    if (dma_status != STATUS_SUCCESS)
    {
        status = DMAW_ERROR_DRIVER_ABORT_FAILED;
    }

    /* TODO: SW-9022: To be removed */
    msg_id = atomic_load_local_16(&DMAW_Write_CB.chan_status_cb[write_chan].msg_id);

    /* Read the channel status from CB */
    write_chan_status.raw_u64 =
        atomic_load_local_64(&DMAW_Write_CB.chan_status_cb[write_chan].status.raw_u64);

    /* Obtain wait latency, start cycles measured for the command
    and obtain current cycles */
    dma_write_cycles.cmd_start_cycles = atomic_load_local_64(
        &DMAW_Write_CB.chan_status_cb[write_chan].dmaw_cycles.cmd_start_cycles);
    dma_write_cycles.exec_start_cycles = atomic_load_local_64(
        &DMAW_Write_CB.chan_status_cb[write_chan].dmaw_cycles.exec_start_cycles);
    dma_write_cycles.wait_cycles =
        atomic_load_local_32(&DMAW_Write_CB.chan_status_cb[write_chan].dmaw_cycles.wait_cycles);

    /* Update global DMA channel status
    NOTE: Channel state must be made idle once all resources are read */
    atomic_store_local_32(
        &DMAW_Write_CB.chan_status_cb[write_chan].status.channel_state, DMA_CHAN_STATE_IDLE);

    /* Decrement the commands count being processed by the
    given SQW. Should be done after clearing channel state */
    SQW_Decrement_Command_Count(write_chan_status.sqw_idx);

    if (status == DMAW_ERROR_DRIVER_ABORT_FAILED)
    {
        read_rsp->status = DEV_OPS_API_DMA_RESPONSE_DRIVER_ABORT_FAILED;
    }
    else
    {
        read_rsp->status = DEV_OPS_API_DMA_RESPONSE_HOST_ABORTED;
    }

    /* Create and transmit DMA command response */
    read_rsp->response_info.rsp_hdr.size =
        sizeof(struct device_ops_data_read_rsp_t) - sizeof(struct cmn_header_t);
    read_rsp->response_info.rsp_hdr.tag_id = write_chan_status.tag_id;
    read_rsp->response_info.rsp_hdr.msg_id = (msg_id_t)(msg_id + 1U);
    /* TODO: SW-9022 To be enabled back
    read_rsp->response_info.rsp_hdr.msg_id =
        DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_RSP */
    read_rsp->device_cmd_start_ts = dma_write_cycles.cmd_start_cycles;
    read_rsp->device_cmd_wait_dur = dma_write_cycles.wait_cycles;
    /* Compute command execution latency */
    read_rsp->device_cmd_execute_dur = PMC_GET_LATENCY(dma_write_cycles.exec_start_cycles);

    status = Host_Iface_CQ_Push_Cmd(0, read_rsp, sizeof(struct device_ops_data_read_rsp_t));

    if (status == STATUS_SUCCESS)
    {
        TRACE_LOG_CMD_STATUS(
            msg_id, write_chan_status.sqw_idx, write_chan_status.tag_id, CMD_STATUS_ABORTED)
        Log_Write(LOG_LEVEL_DEBUG, "DMAW:Pushed:DATA_WRITE_CMD_RSP->Host_CQ\r\n");
    }
    else
    {
        TRACE_LOG_CMD_STATUS(
            msg_id, write_chan_status.sqw_idx, write_chan_status.tag_id, CMD_STATUS_FAILED)
        Log_Write(
            LOG_LEVEL_ERROR, "DMAW:Tag_ID=%u:HostIface:Push:Failed\r\n", write_chan_status.tag_id);
        SP_Iface_Report_Error(MM_RECOVERABLE_FW_MM_DMAW_ERROR, MM_CQ_PUSH_ERROR);
    }

    /* Report device API error to SP */
    SP_Iface_Report_Error(MM_RECOVERABLE_OPS_API_DMA_READLIST, (int16_t)read_rsp->status);
}

/************************************************************************
*
*   FUNCTION
*
*       dmaw_launch_read_worker
*
*   DESCRIPTION
*
*       Launch a DMA Worker on HART ID requested
*
*   INPUTS
*
*       uint32_t   HART ID to launch the DMA Worker
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
__attribute__((noreturn)) static inline void dmaw_launch_read_worker(uint32_t hart_id)
{
    struct device_ops_data_write_rsp_t write_rsp;
    uint32_t read_chan_state;
    bool channel_aborted[PCIE_DMA_RD_CHANNEL_COUNT] = { false, false, false, false };

    while (1)
    {
        for (uint8_t read_ch_index = 0; read_ch_index < PCIE_DMA_RD_CHANNEL_COUNT; read_ch_index++)
        {
            read_chan_state = atomic_load_local_32(
                &DMAW_Read_CB.chan_status_cb[read_ch_index].status.channel_state);

            /* Check if HW DMA chan status is done and update
            global DMA channel status for read channels */
            if (read_chan_state == DMA_CHAN_STATE_IN_USE)
            {
                Log_Write(
                    LOG_LEVEL_DEBUG, "DMAW:%d:read_chan_active:%d\r\n", hart_id, read_ch_index);

                process_dma_read_chan_in_use(read_ch_index, &write_rsp);
            }
            else if (read_chan_state == DMA_CHAN_STATE_ABORTING)
            {
                Log_Write(
                    LOG_LEVEL_ERROR, "DMAW:%d:read_chan_aborting:%d\r\n", hart_id, read_ch_index);

                process_dma_read_chan_aborting(read_ch_index, &write_rsp, channel_aborted);
            }
        }
    }
}

/************************************************************************
*
*   FUNCTION
*
*       dmaw_launch_write_worker
*
*   DESCRIPTION
*
*       Launch a DMA Worker on HART ID requested
*
*   INPUTS
*
*       uint32_t   HART ID to launch the DMA Worker
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
__attribute__((noreturn)) static inline void dmaw_launch_write_worker(uint32_t hart_id)
{
    struct device_ops_data_read_rsp_t read_rsp;
    uint32_t write_chan_state;
    bool channel_aborted[PCIE_DMA_WRT_CHANNEL_COUNT] = { false, false, false, false };

    while (1)
    {
        for (uint8_t write_ch_index = 0; write_ch_index < PCIE_DMA_WRT_CHANNEL_COUNT;
             write_ch_index++)
        {
            write_chan_state = atomic_load_local_32(
                &DMAW_Write_CB.chan_status_cb[write_ch_index].status.channel_state);

            if (write_chan_state == DMA_CHAN_STATE_IN_USE)
            {
                Log_Write(
                    LOG_LEVEL_DEBUG, "DMAW:%d:write_chan_active:%d\r\n", hart_id, write_ch_index);

                process_dma_write_chan_in_use(write_ch_index, &read_rsp);
            }
            else if (write_chan_state == DMA_CHAN_STATE_ABORTING)
            {
                Log_Write(
                    LOG_LEVEL_ERROR, "DMAW:%d:write_chan_aborting:%d\r\n", hart_id, write_ch_index);

                process_dma_write_chan_aborting(write_ch_index, &read_rsp, channel_aborted);
            }
        }
    }
}

/************************************************************************
*
*   FUNCTION
*
*       DMAW_Launch
*
*   DESCRIPTION
*
*       Launch a DMA Worker on HART ID requested
*
*   INPUTS
*
*       uint32_t   HART ID to launch the DMA Worker
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void DMAW_Launch(uint32_t hart_id)
{
    uint8_t dma_chan_id;

    Log_Write(LOG_LEVEL_INFO, "DMAW:H%d\r\n", hart_id);

    /* Design Notes: Note a DMA write command from host will trigger
    the implementation to configure a DMA read channel on device to move
    data from host to device, similarly a read command from host will
    trigger the implementation to configure a DMA write channel on device
    to move data from device to host */
    if (hart_id == DMAW_FOR_READ)
    {
        /* Enable the DMA read engine */
        dma_enable_read_engine();

        for (dma_chan_id = DMA_CHAN_ID_READ_0; dma_chan_id <= DMA_CHAN_ID_READ_3; dma_chan_id++)
        {
            dma_configure_read(dma_chan_id);
        }

        /* Launch DMA read worker, this function never returns. */
        dmaw_launch_read_worker(hart_id);
    }
    else if (hart_id == DMAW_FOR_WRITE)
    {
        /* Enable the DMA write engine */
        dma_enable_write_engine();

        for (dma_chan_id = DMA_CHAN_ID_WRITE_0; dma_chan_id <= DMA_CHAN_ID_WRITE_3; dma_chan_id++)
        {
            dma_configure_write(dma_chan_id);
        }

        /* Launch DMA read worker, this function never returns. */
        dmaw_launch_write_worker(hart_id);
    }
    else
    {
        Log_Write(LOG_LEVEL_ERROR, "DMAW:Launch Invalid DMA Hart ID %d\r\n", hart_id);
    }

    return;
}

/************************************************************************
*
*   FUNCTION
*
*       DMAW_Abort_All_Dispatched_Read_Channels
*
*   DESCRIPTION
*
*       Blocking function call that sets the status of each DMA read
*       channel to abort and waits until the channel is idle
*
*   INPUTS
*
*       sqw_idx     Submission Queue Worker index
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void DMAW_Abort_All_Dispatched_Read_Channels(uint8_t sqw_idx)
{
    Log_Write(LOG_LEVEL_DEBUG, "DMAW:SQ=%d: Abort all DMA read channels\r\n", sqw_idx);

    /* Traverse all read channels and abort them */
    for (uint8_t read_chan = 0; read_chan < PCIE_DMA_RD_CHANNEL_COUNT; read_chan++)
    {
        /* Spin-wait if DMA channel state is reserved.
        Reserved channel needs to transition to Idle or in use before we can abort it */
        do
        {
            asm volatile("fence\n" ::: "memory");
        } while (
            atomic_load_local_32(&DMAW_Read_CB.chan_status_cb[read_chan].status.channel_state) ==
            DMA_CHAN_STATE_RESERVED);

        /* Check if this channel is dispatched by the given sqw_idx and
        DMA channel is in use, then abort it */
        if ((atomic_load_local_8(&DMAW_Read_CB.chan_status_cb[read_chan].status.sqw_idx) ==
                sqw_idx) &&
            atomic_compare_and_exchange_local_32(
                &DMAW_Read_CB.chan_status_cb[read_chan].status.channel_state, DMA_CHAN_STATE_IN_USE,
                DMA_CHAN_STATE_ABORTING) == DMA_CHAN_STATE_IN_USE)
        {
            Log_Write(LOG_LEVEL_DEBUG, "DMAW:SQ=%d:chan=%d: Aborting DMA read channel\r\n", sqw_idx,
                read_chan);

            /* Spin-wait if the DMA state is aborting */
            do
            {
                asm volatile("fence\n" ::: "memory");
            } while (atomic_load_local_32(
                         &DMAW_Read_CB.chan_status_cb[read_chan].status.channel_state) ==
                     DMA_CHAN_STATE_ABORTING);

            Log_Write(LOG_LEVEL_DEBUG, "DMAW:SQ=%d:chan=%d: Aborted DMA read channel\r\n", sqw_idx,
                read_chan);
        }
    }
}

/************************************************************************
*
*   FUNCTION
*
*       DMAW_Abort_All_Dispatched_Write_Channels
*
*   DESCRIPTION
*
*       Blocking function call that sets the status of each DMA write
*       channel to abort which was dispatched by the given SQW and waits
*       until the channel is idle.
*
*   INPUTS
*
*       sqw_idx     Submission Queue Worker index
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void DMAW_Abort_All_Dispatched_Write_Channels(uint8_t sqw_idx)
{
    Log_Write(LOG_LEVEL_DEBUG, "DMAW:SQ=%d: Abort all DMA write channels\r\n", sqw_idx);

    /* Traverse all write channels and abort them */
    for (uint8_t write_chan = 0; write_chan < PCIE_DMA_WRT_CHANNEL_COUNT; write_chan++)
    {
        /* Spin-wait if DMA channel state is reserved.
        Reserved channel needs to transition to Idle or in use before we can abort it */
        do
        {
            asm volatile("fence\n" ::: "memory");
        } while (
            atomic_load_local_32(&DMAW_Write_CB.chan_status_cb[write_chan].status.channel_state) ==
            DMA_CHAN_STATE_RESERVED);

        /* Check if this channel is dispatched by the given sqw_idx and
        DMA channel is in use, then abort it */
        if ((atomic_load_local_8(&DMAW_Write_CB.chan_status_cb[write_chan].status.sqw_idx) ==
                sqw_idx) &&
            (atomic_compare_and_exchange_local_32(
                 &DMAW_Write_CB.chan_status_cb[write_chan].status.channel_state,
                 DMA_CHAN_STATE_IN_USE, DMA_CHAN_STATE_ABORTING) == DMA_CHAN_STATE_IN_USE))
        {
            Log_Write(LOG_LEVEL_DEBUG, "DMAW:SQ=%d:chan=%d: Aborting DMA write channel\r\n",
                sqw_idx, write_chan);

            /* Spin-wait if the DMA state is aborting */
            do
            {
                asm volatile("fence\n" ::: "memory");
            } while (atomic_load_local_32(
                         &DMAW_Write_CB.chan_status_cb[write_chan].status.channel_state) ==
                     DMA_CHAN_STATE_ABORTING);

            Log_Write(LOG_LEVEL_DEBUG, "DMAW:SQ=%d:chan=%d: Aborted DMA write channel\r\n", sqw_idx,
                write_chan);
        }
    }
}
