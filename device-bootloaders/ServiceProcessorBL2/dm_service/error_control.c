/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------

************************************************************************

************************************************************************
*
*   DESCRIPTION
*
*       This file implements the following Error Control services.
*
*   FUNCTIONS
*
*       Host Requested Services functions:
*       
*       - error_ctl_set_ddr_ecc_count
*       - error_ctl_set_pcie_ecc_count
*       - error_ctl_set_sram_ecc_count
*       - error_control_process_request
*
***********************************************************************/

#include "dm.h"
#include "dm_service.h"
#include "sp_host_iface.h"
#include "bl2_error_control.h"

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

static void error_ctl_set_ddr_ecc_count(tag_id_t tag_id, uint64_t req_start_time, uint32_t ecc_count)
{
    struct device_mgmt_get_error_count_rsp_t dm_rsp;

    (void)ecc_count;
    //TODO: Set the DDR ECC Count in BL2 Error data struct
    FILL_RSP_HEADER(dm_rsp, tag_id,
                    DM_CMD_GET_ASIC_LATENCY,
                    timer_get_ticks_count() - req_start_time,
                    DM_STATUS_SUCCESS);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct dev_mgmt_rsp_header_t))) {
        printf("error_ctl_set_ddr_ecc_count: Cqueue push error !\n");
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
static void error_ctl_set_pcie_ecc_count(tag_id_t tag_id, uint64_t req_start_time, uint32_t ecc_count)
{
    struct device_mgmt_get_error_count_rsp_t dm_rsp;

    (void)ecc_count;
    //TODO: Set the PCIE ECC Count.
    FILL_RSP_HEADER(dm_rsp, tag_id,
                    DM_CMD_GET_ASIC_LATENCY,
                    timer_get_ticks_count() - req_start_time,
                    DM_STATUS_SUCCESS);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct dev_mgmt_rsp_header_t))) {
        printf("error_ctl_set_pcie_ecc_count: Cqueue push error !\n");
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
static void error_ctl_set_sram_ecc_count(tag_id_t tag_id, uint64_t req_start_time, uint32_t ecc_count)
{
    struct device_mgmt_get_error_count_rsp_t dm_rsp;

    (void)ecc_count;
    //TODO: Set the SRAM ECC Count in BL2 Error data struct.
    FILL_RSP_HEADER(dm_rsp, tag_id,
                    DM_CMD_GET_ASIC_LATENCY,
                    timer_get_ticks_count() - req_start_time,
                    DM_STATUS_SUCCESS);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct dev_mgmt_rsp_header_t))) {
        printf("error_ctl_set_sram_ecc_count: Cqueue push error !\n");
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
*       cmd_id      Unique enum representing specific command   
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
