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
    This module implements;
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
#include    "services/sw_timer.h"
#include    <esperanto/device-apis/operations-api/device_ops_api_spec.h>
#include    <esperanto/device-apis/operations-api/device_ops_api_rpc_types.h>
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
    bool channel_reserved = false;

    /* TODO: SW-4450: Setup timer here with DMAW_FIND_IDLE_CH_TIMEOUT value.
    Register DMAW_Read_Ch_Search_Timeout_Callback with payload as sqw_idx */

    /* Try to find idle channel until timeout occurs */
    do
    {
        /* Find the idle channel and reserve it */
        for (uint8_t ch = 0; (ch < PCIE_DMA_RD_CHANNEL_COUNT) && (!channel_reserved); ch++)
        {
            /* Compare for idle state and reserve */
            if (atomic_compare_and_exchange_local_32(
                &DMAW_Read_CB.chan_status_cb[ch].status.channel_state,
                DMA_CHAN_STATE_IDLE, DMA_CHAN_STATE_RESERVED) == DMA_CHAN_STATE_IDLE)
            {
                /* Return the DMA channel ID */
                *chan_id = ch;
                status = STATUS_SUCCESS;
                channel_reserved = true;
                /* TODO: SW-4450: Cancel timer */
            }
        }
    } while(!channel_reserved && (atomic_load_local_8(&DMAW_Read_CB.chan_search_timeout_flag[sqw_idx]) == 0U));

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
    bool channel_reserved = false;

    /* TODO: SW-4450: Setup timer here with DMAW_FIND_IDLE_CH_TIMEOUT value.
    Register DMAW_Write_Ch_Search_Timeout_Callback with payload as sqw_idx */

    /* Try to find idle channel until timeout occurs */
    do
    {
        /* Find the idle channel and reserve it */
        for (uint8_t ch = 0; (ch < PCIE_DMA_WRT_CHANNEL_COUNT) && (!channel_reserved); ch++)
        {
            /* Compare for idle state and reserve */
            if (atomic_compare_and_exchange_local_32(
                &DMAW_Write_CB.chan_status_cb[ch].status.channel_state,
                DMA_CHAN_STATE_IDLE, DMA_CHAN_STATE_RESERVED) == DMA_CHAN_STATE_IDLE)
            {
                /* Return the DMA channel ID */
                *chan_id = ch + DMA_CHAN_ID_WRITE_0;
                status = STATUS_SUCCESS;
                channel_reserved = true;
                /* TODO: SW-4450: Cancel timer */
            }
        }
    } while(!channel_reserved && (atomic_load_local_8(&DMAW_Write_CB.chan_search_timeout_flag[sqw_idx]) == 0U));

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
*       src_addr        Source address
*       dest_addr       Destination address
*       size            Size of DMA transaction
*       sqw_idx         SQW ID
*       tag_id          Tag ID of the command
*       cycles          Pointer to latency cycles struct
*       sw_timer_idx    Index of SW Timer used for timeout
*
*   OUTPUTS
*
*       int8_t          status success or error
*
***********************************************************************/
int8_t DMAW_Read_Trigger_Transfer(dma_chan_id_e chan_id,
    uint64_t src_addr, uint64_t dest_addr, uint64_t size, uint8_t sqw_idx,
    uint16_t tag_id, exec_cycles_t *cycles, uint8_t sw_timer_idx)
{
    int8_t status;
    uint8_t rd_ch_idx = (uint8_t)(chan_id - DMA_CHAN_ID_READ_0);
    dma_channel_status_t chan_status;

    /* Set tag ID, set channel state to active, set SQW Index */
    chan_status.tag_id = tag_id;
    chan_status.sqw_idx = sqw_idx;
    chan_status.channel_state = DMA_CHAN_STATE_IN_USE;
    chan_status.sw_timer_idx = sw_timer_idx;

    atomic_store_local_64(&DMAW_Read_CB.chan_status_cb[rd_ch_idx].status.raw_u64,
        chan_status.raw_u64);

    /* Call the DMA device driver function */
    status = (int8_t)dma_trigger_transfer(src_addr, dest_addr, size, chan_id);

    if(status == DMA_OPERATION_SUCCESS)
    {
        /* Update cycles value into the Global Channel Status data structure */
        atomic_store_local_64(
            &DMAW_Read_CB.chan_status_cb[rd_ch_idx].dmaw_cycles.raw_u64, cycles->raw_u64);

        Log_Write(LOG_LEVEL_DEBUG, "DMAW:DMAW_Trigger_Transfer:Success!\r\n");

        status = STATUS_SUCCESS;
    }
    else
    {
        /* Cancel the rimeout registered */
        SW_Timer_Cancel_Timeout(sw_timer_idx);

        /* Release the DMA resources */
        chan_status.tag_id = 0;
        chan_status.sqw_idx = 0;
        chan_status.channel_state = DMA_CHAN_STATE_IDLE;

        atomic_store_local_64(
            &DMAW_Read_CB.chan_status_cb[rd_ch_idx].status.raw_u64,
            chan_status.raw_u64);
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
*       src_addr        Source address
*       dest_addr       Destination address
*       size            Size of DMA transaction
*       sqw_idx         SQW ID
*       tag_id          Tag ID of the command
*       cycles          Pointer to latency cycles struct
*       sw_timer_idx    Index of SW Timer used for timeout
*
*   OUTPUTS
*
*       int8_t     status success or error
*
***********************************************************************/
int8_t DMAW_Write_Trigger_Transfer(dma_chan_id_e chan_id,
    uint64_t src_addr, uint64_t dest_addr, uint64_t size, uint8_t sqw_idx,
    uint16_t tag_id, exec_cycles_t *cycles, uint8_t sw_timer_idx)
{
    int8_t status;
    uint8_t wrt_ch_idx = (uint8_t)(chan_id - DMA_CHAN_ID_WRITE_0);
    dma_channel_status_t chan_status;

    /* Set tag ID, set channel state to active, set SQW Index */
    chan_status.tag_id = tag_id;
    chan_status.sqw_idx = sqw_idx;
    chan_status.channel_state = DMA_CHAN_STATE_IN_USE;
    chan_status.sw_timer_idx = sw_timer_idx;

    atomic_store_local_64(&DMAW_Write_CB.chan_status_cb[wrt_ch_idx].status.raw_u64,
        chan_status.raw_u64);

    /* Call the DMA device driver function */
    status = (int8_t)dma_trigger_transfer(src_addr, dest_addr, size, chan_id);

    if(status == DMA_OPERATION_SUCCESS)
    {
        /* Update cycles value into the Global Channel Status data structure */
        atomic_store_local_64(
            &DMAW_Write_CB.chan_status_cb[wrt_ch_idx].dmaw_cycles.raw_u64, cycles->raw_u64);

        Log_Write(LOG_LEVEL_DEBUG, "DMAW:DMAW_Trigger_Transfer:Success!\r\n");

        status = STATUS_SUCCESS;
    }
    else
    {
        /* Cancel the rimeout registered */
        SW_Timer_Cancel_Timeout(sw_timer_idx);

        /* Release the DMA resources */
        chan_status.tag_id = 0;
        chan_status.sqw_idx = 0;
        chan_status.channel_state = DMA_CHAN_STATE_IDLE;

        atomic_store_local_64(
            &DMAW_Write_CB.chan_status_cb[wrt_ch_idx].status.raw_u64,
            chan_status.raw_u64);
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
    struct device_ops_data_read_rsp_t read_rsp;
    struct device_ops_data_write_rsp_t write_rsp;
    dma_chan_id_e dma_chan_id;
    dma_channel_status_t chan_status;
    exec_cycles_t dma_cycles;
    uint32_t channel_state;
    uint8_t ch_index;
    uint32_t dma_chan_status;
    bool dma_done = false;
    bool dma_aborted = false;
    int8_t status = STATUS_SUCCESS;
    DMA_STATUS_e dma_status = DMA_OPERATION_SUCCESS;

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

        while(1)
        {
            for(ch_index = 0;
                ch_index < PCIE_DMA_RD_CHANNEL_COUNT;
                ch_index++)
            {
                channel_state = atomic_load_local_32(
                    &DMAW_Read_CB.chan_status_cb[ch_index].status.channel_state);

                /* Check if HW DMA chan status is done and update
                global DMA channel status for read channels */
                if(channel_state == DMA_CHAN_STATE_IN_USE)
                {
                    Log_Write(LOG_LEVEL_DEBUG, "DMAW:%d:read_chan_active:%d\r\n", hart_id, ch_index);

                    /* Populate the DMA channel index */
                    dma_chan_id = ch_index + DMA_CHAN_ID_READ_0;

                    dma_chan_status = dma_get_read_int_status();
                    dma_done = dma_check_read_done(ch_index, dma_chan_status);
                    if(!dma_done)
                    {
                        dma_aborted = dma_check_read_abort(ch_index, dma_chan_status);
                    }

                    if (dma_done || dma_aborted)
                    {
                        if(dma_done)
                        {
                            /* DMA transfer complete, clear interrupt status */
                            dma_clear_done(dma_chan_id);
                            write_rsp.status = DEV_OPS_API_DMA_RESPONSE_COMPLETE;
                        }
                        else
                        {
                            /* DMA transfer aborted, clear interrupt status */
                            dma_clear_read_abort(dma_chan_id);
                            dma_configure_read(dma_chan_id);
                            write_rsp.status = DEV_OPS_API_DMA_RESPONSE_ERROR;
                        }

                        /* Read the channel status from CB */
                        chan_status.raw_u64 = atomic_load_local_64(
                            &DMAW_Read_CB.chan_status_cb[ch_index].status.raw_u64);

                        /* Obtain wait latency, start cycles measured
                        for the command and obtain current cycles */
                        dma_cycles.raw_u64 = atomic_load_local_64
                            (&DMAW_Read_CB.chan_status_cb[ch_index].dmaw_cycles.raw_u64);

                        /* Free the registered SW Timeout slot */
                        SW_Timer_Cancel_Timeout(chan_status.sw_timer_idx);

                        /* Update global DMA channel status
                        NOTE: Channel state must be made idle once all resources are read */
                        atomic_store_local_32
                            (&DMAW_Read_CB.chan_status_cb[ch_index].status.channel_state,
                            DMA_CHAN_STATE_IDLE);

                        /* Decrement the commands count being processed by the
                        given SQW. Should be done after clearing channel state */
                        SQW_Decrement_Command_Count(chan_status.sqw_idx);

                        /* Create and transmit DMA command response */
                        write_rsp.response_info.rsp_hdr.size =
                            sizeof(write_rsp);
                        write_rsp.response_info.rsp_hdr.tag_id = chan_status.tag_id;
                        write_rsp.response_info.rsp_hdr.msg_id =
                            DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_RSP;
                        write_rsp.cmd_wait_time = dma_cycles.wait_cycles;
                        /* Compute command execution latency */
                        write_rsp.cmd_execution_time = PMC_GET_LATENCY(dma_cycles.start_cycles);

                        status = Host_Iface_CQ_Push_Cmd(0, &write_rsp, sizeof(write_rsp));

                        if(status == STATUS_SUCCESS)
                        {
                            Log_Write(LOG_LEVEL_DEBUG,
                                "DMAW:Pushed:DATA_WRITE_CMD_RSP:tag_id=%x->Host_CQ\r\n",
                                    write_rsp.response_info.rsp_hdr.tag_id);
                        }
                        else
                        {
                            Log_Write(LOG_LEVEL_ERROR, "DMAW:HostIface:Push:Failed\r\n");
                        }
                    }
                }
                else if (channel_state == DMA_CHAN_STATE_ABORTING)
                {
                    Log_Write(LOG_LEVEL_ERROR, "DMAW:%d:read_chan_aborting:%d\r\n", hart_id, ch_index);

                    /* Populate the DMA channel index */
                    dma_chan_id = ch_index + DMA_CHAN_ID_READ_0;

                    /* Obtain wait latency, start cycles measured for the command
                    and obtain current cycles */
                    dma_cycles.raw_u64 = atomic_load_local_64
                        (&DMAW_Read_CB.chan_status_cb[ch_index].dmaw_cycles.raw_u64);

                    /* Abort the channel */
                    dma_status = dma_abort_read(dma_chan_id);

                    if(dma_status == DMA_OPERATION_SUCCESS)
                    {
                        /* DMA transfer aborted, clear interrupt status */
                        dma_status = dma_clear_read_abort(dma_chan_id);
                    }

                    if(dma_status == DMA_OPERATION_SUCCESS)
                    {
                        /* Update global DMA channel status
                        NOTE: Channel state must be made idle once all resources are read */
                        atomic_store_local_32
                            (&DMAW_Read_CB.chan_status_cb[ch_index].status.channel_state,
                            DMA_CHAN_STATE_IDLE);

                        /* Decrement the commands count being processed by the
                        given SQW. Should be done after clearing channel state */
                        SQW_Decrement_Command_Count(chan_status.sqw_idx);

                        /* Create and transmit DMA command response */
                        write_rsp.status = DEV_OPS_API_DMA_RESPONSE_TIMEOUT_HANG;
                        write_rsp.response_info.rsp_hdr.size =
                            sizeof(write_rsp);
                        write_rsp.response_info.rsp_hdr.tag_id = chan_status.tag_id;
                        write_rsp.response_info.rsp_hdr.msg_id =
                            DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_RSP;
                        write_rsp.cmd_wait_time = dma_cycles.wait_cycles;
                        /* Compute command execution latency */
                        write_rsp.cmd_execution_time = PMC_GET_LATENCY(dma_cycles.start_cycles);

                        status = Host_Iface_CQ_Push_Cmd(0, &write_rsp, sizeof(write_rsp));

                        if(status == STATUS_SUCCESS)
                        {
                            Log_Write(LOG_LEVEL_DEBUG,
                                "DMAW:Pushed:DATA_WRITE_CMD_RSP->Host_CQ\r\n");
                        }
                        else
                        {
                            Log_Write(LOG_LEVEL_ERROR, "DMAW:HostIface:Push:Failed\r\n");
                        }
                    }
                }
            }
        }
    }
    else if(hart_id == DMAW_FOR_WRITE)
    {
        for(dma_chan_id = DMA_CHAN_ID_WRITE_0;
            dma_chan_id <= DMA_CHAN_ID_WRITE_3;
            dma_chan_id++)
        {
            dma_configure_write(dma_chan_id);
        }

        while(1)
        {
            for(ch_index = 0;
                ch_index < PCIE_DMA_WRT_CHANNEL_COUNT;
                ch_index++)
            {
                channel_state = atomic_load_local_32(
                    &DMAW_Write_CB.chan_status_cb[ch_index].status.channel_state);

                if(channel_state == DMA_CHAN_STATE_IN_USE)
                {
                    Log_Write(LOG_LEVEL_DEBUG, "DMAW:%d:write_chan_active:%d\r\n",
                        hart_id, ch_index);

                    /* Populate the DMA channel index */
                    dma_chan_id = ch_index + DMA_CHAN_ID_WRITE_0;

                    dma_chan_status = dma_get_write_int_status();
                    dma_done = dma_check_write_done(ch_index, dma_chan_status);
                    if(!dma_done)
                    {
                        dma_aborted = dma_check_write_abort(ch_index, dma_chan_status);
                    }

                    if (dma_done || dma_aborted)
                    {
                        if(dma_done)
                        {
                            /* DMA transfer complete, clear interrupt status */
                            dma_clear_done(dma_chan_id);
                            read_rsp.status = DEV_OPS_API_DMA_RESPONSE_COMPLETE;
                        }
                        else
                        {
                            /* DMA transfer aborted, clear interrupt status */
                            dma_clear_write_abort(dma_chan_id);
                            dma_configure_write(dma_chan_id);
                            read_rsp.status = DEV_OPS_API_DMA_RESPONSE_ERROR;
                        }

                        /* Read the channel status from CB */
                        chan_status.raw_u64 = atomic_load_local_64(
                            &DMAW_Write_CB.chan_status_cb[ch_index].status.raw_u64);

                        /* Obtain wait latency, start cycles measured
                        for the command and obtain current cycles */
                        dma_cycles.raw_u64 = atomic_load_local_64
                            (&DMAW_Write_CB.chan_status_cb[ch_index].dmaw_cycles.raw_u64);

                        /* Free the registered SW Timeout slot */
                        SW_Timer_Cancel_Timeout(chan_status.sw_timer_idx);

                        /* Update global DMA channel status
                        NOTE: Channel state must be made idle once all resources are read */
                        atomic_store_local_32
                            (&DMAW_Write_CB.chan_status_cb[ch_index].status.channel_state,
                            DMA_CHAN_STATE_IDLE);

                        /* Decrement the commands count being processed by the
                        given SQW. Should be done after clearing channel state */
                        SQW_Decrement_Command_Count(chan_status.sqw_idx);

                        /* Create and transmit DMA command response */
                        read_rsp.response_info.rsp_hdr.size = sizeof(read_rsp);
                        read_rsp.response_info.rsp_hdr.tag_id = chan_status.tag_id;
                        read_rsp.response_info.rsp_hdr.msg_id =
                            DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_RSP;
                        read_rsp.cmd_wait_time = dma_cycles.wait_cycles;
                        /* Compute command execution latency */
                        read_rsp.cmd_execution_time = PMC_GET_LATENCY(dma_cycles.start_cycles);

                        status = Host_Iface_CQ_Push_Cmd(0, &read_rsp, sizeof(read_rsp));

                        if(status == STATUS_SUCCESS)
                        {
                            Log_Write(LOG_LEVEL_DEBUG,
                                "DMAW:Pushed:DATA_READ_CMD_RSP:tag_id=%x->Host_CQ\r\n",
                                read_rsp.response_info.rsp_hdr.tag_id);
                        }
                        else
                        {
                            Log_Write(LOG_LEVEL_ERROR, "DMAW:HostIface:Push:Failed\r\n");
                        }
                    }
                }
                else if (channel_state == DMA_CHAN_STATE_ABORTING)
                {
                    Log_Write(LOG_LEVEL_ERROR, "DMAW:%d:write_chan_aborting:%d\r\n", hart_id, ch_index);

                    /* Populate the DMA channel index */
                    dma_chan_id = ch_index + DMA_CHAN_ID_WRITE_0;

                    /* Obtain wait latency, start cycles measured for the command
                    and obtain current cycles */
                    dma_cycles.raw_u64 = atomic_load_local_64
                        (&DMAW_Write_CB.chan_status_cb[ch_index].dmaw_cycles.raw_u64);

                    /* Abort the channel */
                    dma_status = dma_abort_write(dma_chan_id);

                    if(dma_status == DMA_OPERATION_SUCCESS)
                    {
                        /* DMA transfer aborted, clear interrupt status */
                        dma_status = dma_clear_write_abort(dma_chan_id);
                    }

                    if(dma_status == DMA_OPERATION_SUCCESS)
                    {
                        /* Update global DMA channel status
                        NOTE: Channel state must be made idle once all resources are read */
                        atomic_store_local_32
                            (&DMAW_Write_CB.chan_status_cb[ch_index].status.channel_state,
                            DMA_CHAN_STATE_IDLE);

                        /* Decrement the commands count being processed by the
                        given SQW. Should be done after clearing channel state */
                        SQW_Decrement_Command_Count(chan_status.sqw_idx);

                        /* Create and transmit DMA command response */
                        read_rsp.status = DEV_OPS_API_DMA_RESPONSE_TIMEOUT_HANG;
                        read_rsp.response_info.rsp_hdr.size =
                            sizeof(read_rsp);
                        read_rsp.response_info.rsp_hdr.tag_id = chan_status.tag_id;
                        read_rsp.response_info.rsp_hdr.msg_id =
                            DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_RSP;
                        read_rsp.cmd_wait_time = dma_cycles.wait_cycles;
                        /* Compute command execution latency */
                        read_rsp.cmd_execution_time = PMC_GET_LATENCY(dma_cycles.start_cycles);

                        status = Host_Iface_CQ_Push_Cmd(0, &read_rsp, sizeof(read_rsp));

                        if(status == STATUS_SUCCESS)
                        {
                            Log_Write(LOG_LEVEL_DEBUG,
                                "DMAW:Pushed:DATA_WRITE_CMD_RSP->Host_CQ\r\n");
                        }
                        else
                        {
                            Log_Write(LOG_LEVEL_ERROR, "DMAW:HostIface:Push:Failed\r\n");
                        }
                    }
                }
            }
        }
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
    Log_Write(LOG_LEVEL_ERROR, "Aborting:DMAW: read channel=%d\r\n", read_chan);

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
    Log_Write(LOG_LEVEL_ERROR, "Aborting:DMAW: write channel=%d\r\n", write_chan);

    atomic_store_local_32
        (&DMAW_Write_CB.chan_status_cb[write_chan - DMA_CHAN_ID_WRITE_0].status.channel_state,
        DMA_CHAN_STATE_ABORTING);
}
