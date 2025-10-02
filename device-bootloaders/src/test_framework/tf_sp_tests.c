#include "tf.h"
#include "bl2_firmware_update.h"
#include "bl2_link_mgmt.h"
#include "perf_mgmt.h"

int8_t SP_PCIE_Retain_Phy_Cmd_Handler(const void* test_cmd);
int8_t SP_Get_Module_ASIC_Freq_Cmd_Handler(const void* test_cmd);

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
