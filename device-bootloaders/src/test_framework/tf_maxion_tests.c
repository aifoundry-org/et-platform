#include "tf.h"
#include <stdio.h>
#include "etsoc/drivers/serial/serial.h"
#include "bl2_sp_pll.h"
#include "bl2_reset.h"
#include "maxion_configuration.h"

int8_t Maxion_Init_Cmd_Handler(void* test_cmd);
int8_t Maxion_Core_PLL_Program_Cmd_Handler(void* test_cmd);
int8_t Maxion_Uncore_PLL_Program_Cmd_Handler(void* test_cmd);
int8_t Maxion_Internal_Init_Cmd_Handler(void* test_cmd);


int8_t Maxion_Init_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    struct tf_rsp_maxion_init_t cmd_rsp_hdr;

    cmd_rsp_hdr.rsp_hdr.id = TF_RSP_MAXION_INIT;
    cmd_rsp_hdr.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    cmd_rsp_hdr.rsp_hdr.payload_size = TF_GET_PAYLOAD_SIZE(cmd_rsp_hdr);
    printf("\n** MAX cold rst **\r\n");
    Maxion_Reset_Cold_Release();
    printf("\n** MAX uncore warm rst **\r\n");
    Maxion_Reset_Warm_Uncore_Release();
    printf("\n** MAX sh ch en **\r\n");
    Maxion_Shire_Channel_Enable();
    printf("\n** MAX pll uncore rst  **\r\n");
    Maxion_Reset_PLL_Uncore_Release();
    printf("\n** MAX pll core rst  **\r\n");
    Maxion_Reset_PLL_Core_Release();
    cmd_rsp_hdr.status = 0x0;
    printf("\n** MAX Init done **\r\n");
    TF_Send_Response(&cmd_rsp_hdr, sizeof(cmd_rsp_hdr));

    return 0;
}

int8_t Maxion_Core_PLL_Program_Cmd_Handler(void* test_cmd)
{
    const struct tf_cmd_maxion_core_pll_program_t* cmd = (struct tf_cmd_maxion_core_pll_program_t*)test_cmd;
    struct tf_rsp_maxion_core_pll_program_t cmd_rsp_hdr;

    cmd_rsp_hdr.rsp_hdr.id = TF_RSP_MAXION_CORE_PLL_PROGRAM;
    cmd_rsp_hdr.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    cmd_rsp_hdr.rsp_hdr.payload_size = TF_GET_PAYLOAD_SIZE(struct tf_cmd_maxion_core_pll_program_t);
    printf("\n** MAX Core mode = %d **\r\n", cmd->mode);
    cmd_rsp_hdr.status = (uint32_t)configure_maxion_pll_core(cmd->mode);
    printf("\n** MAX Core done **\r\n");
    TF_Send_Response(&cmd_rsp_hdr, sizeof(struct tf_cmd_maxion_core_pll_program_t));

    return 0;
}

int8_t Maxion_Uncore_PLL_Program_Cmd_Handler(void* test_cmd)
{
    const struct tf_cmd_maxion_uncore_pll_program_t* cmd = (struct tf_cmd_maxion_uncore_pll_program_t*)test_cmd;
    struct tf_rsp_maxion_uncore_pll_program_t cmd_rsp_hdr;

    cmd_rsp_hdr.rsp_hdr.id = TF_RSP_MAXION_UNCORE_PLL_PROGRAM;
    cmd_rsp_hdr.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    cmd_rsp_hdr.rsp_hdr.payload_size = TF_GET_PAYLOAD_SIZE(struct tf_rsp_maxion_uncore_pll_program_t);
    printf("\n** MAX Uncore mode = %d **\r\n", cmd->mode);
    cmd_rsp_hdr.status = (uint32_t)configure_maxion_pll_uncore(cmd->mode);
    printf("\n** MAX Uncore done **\r\n");
    TF_Send_Response(&cmd_rsp_hdr, sizeof(struct tf_rsp_maxion_uncore_pll_program_t));

    return 0;
}


int8_t Maxion_Internal_Init_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    struct tf_rsp_maxion_internal_init_t cmd_rsp_hdr;

    cmd_rsp_hdr.rsp_hdr.id = TF_RSP_MAXION_INTERNAL_INIT;
    cmd_rsp_hdr.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    cmd_rsp_hdr.rsp_hdr.payload_size = TF_GET_PAYLOAD_SIZE(struct tf_rsp_maxion_internal_init_t);
    printf("\n** MAX internal init done **\r\n");
    TF_Send_Response(&cmd_rsp_hdr, sizeof(struct tf_rsp_maxion_internal_init_t));

    return 0;
}
