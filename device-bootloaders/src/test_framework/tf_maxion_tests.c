#include "tf.h"
#include <stdio.h>
#include "serial.h"
#include "bl2_sp_pll.h"
#include "maxion_configuration.h"

int8_t Maxion_Init_Cmd_Handler(void* test_cmd);
int8_t Maxion_Core_PLL_Program_Cmd_Handler(void* test_cmd);
int8_t Maxion_Uncore_PLL_Program_Cmd_Handler(void* test_cmd);
int8_t Maxion_Internal_Init_Cmd_Handler(void* test_cmd);


int8_t Maxion_Init_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    struct tf_cmd_rsp_t cmd_rsp_hdr;
    
    cmd_rsp_hdr.rsp_hdr.id = TF_RSP_MAXION_INIT;
    cmd_rsp_hdr.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    cmd_rsp_hdr.rsp_hdr.payload_size = TF_GET_PAYLOAD_SIZE(struct tf_cmd_rsp_t);
    printf("\n** MAX cold rst **\r\n");
    Maxion_Reset_Cold_Deassert();
    printf("\n** MAX uncore warm rst **\r\n");
    Maxion_Reset_Warm_Uncore_Deassert();
    printf("\n** MAX sh ch en **\r\n");
    Maxion_Shire_Channel_Enable();
    printf("\n** MAX pll uncore rst  **\r\n");
    Maxion_Reset_PLL_Uncore_Deassert();
    printf("\n** MAX pll core rst  **\r\n");
    Maxion_Reset_PLL_Core_Deassert();
    cmd_rsp_hdr.status = 0x0;
    printf("\n** MAX Init done **\r\n");
    TF_Send_Response(&cmd_rsp_hdr, sizeof(struct tf_cmd_rsp_t));

    return 0;
}

int8_t Maxion_Core_PLL_Program_Cmd_Handler(void* test_cmd)
{
    struct tf_pll_cmd_t* cmd = (struct tf_pll_cmd_t*)test_cmd;
    struct tf_cmd_rsp_t cmd_rsp_hdr;
    
    cmd_rsp_hdr.rsp_hdr.id = TF_RSP_MAXION_CORE_PLL_PROGRAM;
    cmd_rsp_hdr.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    cmd_rsp_hdr.rsp_hdr.payload_size = TF_GET_PAYLOAD_SIZE(struct tf_cmd_rsp_t);
    printf("\n** MAX Core mode = %d **\r\n", cmd->cmd_payload);
    cmd_rsp_hdr.status = (uint32_t)configure_maxion_pll_core(cmd->cmd_payload);
    printf("\n** MAX Core done **\r\n");
    TF_Send_Response(&cmd_rsp_hdr, sizeof(struct tf_cmd_rsp_t));

    return 0;
}

int8_t Maxion_Uncore_PLL_Program_Cmd_Handler(void* test_cmd)
{
    struct tf_pll_cmd_t* cmd = (struct tf_pll_cmd_t*)test_cmd;
    struct tf_cmd_rsp_t cmd_rsp_hdr;
    
    cmd_rsp_hdr.rsp_hdr.id = TF_RSP_MAXION_UNCORE_PLL_PROGRAM;
    cmd_rsp_hdr.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    cmd_rsp_hdr.rsp_hdr.payload_size = TF_GET_PAYLOAD_SIZE(struct tf_cmd_rsp_t);
    printf("\n** MAX Uncore mode = %d **\r\n", cmd->cmd_payload);
    cmd_rsp_hdr.status = (uint32_t)configure_maxion_pll_uncore(cmd->cmd_payload);
    printf("\n** MAX Uncore done **\r\n");
    TF_Send_Response(&cmd_rsp_hdr, sizeof(struct tf_cmd_rsp_t));

    return 0;
}


int8_t Maxion_Internal_Init_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    struct tf_cmd_rsp_t cmd_rsp_hdr;
    
    cmd_rsp_hdr.rsp_hdr.id = TF_RSP_MAXION_INTERNAL_INIT;
    cmd_rsp_hdr.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    cmd_rsp_hdr.rsp_hdr.payload_size = TF_GET_PAYLOAD_SIZE(struct tf_cmd_rsp_t);
    printf("\n** MAX internal init done **\r\n");
    TF_Send_Response(&cmd_rsp_hdr, sizeof(struct tf_cmd_rsp_t));

    return 0;
}
