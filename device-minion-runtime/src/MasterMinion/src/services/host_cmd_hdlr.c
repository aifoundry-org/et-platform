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
/*! \file host_cmd_hdlr.c
    \brief A C module that implements the host command handler
    responsible for handling all commands from host

    Public interfaces:
        Host_Command_Handler
*/
/***********************************************************************/
#include "services/host_cmd_hdlr.h"
#include "services/host_iface.h"
#include "services/worker_iface.h"
#include "services/log1.h"
#include "workers/dmaw.h"
#include "workers/sqw.h"
#include "pcie_dma.h"
#include "pmu.h"

/************************************************************************
*
*   FUNCTION
*
*       Host_Command_Handler
*  
*   DESCRIPTION
*
*       Handle a host command and process its response.
*
*   INPUTS
*
*       command_buffer   Buffer containing command to process
*       start_cycle      Cycle count to measure wait latency
*
*   OUTPUTS
*
*       int8_t           Successful status or error code.
*
***********************************************************************/
int8_t Host_Command_Handler(void* command_buffer, uint8_t sqw_idx, 
    uint64_t start_cycles)
{
    int8_t status = STATUS_SUCCESS;
    struct cmd_header_t *hdr = command_buffer;
    dma_channel_status_t *p_DMA_Channel_Status;

    p_DMA_Channel_Status = 
        (dma_channel_status_t*)DMAW_Get_DMA_Channel_Status_Addr();

    switch (hdr->cmd_hdr.msg_id) 
    {
        case DEV_OPS_API_MID_CHECK_DEVICE_OPS_API_COMPATIBILITY_CMD:
        {
            struct device_ops_api_compatibility_rsp_t rsp;
            
            Log_Write(LOG_LEVEL_DEBUG, "%s", 
                "HostCommandHandler:Processing:COMPATIBILITY_CMD\r\n");

            /* Construct and transmit response */
            rsp.response_info.rsp_hdr.tag_id = hdr->cmd_hdr.tag_id;
            rsp.response_info.rsp_hdr.msg_id = 
                DEV_OPS_API_MID_DEVICE_OPS_API_COMPATIBILITY_RSP;
            rsp.response_info.rsp_hdr.size = 
                sizeof(struct device_ops_api_compatibility_rsp_t);
            rsp.major = DEVICE_OPS_API_MAJOR;
            rsp.minor = DEVICE_OPS_API_MINOR;
            rsp.patch = DEVICE_OPS_API_PATCH;

            status = Host_Iface_CQ_Push_Cmd(0, &rsp, sizeof(rsp));

            if(status == STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_DEBUG, "%s", 
                "HostCommandHandler:Pushed:COMPATIBILITY_CMD->Host_CQ\r\n");
            }
            else
            {
                Log_Write(LOG_LEVEL_DEBUG, "%s", 
                "HostCommandHandler:HostIface:Push:Failed\r\n");
            }

            /* Decrement commands count being processed by given SQW */
            SQW_Decrement_Command_Count(sqw_idx);

            break;
        }
        case DEV_OPS_API_MID_DEVICE_OPS_DEVICE_FW_VERSION_CMD:
        {
            struct device_ops_device_fw_version_cmd_t *cmd = (void *)hdr;
            struct device_ops_fw_version_rsp_t rsp;

            Log_Write(LOG_LEVEL_DEBUG, "%s", 
                "HostCommandHandler:Processing:FW_VERSION_CMD\r\n");

            /* Construct and transmit response */
            rsp.response_info.rsp_hdr.tag_id = hdr->cmd_hdr.tag_id;
            rsp.response_info.rsp_hdr.msg_id = 
                DEV_OPS_API_MID_DEVICE_OPS_FW_VERSION_RSP;
            rsp.response_info.rsp_hdr.size = 
                sizeof(struct device_ops_fw_version_rsp_t);
            if (cmd->firmware_type == DEV_OPS_FW_TYPE_MASTER_MINION_FW) 
            {
                /* TODO: implement proper logic to fetch and 
                return firmware version */
                rsp.major = 1;
                rsp.minor = 0;
                rsp.patch = 0;
                rsp.type = DEV_OPS_FW_TYPE_MASTER_MINION_FW;
            } 
            else if (cmd->firmware_type == DEV_OPS_FW_TYPE_MACHINE_MINION_FW) 
            {
                /*TODO: implement proper logic to fetch and return 
                firmware version */
                rsp.major = 1;
                rsp.minor = 0;
                rsp.patch = 0;
                rsp.type = DEV_OPS_FW_TYPE_MACHINE_MINION_FW;
            } 
            else if (cmd->firmware_type == DEV_OPS_FW_TYPE_WORKER_MINION_FW) 
            {
                /*TODO: implement proper logic to fetch and return 
                firmware version */
                rsp.major = 1;
                rsp.minor = 0;
                rsp.patch = 0;
                rsp.type = DEV_OPS_FW_TYPE_WORKER_MINION_FW;
            }
            
            status = Host_Iface_CQ_Push_Cmd(0, &rsp, sizeof(rsp));

            if(status == STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_DEBUG, "%s", 
                    "HostCommandHandler:Pushed:FW_VERSION_CMD->Host_CQ\r\n");
            }
            else
            {
                Log_Write(LOG_LEVEL_DEBUG, "%s", 
                    "HostCommandHandler:HostIface:Push:Failed\r\n");
            }

            /* Decrement commands count being processed by given SQW */
            SQW_Decrement_Command_Count(sqw_idx);

            break;
        }
        case DEV_OPS_API_MID_DEVICE_OPS_ECHO_CMD:
        {
            struct device_ops_echo_cmd_t *cmd = (void *)hdr;
            struct device_ops_echo_rsp_t rsp;

            Log_Write(LOG_LEVEL_DEBUG, "%s", 
                "HostCommandHandler:Processing:ECHO_CMD\r\n");

            /* Construct and transmit response */
            rsp.response_info.rsp_hdr.tag_id = hdr->cmd_hdr.tag_id;
            rsp.response_info.rsp_hdr.msg_id = 
                DEV_OPS_API_MID_DEVICE_OPS_ECHO_RSP;
            rsp.response_info.rsp_hdr.size = 
                sizeof(struct device_ops_echo_rsp_t);
            rsp.echo_payload = cmd->echo_payload;            

            status = Host_Iface_CQ_Push_Cmd(0, &rsp, sizeof(rsp));

            if(status == STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_DEBUG, "%s", 
                    "HostCommandHandler:Pushed:ECHO_CMD->Host_CQ \r\n");
            }
            else
            {
                Log_Write(LOG_LEVEL_DEBUG, "%s", 
                    "HostCommandHandler:HostIface:Push:Failed\r\n");
            }

            /* Decrement commands count being processed by given SQW */
            SQW_Decrement_Command_Count(sqw_idx);

            break;
        }
        case DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_CMD:
        case DEV_OPS_API_MID_DEVICE_OPS_KERNEL_ABORT_CMD:
        case DEV_OPS_API_MID_DEVICE_OPS_KERNEL_STATE_CMD:
        {
            Log_Write(LOG_LEVEL_DEBUG, "%s", 
                "HostCommandHandler:UnsupportedCmd \r\n");

            break;
        }
        case DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_CMD:
        {
            struct device_ops_data_read_cmd_t *cmd = (void *)hdr;
            et_dma_chan_id_e chan;
            DMA_STATUS_e dma_status = DMA_OPERATION_NOT_SUCCESS;
            uint32_t temp;

            /* Design Notes: Note a DMA write command from host will trigger 
            the implementation to configure a DMA read channel on device to move 
            data from host to device, similarly a read command from host will 
            trigger the implementation to configure a DMA write channel on device
            to move data from device to host */            
            Log_Write(LOG_LEVEL_DEBUG, "%s", 
                "HostCommandHandler:Processing:DATA_READ_CMD\r\n");

            /* Obtain the next available DMA read channel */
            dma_status = dma_chan_find_idle(DMA_DEVICE_TO_HOST, &chan);

            if(dma_status == DMA_OPERATION_SUCCESS)
            {
                Log_Write(LOG_LEVEL_DEBUG, 
                    "%s%d%s", "DMA_READ:channel_used:",chan, "\r\n");
                Log_Write(LOG_LEVEL_DEBUG, 
                    "%s%llx%s", "DMA_READ:src_device_phy_addr:",
                    cmd->src_device_phy_addr, "\r\n");
                Log_Write(LOG_LEVEL_DEBUG, 
                    "%s%llx%s", "DMA_READ:dst_host_phy_addr:",
                    cmd->dst_host_phy_addr, "\r\n");
                Log_Write(LOG_LEVEL_DEBUG, 
                    "%s%d%s", "DMA_READ:size:",cmd->size, "\r\n");
                
                /* Update the Global DMA Channel Status data structure
                - Set tag ID, set channel state to active, set SQW Index */
                temp = (uint32_t)((sqw_idx << 24)| 
                    (DMA_CHANNEL_IN_USE << 16) | hdr->cmd_hdr.tag_id); 
                atomic_store_local_32
                    ((volatile uint32_t*)&p_DMA_Channel_Status->dma_wrt_chan[chan], 
                    temp);
                /* Update the Global Channel Status data structure 
                - set wait latency, set cmd_dispatch_start_cycles */
                temp = PMC_GET_LATENCY(start_cycles);
                atomic_store_local_32
                ((volatile uint32_t*)&p_DMA_Channel_Status->dma_wrt_chan[chan].wait_latency, 
                temp);
                temp =  (uint32_t)(PMC_GET_CURRENT_CYCLES & 0xFFFFFFFF);
                atomic_store_local_32
                ((volatile uint32_t*)&p_DMA_Channel_Status->dma_wrt_chan[chan].cmd_dispatch_start_cycles, 
                temp);

                /* Initiate DMA transfer */
                /* TODO: BAR relative address will be received form host
                (host_phy_addr) this needs to fixed up here to device address 
                before DMA is triggered */
                dma_status = dma_trigger_transfer2(DMA_DEVICE_TO_HOST, 
                    cmd->src_device_phy_addr, cmd->dst_host_phy_addr, 
                    cmd->size, chan);

                if(dma_status == DMA_OPERATION_SUCCESS)
                {
                    Log_Write(LOG_LEVEL_DEBUG, "%s", 
                        "HostCommandHandler:DMATriggerTransfer:Success! \r\n");
                }
            }

            break;            
        }

        case DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_CMD:
        {
            struct device_ops_data_write_cmd_t  *cmd = (void *)hdr;
            et_dma_chan_id_e chan;
            DMA_STATUS_e dma_status = DMA_OPERATION_NOT_SUCCESS;
            uint32_t temp;

            /* Design Notes: Note a DMA write command from host will trigger 
            the implementation to configure a DMA read channel on device to move 
            data from host to device, similarly a read command from host will 
            trigger the implementation to configure a DMA write channel on device
            to move data from device to host */
            Log_Write(LOG_LEVEL_DEBUG, "%s", 
                "HostCommandHandler:Processing:DATA_WRITE_CMD\r\n");

            /* Obtain the next available DMA write channel */
            dma_status = dma_chan_find_idle(DMA_HOST_TO_DEVICE, &chan);

            if(dma_status == DMA_OPERATION_SUCCESS)
            {
                Log_Write(LOG_LEVEL_DEBUG, 
                    "%s%d%s", "DMA_WRITE:channel_used:",chan, "\r\n");
                Log_Write(LOG_LEVEL_DEBUG, 
                    "%s%llx%s", "DMA_WRITE:src_host_virt_addr:",
                    cmd->src_host_virt_addr, "\r\n");
                Log_Write(LOG_LEVEL_DEBUG, 
                    "%s%llx%s", "DMA_WRITE:src_host_phy_addr:",
                    cmd->src_host_phy_addr, "\r\n");
                Log_Write(LOG_LEVEL_DEBUG, 
                    "%s%llx%s", "DMA_WRITE:dst_device_phy_addr:",
                    cmd->dst_device_phy_addr, "\r\n");
                Log_Write(LOG_LEVEL_DEBUG, 
                    "%s%d%s", "DMA_WRITE:size:",cmd->size, "\r\n");

                /* Update the Global DMA Channel Status data structure
                - Set tag ID, set channel state to active, set SQW Index */
                temp = (uint32_t)((sqw_idx << 24)| 
                    (DMA_CHANNEL_IN_USE << 16) | hdr->cmd_hdr.tag_id); 
                atomic_store_local_32
                    ((volatile uint32_t*)&p_DMA_Channel_Status->dma_rd_chan[chan], 
                    temp);

                /* Update the Global Channel Status data structure 
                - set wait latency, set cmd_dispatch_start_cycles */
                temp = PMC_GET_LATENCY(start_cycles);
                atomic_store_local_32
                ((volatile uint32_t*)&p_DMA_Channel_Status->dma_rd_chan[chan].wait_latency, 
                temp);
                temp = (uint32_t)(PMC_GET_CURRENT_CYCLES & 0xFFFFFFFF);
                atomic_store_local_32
                ((volatile uint32_t*)&p_DMA_Channel_Status->dma_rd_chan[chan].cmd_dispatch_start_cycles, 
                temp);

                /* Initiate DMA transfer */
                /* TODO: BAR relative address will be received form host
                (host_phy_addr) this needs to fixed up here to device address 
                before DMA is triggered */
                dma_status = dma_trigger_transfer2(DMA_HOST_TO_DEVICE, 
                    cmd->src_host_phy_addr, cmd->dst_device_phy_addr, 
                    cmd->size, chan);

                if(dma_status == DMA_OPERATION_SUCCESS)
                {
                    Log_Write(LOG_LEVEL_DEBUG, "%s", 
                        "HostCommandHandler:DMATriggerTransfer:Success! \r\n");
                }
            }

            break;
        }
        default: 
        {
            /* Decrement commands count being processed by given SQW */
            SQW_Decrement_Command_Count(sqw_idx);
            
            Log_Write(LOG_LEVEL_DEBUG, "%s", 
                "HostCommandHandler:UnsupportedCmd \r\n");
            status = -1;
            break;
        }
    }

    return status;
}
