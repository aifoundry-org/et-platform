#include "tf.h"
#include "bl2_link_mgmt.h"

int8_t PCIE_PSHIRE_Init_Cmd_Handler(void* test_cmd);
int8_t PCIE_PSHIRE_Voltage_Update_Cmd_Handler(void* test_cmd);
int8_t PCIE_PSHIRE_PLL_Program_Cmd_Handler(void* test_cmd);
int8_t PCIE_PSHIRE_NOC_Update_Routing_Table_Cmd_Handler(void* test_cmd);
int8_t PCIE_PSHIRE_Cntr_Init_Cmd_Handler(void* test_cmd);
int8_t PCIE_PSHIRE_Phy_Init_Cmd_Handler(void* test_cmd);
int8_t PCIE_PSHIRE_Cntr_Init_Interrupts(void* test_cmd);
int8_t PCIE_PSHIRE_Cntr_Init_Link_Params(void* test_cmd);
int8_t PCIE_PSHIRE_Cntr_ATU_Init(void* test_cmd);
int8_t PCIE_PSHIRE_Phy_FW_init(void* test_cmd);
int8_t Get_Pcie_Init_Status(const void* test_cmd);

int8_t PCIE_PSHIRE_Init_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;
    
    rsp_hdr.id = TF_RSP_UNSUPPORTED;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t PCIE_PSHIRE_Voltage_Update_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;
    
    rsp_hdr.id = TF_RSP_UNSUPPORTED;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t PCIE_PSHIRE_PLL_Program_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;
    
    rsp_hdr.id = TF_RSP_UNSUPPORTED;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t PCIE_PSHIRE_NOC_Update_Routing_Table_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;
    
    rsp_hdr.id = TF_RSP_UNSUPPORTED;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t PCIE_PSHIRE_Cntr_Init_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;
    
    rsp_hdr.id = TF_RSP_UNSUPPORTED;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t PCIE_PSHIRE_Phy_Init_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;
    
    rsp_hdr.id = TF_RSP_UNSUPPORTED;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t PCIE_PSHIRE_Cntr_Init_Interrupts(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;
    
    rsp_hdr.id = TF_RSP_UNSUPPORTED;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;

    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));

    return 0;
}

int8_t PCIE_PSHIRE_Cntr_Init_Link_Params(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;
    
    rsp_hdr.id = TF_RSP_UNSUPPORTED;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;
    
    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));
    
    return 0;
}

int8_t PCIE_PSHIRE_Cntr_ATU_Init(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;
    
    rsp_hdr.id = TF_RSP_UNSUPPORTED;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;
    
    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));
    
    return 0;
}

int8_t PCIE_PSHIRE_Phy_FW_init(void* test_cmd)
{
    (void) test_cmd;
    tf_rsp_hdr_t rsp_hdr;
    
    rsp_hdr.id = TF_RSP_UNSUPPORTED;
    rsp_hdr.flags = TF_RSP_ONLY;
    rsp_hdr.payload_size =  0;
    
    TF_Send_Response(&rsp_hdr, sizeof(rsp_hdr));
    
    return 0;
}

int8_t Get_Pcie_Init_Status(const void* test_cmd)
{
    (void) test_cmd;
    struct tf_rsp_get_pcie_init_status_t rsp;

    rsp.rsp_hdr.id = TF_RSP_GET_PCIE_INIT_STATUS;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size =  TF_GET_PAYLOAD_SIZE(struct tf_rsp_get_pcie_init_status_t);
    rsp.status = PCIE_Init_Status();

    TF_Send_Response(&rsp, sizeof(struct tf_rsp_get_pcie_init_status_t));

    return 0;
}
