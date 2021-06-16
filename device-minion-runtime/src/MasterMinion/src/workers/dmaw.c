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
*/
/***********************************************************************/
#include    "workers/dmaw.h"
#include    "workers/sqw.h"
#include    "services/log.h"
#include    "services/host_iface.h"
#include    "services/sp_iface.h"
#include    "services/sw_timer.h"
#include    "pmu.h"
#include    "sync.h"

/*! \struct dmaw_read_cb_t
    \brief DMA Worker Read Control Block structure.
    Used to maintain DMA Worker Read related resources.
*/
typedef struct dmaw_read_cb {
    uint8_t                 chan_search_timeout_flag[SQW_NUM];
    dma_channel_status_cb_t chan_status_cb[PCIE_DMA_RD_CHANNEL_COUNT];
} dmaw_read_cb_t;

/*! \struct dmaw_write_cb_t
    \brief DMA Worker Write Control Block structure.
    Used to maintain DMA Worker Write related resources.
*/
typedef struct dmaw_write_cb {
    uint8_t                 chan_search_timeout_flag[SQW_NUM];
    dma_channel_status_cb_t chan_status_cb[PCIE_DMA_WRT_CHANNEL_COUNT];
} dmaw_write_cb_t;

/*! \var dmaw_read_cb_t DMAW_Read_CB
    \brief Global DMA Read Control Block
    \warning Not thread safe!, used by minions in
    master shire, use local atomics for access.
*/
static dmaw_read_cb_t DMAW_Read_CB __attribute__((aligned(64))) = {0};

/*! \var dmaw_write_cb_t DMAW_Write_CB
    \brief Global DMA Write Control Block
    \warning Not thread safe!, used by minions in
    master shire, use local atomics for access.
*/
static dmaw_write_cb_t DMAW_Write_CB __attribute__((aligned(64))) = {0};

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

    for(int i = 0; i < SQW_NUM; i++)
    {
        /* Reset the read channel timeout flag */
        atomic_store_local_8(&DMAW_Read_CB.chan_search_timeout_flag[i], 0U);

        /* Reset the write channel timeout flag */
        atomic_store_local_8(&DMAW_Write_CB.chan_search_timeout_flag[i], 0U);
    }

    /* Initialize DMA Read channel status */
    for(int i = 0; i < PCIE_DMA_RD_CHANNEL_COUNT; i++)
    {
        /* Store tag id, channel state and sqw idx */
        atomic_store_local_64(&DMAW_Read_CB.chan_status_cb[i].status.raw_u64,
            chan_status.raw_u64);
    }

    /* Initialize DMA Write channel status */
    for(int i = 0; i < PCIE_DMA_WRT_CHANNEL_COUNT; i++)
    {
        /* Store tag id, channel state and sqw idx */
        atomic_store_local_64(&DMAW_Write_CB.chan_status_cb[i].status.raw_u64,
            chan_status.raw_u64);
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
*       int8_t     status success or error
*
***********************************************************************/
int8_t DMAW_Read_Find_Idle_Chan_And_Reserve(dma_chan_id_e *chan_id, uint8_t sqw_idx)
{
    int8_t status = DMAW_ERROR_TIMEOUT_FIND_IDLE_CHANNEL;
    bool read_chan_reserved = false;

    /* TODO: SW-4450: Setup timer here with DMAW_FIND_IDLE_CH_TIMEOUT value.
    Register DMAW_Read_Ch_Search_Timeout_Callback with payload as sqw_idx */

    /* Try to find idle channel until timeout occurs */
    do
    {
        /* Find the idle channel and reserve it */
        for (uint8_t ch = 0; ch < PCIE_DMA_RD_CHANNEL_COUNT; ch++)
        {
            /* Compare for idle state and reserve */
            if (atomic_compare_and_exchange_local_32(
                &DMAW_Read_CB.chan_status_cb[ch].status.channel_state,
                DMA_CHAN_STATE_IDLE, DMA_CHAN_STATE_RESERVED) == DMA_CHAN_STATE_IDLE)
            {
                /* Return the DMA channel ID */
                *chan_id = ch;
                status = STATUS_SUCCESS;
                read_chan_reserved = true;
                break;
                /* TODO: SW-4450: Cancel timer */
            }
        }
    } while(!read_chan_reserved && (atomic_load_local_8(&DMAW_Read_CB.chan_search_timeout_flag[sqw_idx]) == 0U));

    /* If timeout occurs then report this event to SP. */
    if(!read_chan_reserved)
    {
        SP_Iface_Report_Error(MM_RECOVERABLE, MM_DMA_TIMEOUT_ERROR);
    }

    /* Reset the timeout flag */
    atomic_store_local_8(&DMAW_Read_CB.chan_search_timeout_flag[sqw_idx], 0U);

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
*       int8_t     status success or error
*
***********************************************************************/
int8_t DMAW_Write_Find_Idle_Chan_And_Reserve(dma_chan_id_e *chan_id, uint8_t sqw_idx)
{
    int8_t status = DMAW_ERROR_TIMEOUT_FIND_IDLE_CHANNEL;
    bool write_chan_reserved = false;

    /* TODO: SW-4450: Setup timer here with DMAW_FIND_IDLE_CH_TIMEOUT value.
    Register DMAW_Write_Ch_Search_Timeout_Callback with payload as sqw_idx */

    /* Try to find idle channel until timeout occurs */
    do
    {
        /* Find the idle channel and reserve it */
        for (uint8_t ch = 0; ch < PCIE_DMA_WRT_CHANNEL_COUNT; ch++)
        {
            /* Compare for idle state and reserve */
            if (atomic_compare_and_exchange_local_32(
                &DMAW_Write_CB.chan_status_cb[ch].status.channel_state,
                DMA_CHAN_STATE_IDLE, DMA_CHAN_STATE_RESERVED) == DMA_CHAN_STATE_IDLE)
            {
                /* Return the DMA channel ID */
                *chan_id = ch + DMA_CHAN_ID_WRITE_0;
                status = STATUS_SUCCESS;
                write_chan_reserved = true;
                break;
                /* TODO: SW-4450: Cancel timer */
            }
        }
    } while(!write_chan_reserved && (atomic_load_local_8(&DMAW_Write_CB.chan_search_timeout_flag[sqw_idx]) == 0U));

    /* If timeout occurs then report this event to SP. */
    if(!write_chan_reserved)
    {
        SP_Iface_Report_Error(MM_RECOVERABLE, MM_DMA_TIMEOUT_ERROR);
    }

    /* Reset the timeout flag */
    atomic_store_local_8(&DMAW_Write_CB.chan_search_timeout_flag[sqw_idx], 0U);

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
*       chan_id         DMA channel ID
*       cmd             Pointer to command buffer
*       xfer_count      Number of transfer nodes in command.
*       sqw_idx         SQW ID
*       cycles          Pointer to latency cycles struct
*       sw_timer_idx    Index of SW Timer used for timeout
*
*   OUTPUTS
*
*       int8_t          status success or error
*
***********************************************************************/
int8_t DMAW_Read_Trigger_Transfer(dma_chan_id_e chan_id, struct device_ops_dma_writelist_cmd_t *cmd,
    uint8_t xfer_count, uint8_t sqw_idx, exec_cycles_t *cycles, uint8_t sw_timer_idx)
{
    int8_t status=DMA_OPERATION_SUCCESS;
    uint8_t rd_ch_idx = (uint8_t)(chan_id - DMA_CHAN_ID_READ_0);
    dma_channel_status_t chan_status;

    /* Set tag ID, set channel state to active, set SQW Index */
    chan_status.tag_id = cmd->command_info.cmd_hdr.tag_id;
    chan_status.sqw_idx = sqw_idx;
    chan_status.channel_state = DMA_CHAN_STATE_IN_USE;
    chan_status.sw_timer_idx = sw_timer_idx;

    atomic_store_local_64(&DMAW_Read_CB.chan_status_cb[rd_ch_idx].status.raw_u64,
        chan_status.raw_u64);

    /* TODO: SW-7137: To be removed */
    uint16_t t_msg_id = cmd->command_info.cmd_hdr.msg_id;
    atomic_store_local_16(
        &DMAW_Read_CB.chan_status_cb[rd_ch_idx].msg_id, t_msg_id);

    /* To start indexing from zero, decrement the count by one. */
    uint8_t last_i = (uint8_t)(xfer_count - 1);

    /* Configure DMA for all transfers one-by-one. */
    for(uint8_t xfer_index=0;
        (xfer_index < last_i) && (status == DMA_OPERATION_SUCCESS); ++xfer_index)
    {
        /* Add DMA list data node for current transfer in the list. */
        status = dma_config_add_data_node(cmd->list[xfer_index].src_host_phy_addr,
                    cmd->list[xfer_index].dst_device_phy_addr, cmd->list[xfer_index].size,
                    chan_id, xfer_index, DMA_NORMAL, false);

        Log_Write(LOG_LEVEL_DEBUG, "DMAW:Config:Added read data node No:%u\r\n", xfer_index);
    }

    if(status == DMA_OPERATION_SUCCESS)
    {
        /* Add DMA list data node for last transfer in the list. Enable interrupt on completion. */
        status = dma_config_add_data_node(cmd->list[last_i].src_host_phy_addr,
                    cmd->list[last_i].dst_device_phy_addr, cmd->list[last_i].size,
                    chan_id, last_i, DMA_NORMAL, true);

        /* Add DMA list link node at the end ot transfer list. */
        dma_config_add_link_node(chan_id, xfer_count);

        Log_Write(LOG_LEVEL_DEBUG, "DMAW:Config:Added last read data node No:%u and Link node.\r\n", last_i);
    }

    if(status == DMA_OPERATION_SUCCESS)
    {
        dma_start_read(chan_id);

        /* Update cycles value into the Global Channel Status data structure */
        atomic_store_local_64(
            &DMAW_Read_CB.chan_status_cb[rd_ch_idx].dmaw_cycles.cmd_start_cycles, cycles->cmd_start_cycles);
        atomic_store_local_64(
            &DMAW_Read_CB.chan_status_cb[rd_ch_idx].dmaw_cycles.raw_u64, cycles->raw_u64);

        Log_Write(LOG_LEVEL_DEBUG, "SQ[%d] DMAW_Read_Trigger_Transfer:Success!\r\n", sqw_idx);
    }
    else
    {
        /* Release the DMA resources */
        chan_status.tag_id = 0;
        chan_status.sqw_idx = 0;
        chan_status.channel_state = DMA_CHAN_STATE_IDLE;

        atomic_store_local_64(
            &DMAW_Read_CB.chan_status_cb[rd_ch_idx].status.raw_u64,
            chan_status.raw_u64);

        SP_Iface_Report_Error(MM_RECOVERABLE, MM_DMA_CONFIG_ERROR);
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
*       chan_id         DMA channel ID
*       cmd             Pointer to command buffer
*       xfer_count      Number of transfer nodes in command.
*       sqw_idx         SQW ID
*       cycles          Pointer to latency cycles struct
*       sw_timer_idx    Index of SW Timer used for timeout
*       flags           DMA flag to set a specific DMA action.
*
*   OUTPUTS
*
*       int8_t     status success or error
*
***********************************************************************/
int8_t DMAW_Write_Trigger_Transfer(dma_chan_id_e chan_id, struct device_ops_dma_readlist_cmd_t *cmd,
    uint8_t xfer_count, uint8_t sqw_idx, exec_cycles_t *cycles, uint8_t sw_timer_idx,
    dma_flags_e flags)
{
    int8_t status=DMA_OPERATION_SUCCESS;
    uint8_t wrt_ch_idx = (uint8_t)(chan_id - DMA_CHAN_ID_WRITE_0);
    dma_channel_status_t chan_status;

    /* Set tag ID, set channel state to active, set SQW Index */
    chan_status.tag_id = cmd->command_info.cmd_hdr.tag_id;
    chan_status.sqw_idx = sqw_idx;
    chan_status.channel_state = DMA_CHAN_STATE_IN_USE;
    chan_status.sw_timer_idx = sw_timer_idx;

    atomic_store_local_64(&DMAW_Write_CB.chan_status_cb[wrt_ch_idx].status.raw_u64,
        chan_status.raw_u64);

    /* TODO: SW-7137: To be removed */
    uint16_t t_msg_id = cmd->command_info.cmd_hdr.msg_id;
    atomic_store_local_16(
        &DMAW_Write_CB.chan_status_cb[wrt_ch_idx].msg_id, t_msg_id);

    /* To start indexing from zero, decrement the count by one. */
    uint8_t last_i = (uint8_t)(xfer_count - 1);

    /* Configure DMA for all transfers one-by-one. */
    for(uint8_t xfer_index=0;
        (xfer_index < last_i) && (status == DMA_OPERATION_SUCCESS); ++xfer_index)
    {
        /* Add DMA list data node for current transfer in the list. */
        status = dma_config_add_data_node(cmd->list[xfer_index].src_device_phy_addr,
                    cmd->list[xfer_index].dst_host_phy_addr, cmd->list[xfer_index].size,
                    chan_id, xfer_index, flags, false);

        Log_Write(LOG_LEVEL_DEBUG, "DMAW:Config:Added write data node No:%u\r\n", xfer_index);
    }

    if(status == DMA_OPERATION_SUCCESS)
    {
        /* Add DMA list data node for last transfer in the list. Enable interrupt on completion. */
        status = dma_config_add_data_node(cmd->list[last_i].src_device_phy_addr,
                    cmd->list[last_i].dst_host_phy_addr, cmd->list[last_i].size,
                    chan_id, last_i, flags, true);

        /* Add DMA list link node at the end ot transfer list. */
        dma_config_add_link_node(chan_id, xfer_count);

        Log_Write(LOG_LEVEL_DEBUG, "DMAW:Config:Added last write data node No:%u and Link node.\r\n", last_i);
    }

    if(status == DMA_OPERATION_SUCCESS)
    {
        dma_start_write(chan_id);

        /* Update cycles value into the Global Channel Status data structure */
        atomic_store_local_64(
            &DMAW_Write_CB.chan_status_cb[wrt_ch_idx].dmaw_cycles.cmd_start_cycles, cycles->cmd_start_cycles);
        atomic_store_local_64(
            &DMAW_Write_CB.chan_status_cb[wrt_ch_idx].dmaw_cycles.raw_u64, cycles->raw_u64);

        Log_Write(LOG_LEVEL_DEBUG, "SQ[%d] DMAW_Write_Trigger_Transfer:Success!\r\n", sqw_idx);

        status = STATUS_SUCCESS;
    }
    else
    {
        /* Release the DMA resources */
        chan_status.tag_id = 0;
        chan_status.sqw_idx = 0;
        chan_status.channel_state = DMA_CHAN_STATE_IDLE;

        atomic_store_local_64(
            &DMAW_Write_CB.chan_status_cb[wrt_ch_idx].status.raw_u64,
            chan_status.raw_u64);

        SP_Iface_Report_Error(MM_RECOVERABLE, MM_DMA_CONFIG_ERROR);
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       DMAW_Read_Ch_Search_Timeout_Callback
*
*   DESCRIPTION
*
*       Callback for read channel search timeout
*
*   INPUTS
*
*       sqw_idx    Submission queue index
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void DMAW_Read_Ch_Search_Timeout_Callback(uint8_t sqw_idx)
{
    /* Set the read channel timeout flag */
    atomic_store_local_8(&DMAW_Read_CB.chan_search_timeout_flag[sqw_idx], 1U);
}

/************************************************************************
*
*   FUNCTION
*
*       DMAW_Write_Ch_Search_Timeout_Callback
*
*   DESCRIPTION
*
*       Callback for write channel search timeout
*
*   INPUTS
*
*       sqw_idx    Submission queue index
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void DMAW_Write_Ch_Search_Timeout_Callback(uint8_t sqw_idx)
{
    /* Set the write channel timeout flag */
    atomic_store_local_8(&DMAW_Write_CB.chan_search_timeout_flag[sqw_idx], 1U);
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
*       uint8_t                      DMA channel index
*       device_ops_data_write_rsp_t  Pointer to buffer for DMA response.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static inline void process_dma_read_chan_in_use(uint8_t read_ch_index,
struct device_ops_data_write_rsp_t *write_rsp)
{
    dma_chan_id_e read_chan_id;
    dma_channel_status_t read_chan_status;
    exec_cycles_t dma_rd_cycles;
    uint32_t dma_read_status;
    bool dma_read_done = false;
    bool dma_read_aborted = false;
    int8_t status = STATUS_SUCCESS;
    uint16_t msg_id; /* TODO: SW-7137: To be removed */

    /* Populate the DMA channel index */
    read_chan_id = read_ch_index + DMA_CHAN_ID_READ_0;

    dma_read_status = dma_get_read_int_status();
    dma_read_done = dma_check_read_done(read_ch_index, dma_read_status);
    if(!dma_read_done)
    {
        dma_read_aborted = dma_check_read_abort(read_ch_index, dma_read_status);
    }

    if (dma_read_done || dma_read_aborted)
    {
        /* Read the channel status from CB */
        read_chan_status.raw_u64 = atomic_load_local_64(
            &DMAW_Read_CB.chan_status_cb[read_ch_index].status.raw_u64);

        /* Free the registered SW Timeout slot */
        SW_Timer_Cancel_Timeout(read_chan_status.sw_timer_idx);

        if(dma_read_done)
        {
            /* DMA transfer complete, clear interrupt status */
            dma_clear_read_done(read_chan_id);
            write_rsp->status = DEV_OPS_API_DMA_RESPONSE_COMPLETE;
            Log_Write(LOG_LEVEL_DEBUG,"DMAW: Read Transfer Completed\r\n");
        }
        else
        {
            /* DMA transfer aborted, clear interrupt status */
            dma_clear_read_abort(read_chan_id);
            dma_configure_read(read_chan_id);
            write_rsp->status = DEV_OPS_API_DMA_RESPONSE_ERROR;
            Log_Write(LOG_LEVEL_ERROR,"DMAW:Tag_ID=%u:Read Transfer Aborted\r\n",
                        read_chan_status.tag_id);
        }

        /* TODO: SW-7137: To be removed */
        msg_id = atomic_load_local_16(
            &DMAW_Read_CB.chan_status_cb[read_ch_index].msg_id);

        Log_Write(LOG_LEVEL_DEBUG,"SQ[%d] DMAW: Read Tag ID:%d Chan ID:%d \r\n",
            read_chan_status.sqw_idx, read_chan_status.tag_id, read_chan_id);
        /* Obtain wait latency, start cycles measured
        for the command and obtain current cycles */
        dma_rd_cycles.cmd_start_cycles = atomic_load_local_64
            (&DMAW_Read_CB.chan_status_cb[read_ch_index].dmaw_cycles.cmd_start_cycles);
        dma_rd_cycles.raw_u64 = atomic_load_local_64
            (&DMAW_Read_CB.chan_status_cb[read_ch_index].dmaw_cycles.raw_u64);

        /* Update global DMA channel status
        NOTE: Channel state must be made idle once all resources are read */
        atomic_store_local_32
            (&DMAW_Read_CB.chan_status_cb[read_ch_index].status.channel_state,
            DMA_CHAN_STATE_IDLE);

        /* Decrement the commands count being processed by the
        given SQW. Should be done after clearing channel state */
        SQW_Decrement_Command_Count(read_chan_status.sqw_idx);

        /* Create and transmit DMA command response */
        write_rsp->response_info.rsp_hdr.size =
            sizeof(struct device_ops_data_write_rsp_t) - sizeof(struct cmn_header_t);
        write_rsp->response_info.rsp_hdr.tag_id = read_chan_status.tag_id;
        write_rsp->response_info.rsp_hdr.msg_id = (msg_id_t)(msg_id + 1U);
        /* TODO: SW-7137 To be enabled back
        write_rsp->response_info.rsp_hdr.msg_id =
            DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_RSP */
        write_rsp->device_cmd_start_ts = dma_rd_cycles.cmd_start_cycles;
        write_rsp->device_cmd_wait_dur = dma_rd_cycles.wait_cycles;
        /* Compute command execution latency */
        write_rsp->device_cmd_execute_dur = PMC_GET_LATENCY(dma_rd_cycles.exec_start_cycles);

        Log_Write(LOG_LEVEL_DEBUG,
            "DMAW:Pushing:DATA_WRITE_CMD_RSP:tag_id=%x->Host_CQ\r\n",
                write_rsp->response_info.rsp_hdr.tag_id);

        status = Host_Iface_CQ_Push_Cmd(0, write_rsp, sizeof(struct device_ops_data_write_rsp_t));

        if(status != STATUS_SUCCESS)
        {
            Log_Write(LOG_LEVEL_ERROR, "DMAW:Tag_ID=%u:HostIface:Push:Failed\r\n",
                        read_chan_status.tag_id);
            SP_Iface_Report_Error(MM_RECOVERABLE, MM_CQ_PUSH_ERROR);
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
*       uint8_t                      DMA channel index
*       device_ops_data_write_rsp_t  Pointer to buffer for DMA response.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static inline void process_dma_read_chan_aborting(uint8_t ch_index,
    struct device_ops_data_write_rsp_t *write_rsp)
{
    dma_chan_id_e read_chan_id;
    dma_channel_status_t read_chan_status;
    exec_cycles_t dma_read_cycles;
    int8_t status = STATUS_SUCCESS;
    DMA_STATUS_e dma_status = DMA_OPERATION_SUCCESS;
    uint16_t msg_id; /* TODO: SW-7137: To be removed */

    /* Populate the DMA channel index */
    read_chan_id = ch_index + DMA_CHAN_ID_READ_0;

    /* Abort the channel */
    dma_status = dma_abort_read(read_chan_id);

    if(dma_status == DMA_OPERATION_SUCCESS)
    {
        /* DMA transfer aborted, clear interrupt status */
        dma_clear_read_abort(read_chan_id);
        dma_configure_read(read_chan_id);
    }

    /* TODO: SW-7137: To be removed */
    msg_id = atomic_load_local_16(
        &DMAW_Read_CB.chan_status_cb[ch_index].msg_id);

    /* Read the channel status from CB */
    read_chan_status.raw_u64 = atomic_load_local_64(
        &DMAW_Read_CB.chan_status_cb[ch_index].status.raw_u64);

    /* Obtain wait latency, start cycles measured for the command
    and obtain current cycles */
    dma_read_cycles.cmd_start_cycles = atomic_load_local_64
        (&DMAW_Read_CB.chan_status_cb[ch_index].dmaw_cycles.cmd_start_cycles);
    dma_read_cycles.raw_u64 = atomic_load_local_64
        (&DMAW_Read_CB.chan_status_cb[ch_index].dmaw_cycles.raw_u64);

    /* Update global DMA channel status
    NOTE: Channel state must be made idle once all resources are read */
    atomic_store_local_32
        (&DMAW_Read_CB.chan_status_cb[ch_index].status.channel_state,
        DMA_CHAN_STATE_IDLE);

    /* Decrement the commands count being processed by the
    given SQW. Should be done after clearing channel state */
    SQW_Decrement_Command_Count(read_chan_status.sqw_idx);

    /* Create and transmit DMA command response */
    write_rsp->status = DEV_OPS_API_DMA_RESPONSE_TIMEOUT_HANG;
    write_rsp->response_info.rsp_hdr.size =
        sizeof(struct device_ops_data_write_rsp_t) - sizeof(struct cmn_header_t);
    write_rsp->response_info.rsp_hdr.tag_id = read_chan_status.tag_id;
    write_rsp->response_info.rsp_hdr.msg_id = (msg_id_t)(msg_id + 1U);
    /* TODO: SW-7137 To be enabled back
    write_rsp->response_info.rsp_hdr.msg_id =
        DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_RSP */
    write_rsp->device_cmd_start_ts = dma_read_cycles.cmd_start_cycles;
    write_rsp->device_cmd_wait_dur = dma_read_cycles.wait_cycles;
    /* Compute command execution latency */
    write_rsp->device_cmd_execute_dur = PMC_GET_LATENCY(dma_read_cycles.exec_start_cycles);

    status = Host_Iface_CQ_Push_Cmd(0, write_rsp, sizeof(struct device_ops_data_write_rsp_t));

    if(status == STATUS_SUCCESS)
    {
        Log_Write(LOG_LEVEL_DEBUG,
            "DMAW:Pushed:DATA_WRITE_CMD_RSP->Host_CQ\r\n");
    }
    else
    {
        Log_Write(LOG_LEVEL_ERROR, "DMAW:Tag_ID=%u:HostIface:Push:Failed\r\n",
                    read_chan_status.tag_id);
        SP_Iface_Report_Error(MM_RECOVERABLE, MM_CQ_PUSH_ERROR);
    }
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
*       uint8_t                      DMA channel index
*       device_ops_data_write_rsp_t  Pointer to buffer for DMA response.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static inline void process_dma_write_chan_in_use(uint8_t write_ch_index,
    struct device_ops_data_read_rsp_t *read_rsp)
{
    /* Populate the DMA channel index */
    dma_chan_id_e write_chan_id = write_ch_index + DMA_CHAN_ID_WRITE_0;
    uint32_t dma_write_status;
    bool dma_write_done = false;
    bool dma_write_aborted = false;
    exec_cycles_t dma_write_cycles;
    dma_channel_status_t write_chan_status;
    int8_t status = STATUS_SUCCESS;
    uint16_t msg_id; /* TODO: SW-7137: To be removed */

    dma_write_status = dma_get_write_int_status();
    dma_write_done = dma_check_write_done(write_ch_index, dma_write_status);
    if(!dma_write_done)
    {
        dma_write_aborted = dma_check_write_abort(write_ch_index, dma_write_status);
    }

    if (dma_write_done || dma_write_aborted)
    {
        /* Read the channel status from CB */
        write_chan_status.raw_u64 = atomic_load_local_64(
            &DMAW_Write_CB.chan_status_cb[write_ch_index].status.raw_u64);

        /* Free the registered SW Timeout slot */
        SW_Timer_Cancel_Timeout(write_chan_status.sw_timer_idx);

        if(dma_write_done)
        {
            /* DMA transfer complete, clear interrupt status */
            dma_clear_write_done(write_chan_id);
            read_rsp->status = DEV_OPS_API_DMA_RESPONSE_COMPLETE;
            Log_Write(LOG_LEVEL_DEBUG,"DMAW: Write Transfer Completed\r\n");
        }
        else
        {
            /* DMA transfer aborted, clear interrupt status */
            dma_clear_write_abort(write_chan_id);
            dma_configure_write(write_chan_id);
            read_rsp->status = DEV_OPS_API_DMA_RESPONSE_ERROR;
            Log_Write(LOG_LEVEL_ERROR,"DMAW:Tag_ID=%u:Write Transfer Aborted\r\n",
                        write_chan_status.tag_id);
        }

        /* TODO: SW-7137: To be removed */
        msg_id = atomic_load_local_16(
            &DMAW_Write_CB.chan_status_cb[write_ch_index].msg_id);

        Log_Write(LOG_LEVEL_DEBUG,"SQ[%d] DMAW: Write Tag ID:%d Chan ID:%d \r\n",
            write_chan_status.sqw_idx, write_chan_status.tag_id, write_chan_id);
        /* Obtain wait latency, start cycles measured
        for the command and obtain current cycles */
        dma_write_cycles.cmd_start_cycles = atomic_load_local_64
            (&DMAW_Write_CB.chan_status_cb[write_ch_index].dmaw_cycles.cmd_start_cycles);
        dma_write_cycles.raw_u64 = atomic_load_local_64
            (&DMAW_Write_CB.chan_status_cb[write_ch_index].dmaw_cycles.raw_u64);

        /* Update global DMA channel status
        NOTE: Channel state must be made idle once all resources are read */
        atomic_store_local_32
            (&DMAW_Write_CB.chan_status_cb[write_ch_index].status.channel_state,
            DMA_CHAN_STATE_IDLE);

        /* Decrement the commands count being processed by the
        given SQW. Should be done after clearing channel state */
        SQW_Decrement_Command_Count(write_chan_status.sqw_idx);

        /* Create and transmit DMA command response */
        read_rsp->response_info.rsp_hdr.size =
            sizeof(struct device_ops_data_read_rsp_t) - sizeof(struct cmn_header_t);
        read_rsp->response_info.rsp_hdr.tag_id = write_chan_status.tag_id;
        read_rsp->response_info.rsp_hdr.msg_id = (msg_id_t)(msg_id + 1U);
        /* TODO: SW-7137 To be enabled back
        read_rsp->response_info.rsp_hdr.msg_id =
            DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_RSP */
        read_rsp->device_cmd_start_ts = dma_write_cycles.cmd_start_cycles;
        read_rsp->device_cmd_wait_dur = dma_write_cycles.wait_cycles;
        /* Compute command execution latency */
        read_rsp->device_cmd_execute_dur = PMC_GET_LATENCY(dma_write_cycles.exec_start_cycles);

        status = Host_Iface_CQ_Push_Cmd(0, read_rsp, sizeof(struct device_ops_data_read_rsp_t));

        if(status == STATUS_SUCCESS)
        {
            Log_Write(LOG_LEVEL_DEBUG,
                "DMAW:Pushed:DATA_READ_CMD_RSP:tag_id=%x->Host_CQ\r\n",
                read_rsp->response_info.rsp_hdr.tag_id);
        }
        else
        {
            Log_Write(LOG_LEVEL_ERROR, "DMAW:Tag_ID=%u:HostIface:Push:Failed\r\n",
                        write_chan_status.tag_id);
            SP_Iface_Report_Error(MM_RECOVERABLE, MM_CQ_PUSH_ERROR);
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
*       uint8_t                      DMA channel index
*       device_ops_data_write_rsp_t  Pointer to buffer for DMA response.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static inline void process_dma_write_chan_aborting(uint8_t ch_index,
    struct device_ops_data_read_rsp_t *read_rsp)
{
    dma_chan_id_e write_chan_id;
    dma_channel_status_t write_chan_status;
    exec_cycles_t dma_write_cycles;
    int8_t status = STATUS_SUCCESS;
    DMA_STATUS_e dma_write_status = DMA_OPERATION_SUCCESS;
    uint16_t msg_id; /* TODO: SW-7137: To be removed */

    /* Populate the DMA channel index */
    write_chan_id = ch_index + DMA_CHAN_ID_WRITE_0;

    /* Abort the channel */
    dma_write_status = dma_abort_write(write_chan_id);

    if(dma_write_status == DMA_OPERATION_SUCCESS)
    {
        /* DMA transfer aborted, clear interrupt status */
        dma_clear_write_abort(write_chan_id);
        dma_configure_write(write_chan_id);
    }

    /* TODO: SW-7137: To be removed */
    msg_id = atomic_load_local_16(
        &DMAW_Write_CB.chan_status_cb[ch_index].msg_id);

    /* Read the channel status from CB */
    write_chan_status.raw_u64 = atomic_load_local_64(
        &DMAW_Write_CB.chan_status_cb[ch_index].status.raw_u64);

    /* Obtain wait latency, start cycles measured for the command
    and obtain current cycles */
    dma_write_cycles.cmd_start_cycles = atomic_load_local_64
        (&DMAW_Write_CB.chan_status_cb[ch_index].dmaw_cycles.cmd_start_cycles);
    dma_write_cycles.raw_u64 = atomic_load_local_64
        (&DMAW_Write_CB.chan_status_cb[ch_index].dmaw_cycles.raw_u64);

    /* Update global DMA channel status
    NOTE: Channel state must be made idle once all resources are read */
    atomic_store_local_32
        (&DMAW_Write_CB.chan_status_cb[ch_index].status.channel_state,
        DMA_CHAN_STATE_IDLE);

    /* Decrement the commands count being processed by the
    given SQW. Should be done after clearing channel state */
    SQW_Decrement_Command_Count(write_chan_status.sqw_idx);

    /* Create and transmit DMA command response */
    read_rsp->status = DEV_OPS_API_DMA_RESPONSE_TIMEOUT_HANG;
    read_rsp->response_info.rsp_hdr.size =
        sizeof(struct device_ops_data_read_rsp_t) - sizeof(struct cmn_header_t);
    read_rsp->response_info.rsp_hdr.tag_id = write_chan_status.tag_id;
    read_rsp->response_info.rsp_hdr.msg_id = (msg_id_t)(msg_id + 1U);
    /* TODO: SW-7137 To be enabled back
    read_rsp->response_info.rsp_hdr.msg_id =
        DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_RSP */
    read_rsp->device_cmd_start_ts = dma_write_cycles.cmd_start_cycles;
    read_rsp->device_cmd_wait_dur = dma_write_cycles.wait_cycles;
    /* Compute command execution latency */
    read_rsp->device_cmd_execute_dur = PMC_GET_LATENCY(dma_write_cycles.exec_start_cycles);

    status = Host_Iface_CQ_Push_Cmd(0, read_rsp, sizeof(struct device_ops_data_read_rsp_t));

    if(status == STATUS_SUCCESS)
    {
        Log_Write(LOG_LEVEL_DEBUG,
            "DMAW:Pushed:DATA_WRITE_CMD_RSP->Host_CQ\r\n");
    }
    else
    {
        Log_Write(LOG_LEVEL_ERROR, "DMAW:Tag_ID=%u:HostIface:Push:Failed\r\n",
                    write_chan_status.tag_id);
        SP_Iface_Report_Error(MM_RECOVERABLE, MM_CQ_PUSH_ERROR);
    }
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

    while(1)
    {
        for(uint8_t read_ch_index = 0;
            read_ch_index < PCIE_DMA_RD_CHANNEL_COUNT;
            read_ch_index++)
        {
            read_chan_state = atomic_load_local_32(
                &DMAW_Read_CB.chan_status_cb[read_ch_index].status.channel_state);

            /* Check if HW DMA chan status is done and update
            global DMA channel status for read channels */
            if(read_chan_state == DMA_CHAN_STATE_IN_USE)
            {
                Log_Write(LOG_LEVEL_DEBUG, "DMAW:%d:read_chan_active:%d\r\n",
                    hart_id, read_ch_index);

                process_dma_read_chan_in_use(read_ch_index, &write_rsp);
            }
            else if (read_chan_state == DMA_CHAN_STATE_ABORTING)
            {
                Log_Write(LOG_LEVEL_ERROR, "DMAW:%d:Timeout:read_chan_aborting:%d\r\n",
                    hart_id, read_ch_index);

                process_dma_read_chan_aborting(read_ch_index, &write_rsp);
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

    while(1)
    {
        for(uint8_t write_ch_index = 0;
            write_ch_index < PCIE_DMA_WRT_CHANNEL_COUNT;
            write_ch_index++)
        {
            write_chan_state = atomic_load_local_32(
                &DMAW_Write_CB.chan_status_cb[write_ch_index].status.channel_state);

            if(write_chan_state == DMA_CHAN_STATE_IN_USE)
            {
                Log_Write(LOG_LEVEL_DEBUG, "DMAW:%d:write_chan_active:%d\r\n",
                    hart_id, write_ch_index);

                process_dma_write_chan_in_use(write_ch_index, &read_rsp);
            }
            else if (write_chan_state == DMA_CHAN_STATE_ABORTING)
            {
                Log_Write(LOG_LEVEL_ERROR, "DMAW:%d:Timeout:write_chan_aborting:%d\r\n",
                    hart_id, write_ch_index);

                process_dma_write_chan_aborting(write_ch_index, &read_rsp);
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
    dma_chan_id_e dma_chan_id;

    Log_Write(LOG_LEVEL_CRITICAL, "DMAW:H%d\r\n", hart_id);

    /* Design Notes: Note a DMA write command from host will trigger
    the implementation to configure a DMA read channel on device to move
    data from host to device, similarly a read command from host will
    trigger the implementation to configure a DMA write channel on device
    to move data from device to host */
    if(hart_id == DMAW_FOR_READ)
    {
        for(dma_chan_id = DMA_CHAN_ID_READ_0;
            dma_chan_id <= DMA_CHAN_ID_READ_3;
            dma_chan_id++)
        {
            dma_configure_read(dma_chan_id);
        }

        /* Launch DMA read worker, this function never returns. */
        dmaw_launch_read_worker(hart_id);
    }
    else if(hart_id == DMAW_FOR_WRITE)
    {
        for(dma_chan_id = DMA_CHAN_ID_WRITE_0;
            dma_chan_id <= DMA_CHAN_ID_WRITE_3;
            dma_chan_id++)
        {
            dma_configure_write(dma_chan_id);
        }

        /* Launch DMA read worker, this function never returns. */
        dmaw_launch_write_worker(hart_id);
    }
    else
    {
        Log_Write(LOG_LEVEL_ERROR,"DMAW:Launch Invalid DMA Hart ID %d\r\n", hart_id);
    }

    return;
}

/************************************************************************
*
*   FUNCTION
*
*       DMAW_Read_Set_Abort_Status
*
*   DESCRIPTION
*
*       Sets the status of DMA read channel to abort it
*
*   INPUTS
*
*       uint8_t   DMA read channel index
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void DMAW_Read_Set_Abort_Status(uint8_t read_chan)
{
    /* Free the registered SW Timeout slot */
    SW_Timer_Cancel_Timeout(atomic_load_local_8(
        &DMAW_Read_CB.chan_status_cb[read_chan - DMA_CHAN_ID_READ_0].status.sw_timer_idx));

    atomic_store_local_32
        (&DMAW_Read_CB.chan_status_cb[read_chan - DMA_CHAN_ID_READ_0].status.channel_state,
        DMA_CHAN_STATE_ABORTING);
}

/************************************************************************
*
*   FUNCTION
*
*       DMAW_Write_Set_Abort_Status
*
*   DESCRIPTION
*
*       Sets the status of DMA write channel to abort it
*
*   INPUTS
*
*       uint8_t   DMA write channel index
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void DMAW_Write_Set_Abort_Status(uint8_t write_chan)
{
    /* Free the registered SW Timeout slot */
    SW_Timer_Cancel_Timeout(atomic_load_local_8(
        &DMAW_Write_CB.chan_status_cb[write_chan - DMA_CHAN_ID_WRITE_0].status.sw_timer_idx));

    atomic_store_local_32
        (&DMAW_Write_CB.chan_status_cb[write_chan - DMA_CHAN_ID_WRITE_0].status.channel_state,
        DMA_CHAN_STATE_ABORTING);
}
