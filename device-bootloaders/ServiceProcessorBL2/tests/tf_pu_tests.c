#include "tests/tf/tf.h"

int8_t PU_UART_Init_Cmd_Handler(void* test_cmd);
int8_t PU_SRAM_Read_Word_Cmd_Handler(void* test_cmd);
int8_t PU_SRAM_Write_Word_Cmd_Handler(void* test_cmd);


int8_t PU_UART_Init_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;
    
    rsp_hdr.id = TF_RSP_UNSUPPORTED_COMMAND;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t PU_SRAM_Read_Word_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;
    
    rsp_hdr.id = TF_RSP_UNSUPPORTED_COMMAND;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t PU_SRAM_Write_Word_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;
    
    rsp_hdr.id = TF_RSP_UNSUPPORTED_COMMAND;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}