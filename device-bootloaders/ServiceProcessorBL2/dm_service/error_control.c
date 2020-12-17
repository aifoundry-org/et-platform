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

#include "bl2_error_control.h"
#include "dm.h"

static uint8_t cqueue_push(char * buf, uint32_t size) 
{
     printf("Pointer to buf %s with payload size: %d\n", buf, size); 
     return 0;
}

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

static void error_ctl_set_ddr_ecc_count(uint64_t req_start_time, uint32_t ecc_count)
{
    struct rsp_hdr_t dm_rsp;
    dm_rsp.status = DM_STATUS_SUCCESS;
    dm_rsp.size = 0;

    (void)ecc_count;
    //TODO: Set the DDR ECC Count in BL2 Error data struct

    dm_rsp.device_latency_usec = timer_get_ticks_count() - req_start_time;

    char buffer[sizeof(dm_rsp)];
    char *p = buffer; 
    memcpy(p, &dm_rsp, sizeof(dm_rsp)); 

   if (0 != cqueue_push(p,sizeof(dm_rsp))) {
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
static void error_ctl_set_pcie_ecc_count(uint64_t req_start_time, uint32_t ecc_count)
{
    struct rsp_hdr_t dm_rsp;
    dm_rsp.status = DM_STATUS_SUCCESS;
    dm_rsp.size = 0;

    (void)ecc_count;
    //TODO: Set the PCIE ECC Count.

    dm_rsp.device_latency_usec = timer_get_ticks_count() - req_start_time;

    char buffer[sizeof(dm_rsp)];
    char *p = buffer; 
    memcpy(p, &dm_rsp, sizeof(dm_rsp)); 

   if (0 != cqueue_push(p,sizeof(dm_rsp))) {
        printf("error_ctl_set_ddr_ecc_count: Cqueue push error !\n");
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
static void error_ctl_set_sram_ecc_count(uint64_t req_start_time, uint32_t ecc_count)
{

    struct rsp_hdr_t dm_rsp;
    dm_rsp.status = DM_STATUS_SUCCESS;
    dm_rsp.size = 0;

    (void)ecc_count;
    //TODO: Set the SRAM ECC Count in BL2 Error data struct.

    dm_rsp.device_latency_usec = timer_get_ticks_count() - req_start_time;

    char buffer[sizeof(dm_rsp)];
    char *p = buffer; 
    memcpy(p, &dm_rsp, sizeof(dm_rsp)); 

   if (0 != cqueue_push(p,sizeof(dm_rsp))) {
        printf("error_ctl_set_ddr_ecc_count: Cqueue push error !\n");
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
void error_control_process_request(uint32_t cmd_id)
{
    uint64_t req_start_time;
    uint32_t ecc_count = 0;

    req_start_time = timer_get_ticks_count();

    // TODO : Retrieve ecc_count

    switch (cmd_id) {
    case SET_DDR_ECC_COUNT: {
        error_ctl_set_ddr_ecc_count(req_start_time, ecc_count);
    } break;
    case SET_PCIE_ECC_COUNT: {
        error_ctl_set_pcie_ecc_count(req_start_time, ecc_count);
    } break;
    case SET_SRAM_ECC_COUNT: {
        error_ctl_set_sram_ecc_count(req_start_time, ecc_count);
    } break;
    }
}
