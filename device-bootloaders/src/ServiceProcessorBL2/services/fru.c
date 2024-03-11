/***********************************************************************
*
* Copyright (C) 2024 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
/*! \file fru.c
    \brief A C module that implements the Field Replaceable Unit (FRU) service

    Public interfaces:
        fru_svc_getfruinfo
        fru_svc_setfruinfo
*/
/***********************************************************************/
#include "bl2_fru.h"

/************************************************************************
*
*   FUNCTION
*
*       fru_svc_getfruinfo
*
*   DESCRIPTION
*
*       This function takes command ID as input from Host,
*       and accordingly retrieves the FRU information from the 
*       PMIC NVM
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
static void fru_svc_getfruinfo(tag_id_t tag_id, uint64_t req_start_time)
{
    int32_t status = 0;
    struct device_mgmt_get_fru_rsp_t dm_rsp = { 0 };
    struct fru_data_t fru_data = { 0 };

    /* Read FRU data from PMIC NVM */
    status = pmic_read_fru(&fru_data);

    if (0 != status)
    {
        Log_Write(LOG_LEVEL_ERROR, "FRU svc error: %s\n", __func__);
    }
    else
    {
        /* Copy contents of FRU to be sent back to Host */
        memcpy(&dm_rsp.fru_data, &fru_data, sizeof(struct fru_data_t));
    }

    Log_Write(LOG_LEVEL_INFO, "FRU response: %s\n", __func__);

    FILL_RSP_HEADER(dm_rsp, tag_id, DM_CMD_GET_FRU, timer_get_ticks_count() - req_start_time,
                    status)

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_get_fru_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "%s: Cqueue push error!\n", __func__);
    }
}

/************************************************************************
*
*   FUNCTION
*
*       fru_svc_setfruinfo
*
*   DESCRIPTION
*
*       This function takes command ID as input from Host,
*       and accordingly updates the FRU information in the 
*       PMIC NVM
*
*   INPUTS
*
*       msg_id      Unique enum representing specific command
*       buffer      Pointer to command buffer
*
*   OUTPUTS
*
*       None
*
***********************************************************************/

static void fru_svc_setfruinfo(tag_id_t tag_id, uint64_t req_start_time, const void *buffer)
{
    int32_t status = 0;
    const struct device_mgmt_set_fru_cmd_t *set_fru_cmd =
        (const struct device_mgmt_set_fru_cmd_t *)buffer;
    struct device_mgmt_set_fru_rsp_t dm_rsp;

    Log_Write(LOG_LEVEL_INFO, "FRU svc request: %s\n", __func__);

    status = pmic_set_fru(&set_fru_cmd->fru_data);

    if (0 != status)
    {
        Log_Write(LOG_LEVEL_ERROR, "FRU svc error: %s\n", __func__);
    }

    Log_Write(LOG_LEVEL_INFO, "FRU svc response: %s\n", __func__);

    FILL_RSP_HEADER(dm_rsp, tag_id, DM_CMD_SET_FRU, timer_get_ticks_count() - req_start_time,
                    status)

    if (0 != SP_Host_Iface_CQ_Push_Cmd((char *)&dm_rsp, sizeof(struct device_mgmt_set_fru_rsp_t)))
    {
        Log_Write(LOG_LEVEL_ERROR, "%s: Cqueue push error!\n", __func__);
    }
}

/************************************************************************
*
*   FUNCTION
*
*       fru_process_request
*
*   DESCRIPTION
*
*       This function takes command ID as input from Host,
*       and accordingly calls the respective FRU functions
*
*   INPUTS
*
*       msg_id      Unique enum representing specific command
*       buffer      Pointer to command buffer
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void fru_process_request(tag_id_t tag_id, msg_id_t msg_id, const void *buffer)
{
    uint64_t req_start_time;

    req_start_time = timer_get_ticks_count();

    switch (msg_id)
    {
        case DM_CMD_GET_FRU:
            fru_svc_getfruinfo(tag_id, req_start_time);
            break;
        case DM_CMD_SET_FRU:
            fru_svc_setfruinfo(tag_id, req_start_time, buffer);
            break;
        default:
            Log_Write(LOG_LEVEL_ERROR, "FRU svc Unsupported Message ID: %d\n", msg_id);
    }
}
