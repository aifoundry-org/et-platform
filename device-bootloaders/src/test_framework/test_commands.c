#include <stdio.h>
#include "tf.h"

/* Extern prototypes for TF command handlers */
extern int8_t SP_Fw_Version_Cmd_Handler(void* test_cmd);
extern int8_t MM_Fw_Version_Cmd_Handler(void* test_cmd);
extern int8_t Echo_To_SP_Cmd_Handler(void* test_cmd);
extern int8_t Echo_To_MM_Cmd_Handler(void* test_cmd);
extern int8_t Move_Data_To_Device_Cmd_Handler(void* test_cmd);
extern int8_t Move_Data_To_Host_Cmd_Handler(void* test_cmd);
/* SPIO tests */
extern int8_t SPIO_RAM_Read_Word_Cmd_Handler(void* test_cmd);
extern int8_t SPIO_RAM_Write_Word_Cmd_Handler(void* test_cmd);
extern int8_t SPIO_Flash_Init_Cmd_Handler(void* test_cmd);
extern int8_t SPIO_Flash_Read_Word_Cmd_Handler(void* test_cmd);
extern int8_t SPIO_Flash_Write_Word_Cmd_Handler(void* test_cmd);
extern int8_t SPIO_OTP_Init_Cmd_Handler(void* test_cmd);
extern int8_t SPIO_OTP_Read_Word_Cmd_Handler(void* test_cmd);
extern int8_t SPIO_OTP_Write_Word_Cmd_Handler(void* test_cmd);
extern int8_t SPIO_Vault_Init_Cmd_Handler(void* test_cmd);
extern int8_t SPIO_Vault_Comand_Issue_Cmd_Handler(void* test_cmd);
extern int8_t SPIO_I2C_Init_Cmd_Handler(void* test_cmd);
extern int8_t SPIO_I2C_PMIC_Read_Word_Cmd_Handler(void* test_cmd);
extern int8_t SPIO_I2C_PMIC_Write_Word_Cmd_Handler(void* test_cmd);
/* PU Tests */
extern int8_t PU_UART_Init_Cmd_Handler(void* test_cmd);
extern int8_t PU_SRAM_Read_Word_Cmd_Handler(void* test_cmd);
extern int8_t PU_SRAM_Write_Word_Cmd_Handler(void* test_cmd);
/* PCIE Tests */
extern int8_t PCIE_PSHIRE_Init_Cmd_Handler(void* test_cmd);
extern int8_t PCIE_PSHIRE_Voltage_Update_Cmd_Handler(void* test_cmd);
extern int8_t PCIE_PSHIRE_PLL_Program_Cmd_Handler(void* test_cmd);
extern int8_t PCIE_PSHIRE_NOC_Update_Routing_Table_Cmd_Handler(void* test_cmd);
extern int8_t PCIE_PSHIRE_Cntr_Init_Cmd_Handler(void* test_cmd);
extern int8_t PCIE_PSHIRE_Phy_Init_Cmd_Handler(void* test_cmd);
/* NOC Tests */
extern int8_t NOC_Voltage_update_Cmd_Handler(void* test_cmd);
extern int8_t NOC_PLL_Program_Cmd_Handler(void* test_cmd);
extern int8_t NOC_Routing_Table_Update_Cmd_Handler(void* test_cmd);
/* Mem Tests */
extern int8_t Mem_MemShire_Init_Cmd_Handler(void* test_cmd);
extern int8_t Mem_MemShire_PLL_Program_Cmd_Handler(void* test_cmd);
extern int8_t Mem_MemShire_Voltage_Update_Cmd_Handler(void* test_cmd);
extern int8_t Mem_DDR_Cntr_Init_Cmd_Handler(void* test_cmd);
extern int8_t Mem_DDR_Cntr_Read_Word_Cmd_Handler(void* test_cmd);
extern int8_t Mem_DDR_Cntr_Write_Word_Cmd_Handler(void* test_cmd);
/* Minion Tests */
extern int8_t Minion_Step_Clk_PLL_Program_Cmd_Handler(void* test_cmd);
extern int8_t Minion_Voltage_Update_Cmd_Handler(void* test_cmd);
extern int8_t Minion_Shire_Enable_Cmd_Handler(void* test_cmd);
extern int8_t Minion_Shire_Boot_Cmd_Handler(void* test_cmd);
extern int8_t Minion_Kernel_Launch_Cmd_Handler(void* test_cmd);
extern int8_t Minion_ESR_Read_Cmd_Handler(void* test_cmd);
extern int8_t Minion_ESR_Write_Cmd_Handler(void* test_cmd);
/* Unregistered Handler */
int8_t Unregistered_Handler(void* test_cmd);
int8_t Unregistered_Handler(void* test_cmd)
{
    (void)test_cmd;
    tf_rsp_hdr_t rsp_hdr;
    
    rsp_hdr.id = TF_RSP_UNSUPPORTED_COMMAND;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

/* Modify the fn pointer table below to add new test,
 for each new test added, define the test handler at the
 array index based on tf_cmd_t enum */
int8_t (*TF_Test_Cmd_Handler[TF_NUM_COMMANDS])(void *test_cmd) = 
{
    Unregistered_Handler,   
    SP_Fw_Version_Cmd_Handler,
    MM_Fw_Version_Cmd_Handler,
    Echo_To_SP_Cmd_Handler,
    Echo_To_MM_Cmd_Handler,
    Move_Data_To_Device_Cmd_Handler,
    Move_Data_To_Host_Cmd_Handler,
    SPIO_RAM_Read_Word_Cmd_Handler,
    SPIO_RAM_Write_Word_Cmd_Handler,
    SPIO_Flash_Init_Cmd_Handler,
    SPIO_Flash_Read_Word_Cmd_Handler,
    SPIO_Flash_Write_Word_Cmd_Handler,
    SPIO_OTP_Init_Cmd_Handler,
    SPIO_OTP_Read_Word_Cmd_Handler,
    SPIO_OTP_Write_Word_Cmd_Handler,
    SPIO_Vault_Init_Cmd_Handler,
    SPIO_Vault_Comand_Issue_Cmd_Handler,
    SPIO_I2C_Init_Cmd_Handler,
    SPIO_I2C_PMIC_Read_Word_Cmd_Handler,
    SPIO_I2C_PMIC_Write_Word_Cmd_Handler,
    PU_UART_Init_Cmd_Handler,
    PU_SRAM_Read_Word_Cmd_Handler,
    PU_SRAM_Write_Word_Cmd_Handler,
    PCIE_PSHIRE_Init_Cmd_Handler,
    PCIE_PSHIRE_Voltage_Update_Cmd_Handler,
    PCIE_PSHIRE_PLL_Program_Cmd_Handler,
    PCIE_PSHIRE_NOC_Update_Routing_Table_Cmd_Handler,
    PCIE_PSHIRE_Cntr_Init_Cmd_Handler,
    PCIE_PSHIRE_Phy_Init_Cmd_Handler,
    NOC_Voltage_update_Cmd_Handler,
    NOC_PLL_Program_Cmd_Handler,
    NOC_Routing_Table_Update_Cmd_Handler,
    Mem_MemShire_Init_Cmd_Handler,
    Mem_MemShire_PLL_Program_Cmd_Handler,
    Mem_MemShire_Voltage_Update_Cmd_Handler,
    Mem_DDR_Cntr_Init_Cmd_Handler,
    Mem_DDR_Cntr_Read_Word_Cmd_Handler,
    Mem_DDR_Cntr_Write_Word_Cmd_Handler,
    Minion_Step_Clk_PLL_Program_Cmd_Handler,
    Minion_Voltage_Update_Cmd_Handler,
    Minion_Shire_Enable_Cmd_Handler,
    Minion_Shire_Boot_Cmd_Handler,
    Minion_Kernel_Launch_Cmd_Handler,
    Minion_ESR_Read_Cmd_Handler,
    Minion_ESR_Write_Cmd_Handler
};
