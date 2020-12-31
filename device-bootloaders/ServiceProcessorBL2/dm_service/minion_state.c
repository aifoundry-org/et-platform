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
*       This file implements the Master minion thread state service.
*
*   FUNCTIONS
*
*       Host Requested Services functions:
*       
*       - mm_state_process_request
*
***********************************************************************/

#include "dm.h"
#include "dm_service.h"
#include "sp_host_iface.h"
#include "bl2_minion_state.h"

/************************************************************************
*
*   FUNCTION
*
*       mm_state_process_request
*  
*   DESCRIPTION
*
*       This function returns the state of master thread state, 
*       VQ submission/completion thread state, VQ worker thread state.
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
void mm_state_process_request(tag_id_t tag_id)
{
    struct device_mgmt_mm_state_rsp_t dm_rsp;
    uint64_t req_start_time;

    req_start_time = timer_get_ticks_count();

    //TODO : SP needs to get the thread states from MM. Currently providing dummy response.
    dm_rsp.mm_state.master_thread_state = 0;
    dm_rsp.mm_state.vq_s_thread_state = 0;
    dm_rsp.mm_state.vq_c_thread_state = 0;
    dm_rsp.mm_state.vq_w_thread_state = 0;

    FILL_RSP_HEADER(dm_rsp, tag_id,
                    DM_CMD_GET_MM_THREADS_STATE,
                    timer_get_ticks_count() - req_start_time,
                    DM_STATUS_SUCCESS);

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_mm_state_rsp_t))) {
        printf("mm_state_process_request: Cqueue push error !\n");
    }
}
