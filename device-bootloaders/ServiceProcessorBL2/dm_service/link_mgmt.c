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
/*! \file link_mgmt.c
    \brief A C module that implements the Link management services

    Public interfaces:
        link_mgmt_process_request
*/
/***********************************************************************/

#include "dm.h"
#include "dm_service.h"
#include "dm_task.h"
#include "sp_host_iface.h"
#include "bl2_link_mgmt.h"
#include "bl2_reset.h"

/************************************************************************
*
*   FUNCTION
*
*       link_mgmt_pcie_reset
*
*   DESCRIPTION
*
*       This function resets the PCIE.
*
*   INPUTS
*
*       req_start_time    Time stamp when the request was received by the Command
*                         Dispatcher
*       pcie_reset_type   PCIE Reset Type Enum value
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void link_mgmt_pcie_reset(uint16_t tag, uint64_t req_start_time, pcie_reset_e pcie_reset_type)
{
    // TODO: Reset SPEC work is in progress. Discuss if we need to send response
    //       to host. For now sending the response.
    struct device_mgmt_default_rsp_t dm_rsp;

    switch (pcie_reset_type) {
    case PCIE_RESET_FLR:
        // TODO : Add FLR reset function
        break;
    case PCIE_RESET_HOT:
        release_etsoc_reset();
        break;
    case PCIE_RESET_WARM:
        // TODO : Add WARM reset function
        break;
    }

    FILL_RSP_HEADER(dm_rsp, tag,
                    DM_CMD_SET_PCIE_RESET,
                    timer_get_ticks_count() - req_start_time,
                    DM_STATUS_SUCCESS);

    dm_rsp.payload = DM_STATUS_SUCCESS;

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_default_rsp_t))) {
        printf("link_mgmt_pcie_reset: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       link_mgmt_set_pcie_max_link_speed
*
*   DESCRIPTION
*
*       This function sets the PCIE maximum link speed.
*
*   INPUTS
*
*       req_start_time    Time stamp when the request was received by the Command
*                         Dispatcher
*       pcie_link_speed   PCIE Link Speed Enum value
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void link_mgmt_set_pcie_max_link_speed(uint16_t tag, uint64_t req_start_time,
                                              pcie_link_speed_e pcie_link_speed)
{
    struct device_mgmt_default_rsp_t dm_rsp;

    switch (pcie_link_speed) {
    case PCIE_LINK_SPEED_GEN3:
        // TODO : Add PCIE GEN3 Link Speed setup function
        break;
    case PCIE_LINK_SPEED_GEN4:
        // TODO : Add PCIE GEN4 Link Speed setup function
        break;
    }

    FILL_RSP_HEADER(dm_rsp, tag,
                    DM_CMD_SET_PCIE_MAX_LINK_SPEED,
                    timer_get_ticks_count() - req_start_time,
                    DM_STATUS_SUCCESS);

    dm_rsp.payload = DM_STATUS_SUCCESS;

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_default_rsp_t))) {
        printf("link_mgmt_set_pcie_max_link_speed: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       link_mgmt_set_pcie_lane_width
*
*   DESCRIPTION
*
*       This function set the PCIE Lane width.
*
*   INPUTS
*
*       req_start_time    Time stamp when the request was received by the Command
*                         Dispatcher
*       pcie_lane_w_split PCIE Lane Width Split Enum value
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void link_mgmt_set_pcie_lane_width(uint16_t tag, uint64_t req_start_time,
                                          pcie_lane_w_split_e pcie_lane_w_split)
{
    struct device_mgmt_default_rsp_t dm_rsp;

    switch (pcie_lane_w_split) {
    case PCIE_LANE_W_SPLIT_x4:
        // TODO : Add the PCIE GEN3 Link Speed setup function
        break;
    case PCIE_LANE_W_SPLIT_x8:
        // TODO : Add the PCIE GEN4 Link Speed setup function
        break;
    }

    FILL_RSP_HEADER(dm_rsp, tag,
                    DM_CMD_SET_PCIE_LANE_WIDTH,
                    timer_get_ticks_count() - req_start_time,
                    DM_STATUS_SUCCESS);

    dm_rsp.payload = DM_STATUS_SUCCESS;

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_default_rsp_t))) {
        printf("link_mgmt_set_pcie_lane_width: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       link_mgmt_pcie_retrain_phy
*
*   DESCRIPTION
*
*       This function starts the PCIE PHY re-training.
*
*   INPUTS
*
*       req_start_time    Time stamp when the request was received by the Command
*                         Dispatcher
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void link_mgmt_pcie_retrain_phy(uint16_t tag, uint64_t req_start_time)
{
    struct device_mgmt_default_rsp_t dm_rsp;

    //TODO : Add PHY Retraining

    FILL_RSP_HEADER(dm_rsp, tag,
                    DM_CMD_SET_PCIE_RETRAIN_PHY,
                    timer_get_ticks_count() - req_start_time,
                    DM_STATUS_SUCCESS);

    dm_rsp.payload = DM_STATUS_SUCCESS;

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_default_rsp_t))) {
        printf("link_mgmt_pcie_retrain_phy: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       link_mgmt_get_module_pcie_ecc_uecc
*
*   DESCRIPTION
*
*       This function gets the PCIE UECC count.
*
*   INPUTS
*
*       req_start_time    Time stamp when the request was received by the Command
*                         Dispatcher
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void link_mgmt_get_module_pcie_ecc_uecc(uint16_t tag, uint64_t req_start_time)
{
    struct device_mgmt_get_error_count_rsp_t dm_rsp;

    //TODO: Get the actual count values. SPEC work in progress.
    dm_rsp.errors_count.ecc = 0;
    dm_rsp.errors_count.uecc = 0;

    FILL_RSP_HEADER(dm_rsp, tag,
                    DM_CMD_GET_MODULE_PCIE_ECC_UECC,
                    timer_get_ticks_count() - req_start_time,
                    DM_STATUS_SUCCESS);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_get_error_count_rsp_t))) {
        printf("link_mgmt_get_module_pcie_ecc_uecc: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       link_mgmt_get_module_dram_uecc
*
*   DESCRIPTION
*
*       This function gets the PCIE DRAM UECC count.
*
*   INPUTS
*
*       req_start_time    Time stamp when the request was received by the Command
*                         Dispatcher
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void link_mgmt_get_module_dram_uecc(uint16_t tag, uint64_t req_start_time)
{
    struct device_mgmt_get_error_count_rsp_t dm_rsp;

    //TODO: Get the actual count values from BL2 Error data structure.
    dm_rsp.errors_count.ecc = 0;
    dm_rsp.errors_count.uecc = 0;

    FILL_RSP_HEADER(dm_rsp, tag,
                    DM_CMD_GET_MODULE_DDR_ECC_UECC,
                    timer_get_ticks_count() - req_start_time,
                    DM_STATUS_SUCCESS);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_get_error_count_rsp_t))) {
        printf("link_mgmt_get_module_dram_uecc: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       link_mgmt_get_module_sram_uecc
*
*   DESCRIPTION
*
*       This function gets the PCIE SRAM UECC count.
*
*   INPUTS
*
*       req_start_time    Time stamp when the request was received by the Command
*                         Dispatcher
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void link_mgmt_get_module_sram_uecc(uint16_t tag, uint64_t req_start_time)
{
    struct device_mgmt_get_error_count_rsp_t dm_rsp;

    //TODO: Get the actual count values from BL2 Error data structure.
    dm_rsp.errors_count.ecc = 0;
    dm_rsp.errors_count.uecc = 0;

    FILL_RSP_HEADER(dm_rsp, tag,
                    DM_CMD_GET_MODULE_SRAM_ECC_UECC,
                    timer_get_ticks_count() - req_start_time,
                    DM_STATUS_SUCCESS);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_get_error_count_rsp_t))) {
        printf("link_mgmt_get_module_sram_uecc: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       link_mgmt_get_module_ddr_bw_counter
*
*   DESCRIPTION
*
*       This function gets the PCIE module DDR BW counter.
*
*   INPUTS
*
*       req_start_time    Time stamp when the request was received by the Command
*                         Dispatcher
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void link_mgmt_get_module_ddr_bw_counter(uint16_t tag, uint64_t req_start_time)
{
    struct device_mgmt_dram_bw_counter_rsp_t dm_rsp;

    dm_rsp.dram_bw_counter.bw_rd_req_sec = get_soc_perf_reg()->dram_bw.read_req_sec;
    dm_rsp.dram_bw_counter.bw_wr_req_sec = get_soc_perf_reg()->dram_bw.write_req_sec;

    FILL_RSP_HEADER(dm_rsp, tag,
                    DM_CMD_GET_MODULE_DDR_BW_COUNTER,
                    timer_get_ticks_count() - req_start_time,
                    DM_STATUS_SUCCESS);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_dram_bw_counter_rsp_t))) {
        printf("link_mgmt_get_module_ddr_bw_counter: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       link_mgmt_process_request
*
*   DESCRIPTION
*
*       This function takes as input the command ID from Host,
*       and accordingly either calls the respective error control info
*       functions
*
*   INPUTS
*
*       msg_id      Unique enum representing specific command
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void link_mgmt_process_request(tag_id_t tag_id, msg_id_t msg_id)
{
    uint64_t req_start_time;

    req_start_time = timer_get_ticks_count();

    switch (msg_id) {
    case DM_CMD_SET_PCIE_RESET: {
        //TODO : PCIE Reset type should be used from command request payload
        link_mgmt_pcie_reset(tag_id, req_start_time, PCIE_RESET_HOT);
    } break;
    case DM_CMD_SET_PCIE_MAX_LINK_SPEED: {
        //TODO : PCIE Link Speed should be used from command request payload
        link_mgmt_set_pcie_max_link_speed(tag_id, req_start_time, PCIE_LINK_SPEED_GEN4);
    } break;
    case DM_CMD_SET_PCIE_LANE_WIDTH: {
        //TODO : PCIE Lane width should be used from command request payload
        link_mgmt_set_pcie_lane_width(tag_id, req_start_time, PCIE_LANE_W_SPLIT_x8);
    } break;
    case DM_CMD_SET_PCIE_RETRAIN_PHY: {
        link_mgmt_pcie_retrain_phy(tag_id, req_start_time);
    } break;
    case DM_CMD_GET_MODULE_PCIE_ECC_UECC: {
        link_mgmt_get_module_pcie_ecc_uecc(tag_id, req_start_time);
    } break;
    case DM_CMD_GET_MODULE_DDR_ECC_UECC: {
        link_mgmt_get_module_dram_uecc(tag_id, req_start_time);
    } break;
    case DM_CMD_GET_MODULE_SRAM_ECC_UECC: {
        link_mgmt_get_module_sram_uecc(tag_id, req_start_time);
    } break;
    case DM_CMD_GET_MODULE_DDR_BW_COUNTER: {
        link_mgmt_get_module_ddr_bw_counter(tag_id, req_start_time);
    } break;
    }
}
