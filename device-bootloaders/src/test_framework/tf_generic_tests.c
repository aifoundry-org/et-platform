#include "tf.h"
#include "bl2_firmware_update.h"
#include "bl2_link_mgmt.h"
#include "perf_mgmt.h"

int8_t SP_Fw_Version_Cmd_Handler(void* test_cmd);
int8_t Echo_To_SP_Cmd_Handler(void* test_cmd);
int8_t Move_Data_To_Device_Cmd_Handler(void* test_cmd);
int8_t Move_Data_To_Host_Cmd_Handler(void* test_cmd);
int8_t SP_PCIE_Retain_Phy_Cmd_Handler(const void* test_cmd);
int8_t SP_Get_Module_ASIC_Freq_Cmd_Handler(const void* test_cmd);

int8_t SP_Fw_Version_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    struct tf_rsp_sp_fw_version_t rsp;

    rsp.rsp_hdr.id = TF_RSP_SP_FW_VERSION;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size =  TF_GET_PAYLOAD_SIZE(struct tf_rsp_sp_fw_version_t);
    rsp.major = 0x01;
    rsp.minor = 0x02;
    rsp.revision = 0x03;

    TF_Send_Response(&rsp, sizeof(struct tf_rsp_sp_fw_version_t));

    return 0;
}

int8_t Echo_To_SP_Cmd_Handler(void* test_cmd)
{
    const struct tf_cmd_echo_to_sp_t* cmd = (struct tf_cmd_echo_to_sp_t*)test_cmd;
    struct tf_rsp_echo_to_sp_t rsp;

    rsp.rsp_hdr.id = TF_RSP_ECHO_TO_SP;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size =  TF_GET_PAYLOAD_SIZE(struct tf_rsp_echo_to_sp_t);
    rsp.cmd_payload = cmd->cmd_payload;

    TF_Send_Response(&rsp, sizeof(struct tf_rsp_echo_to_sp_t));

    return 0;
}

int8_t Move_Data_To_Device_Cmd_Handler(void* test_cmd)
{
    struct tf_cmd_move_data_to_device_t *cmd =
        (struct tf_cmd_move_data_to_device_t *)test_cmd;
    struct tf_rsp_move_data_to_device_t rsp;

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

static struct tf_rsp_move_data_to_host_t   read_rsp;

int8_t Move_Data_To_Host_Cmd_Handler(void* test_cmd)
{
    const struct tf_cmd_move_data_to_host_t  *cmd =
        (const struct tf_cmd_move_data_to_host_t  *)test_cmd;

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

    read_rsp.rsp_hdr.id = TF_RSP_MOVE_DATA_TO_HOST;
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


int8_t SP_PCIE_Retain_Phy_Cmd_Handler(const void* test_cmd)
{
    (void) test_cmd;
    struct tf_rsp_sp_pcie_retain_phy_t rsp;

    rsp.rsp_hdr.id = TF_RSP_SP_PCIE_RETAIN_PHY;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size =  TF_GET_PAYLOAD_SIZE(struct tf_rsp_sp_pcie_retain_phy_t);
    rsp.status = pcie_retrain_phy();

    TF_Send_Response(&rsp, sizeof(struct tf_rsp_sp_pcie_retain_phy_t));

    return 0;
}

int8_t SP_Get_Module_ASIC_Freq_Cmd_Handler(const void* test_cmd)
{
    (void) test_cmd;
    struct asic_frequencies_t asic_frequencies;
    struct tf_rsp_get_module_asic_freq_t rsp = {0};

    rsp.rsp_hdr.id = TF_RSP_GET_MODULE_ASIC_FREQ;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size =  TF_GET_PAYLOAD_SIZE(struct tf_rsp_get_module_asic_freq_t);

    if(0 == get_module_asic_frequencies(&asic_frequencies))
    {
        rsp.minion_shire_mhz = asic_frequencies.minion_shire_mhz;
        rsp.noc_mhz = asic_frequencies.noc_mhz;
        rsp.mem_shire_mhz = asic_frequencies.mem_shire_mhz;
        rsp.ddr_mhz = asic_frequencies.ddr_mhz;
        rsp.pcie_shire_mhz = asic_frequencies.pcie_shire_mhz;
        rsp.io_shire_mhz = asic_frequencies.io_shire_mhz;
    }

    TF_Send_Response(&rsp, sizeof(struct tf_rsp_get_module_asic_freq_t));

    return 0;
}
