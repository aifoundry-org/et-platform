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
    interfaces.

    Public interfaces:
        DMAW_Init
        DMAW_Notify
        DMAW_Launch
*/
/***********************************************************************/
#include    "workers/dmaw.h"
#include    "workers/sqw.h"
#include    "services/log1.h"
#include    "services/host_iface.h"
#include    <esperanto/device-apis/operations-api/device_ops_api_spec.h>
#include    <esperanto/device-apis/operations-api/device_ops_api_rpc_types.h>
#include    "pcie_dma.h"
#include    "pmu.h"

/*! \var dma_channel_status_t DMA_Channel_Status
    \brief Global Global DMA CHannel Status
    \warning Not thread safe!, used by minions in
    master shire, use local atomics for access.
*/
dma_channel_status_t DMA_Channel_Status __attribute__((aligned(64))) = {0};

extern spinlock_t Launch_Lock;

/************************************************************************
*
*   FUNCTION
*
*       DMAW_Get_DMA_Channel_Status_Addr
*  
*   DESCRIPTION
*
*       Interface to obtain DMA Channel Status address
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       void*   Address if DMA Channel Status
*
***********************************************************************/
void* DMAW_Get_DMA_Channel_Status_Addr(void)
{
    return &DMA_Channel_Status;
}

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
    for(int i = 0; i < PCIE_DMA_RD_CHANNEL_COUNT; i++)
    {
        atomic_store_local_16
            (&DMA_Channel_Status.dma_rd_chan[i].tag_id, 0);
        atomic_store_local_8
            (&DMA_Channel_Status.dma_rd_chan[i].channel_state, 0);
        atomic_store_local_8
            (&DMA_Channel_Status.dma_rd_chan[i].sqw_idx, 0);

    }

    for(int i = 0; i < PCIE_DMA_WRT_CHANNEL_COUNT; i++)
    {
        atomic_store_local_16
            (&DMA_Channel_Status.dma_wrt_chan[i].tag_id, 0);
        atomic_store_local_8
            (&DMA_Channel_Status.dma_wrt_chan[i].channel_state, 0);
        atomic_store_local_8
            (&DMA_Channel_Status.dma_rd_chan[i].sqw_idx, 0);
    }
    
    return;
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
*       uint32_t   HART ID to launch the dispatcher
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void DMAW_Launch(uint32_t hart_id)
{
    etsoc_dma_chan_id_e   dma_chan_id;
    struct device_ops_data_read_rsp_t read_rsp;
    struct device_ops_data_write_rsp_t write_rsp;
    uint16_t tag_id; 
    uint16_t channel_state;
    int8_t status = STATUS_SUCCESS;
    uint32_t start_cycles=0;

    /* Release the launch lock to let other workers acquire it */
    release_local_spinlock(&Launch_Lock);

    Log_Write(LOG_LEVEL_CRITICAL, "%s%d%s", "DMAW:HART=", hart_id, "\r\n");

    /* Design Notes: Note a DMA write command from host will trigger 
    the implementation to configure a DMA read channel on device to move 
    data from host to device, similarly a read command from host will 
    trigger the implementation to configure a DMA write channel on device
    to move data from device to host */
    while(1)
    {
        for(dma_chan_id = ETSOC_DMA_CHAN_ID_READ_0; 
            dma_chan_id <  (ETSOC_DMA_CHAN_ID_READ_0 + PCIE_DMA_RD_CHANNEL_COUNT); 
            dma_chan_id++)
        {
            channel_state = atomic_load_local_8
                (&DMA_Channel_Status.dma_rd_chan[dma_chan_id].channel_state); 

            /* Check if HW DMA chan status is done and update 
            global DMA channel status for read channels */
            if((channel_state == DMA_CHANNEL_IN_USE))
            {
                Log_Write(LOG_LEVEL_DEBUG, "%s%d%s", 
                    "DMAW:read_chan_active:", dma_chan_id, "\r\n");

                /* TODO: This needs to be improved to detect error conditions,
                check DMA_READ_INT_STATUS bits 16:25, to detect and 
                check ABORT status */
                if(dma_check_done(dma_chan_id))
                {
                    /* DMA transfer complete, clear interrupt status */    
                    dma_clear_done(dma_chan_id);

                    /* Decrement the commands count being processed by the given SQW */
                    SQW_Decrement_Command_Count(
                        atomic_load_local_8(&DMA_Channel_Status.dma_rd_chan[dma_chan_id].sqw_idx));

                    /* Update global  DMA channel status */
                    atomic_store_local_8
                        (&DMA_Channel_Status.dma_rd_chan[dma_chan_id].channel_state, 
                        DMA_CHANNEL_AVAILABLE);
                    
                    tag_id = atomic_load_local_16
                        (&DMA_Channel_Status.dma_rd_chan[dma_chan_id].tag_id);
                    
                    /* Create and transmit DMA command response */
                    write_rsp.response_info.rsp_hdr.size = sizeof(write_rsp);
                    write_rsp.response_info.rsp_hdr.tag_id = tag_id;
                    write_rsp.response_info.rsp_hdr.msg_id = 
                        DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_RSP;
                    /* Obtain wait latency, start cycles measured for the command */ 
                    write_rsp.cmd_wait_time = atomic_load_local_32
                        (&DMA_Channel_Status.dma_rd_chan[dma_chan_id].wait_latency);
                    start_cycles = atomic_load_local_32
                        (&DMA_Channel_Status.dma_rd_chan[dma_chan_id].cmd_dispatch_start_cycles);
                    /* Obtain current cycles, and compute command execution
                    latency */
                    write_rsp.cmd_execution_time = PMC_GET_LATENCY(start_cycles);
                    /* TODO: We should be able to capture other DMA states as well */
                    write_rsp.status = ETSOC_DMA_STATE_DONE;

                    status = Host_Iface_CQ_Push_Cmd
                        (0, &write_rsp, sizeof(write_rsp));

                    if(status == STATUS_SUCCESS)
                    {
                        Log_Write(LOG_LEVEL_DEBUG, "%s", 
                            "DMAW:Pushed:DATA_WRITE_CMD_RSP->Host_CQ \r\n");
                    }
                    else
                    {
                        Log_Write(LOG_LEVEL_DEBUG, "%s", 
                            "DMAW:HostIface:Push:Failed\r\n");
                    }
                }
            }
        }

        for(dma_chan_id = ETSOC_DMA_CHAN_ID_WRITE_0; 
            dma_chan_id <  (ETSOC_DMA_CHAN_ID_WRITE_0 + PCIE_DMA_WRT_CHANNEL_COUNT); 
            dma_chan_id++)
        {

            channel_state = atomic_load_local_8
                (&DMA_Channel_Status.dma_wrt_chan[dma_chan_id].channel_state); 

            if((channel_state == DMA_CHANNEL_IN_USE))
            {
                Log_Write(LOG_LEVEL_DEBUG, "%s%d%s", 
                    "DMAW:write_chan_active:", dma_chan_id, "\r\n");

                /* TODO: This needs to be improved to detect error conditions,
                check DMA_WRITED_INT_STATUS bits 16:25, to detect and 
                check ABORT status */
                if(dma_check_done(dma_chan_id))
                {
                    /* TODO: using the old implementation available in driver for now, 
                    this needs to be improved to detect error conditions
                    DMA_WRITE_INT_STATUS bits 16:25 */    
                    dma_clear_done(dma_chan_id);

                    /* Decrement the commands count being processed by the given SQW */
                    SQW_Decrement_Command_Count(
                        atomic_load_local_8(&DMA_Channel_Status.dma_wrt_chan[dma_chan_id].sqw_idx));

                    /* Update global  DMA channel status */
                    atomic_store_local_8
                        (&DMA_Channel_Status.dma_wrt_chan[dma_chan_id].channel_state, 
                        DMA_CHANNEL_AVAILABLE);
                    
                    /* Create and transmit DMA command response */
                    tag_id = atomic_load_local_16
                        (&DMA_Channel_Status.dma_wrt_chan[dma_chan_id].tag_id);

                    /* Create and transmit DMA command response */
                    read_rsp.response_info.rsp_hdr.size = sizeof(read_rsp);
                    read_rsp.response_info.rsp_hdr.tag_id = tag_id;
                    read_rsp.response_info.rsp_hdr.msg_id = 
                        DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_RSP;
                    /* Obtain wait latency, start cycles measured for the command */ 
                    read_rsp.cmd_wait_time = atomic_load_local_32
                        (&DMA_Channel_Status.dma_wrt_chan[dma_chan_id].wait_latency);
                    start_cycles = atomic_load_local_32
                        (&DMA_Channel_Status.dma_wrt_chan[dma_chan_id].cmd_dispatch_start_cycles);
                    /* Obtain current cycles, and compute command execution
                    latency */
                    read_rsp.cmd_execution_time = PMC_GET_LATENCY(start_cycles);
                    /* TODO: We should be able to capture other DMA states as well */
                    read_rsp.status = ETSOC_DMA_STATE_DONE; 

                    status = Host_Iface_CQ_Push_Cmd
                        (0, &read_rsp, sizeof(read_rsp));

                    if(status == STATUS_SUCCESS)
                    {
                        Log_Write(LOG_LEVEL_DEBUG, "%s", 
                            "DMAW:Pushed:DATA_READ_CMD_RSP->Host_CQ \r\n");
                    }
                    else
                    {
                        Log_Write(LOG_LEVEL_DEBUG, "%s", 
                            "DMAW:HostIface:Push:Failed\r\n");
                    }
                }
            }
        }
    };

    return;
}
