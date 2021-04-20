#include "tests/tf/tf.h"

int8_t SP_Fw_Version_Cmd_Handler(void* test_cmd);
int8_t MM_Fw_Version_Cmd_Handler(void* test_cmd);
int8_t Echo_To_SP_Cmd_Handler(void* test_cmd);
int8_t Echo_To_MM_Cmd_Handler(void* test_cmd);
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

int8_t MM_Fw_Version_Cmd_Handler(void* test_cmd)
{
    (void)test_cmd;
    struct tf_get_mmfw_ver_rsp_t rsp;

    rsp.rsp_hdr.id = TF_RSP_MM_FW_VERSION;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size =  TF_GET_PAYLOAD_SIZE(struct tf_get_mmfw_ver_rsp_t);
    rsp.major = 0x01;
    rsp.minor = 0x02;
    rsp.version = 0x03;

    TF_Send_Response(&rsp, sizeof(struct tf_get_mmfw_ver_rsp_t));

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

int8_t Echo_To_MM_Cmd_Handler(void* test_cmd)
{
    struct tf_echo_cmd_t* cmd = (struct tf_echo_cmd_t*)test_cmd;
    struct tf_echo_rsp_t rsp;

    rsp.rsp_hdr.id = TF_RSP_ECHO_TO_MM;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size =  TF_GET_PAYLOAD_SIZE(struct tf_echo_rsp_t);
    rsp.rsp_payload = cmd->cmd_payload;

    TF_Send_Response(&rsp, sizeof(struct tf_echo_rsp_t));

    return 0;
}

int8_t Move_Data_To_Device_Cmd_Handler(void* test_cmd)
{
    (void)test_cmd;
    tf_rsp_hdr_t rsp_hdr;
    
    rsp_hdr.id = TF_RSP_UNSUPPORTED_COMMAND;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t Move_Data_To_Host_Cmd_Handler(void* test_cmd)
{
    (void)test_cmd;
    tf_rsp_hdr_t rsp_hdr;
    
    rsp_hdr.id = TF_RSP_UNSUPPORTED_COMMAND;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}