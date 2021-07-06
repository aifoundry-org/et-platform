#include "tf.h"
#include "bl2_reset.h"
#include "mem_controller.h"

int8_t Mem_MemShire_Init_Cmd_Handler(void* test_cmd);
int8_t Mem_MemShire_PLL_Program_Cmd_Handler(void* test_cmd);
int8_t Mem_MemShire_Voltage_Update_Cmd_Handler(void* test_cmd);
int8_t Mem_DDR_Cntr_Init_Cmd_Handler(void* test_cmd);
int8_t Mem_Subsystem_Config_Cmd_Handler(void* test_cmd);
int8_t Mem_DDR_Cntr_Read_Word_Cmd_Handler(void* test_cmd);
int8_t Mem_DDR_Cntr_Write_Word_Cmd_Handler(void* test_cmd);

int8_t Mem_MemShire_Init_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;
    
    rsp_hdr.id = TF_RSP_UNSUPPORTED;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t Mem_MemShire_PLL_Program_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;
    
    rsp_hdr.id = TF_RSP_UNSUPPORTED;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t Mem_MemShire_Voltage_Update_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;
    
    rsp_hdr.id = TF_RSP_UNSUPPORTED;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t Mem_DDR_Cntr_Init_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;
    
    rsp_hdr.id = TF_RSP_UNSUPPORTED;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

// Master function to enable memshires, memshire pll config and ddr init
int8_t Mem_Subsystem_Config_Cmd_Handler(void* test_cmd)
{
    struct tf_mem_cmd_t {
	tf_cmd_hdr_t  cmd_hdr;
	DDR_MODE tf_ddr_mode;
    };
    struct tf_mem_cmd_t* cmd = (struct tf_mem_cmd_t*)test_cmd;
    struct tf_cmd_rsp_t cmd_rsp_hdr;
    
    cmd_rsp_hdr.rsp_hdr.id = TF_RSP_MEM_SUBSYSTEM_CONFIG;
    cmd_rsp_hdr.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    cmd_rsp_hdr.rsp_hdr.payload_size = TF_GET_PAYLOAD_SIZE(struct tf_cmd_rsp_t);
    cmd_rsp_hdr.status = 0x0;
    printf("\n** Mem config - freq = %x cap = %x ecc = %d training = %d sim_only = %d **\r\n", 
	   cmd->tf_ddr_mode.frequency, cmd->tf_ddr_mode.capacity, cmd->tf_ddr_mode.ecc, 
	   cmd->tf_ddr_mode.training, cmd->tf_ddr_mode.sim_only);
    if (0 != release_memshire_from_reset()) {
	goto mem_subsystem_config_error;
    }
    printf("\n** Mem rst done **\r\n");
    if (0 != configure_memshire_plls(&cmd->tf_ddr_mode)) {
    	goto mem_subsystem_config_error;
    }
    printf("\n** Mem PLL done **\r\n");
    if (0 != ddr_config(&cmd->tf_ddr_mode)) {
    	goto mem_subsystem_config_error;
    }
    printf("\n** ddr config done **\r\n");
    goto skip_mem_subsystem_config_error;
mem_subsystem_config_error:    
    cmd_rsp_hdr.status = 0x1;
    printf("\n** Mem config Error **\r\n");
skip_mem_subsystem_config_error:    
    TF_Send_Response(&cmd_rsp_hdr, sizeof(struct tf_cmd_rsp_t));
    return 0;
}


int8_t Mem_DDR_Cntr_Read_Word_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;
    
    rsp_hdr.id = TF_RSP_UNSUPPORTED;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t Mem_DDR_Cntr_Write_Word_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;
    
    rsp_hdr.id = TF_RSP_UNSUPPORTED;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}
