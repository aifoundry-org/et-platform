#include "tf.h"

int8_t SP_Fw_Version_Cmd_Handler(void* test_cmd);
int8_t Echo_To_SP_Cmd_Handler(void* test_cmd);
int8_t Move_Data_To_Device_Cmd_Handler(void* test_cmd);
int8_t Move_Data_To_Host_Cmd_Handler(void* test_cmd);


int8_t SP_Fw_Version_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    struct tf_get_spfw_ver_rsp_t rsp;

    rsp.rsp_hdr.id = TF_RSP_SP_FW_VERSION;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size =  TF_GET_PAYLOAD_SIZE(struct tf_get_spfw_ver_rsp_t);
    rsp.major = 0x01;
    rsp.minor = 0x02;
    rsp.version = 0x03;

    TF_Send_Response(&rsp, sizeof(struct tf_get_spfw_ver_rsp_t));

    return 0;
}

int8_t Echo_To_SP_Cmd_Handler(void* test_cmd)
{
    struct tf_echo_cmd_t* cmd = (struct tf_echo_cmd_t*)test_cmd;
    struct tf_echo_rsp_t rsp;

    rsp.rsp_hdr.id = TF_RSP_ECHO_TO_SP;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size =  TF_GET_PAYLOAD_SIZE(struct tf_echo_rsp_t);
    rsp.rsp_payload = cmd->cmd_payload;

    TF_Send_Response(&rsp, sizeof(struct tf_echo_rsp_t));

    return 0;
}

int8_t Move_Data_To_Device_Cmd_Handler(void* test_cmd)
{
    struct tf_move_data_to_device_cmd_t *cmd =
        (struct tf_move_data_to_device_cmd_t *)test_cmd;
    struct tf_move_data_to_device_rsp_t rsp;

    char* dst = (char*)cmd->dst_addr;
    const char* src = (char*)cmd->data;
    uint32_t size = cmd->size;
    uint32_t bytes_written = 0;

    /* TODO: This is a naive approach,
    can be made optimized */
    while(size) {
        *dst = *src;
        dst++;src++;
        size--;
        bytes_written++;
    }

    rsp.rsp_hdr.id = TF_RSP_MOVE_DATA_TO_DEVICE;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size =  sizeof(rsp.bytes_written);
    rsp.bytes_written = bytes_written;

    TF_Send_Response(&rsp, sizeof(rsp));

    return 0;
}

static struct tf_move_data_to_host_rsp_t   read_rsp;

int8_t Move_Data_To_Host_Cmd_Handler(void* test_cmd)
{
    const struct tf_move_data_to_host_cmd_t  *cmd =
        (const struct tf_move_data_to_host_cmd_t  *)test_cmd;

    const char* src = (char*)cmd->src_addr;
    uint32_t size = cmd->size;
    char* dst = (char*)read_rsp.data;
    uint32_t bytes_read = 0;

    while(size) {
        *dst = *src;
        dst++;src++;
        size--;
        bytes_read++;
    }

    read_rsp.rsp_hdr.id = TF_RSP_MOVE_DATA_To_HOST;
    read_rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    read_rsp.rsp_hdr.payload_size =
        (uint32_t)(sizeof(uint32_t) + bytes_read );
    read_rsp.bytes_read = bytes_read;

    TF_Send_Response(&read_rsp,
        (uint32_t)(sizeof(tf_rsp_hdr_t) +
        sizeof(uint32_t) +
        bytes_read));

    return 0;
}
