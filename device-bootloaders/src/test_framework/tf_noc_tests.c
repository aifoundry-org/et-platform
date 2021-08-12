#include <stdio.h>
#include "serial.h"
#include "tf.h"
#include "bl2_sp_pll.h"

int8_t NOC_Voltage_update_Cmd_Handler(void* test_cmd);
int8_t NOC_PLL_Program_Cmd_Handler(void* test_cmd);
int8_t NOC_Routing_Table_Update_Cmd_Handler(void* test_cmd);

int8_t NOC_Voltage_update_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;

    rsp_hdr.id = TF_RSP_UNSUPPORTED;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t NOC_PLL_Program_Cmd_Handler(void* test_cmd)
{
    const struct tf_cmd_noc_pll_program_t* cmd = (struct tf_cmd_noc_pll_program_t*)test_cmd;
    struct tf_rsp_noc_pll_program_t cmd_rsp_hdr;

    cmd_rsp_hdr.rsp_hdr.id = TF_RSP_NOC_PLL_PROGRAM;
    cmd_rsp_hdr.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    cmd_rsp_hdr.rsp_hdr.payload_size = TF_GET_PAYLOAD_SIZE(struct tf_rsp_noc_pll_program_t);

    printf("\n** NOCPLL mode = %d **\r\n", cmd->cmd_payload);
    cmd_rsp_hdr.status = (uint32_t)configure_sp_pll_4(cmd->cmd_payload);
    printf("\n** NOCPLL done **\r\n");
    TF_Send_Response(&cmd_rsp_hdr, sizeof(struct tf_rsp_noc_pll_program_t));

    return 0;
}

int8_t NOC_Routing_Table_Update_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;

    rsp_hdr.id = TF_RSP_UNSUPPORTED;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}
