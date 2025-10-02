#include <stdio.h>
#include "etsoc/drivers/serial/serial.h"
#include "tf.h"
#include "bl2_sp_pll.h"

int8_t PU_UART_Init_Cmd_Handler(void *test_cmd);
int8_t PU_SRAM_Read_Word_Cmd_Handler(void *test_cmd);
int8_t PU_SRAM_Write_Word_Cmd_Handler(void *test_cmd);
int8_t PU_PLL_Program_Cmd_Handler(void *test_cmd);

int8_t PU_UART_Init_Cmd_Handler(void *test_cmd)
{
    (void)test_cmd;
    tf_rsp_hdr_t rsp_hdr;

    rsp_hdr.id = TF_RSP_UNSUPPORTED;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size = 0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t PU_SRAM_Read_Word_Cmd_Handler(void *test_cmd)
{
    (void)test_cmd;
    tf_rsp_hdr_t rsp_hdr;

    rsp_hdr.id = TF_RSP_UNSUPPORTED;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size = 0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t PU_SRAM_Write_Word_Cmd_Handler(void *test_cmd)
{
    (void)test_cmd;
    tf_rsp_hdr_t rsp_hdr;

    rsp_hdr.id = TF_RSP_UNSUPPORTED;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size = 0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t PU_PLL_Program_Cmd_Handler(void *test_cmd)
{
    const struct tf_cmd_pu_pll_program_t *cmd = (struct tf_cmd_pu_pll_program_t *)test_cmd;
    struct tf_rsp_pu_pll_program_t rsp;

    rsp.rsp_hdr.id = TF_RSP_PU_PLL_PROGRAM;
    rsp.rsp_hdr.flags = TF_RSP_ONLY;
    rsp.rsp_hdr.payload_size = 0;
    printf("\n** PUPLL mode = %d **\r\n", cmd->cmd_payload);
    configure_sp_pll_1(cmd->cmd_payload);
    printf("\n** PUPLL done **\r\n");
    TF_Send_Response(&rsp, sizeof(rsp));

    return 0;
}
