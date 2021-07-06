#include <stdio.h>
#include "tf.h"

/* Extern prototypes for TF command handlers */
extern int8_t SP_Fw_Version_Cmd_Handler(void* test_cmd);
extern int8_t MM_Fw_Version_Cmd_Handler(void* test_cmd);
extern int8_t Echo_To_SP_Cmd_Handler(void* test_cmd);
extern int8_t Echo_To_MM_Cmd_Handler(void* test_cmd);
extern int8_t Move_Data_To_Device_Cmd_Handler(void* test_cmd);
extern int8_t Move_Data_To_Host_Cmd_Handler(void* test_cmd);

#if !TF_CORE
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
extern int8_t SPIO_PLL_Program_Cmd_Handler(void* test_cmd);
extern int8_t SPIO_IO_Read_Cmd_Handler(void* test_cmd);
extern int8_t SPIO_IO_Write_Cmd_Handler(void* test_cmd);
extern int8_t SPIO_IO_RMW_Cmd_Handler(void* test_cmd);
/* PU Tests */
extern int8_t PU_UART_Init_Cmd_Handler(void* test_cmd);
extern int8_t PU_SRAM_Read_Word_Cmd_Handler(void* test_cmd);
extern int8_t PU_SRAM_Write_Word_Cmd_Handler(void* test_cmd);
extern int8_t PU_PLL_Program_Cmd_Handler(void* test_cmd);
/* PCIE Tests */
extern int8_t PCIE_PSHIRE_Init_Cmd_Handler(void* test_cmd);
extern int8_t PCIE_PSHIRE_Voltage_Update_Cmd_Handler(void* test_cmd);
extern int8_t PCIE_PSHIRE_PLL_Program_Cmd_Handler(void* test_cmd);
extern int8_t PCIE_PSHIRE_NOC_Update_Routing_Table_Cmd_Handler(void* test_cmd);
/* NOC Tests */
extern int8_t NOC_Voltage_update_Cmd_Handler(void* test_cmd);
extern int8_t NOC_PLL_Program_Cmd_Handler(void* test_cmd);
extern int8_t NOC_Routing_Table_Update_Cmd_Handler(void* test_cmd);
/* Mem Tests */
extern int8_t Mem_MemShire_Init_Cmd_Handler(void* test_cmd);
extern int8_t Mem_MemShire_PLL_Program_Cmd_Handler(void* test_cmd);
extern int8_t Mem_MemShire_Voltage_Update_Cmd_Handler(void* test_cmd);
extern int8_t Mem_Subsystem_Config_Cmd_Handler(void* test_cmd);
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
extern int8_t Minion_ESR_RMW_Cmd_Handler(void* test_cmd);
extern int8_t MM_Tests_Offset_Cmd_Handler(void* test_cmd);
/* Maxion Tests */
extern int8_t Maxion_Init_Cmd_Handler(void* test_cmd);
extern int8_t Maxion_Core_PLL_Program_Cmd_Handler(void* test_cmd);
extern int8_t Maxion_Uncore_PLL_Program_Cmd_Handler(void* test_cmd);
extern int8_t Maxion_Internal_Init_Cmd_Handler(void* test_cmd);
/* Asset Tracking Tests */
extern int8_t Asset_Tracking_Manufacturer_Name_Cmd_Handler(void* test_cmd);
extern int8_t Asset_Tracking_Part_Number_Cmd_Handler(void* test_cmd);
extern int8_t Asset_Tracking_Serial_Number_Cmd_Handler(void* test_cmd);
extern int8_t Asset_Tracking_Chip_Revision_Cmd_Handler(void* test_cmd);
extern int8_t Asset_Tracking_PCIe_Max_Speed_Cmd_Handler(void* test_cmd);
extern int8_t Asset_Tracking_Module_Revision_Cmd_Handler(void* test_cmd);
extern int8_t Asset_Tracking_Form_Factor_Cmd_Handler(void* test_cmd);
extern int8_t Asset_Tracking_Memory_Details_Cmd_Handler(void* test_cmd);
extern int8_t Asset_Tracking_Memory_Size_Cmd_Handler(void* test_cmd);
extern int8_t Asset_Tracking_Memory_Type_Cmd_Handler(void* test_cmd);
#endif  // !TF_CORE

/* Unregistered Handler */
int8_t Unregistered_Handler(void* test_cmd);
int8_t Unregistered_Handler(void* test_cmd)
{
    (void)test_cmd;
    tf_rsp_hdr_t rsp_hdr;
    
    rsp_hdr.id = TF_RSP_UNSUPPORTED;
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
// 0
    Unregistered_Handler, 
// 1  
    SP_Fw_Version_Cmd_Handler,
// 2
    MM_Fw_Version_Cmd_Handler,
// 3
    Echo_To_SP_Cmd_Handler,
// 4
    Echo_To_MM_Cmd_Handler,
// 5
    Move_Data_To_Device_Cmd_Handler,
// 6
    Move_Data_To_Host_Cmd_Handler,
// 7
#if !TF_CORE
    SPIO_RAM_Read_Word_Cmd_Handler,
// 8
    SPIO_RAM_Write_Word_Cmd_Handler,
// 9
    SPIO_Flash_Init_Cmd_Handler,
// 10
    SPIO_Flash_Read_Word_Cmd_Handler,
// 11
    SPIO_Flash_Write_Word_Cmd_Handler,
// 12
    SPIO_OTP_Init_Cmd_Handler,
// 13
    SPIO_OTP_Read_Word_Cmd_Handler,
// 14
    SPIO_OTP_Write_Word_Cmd_Handler,
// 15
    SPIO_Vault_Init_Cmd_Handler,
// 16
    SPIO_Vault_Comand_Issue_Cmd_Handler,
// 17
    SPIO_I2C_Init_Cmd_Handler,
// 18
    SPIO_I2C_PMIC_Read_Word_Cmd_Handler,
// 19
    SPIO_I2C_PMIC_Write_Word_Cmd_Handler,
// 20
    PU_UART_Init_Cmd_Handler,
// 21
    PU_SRAM_Read_Word_Cmd_Handler,
// 22
    PU_SRAM_Write_Word_Cmd_Handler,
// 23
    PCIE_PSHIRE_Init_Cmd_Handler,
// 24
    PCIE_PSHIRE_Voltage_Update_Cmd_Handler,
// 25
    PCIE_PSHIRE_PLL_Program_Cmd_Handler,
// 26
    PCIE_PSHIRE_NOC_Update_Routing_Table_Cmd_Handler,
// 27
    Unregistered_Handler,
// 28
    Unregistered_Handler,
// 29
    Unregistered_Handler,
// 30
    Unregistered_Handler,
// 31
    Unregistered_Handler,
// 32
    Unregistered_Handler,
// 33
    NOC_Voltage_update_Cmd_Handler,
// 34
    NOC_PLL_Program_Cmd_Handler,
// 35
    NOC_Routing_Table_Update_Cmd_Handler,
// 36
    Mem_MemShire_Init_Cmd_Handler,
// 37
    Mem_MemShire_PLL_Program_Cmd_Handler,
// 38
    Mem_MemShire_Voltage_Update_Cmd_Handler,
// 39
    Unregistered_Handler,
// 40
    Mem_Subsystem_Config_Cmd_Handler,
// 41
    Mem_DDR_Cntr_Read_Word_Cmd_Handler,
// 42
    Mem_DDR_Cntr_Write_Word_Cmd_Handler,
// 43
    Minion_Step_Clk_PLL_Program_Cmd_Handler,
// 44
    Minion_Voltage_Update_Cmd_Handler,
// 45
    Minion_Shire_Enable_Cmd_Handler,
// 46
    Minion_Shire_Boot_Cmd_Handler,
// 47
    Minion_Kernel_Launch_Cmd_Handler,
// 48
    Minion_ESR_Read_Cmd_Handler,
// 49
    Minion_ESR_Write_Cmd_Handler,
// 50
    Minion_ESR_RMW_Cmd_Handler,
// 51
    SPIO_PLL_Program_Cmd_Handler,
// 52
    PU_PLL_Program_Cmd_Handler,
// 53
    SPIO_IO_Read_Cmd_Handler,
// 54
    SPIO_IO_Write_Cmd_Handler,
// 55
    SPIO_IO_RMW_Cmd_Handler,
// 56
    Maxion_Init_Cmd_Handler,
// 57
    Maxion_Core_PLL_Program_Cmd_Handler,
// 58
    Maxion_Uncore_PLL_Program_Cmd_Handler,
// 59
    Maxion_Internal_Init_Cmd_Handler,
// 60
    Unregistered_Handler, // Future HW Use
// 61
    Unregistered_Handler, // Future HW Use
// 62
    Unregistered_Handler, // Future HW Use
// 63
    Unregistered_Handler, // Future HW Use
// 64
    Unregistered_Handler, // Future HW Use
// 65
    Unregistered_Handler, // Future HW Use
// 66
    Unregistered_Handler, // Future HW Use
// 67
    Unregistered_Handler, // Future HW Use
// 68
    Unregistered_Handler, // Future HW Use
// 69
    Unregistered_Handler, // Future HW Use
// 70
    Unregistered_Handler, // Future HW Use
// 71
    Unregistered_Handler, // Future HW Use
// 72
    Unregistered_Handler, // Future HW Use
// 73
    Unregistered_Handler, // Future HW Use
// 74
    Unregistered_Handler, // Future HW Use
// 75
    Unregistered_Handler, // Future HW Use
// 76
    Unregistered_Handler, // Future HW Use
// 77
    Unregistered_Handler, // Future HW Use
// 78
    Unregistered_Handler, // Future HW Use
// 79
    Unregistered_Handler, // Future HW Use
// 80
    Unregistered_Handler, // Future HW Use
// 81
    Unregistered_Handler, // Future HW Use
// 82
    Unregistered_Handler, // Future HW Use
// 83
    Unregistered_Handler, // Future HW Use
// 84
    Unregistered_Handler, // Future HW Use
// 85
    Unregistered_Handler, // Future HW Use
// 86
    Unregistered_Handler, // Future HW Use
// 87
    Unregistered_Handler, // Future HW Use
// 88
    Unregistered_Handler, // Future HW Use
// 89
    Unregistered_Handler, // Future HW Use
// 90
    Unregistered_Handler, // Future HW Use
// 91
    Unregistered_Handler, // Future HW Use
// 92
    Unregistered_Handler, // Future HW Use
// 93
    Unregistered_Handler, // Future HW Use
// 94
    Unregistered_Handler, // Future HW Use
// 95
    Unregistered_Handler, // Future HW Use
// 96
    Unregistered_Handler, // Future HW Use
// 97
    Asset_Tracking_Manufacturer_Name_Cmd_Handler,
// 98
    Asset_Tracking_Part_Number_Cmd_Handler,
// 99
    Asset_Tracking_Serial_Number_Cmd_Handler,
// 100
    Asset_Tracking_Chip_Revision_Cmd_Handler,
// 101
    Asset_Tracking_PCIe_Max_Speed_Cmd_Handler,
// 102
    Asset_Tracking_Module_Revision_Cmd_Handler,
// 103
    Asset_Tracking_Form_Factor_Cmd_Handler,
// 104
    Asset_Tracking_Memory_Details_Cmd_Handler,
// 105
    Asset_Tracking_Memory_Size_Cmd_Handler,
// 106
    Asset_Tracking_Memory_Type_Cmd_Handler,
#endif  // !TF_CORE
};
