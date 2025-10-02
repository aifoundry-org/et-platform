#include "tf.h"
#include <string.h>

int8_t TF_Set_Entry_Point_Handler(void* test_cmd);
int8_t SP_Fw_Version_Cmd_Handler(void* test_cmd);
int8_t Echo_To_SP_Cmd_Handler(void* test_cmd);
int8_t Move_Data_To_Device_Cmd_Handler(void* test_cmd);
int8_t Move_Data_To_Host_Cmd_Handler(void* test_cmd);
/* TODO: Dump Memory */
/* TODO: Dump Registers */

int8_t TF_Set_Entry_Point_Handler(void* test_cmd)
{
    const struct tf_cmd_set_intercept_t* cmd = (struct tf_cmd_set_intercept_t*) test_cmd;
    struct tf_rsp_set_intercept_t rsp;
    uint8_t retval;
    uint8_t orig_val;
    int8_t rtn_arg;

    orig_val = TF_Get_Entry_Point();
    retval = TF_Set_Entry_Point(cmd->target_intercept);

    rsp.rsp_hdr.id = TF_RSP_SET_INTERCEPT;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size =  TF_GET_PAYLOAD_SIZE(struct tf_rsp_set_intercept_t);
    rsp.current_intercept = retval;

    TF_Send_Response(&rsp, sizeof(struct tf_rsp_set_intercept_t));

    if (retval != TF_DEFAULT_ENTRY && retval > orig_val)
    {
        rtn_arg = TF_EXIT_FROM_TF_LOOP;
    }
    else
    {
        rtn_arg = 0x0;
    }

    return rtn_arg;
}

int8_t SP_Fw_Version_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    struct tf_rsp_sp_fw_version_t rsp;

    rsp.rsp_hdr.id = TF_RSP_SP_FW_VERSION;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size =  TF_GET_PAYLOAD_SIZE(struct tf_rsp_sp_fw_version_t);
    rsp.major = 0x01;
    rsp.minor = 0x02;
    rsp.revision = 0x03;

    TF_Send_Response(&rsp, sizeof(struct tf_rsp_sp_fw_version_t));

    return 0;
}

int8_t Echo_To_SP_Cmd_Handler(void* test_cmd)
{
    const struct tf_cmd_echo_to_sp_t* cmd = (struct tf_cmd_echo_to_sp_t*)test_cmd;
    struct tf_rsp_echo_to_sp_t rsp;

    rsp.rsp_hdr.id = TF_RSP_ECHO_TO_SP;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size =  TF_GET_PAYLOAD_SIZE(struct tf_rsp_echo_to_sp_t);
    rsp.cmd_payload = cmd->cmd_payload;

    TF_Send_Response(&rsp, sizeof(struct tf_rsp_echo_to_sp_t));

    return 0;
}

int8_t Move_Data_To_Device_Cmd_Handler(void* test_cmd)
{
    struct tf_cmd_move_data_to_device_t *cmd =
        (struct tf_cmd_move_data_to_device_t *)test_cmd;
    struct tf_rsp_move_data_to_device_t rsp;

    /* Copy the data from command to given destination address.
       NOTE: Since data size is given within TF command, so it is safe
       here to copy given data size from variable length data source. */
    memcpy((void*)cmd->dst_addr, (void*)cmd->data, cmd->size);

    rsp.rsp_hdr.id = TF_RSP_MOVE_DATA_TO_DEVICE;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size =  sizeof(rsp.bytes_written);
    rsp.bytes_written = cmd->size;

    TF_Send_Response(&rsp, sizeof(rsp));

    return 0;
}

int8_t Move_Data_To_Host_Cmd_Handler(void* test_cmd)
{
    const struct tf_cmd_move_data_to_host_t  *cmd =
        (const struct tf_cmd_move_data_to_host_t  *)test_cmd;
    struct tf_rsp_move_data_to_host_t rsp;

    rsp.rsp_hdr.id = TF_RSP_MOVE_DATA_TO_HOST;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size =
        (uint32_t)(sizeof(uint32_t) + cmd->size );
    rsp.bytes_read = cmd->size;

    TF_Send_Response_With_Payload(&rsp,
        (uint32_t) (sizeof(tf_rsp_hdr_t) + sizeof(uint32_t)),
        (void*) cmd->src_addr, rsp.bytes_read);

    return 0;
}
