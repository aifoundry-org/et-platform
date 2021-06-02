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

#include "bl2_link_mgmt.h"

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
static void link_mgmt_pcie_reset(uint16_t tag, uint64_t req_start_time,
                                 pcie_reset_e pcie_reset_type)
{
    // TODO: Reset SPEC work is in progress. Discuss if we need to send response
    //       to host. For now sending the response.
    struct device_mgmt_default_rsp_t dm_rsp;

    switch (pcie_reset_type) {
    case PCIE_RESET_FLR:
        pcie_reset_flr();
        break;
    case PCIE_RESET_HOT:
        release_etsoc_reset();
        break;
    case PCIE_RESET_WARM:
        pcie_reset_warm();
        break;
    }

    FILL_RSP_HEADER(dm_rsp, tag, DM_CMD_SET_PCIE_RESET, timer_get_ticks_count() - req_start_time,
                    DM_STATUS_SUCCESS);

    dm_rsp.payload = DM_STATUS_SUCCESS;

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_default_rsp_t))) {
        Log_Write(LOG_LEVEL_ERROR, "link_mgmt_pcie_reset: Cqueue push error!\n");
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
    int status = DM_STATUS_SUCCESS;

    switch (pcie_link_speed) {
    case PCIE_LINK_SPEED_GEN3:
        status = setup_pcie_gen3_link_speed();
        break;
    case PCIE_LINK_SPEED_GEN4:
        status = setup_pcie_gen3_link_speed();
        break;
    }

    FILL_RSP_HEADER(dm_rsp, tag, DM_CMD_SET_PCIE_MAX_LINK_SPEED,
                    timer_get_ticks_count() - req_start_time, status);

    dm_rsp.payload = status;

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_default_rsp_t))) {
        Log_Write(LOG_LEVEL_ERROR, "link_mgmt_set_pcie_max_link_speed: Cqueue push error!\n");
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
    int32_t status = DM_STATUS_SUCCESS;

    switch (pcie_lane_w_split) {
    case PCIE_LANE_W_SPLIT_x4:
        status = setup_pcie_lane_width_x4();
        break;
    case PCIE_LANE_W_SPLIT_x8:
        status = setup_pcie_lane_width_x8();
        break;
    }

    FILL_RSP_HEADER(dm_rsp, tag, DM_CMD_SET_PCIE_LANE_WIDTH,
                    timer_get_ticks_count() - req_start_time, status);

    dm_rsp.payload = status;

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_default_rsp_t))) {
        Log_Write(LOG_LEVEL_ERROR, "link_mgmt_set_pcie_lane_width: Cqueue push error!\n");
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
    int32_t status;

    status = pcie_retrain_phy();

    FILL_RSP_HEADER(dm_rsp, tag, DM_CMD_SET_PCIE_RETRAIN_PHY,
                    timer_get_ticks_count() - req_start_time, status);

    dm_rsp.payload = status;

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_default_rsp_t))) {
        Log_Write(LOG_LEVEL_ERROR, "link_mgmt_pcie_retrain_phy: Cqueue push error!\n");
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
    uint32_t ce_count;
    uint32_t uce_count;
    int32_t status;

    status = pcie_get_ce_count(&ce_count);

    if (0 != status) {
        Log_Write(LOG_LEVEL_ERROR, "pcie_get_ce_count : driver error !\n");
    } else {
        dm_rsp.errors_count.ecc = ce_count;
    }

    status = pcie_get_uce_count(&uce_count);

    if (0 != status) {
        Log_Write(LOG_LEVEL_ERROR, "pcie_get_uce_count : driver error !\n");
    } else {
        dm_rsp.errors_count.uecc = uce_count;
    }

    FILL_RSP_HEADER(dm_rsp, tag, DM_CMD_GET_MODULE_PCIE_ECC_UECC,
                    timer_get_ticks_count() - req_start_time, status);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp,
                                       sizeof(struct device_mgmt_get_error_count_rsp_t))) {
        Log_Write(LOG_LEVEL_ERROR, "link_mgmt_get_module_pcie_ecc_uecc: Cqueue push error!\n");
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
    uint32_t ce_count;
    uint32_t uce_count;
    int32_t status;

    status = ddr_get_ce_count(&ce_count);

    if (0 != status) {
        Log_Write(LOG_LEVEL_ERROR, "ddr_get_ce_count : driver error !\n");
    } else {
        dm_rsp.errors_count.ecc = ce_count;
    }

    status = ddr_get_uce_count(&uce_count);

    if (0 != status) {
        Log_Write(LOG_LEVEL_ERROR, "ddr_get_ce_count : driver error !\n");
    } else {
        dm_rsp.errors_count.uecc = uce_count;
    }

    FILL_RSP_HEADER(dm_rsp, tag, DM_CMD_GET_MODULE_DDR_ECC_UECC,
                    timer_get_ticks_count() - req_start_time, status);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp,
                                       sizeof(struct device_mgmt_get_error_count_rsp_t))) {
        Log_Write(LOG_LEVEL_ERROR, "link_mgmt_get_module_dram_uecc: Cqueue push error!\n");
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
    uint32_t ce_count;
    uint32_t uce_count;
    int32_t status;

    status = sram_get_ce_count(&ce_count);

    if (0 != status) {
        Log_Write(LOG_LEVEL_ERROR, "sram_get_ce_count : driver error !\n");
    } else {
        dm_rsp.errors_count.ecc = ce_count;
    }

    status = sram_get_uce_count(&uce_count);

    if (0 != status) {
        Log_Write(LOG_LEVEL_ERROR, "sram_get_uce_count : driver error !\n");
    } else {
        dm_rsp.errors_count.uecc = uce_count;
    }

    FILL_RSP_HEADER(dm_rsp, tag, DM_CMD_GET_MODULE_SRAM_ECC_UECC,
                    timer_get_ticks_count() - req_start_time, status);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp,
                                       sizeof(struct device_mgmt_get_error_count_rsp_t))) {
        Log_Write(LOG_LEVEL_ERROR, "link_mgmt_get_module_sram_uecc: Cqueue push error!\n");
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
    struct dram_bw_t dram_bw;
    int32_t status;

    status = get_module_dram_bw(&dram_bw);

    if (0 != status) {
        Log_Write(LOG_LEVEL_ERROR, " perf mgmt error: get_module_dram_bw()\r\n");
    } else {
        dm_rsp.dram_bw_counter.bw_rd_req_sec = dram_bw.read_req_sec;
        dm_rsp.dram_bw_counter.bw_wr_req_sec = dram_bw.write_req_sec;
    }

    FILL_RSP_HEADER(dm_rsp, tag, DM_CMD_GET_MODULE_DDR_BW_COUNTER,
                    timer_get_ticks_count() - req_start_time, status);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp,
                                       sizeof(struct device_mgmt_dram_bw_counter_rsp_t))) {
        Log_Write(LOG_LEVEL_ERROR, "link_mgmt_get_module_ddr_bw_counter: Cqueue push error!\n");
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
void link_mgmt_process_request(tag_id_t tag_id, msg_id_t msg_id, void *buffer)
{
    uint64_t req_start_time;

    req_start_time = timer_get_ticks_count();

    switch (msg_id) {
    case DM_CMD_SET_PCIE_RESET: {
        struct device_mgmt_pcie_reset_cmd_t *pcie_reset_cmd =
            (struct device_mgmt_pcie_reset_cmd_t *)buffer;
        link_mgmt_pcie_reset(tag_id, req_start_time, pcie_reset_cmd->reset_type);
        break;
    }
    case DM_CMD_SET_PCIE_MAX_LINK_SPEED: {
        struct device_mgmt_pcie_link_speed_cmd_t *pcie_link_speed_cmd =
            (struct device_mgmt_pcie_link_speed_cmd_t *)buffer;
        link_mgmt_set_pcie_max_link_speed(tag_id, req_start_time, pcie_link_speed_cmd->speed);
        break;
    }
    case DM_CMD_SET_PCIE_LANE_WIDTH: {
        struct device_mgmt_pcie_lane_width_cmd_t *pcie_lane_width_cmd =
            (struct device_mgmt_pcie_lane_width_cmd_t *)buffer;
        link_mgmt_set_pcie_lane_width(tag_id, req_start_time, pcie_lane_width_cmd->lane_width);
        break;
    }
    case DM_CMD_SET_PCIE_RETRAIN_PHY: {
        link_mgmt_pcie_retrain_phy(tag_id, req_start_time);
        break;
    }
    case DM_CMD_GET_MODULE_PCIE_ECC_UECC: {
        link_mgmt_get_module_pcie_ecc_uecc(tag_id, req_start_time);
        break;
    }
    case DM_CMD_GET_MODULE_DDR_ECC_UECC: {
        link_mgmt_get_module_dram_uecc(tag_id, req_start_time);
        break;
    }
    case DM_CMD_GET_MODULE_SRAM_ECC_UECC: {
        link_mgmt_get_module_sram_uecc(tag_id, req_start_time);
        break;
    }
    case DM_CMD_GET_MODULE_DDR_BW_COUNTER: {
        link_mgmt_get_module_ddr_bw_counter(tag_id, req_start_time);
        break;
    }
    }
}
