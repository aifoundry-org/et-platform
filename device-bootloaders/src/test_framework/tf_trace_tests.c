/*-------------------------------------------------------------------------
* Copyright (C) 2021, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/


#include "tf.h"
#include "trace.h"

/* mm_rt_helpers */
#include "system/layout.h"

int8_t Sp_Trace_Run_Control_Cmd_Handler(const void* test_cmd);
int8_t Sp_Trace_Run_Config_Cmd_Handler(const void* test_cmd);
int8_t Sp_Trace_Get_Info_Cmd_Handler(const void* test_cmd);
int8_t Sp_Trace_Get_Buffer_Cmd_Handler(const void* test_cmd);
int8_t MM_Trace_Get_Info_Cmd_Handler(const void* test_cmd);
int8_t MM_Trace_Get_Buffer_Cmd_Handler(const void* test_cmd);
int8_t CM_Trace_Get_Info_Cmd_Handler(const void* test_cmd);
int8_t CM_Trace_Get_Buffer_Cmd_Handler(const void* test_cmd);


int8_t Sp_Trace_Run_Control_Cmd_Handler(const void* test_cmd)
{
    struct device_mgmt_trace_run_control_cmd_t dm_cmd;
    const struct tf_cmd_sp_trace_run_control_t* run_control =
        (const struct tf_cmd_sp_trace_run_control_t*) test_cmd;

    dm_cmd.control = run_control->control;
    Trace_Process_Control_Cmd(&dm_cmd);

    struct tf_rsp_sp_trace_run_control_t rsp = {0};

    rsp.rsp_hdr.id = TF_RSP_SP_TRACE_RUN_CONTROL;
    rsp.rsp_hdr.flags = TF_RSP_ONLY;
    rsp.rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp, sizeof(rsp));

    return 0;
}

int8_t Sp_Trace_Run_Config_Cmd_Handler(const void* test_cmd)
{
    struct device_mgmt_trace_config_cmd_t dm_cmd;
    const struct tf_cmd_sp_trace_run_config_t* config =
        (const struct tf_cmd_sp_trace_run_config_t*) test_cmd;

    dm_cmd.event_mask = config->event_mask;
    dm_cmd.filter_mask = config->filter_mask;

    Trace_Process_Config_Cmd(&dm_cmd);

    struct tf_rsp_sp_trace_run_config_t rsp = {0};

    rsp.rsp_hdr.id = TF_RSP_SP_TRACE_RUN_CONFIG;
    rsp.rsp_hdr.flags = TF_RSP_ONLY;
    rsp.rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp, sizeof(rsp));

    return 0;
}

int8_t Sp_Trace_Get_Info_Cmd_Handler(const void* test_cmd)
{
    (void) test_cmd;

    struct tf_rsp_sp_trace_get_info_t rsp = {0};
    const struct trace_control_block_t* cb = Trace_Get_SP_CB();

    rsp.rsp_hdr.id = TF_RSP_SP_TRACE_GET_INFO;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size =  TF_GET_PAYLOAD_SIZE(rsp);
    rsp.offset = cb->offset_per_hart;
    rsp.size = cb->size_per_hart;
    rsp.base = cb->base_per_hart;

    TF_Send_Response(&rsp, sizeof(rsp));

    return 0;
}

int8_t Sp_Trace_Get_Buffer_Cmd_Handler(const void* test_cmd)
{
    (void) test_cmd;
    struct tf_rsp_sp_trace_get_buffer_t rsp;
    const struct trace_control_block_t* cb = Trace_Get_SP_CB();

    rsp.bytes_read = cb->offset_per_hart;
    rsp.rsp_hdr.id = TF_RSP_SP_TRACE_GET_BUFFER;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size =
        (uint32_t)(sizeof(uint32_t) + rsp.bytes_read);

    TF_Send_Response_With_Payload(&rsp,
        (uint32_t) (sizeof(tf_rsp_hdr_t) + sizeof(uint32_t)),
        (void*) cb->base_per_hart, rsp.bytes_read);

    return 0;
}

int8_t MM_Trace_Get_Info_Cmd_Handler(const void* test_cmd)
{
    (void) test_cmd;

    struct tf_rsp_mm_trace_get_info_t rsp = {0};
    struct trace_buffer_std_header_t trace_header;

    memcpy(&trace_header, (void*)MM_TRACE_BUFFER_BASE, sizeof(struct trace_buffer_std_header_t));

    rsp.rsp_hdr.id = TF_RSP_MM_TRACE_GET_INFO;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size =  TF_GET_PAYLOAD_SIZE(rsp);
    rsp.offset = trace_header.data_size;
    rsp.size = MM_TRACE_BUFFER_SIZE;
    rsp.base = MM_TRACE_BUFFER_BASE;

    TF_Send_Response(&rsp, sizeof(rsp));

    return 0;
}

int8_t MM_Trace_Get_Buffer_Cmd_Handler(const void* test_cmd)
{
    (void) test_cmd;
    struct tf_rsp_mm_trace_get_buffer_t rsp;
    struct trace_buffer_std_header_t trace_header;

    memcpy(&trace_header, (void*)MM_TRACE_BUFFER_BASE, sizeof(struct trace_buffer_std_header_t));

    rsp.bytes_read = trace_header.data_size;
    rsp.rsp_hdr.id = TF_RSP_MM_TRACE_GET_BUFFER;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size =
        (uint32_t)(sizeof(uint32_t) + rsp.bytes_read);

    TF_Send_Response_With_Payload(&rsp,
        (uint32_t) (sizeof(tf_rsp_hdr_t) + sizeof(uint32_t)),
        (void*) MM_TRACE_BUFFER_BASE, rsp.bytes_read);

    return 0;
}

int8_t CM_Trace_Get_Info_Cmd_Handler(const void* test_cmd)
{
    const struct tf_cmd_cm_trace_get_info_t *cmd =
        (const struct tf_cmd_cm_trace_get_info_t *)test_cmd;
    struct tf_rsp_cm_trace_get_info_t rsp = {0};

    if (cmd->buffer_id == 0)
    {
        struct trace_buffer_std_header_t trace_header;
        memcpy(&trace_header,
            (void*)CM_SMODE_TRACE_BUFFER_BASE,
            sizeof(struct trace_buffer_std_header_t));
        rsp.offset = trace_header.data_size;
        rsp.base = CM_SMODE_TRACE_BUFFER_BASE;
    }
    else
    {
        struct trace_buffer_size_header_t trace_header;
        memcpy(&trace_header,
            (void*)(CM_SMODE_TRACE_BUFFER_BASE + (cmd->buffer_id * CM_SMODE_TRACE_BUFFER_SIZE_PER_HART)),
            sizeof(struct trace_buffer_size_header_t));
        rsp.offset = trace_header.data_size;
        rsp.base = CM_SMODE_TRACE_BUFFER_BASE + (cmd->buffer_id * CM_SMODE_TRACE_BUFFER_SIZE_PER_HART);
    }

    rsp.rsp_hdr.id = TF_RSP_CM_TRACE_GET_INFO;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size =  TF_GET_PAYLOAD_SIZE(rsp);
    rsp.size = CM_SMODE_TRACE_BUFFER_SIZE_PER_HART;

    TF_Send_Response(&rsp, sizeof(rsp));

    return 0;
}

int8_t CM_Trace_Get_Buffer_Cmd_Handler(const void* test_cmd)
{
    (void) test_cmd;
    struct tf_rsp_cm_trace_get_buffer_t rsp;
    struct trace_buffer_std_header_t trace_header;

    memcpy(&trace_header, (void*)CM_SMODE_TRACE_BUFFER_BASE, sizeof(struct trace_buffer_std_header_t));

    rsp.bytes_read = trace_header.data_size;
    rsp.rsp_hdr.id = TF_RSP_CM_TRACE_GET_BUFFER;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size =
        (uint32_t)(sizeof(uint32_t) + rsp.bytes_read);

    TF_Send_Response_With_Payload(&rsp,
        (uint32_t) (sizeof(tf_rsp_hdr_t) + sizeof(uint32_t)),
        (void*) CM_SMODE_TRACE_BUFFER_BASE, rsp.bytes_read);

    return 0;
}
