#include "tests/tf/tf.h"

int8_t Minion_Step_Clk_PLL_Program_Cmd_Handler(void* test_cmd);
int8_t Minion_Voltage_Update_Cmd_Handler(void* test_cmd);
int8_t Minion_Shire_Enable_Cmd_Handler(void* test_cmd);
int8_t Minion_Shire_Boot_Cmd_Handler(void* test_cmd);
int8_t Minion_Kernel_Launch_Cmd_Handler(void* test_cmd);
int8_t Minion_ESR_Read_Cmd_Handler(void* test_cmd);
int8_t Minion_ESR_Write_Cmd_Handler(void* test_cmd);

int8_t Minion_Step_Clk_PLL_Program_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;
    
    rsp_hdr.id = TF_RSP_UNSUPPORTED_COMMAND;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t Minion_Voltage_Update_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;
    
    rsp_hdr.id = TF_RSP_UNSUPPORTED_COMMAND;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t Minion_Shire_Enable_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;
    
    rsp_hdr.id = TF_RSP_UNSUPPORTED_COMMAND;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t Minion_Shire_Boot_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;
    
    rsp_hdr.id = TF_RSP_UNSUPPORTED_COMMAND;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t Minion_Kernel_Launch_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;
    
    rsp_hdr.id = TF_RSP_UNSUPPORTED_COMMAND;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t Minion_ESR_Read_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;
    
    rsp_hdr.id = TF_RSP_UNSUPPORTED_COMMAND;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t Minion_ESR_Write_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;
    
    rsp_hdr.id = TF_RSP_UNSUPPORTED_COMMAND;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}