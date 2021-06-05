/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------

*************************************************************************/
/*! \file error_control.c
    \brief A C module that implements the Error control services

    Public interfaces:
        error_ctl_set_ddr_ecc_count
        error_ctl_set_pcie_ecc_count
        error_ctl_set_sram_ecc_count
        error_control_process_request
*/
/***********************************************************************/

#include "bl2_error_control.h"
#include "sp_host_iface.h"
#include "bl2_cache_control.h"
#include "mem_controller.h"
#include "pcie_configuration.h"

/************************************************************************
*
*   FUNCTION
*
*       error_ctl_set_ddr_ecc_count
*
*   DESCRIPTION
*
*       This function sets the DDR ECC count.
*
*   INPUTS
*
*       req_start_time    Time stamp when the request was received by the Command
*                         Dispatcher
*       ecc_count         ECC error count
*
*   OUTPUTS
*
*       None
*
***********************************************************************/

static void error_ctl_set_ddr_ecc_count(tag_id_t tag_id, uint64_t req_start_time,
                                        uint32_t ecc_count)
{
    struct device_mgmt_default_rsp_t dm_rsp;
    int32_t status;

    status = ddr_set_ce_threshold(ecc_count);

    if (status) {
        Log_Write(LOG_LEVEL_ERROR, "error_ctl_set_ddr_ecc_count: driver error !\n");
    }

    FILL_RSP_HEADER(dm_rsp, tag_id, DM_CMD_SET_DDR_ECC_COUNT,
                    timer_get_ticks_count() - req_start_time, status);

    dm_rsp.payload = status;

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_default_rsp_t))) {
        Log_Write(LOG_LEVEL_ERROR, "error_ctl_set_ddr_ecc_count: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       error_ctl_set_pcie_ecc_count
*
*   DESCRIPTION
*
*       This function sets the PCIE ECC count.
*
*   INPUTS
*
*       req_start_time    Time stamp when the request was received by the Command
*                         Dispatcher
*       ecc_count         ECC error count
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void error_ctl_set_pcie_ecc_count(tag_id_t tag_id, uint64_t req_start_time,
                                         uint32_t ecc_count)
{
    struct device_mgmt_default_rsp_t dm_rsp;
    int32_t status;

    status = pcie_set_ce_threshold(ecc_count);

    if (status) {
        Log_Write(LOG_LEVEL_ERROR, "error_ctl_set_pcie_ecc_count: driver error !\n");
    }

    FILL_RSP_HEADER(dm_rsp, tag_id, DM_CMD_SET_PCIE_ECC_COUNT,
                    timer_get_ticks_count() - req_start_time, status);

    dm_rsp.payload = status;

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_default_rsp_t))) {
        Log_Write(LOG_LEVEL_ERROR, "error_ctl_set_pcie_ecc_count: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       error_ctl_set_sram_ecc_count
*
*   DESCRIPTION
*
*       This function sets the DDR ECC count.
*
*   INPUTS
*
*       req_start_time    Time stamp when the request was received by the Command
*                         Dispatcher
*       ecc_count         ECC error count
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
static void error_ctl_set_sram_ecc_count(tag_id_t tag_id, uint64_t req_start_time,
                                         uint32_t ecc_count)
{
    struct device_mgmt_default_rsp_t dm_rsp;
    int32_t status;

    status = sram_set_ce_threshold(ecc_count);

    if (status) {
        Log_Write(LOG_LEVEL_ERROR, "error_ctl_set_sram_ecc_count: driver error !\n");
    }

    FILL_RSP_HEADER(dm_rsp, tag_id, DM_CMD_SET_SRAM_ECC_COUNT,
                    timer_get_ticks_count() - req_start_time, status);

    dm_rsp.payload = status;

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_default_rsp_t))) {
        Log_Write(LOG_LEVEL_ERROR, "error_ctl_set_sram_ecc_count: Cqueue push error!\n");
    }
}

/************************************************************************
*
*   FUNCTION
*
*       error_control_process_request
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
void error_control_process_request(tag_id_t tag_id, msg_id_t msg_id)
{
    uint64_t req_start_time;
    uint32_t ecc_count = 0;

    req_start_time = timer_get_ticks_count();

    // TODO : Retrieve ecc_count

    switch (msg_id) {
    case DM_CMD_SET_DDR_ECC_COUNT: {
        error_ctl_set_ddr_ecc_count(tag_id, req_start_time, ecc_count);
    } break;
    case DM_CMD_SET_PCIE_ECC_COUNT: {
        error_ctl_set_pcie_ecc_count(tag_id, req_start_time, ecc_count);
    } break;
    case DM_CMD_SET_SRAM_ECC_COUNT: {
        error_ctl_set_sram_ecc_count(tag_id, req_start_time, ecc_count);
    } break;
    }
}
