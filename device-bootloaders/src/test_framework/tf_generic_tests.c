#include "tf.h"
#include "bl2_firmware_update.h"
#include "bl2_link_mgmt.h"
#include "perf_mgmt.h"

int8_t SP_Fw_Version_Cmd_Handler(void* test_cmd);
int8_t Echo_To_SP_Cmd_Handler(void* test_cmd);
int8_t Move_Data_To_Device_Cmd_Handler(void* test_cmd);
int8_t Move_Data_To_Host_Cmd_Handler(void* test_cmd);
int8_t SP_PCIE_Retain_Phy_Cmd_Handler(void* test_cmd);
int8_t SP_Get_Module_ASIC_Freq_Cmd_Handler(void* test_cmd);

int8_t SP_Fw_Version_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    SERVICE_PROCESSOR_BL2_DATA_t *sp_bl2_data;
    struct tf_get_spfw_ver_rsp_t rsp;
    uint8_t major;
    uint8_t minor;
    uint8_t revision;

    rsp.rsp_hdr.id = TF_RSP_SP_FW_VERSION;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size =  TF_GET_PAYLOAD_SIZE(struct tf_get_spfw_ver_rsp_t);
    
    // Get BL1 version from BL2 data.
    sp_bl2_data = get_service_processor_bl2_data();

    rsp.bl1_v =
        FORMAT_VERSION((uint32_t)sp_bl2_data->service_processor_bl1_image_file_version_major,
                       (uint32_t)sp_bl2_data->service_processor_bl1_image_file_version_minor,
                       (uint32_t)sp_bl2_data->service_processor_bl1_image_file_version_revision);

    rsp.bl2_v = sp_get_image_version_info();

    // Get the MM FW version values
    firmware_service_get_mm_version(&major, &minor, &revision);
    rsp.mm_v = FORMAT_VERSION((uint32_t)major, (uint32_t)minor, (uint32_t)revision);

    // Get the WM FW version values
    firmware_service_get_wm_version(&major, &minor, &revision);
    rsp.wm_v = FORMAT_VERSION((uint32_t)major, (uint32_t)minor, (uint32_t)revision);

    // Get the Machine FW version values
    firmware_service_get_machm_version(&major, &minor, &revision);
    rsp.machm_v = FORMAT_VERSION((uint32_t)major, (uint32_t)minor, (uint32_t)revision);

    TF_Send_Response(&rsp, sizeof(struct tf_get_spfw_ver_rsp_t));

    return 0;
}

int8_t Echo_To_SP_Cmd_Handler(void* test_cmd)
{
    struct tf_echo_cmd_t* cmd = (struct tf_echo_cmd_t*)test_cmd;
    struct tf_echo_rsp_t rsp;

    rsp.rsp_hdr.id = TF_RSP_ECHO_TO_SP;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size =  TF_GET_PAYLOAD_SIZE(struct tf_echo_rsp_t);
    rsp.rsp_payload = cmd->cmd_payload;

    TF_Send_Response(&rsp, sizeof(struct tf_echo_rsp_t));

    return 0;
}

int8_t Move_Data_To_Device_Cmd_Handler(void* test_cmd)
{
    struct tf_move_data_to_device_cmd_t *cmd =
        (struct tf_move_data_to_device_cmd_t *)test_cmd;
    struct tf_move_data_to_device_rsp_t rsp;

    char* dst = (char*)cmd->dst_addr;
    const char* src = (char*)cmd->data;
    uint32_t size = cmd->size;
    uint32_t bytes_written = 0;

    /* TODO: This is a naive approach,
    can be made optimized */
    while(size) {
        *dst = *src;
        dst++;src++;
        size--;
        bytes_written++;
    }

    rsp.rsp_hdr.id = TF_RSP_MOVE_DATA_TO_DEVICE;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size =  sizeof(rsp.bytes_written);
    rsp.bytes_written = bytes_written;

    TF_Send_Response(&rsp, sizeof(rsp));

    return 0;
}

static struct tf_move_data_to_host_rsp_t   read_rsp;

int8_t Move_Data_To_Host_Cmd_Handler(void* test_cmd)
{
    const struct tf_move_data_to_host_cmd_t  *cmd =
        (const struct tf_move_data_to_host_cmd_t  *)test_cmd;

    const char* src = (char*)cmd->src_addr;
    uint32_t size = cmd->size;
    char* dst = (char*)read_rsp.data;
    uint32_t bytes_read = 0;

    while(size) {
        *dst = *src;
        dst++;src++;
        size--;
        bytes_read++;
    }

    read_rsp.rsp_hdr.id = TF_RSP_MOVE_DATA_To_HOST;
    read_rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    read_rsp.rsp_hdr.payload_size =
        (uint32_t)(sizeof(uint32_t) + bytes_read );
    read_rsp.bytes_read = bytes_read;

    TF_Send_Response(&read_rsp,
        (uint32_t)(sizeof(tf_rsp_hdr_t) +
        sizeof(uint32_t) +
        bytes_read));

    return 0;
}


int8_t SP_PCIE_Retain_Phy_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    struct tf_sp_pcie_retain_phy_rsp_t rsp;

    rsp.rsp_hdr.id = TF_RSP_SP_PCIE_RETAIN_PHY;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size =  TF_GET_PAYLOAD_SIZE(struct tf_sp_pcie_retain_phy_rsp_t);
    rsp.status = pcie_retrain_phy();

    TF_Send_Response(&rsp, sizeof(struct tf_sp_pcie_retain_phy_rsp_t));

    return 0;
}

int8_t SP_Get_Module_ASIC_Freq_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    struct asic_frequencies_t asic_frequencies;
    struct tf_sp_get_asic_freq_rsp_t rsp = {0};

    rsp.rsp_hdr.id = TF_RSP_GET_MODULE_ASIC_FREQ;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size =  TF_GET_PAYLOAD_SIZE(struct tf_sp_get_asic_freq_rsp_t);

    if(0 == get_module_asic_frequencies(&asic_frequencies))
    {
        rsp.minion_shire_mhz = asic_frequencies.minion_shire_mhz;
        rsp.noc_mhz = asic_frequencies.noc_mhz;
        rsp.mem_shire_mhz = asic_frequencies.mem_shire_mhz;
        rsp.ddr_mhz = asic_frequencies.ddr_mhz;
        rsp.pcie_shire_mhz = asic_frequencies.pcie_shire_mhz;
        rsp.io_shire_mhz = asic_frequencies.io_shire_mhz;
    }

    TF_Send_Response(&rsp, sizeof(struct tf_sp_get_asic_freq_rsp_t));

    return 0;
}