/*************************************************************************
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
*************************************************************************/
/*! \file firmware_update.c
    \brief A C module that implements the master minion state service.

    Public interfaces:
        Minion_State_Host_Iface_Process_Request
*/
/***********************************************************************/

#include "dm.h"
#include "dm_service.h"
#include "sp_host_iface.h"
#include "sp_mm_iface.h"
#include "mm_sp_cmd_spec.h"
#include "minion_state.h"

static uint64_t g_active_shire_mask = 0;

void Minion_State_Init(uint64_t active_shire_mask)
{
    g_active_shire_mask = active_shire_mask;
}

void Minion_State_Host_Iface_Process_Request(tag_id_t tag_id, msg_id_t msg_id)
{
    uint64_t req_start_time;

    req_start_time = timer_get_ticks_count();

    switch (msg_id) {
    case DM_CMD_GET_MM_THREADS_STATE: {
        struct device_mgmt_mm_state_rsp_t dm_rsp;
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
            printf("Minion_State_Host_Iface_Process_Request: Cqueue push error!\n");
        }
        break;
    }
    }
}

void Minion_State_MM_Iface_Process_Request(uint8_t msg_id)
{
    switch (msg_id) {
    case MM2SP_CMD_GET_ACTIVE_SHIRE_MASK: {
        struct mm2sp_get_active_shire_mask_rsp_t rsp;
        rsp.msg_hdr.msg_id = MM2SP_RSP_GET_ACTIVE_SHIRE_MASK;
        rsp.active_shire_mask = (uint32_t)(g_active_shire_mask & 0xFFFFFFFF); // Compute shires

        if (0 != SP_MM_Iface_CQ_Push_Cmd((char *)&rsp, sizeof(rsp))) {
            printf("SP_MM_Iface_CQ_Push_Cmd: Cqueue push error!\n");
        }
        break;
    }
    }
}
