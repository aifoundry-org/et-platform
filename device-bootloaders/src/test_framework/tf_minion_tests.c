#include "tf.h"
#include <stdio.h>
#include "serial.h"
#include "esr.h"
#include "bl2_sp_pll.h"
#include "bl2_reset.h"
#include "minion_configuration.h"

int8_t Minion_Step_Clk_PLL_Program_Cmd_Handler(void* test_cmd);
int8_t Minion_Voltage_Update_Cmd_Handler(void* test_cmd);
int8_t Minion_Shire_Enable_Cmd_Handler(void* test_cmd);
int8_t Minion_Shire_Boot_Cmd_Handler(void* test_cmd);
int8_t Minion_Kernel_Launch_Cmd_Handler(void* test_cmd);
int8_t Minion_ESR_Read_Cmd_Handler(void* test_cmd);
int8_t Minion_ESR_Write_Cmd_Handler(void* test_cmd);
int8_t Minion_ESR_RMW_Cmd_Handler(void* test_cmd);
int8_t MM_Tests_Offset_Cmd_Handler(void* test_cmd);

int8_t Minion_Step_Clk_PLL_Program_Cmd_Handler(void* test_cmd)
{
    const struct tf_cmd_minion_step_clock_pll_program_t* cmd = (struct tf_cmd_minion_step_clock_pll_program_t*)test_cmd;
    struct tf_rsp_minion_step_clock_pll_program_t cmd_rsp_hdr;

    cmd_rsp_hdr.rsp_hdr.id = TF_RSP_MINION_STEP_CLOCK_PLL_PROGRAM;
    cmd_rsp_hdr.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    cmd_rsp_hdr.rsp_hdr.payload_size = TF_GET_PAYLOAD_SIZE(struct tf_rsp_minion_step_clock_pll_program_t);
    printf("\n** STEPPLL4 mode = %d **\r\n", cmd->cmd_payload);
    cmd_rsp_hdr.status = (uint32_t)configure_sp_pll_4(cmd->cmd_payload);
    printf("\n** STEPPLL4 done **\r\n");
    TF_Send_Response(&cmd_rsp_hdr, sizeof(struct tf_rsp_minion_step_clock_pll_program_t));

    return 0;
}

int8_t Minion_Voltage_Update_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;

    rsp_hdr.id = TF_RSP_UNSUPPORTED;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t Minion_Shire_Enable_Cmd_Handler(void* test_cmd)
{
    const struct tf_cmd_minion_shire_enable_t* cmd = (struct tf_cmd_minion_shire_enable_t*)test_cmd;
    struct tf_rsp_minion_shire_enable_t cmd_rsp_hdr;

    cmd_rsp_hdr.rsp_hdr.id = TF_RSP_MINION_SHIRE_ENABLE;
    cmd_rsp_hdr.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    cmd_rsp_hdr.rsp_hdr.payload_size = TF_GET_PAYLOAD_SIZE(struct tf_rsp_minion_shire_enable_t);

    printf("\n** Sh EN id %lx mode %x **\r\n", cmd->shire_mask, cmd->pll4_mode);
    cmd_rsp_hdr.status = (uint32_t)Minion_Configure_Minion_Clock_Reset(cmd->shire_mask, cmd->pll4_mode, cmd->pll4_mode, false);
    // enable the neighborhoods
    cmd_rsp_hdr.status |= (uint32_t)Minion_Enable_Shire_Cache_and_Neighborhoods(cmd->shire_mask);
    printf("\n** Sh EN done **\r\n");
    TF_Send_Response(&cmd_rsp_hdr, sizeof(struct tf_rsp_minion_shire_enable_t));

    return 0;
}

int8_t Minion_Shire_Boot_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;

    rsp_hdr.id = TF_RSP_UNSUPPORTED;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t Minion_Kernel_Launch_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;

    rsp_hdr.id = TF_RSP_UNSUPPORTED;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t Minion_ESR_Read_Cmd_Handler(void* test_cmd)
{
    const struct tf_cmd_minion_esr_read_t* cmd = (struct tf_cmd_minion_esr_read_t*)test_cmd;
    struct tf_rsp_minion_esr_read_t rsp;

    rsp.rsp_hdr.id = TF_RSP_MINION_ESR_READ;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size =  TF_GET_PAYLOAD_SIZE(struct tf_rsp_minion_esr_read_t);
    printf("\n** ESR RD adr %x **\r\n", cmd->addr);
    //NOSONAR rsp.value = Minion_ESR_read(cmd->addr);
    rsp.value = 0x0;
    printf("\n** ESR RD done **\r\n");
    TF_Send_Response(&rsp, sizeof(struct tf_rsp_minion_esr_read_t));

    return 0;
}

int8_t Minion_ESR_Write_Cmd_Handler(void* test_cmd)
{
    const struct tf_cmd_minion_esr_write_t* cmd = (struct tf_cmd_minion_esr_write_t*)test_cmd;
    struct tf_rsp_minion_esr_write_t rsp;

    rsp.rsp_hdr.id = TF_RSP_MINION_ESR_WRITE;
    rsp.rsp_hdr.flags = TF_RSP_ONLY;
    rsp.rsp_hdr.payload_size =  0;
    printf("\n** ESR WR adr %x val %lx mask %lx **\r\n", cmd->addr, cmd->value, cmd->mask);
    //NOSONAR Minion_ESR_write(cmd->addr, cmd->value, cmd->mask);
    printf("\n** ESR WR done **\r\n");
    TF_Send_Response(&rsp, sizeof(rsp));

    return 0;
}

int8_t Minion_ESR_RMW_Cmd_Handler(void* test_cmd)
{
    //NOSONAR uint64_t regVal;
    const struct tf_cmd_minion_esr_rmw_t* cmd = (struct tf_cmd_minion_esr_rmw_t*)test_cmd;
    struct tf_rsp_minion_esr_rmw_t rsp;

    rsp.rsp_hdr.id = TF_RSP_MINION_ESR_RMW;
    rsp.rsp_hdr.flags = TF_RSP_ONLY;
    rsp.rsp_hdr.payload_size =  0;
    printf("\n** ESR RMW adr %x val %lx mask %lx **\r\n", cmd->addr, cmd->value, cmd->mask);
    //NOSONAR regVal = Minion_ESR_read(cmd->addr);
    //NOSONAR regVal = (regVal & (~cmd->mask)) | cmd->value;
    //NOSONAR Minion_ESR_write(cmd->addr, regVal, cmd->mask);
    printf("\n** ESR RMW done **\r\n");
    TF_Send_Response(&rsp, sizeof(rsp));

    return 0;
}

int8_t MM_Tests_Offset_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;

    rsp_hdr.id = TF_RSP_UNSUPPORTED;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}
