#include "tf.h"
#include "bl2_asset_trk_mgmt.h"

int8_t Asset_Tracking_Manufacturer_Name_Cmd_Handler(void* test_cmd);
int8_t Asset_Tracking_Part_Number_Cmd_Handler(void* test_cmd);
int8_t Asset_Tracking_Serial_Number_Cmd_Handler(void* test_cmd);
int8_t Asset_Tracking_Chip_Revision_Cmd_Handler(void* test_cmd);
int8_t Asset_Tracking_PCIe_Max_Speed_Cmd_Handler(void* test_cmd);
int8_t Asset_Tracking_Module_Revision_Cmd_Handler(void* test_cmd);
int8_t Asset_Tracking_Form_Factor_Cmd_Handler(void* test_cmd);
int8_t Asset_Tracking_Memory_Details_Cmd_Handler(void* test_cmd);
int8_t Asset_Tracking_Memory_Size_Cmd_Handler(void* test_cmd);
int8_t Asset_Tracking_Memory_Type_Cmd_Handler(void* test_cmd);

int8_t Asset_Tracking_Manufacturer_Name_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    struct tf_rsp_at_manufacturer_name_t rsp = { 0 };

    rsp.rsp_hdr.id = TF_RSP_AT_MANUFACTURER_NAME;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size = TF_GET_PAYLOAD_SIZE(rsp);

    get_manufacturer_name((char*)&rsp.mfr_name);

    TF_Send_Response(&rsp, sizeof(rsp));

    return 0;
}

int8_t Asset_Tracking_Part_Number_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    struct tf_rsp_at_part_number_t rsp = { 0 };

    rsp.rsp_hdr.id = TF_RSP_AT_PART_NUMBER;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size = TF_GET_PAYLOAD_SIZE(rsp);

    get_part_number((char*)&rsp.part_num);

    TF_Send_Response(&rsp, sizeof(rsp));

    return 0;
}

int8_t Asset_Tracking_Serial_Number_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    struct tf_rsp_at_serial_number_t rsp = { 0 };

    rsp.rsp_hdr.id = TF_RSP_AT_SERIAL_NUMBER;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size = TF_GET_PAYLOAD_SIZE(rsp);

    get_serial_number((char*)&rsp.ser_num);

    TF_Send_Response(&rsp, sizeof(rsp));

    return 0;
}

int8_t Asset_Tracking_Chip_Revision_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    struct tf_rsp_at_chip_revision_t rsp = { 0 };

    rsp.rsp_hdr.id = TF_RSP_AT_CHIP_REVISION;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size = TF_GET_PAYLOAD_SIZE(rsp);

    get_chip_revision((char*)&rsp.chip_rev);

    TF_Send_Response(&rsp, sizeof(rsp));

    return 0;
}

int8_t Asset_Tracking_PCIe_Max_Speed_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    struct tf_rsp_at_pcie_max_speed_t rsp = { 0 };

    rsp.rsp_hdr.id = TF_RSP_AT_PCIE_MAX_SPEED;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size = TF_GET_PAYLOAD_SIZE(rsp);

    get_PCIE_speed((char*)&rsp.pcie_max_speed);

    TF_Send_Response(&rsp, sizeof(rsp));

    return 0;
}

int8_t Asset_Tracking_Module_Revision_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    struct tf_rsp_at_module_revision_t rsp = { 0 };

    rsp.rsp_hdr.id = TF_RSP_AT_MODULE_REVISION;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size = TF_GET_PAYLOAD_SIZE(rsp);

    get_module_rev((char*)&rsp.module_rev);

    TF_Send_Response(&rsp, sizeof(rsp));

    return 0;
}

int8_t Asset_Tracking_Form_Factor_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    struct tf_rsp_at_form_factor_t rsp = { 0 };

    rsp.rsp_hdr.id = TF_RSP_AT_FORM_FACTOR;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size = TF_GET_PAYLOAD_SIZE(rsp);

    get_form_factor((char*)&rsp.form_factor);

    TF_Send_Response(&rsp, sizeof(rsp));

    return 0;
}

int8_t Asset_Tracking_Memory_Details_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    struct tf_rsp_at_memory_details_t rsp = { 0 };

    rsp.rsp_hdr.id = TF_RSP_AT_MEMORY_DETAILS;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size = TF_GET_PAYLOAD_SIZE(rsp);

    get_memory_details((char*)&rsp.mem_details);

    TF_Send_Response(&rsp, sizeof(rsp));

    return 0;
}

int8_t Asset_Tracking_Memory_Size_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    struct tf_rsp_at_memory_size_mb_t rsp = { 0 };

    rsp.rsp_hdr.id = TF_RSP_AT_MEMORY_SIZE_MB;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size = TF_GET_PAYLOAD_SIZE(rsp);

    get_memory_size((char*)&rsp.mem_size_mb);

    TF_Send_Response(&rsp, sizeof(rsp));

    return 0;
}

int8_t Asset_Tracking_Memory_Type_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    struct tf_rsp_at_memory_type_t rsp = { 0 };

    rsp.rsp_hdr.id = TF_RSP_AT_MEMORY_TYPE;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size = TF_GET_PAYLOAD_SIZE(rsp);

    get_memory_type((char*)&rsp.mem_type);

    TF_Send_Response(&rsp, sizeof(rsp));

    return 0;
}
