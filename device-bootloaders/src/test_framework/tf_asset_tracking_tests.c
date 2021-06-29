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

// Helper
int8_t Asset_Tracking_Send_TF(uint16_t id);

int8_t Asset_Tracking_Send_TF(uint16_t id)
{
    struct tf_asset_tracking_rsp_t rsp;

    rsp.rsp_hdr.id = id;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size = TF_GET_PAYLOAD_SIZE(struct tf_asset_tracking_rsp_t);
    
    memset(rsp.data, 0, rsp.rsp_hdr.payload_size);

    switch(rsp.rsp_hdr.id) {
        case TF_RSP_AT_MANUFACTURER_NAME:
            get_manufacturer_name(rsp.data);
        break;

        case TF_RSP_AT_PART_NUMBER:
            get_part_number(rsp.data);
        break;

        case TF_RSP_AT_SERIAL_NUMBER:
            get_serial_number(rsp.data);
        break;

        case TF_RSP_AT_CHIP_REVISION:
            get_chip_revision(rsp.data);
        break;

        case TF_RSP_AT_PCIE_MAX_SPEED:
            get_PCIE_speed(rsp.data);
        break;

        case TF_RSP_AT_MODULE_REVISION:
            get_module_rev(rsp.data);
        break;

        case TF_RSP_AT_FORM_FACTOR:
            get_form_factor(rsp.data);
        break;

        case TF_RSP_AT_MEMORY_DETAILS:
            get_memory_details(rsp.data);
        break;

        case TF_RSP_AT_MEMORY_SIZE_MB:
            get_memory_size(rsp.data);
        break;

        case TF_RSP_AT_MEMORY_TYPE:
            get_memory_type(rsp.data);
        break;

        default:
            // Invalid commad id
            return -1;
    }

    TF_Send_Response(&rsp, sizeof(struct tf_asset_tracking_rsp_t));

    return 0;
}

int8_t Asset_Tracking_Manufacturer_Name_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    return Asset_Tracking_Send_TF(TF_RSP_AT_MANUFACTURER_NAME);
}

int8_t Asset_Tracking_Part_Number_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    return Asset_Tracking_Send_TF(TF_RSP_AT_PART_NUMBER);
}

int8_t Asset_Tracking_Serial_Number_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    return Asset_Tracking_Send_TF(TF_RSP_AT_SERIAL_NUMBER);
}

int8_t Asset_Tracking_Chip_Revision_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    return Asset_Tracking_Send_TF(TF_RSP_AT_CHIP_REVISION);
}

int8_t Asset_Tracking_PCIe_Max_Speed_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    return Asset_Tracking_Send_TF(TF_RSP_AT_PCIE_MAX_SPEED);
}

int8_t Asset_Tracking_Module_Revision_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    return Asset_Tracking_Send_TF(TF_RSP_AT_MODULE_REVISION);
}

int8_t Asset_Tracking_Form_Factor_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    return Asset_Tracking_Send_TF(TF_RSP_AT_FORM_FACTOR);
}

int8_t Asset_Tracking_Memory_Details_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    return Asset_Tracking_Send_TF(TF_RSP_AT_MEMORY_DETAILS);
}

int8_t Asset_Tracking_Memory_Size_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    return Asset_Tracking_Send_TF(TF_RSP_AT_MEMORY_SIZE_MB);
}

int8_t Asset_Tracking_Memory_Type_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    return Asset_Tracking_Send_TF(TF_RSP_AT_MEMORY_TYPE);
}

