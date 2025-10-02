#include "tf.h"
#include "dm.h"
#include "bl2_pmic_controller.h"
#include "thermal_pwr_mgmt.h"

int8_t PMIC_Module_Temperature_Cmd_Handler(void *test_cmd);
int8_t PMIC_Module_Power_Cmd_Handler(void *test_cmd);
int8_t PMIC_Module_Voltage_Cmd_Handler(const void *test_cmd);
int8_t PMIC_Module_Uptime_Cmd_Handler(const void *test_cmd);

int8_t PMIC_Module_Temperature_Cmd_Handler(void *test_cmd)
{
    (void)test_cmd;
    struct tf_rsp_pmic_module_temperature_t rsp = { 0 };

    rsp.rsp_hdr.id = TF_RSP_PMIC_MODULE_TEMPERATURE;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size = TF_GET_PAYLOAD_SIZE(rsp);

    pmic_get_temperature(&rsp.mod_temperature);

    TF_Send_Response(&rsp, sizeof(rsp));

    return 0;
}

int8_t PMIC_Module_Power_Cmd_Handler(void *test_cmd)
{
    (void)test_cmd;

    struct tf_rsp_pmic_module_power_t rsp = { 0 };

    rsp.rsp_hdr.id = TF_RSP_PMIC_MODULE_POWER;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size = TF_GET_PAYLOAD_SIZE(rsp);

    pmic_read_instantaneous_soc_power(&rsp.mod_power);
    TF_Send_Response(&rsp, sizeof(rsp));

    return 0;
}

int8_t PMIC_Module_Voltage_Cmd_Handler(const void *test_cmd)
{
    (void)test_cmd;
    struct module_voltage_t module_voltage = { 0 };
    struct tf_rsp_pmic_module_voltage_t rsp;

    rsp.rsp_hdr.id = TF_RSP_PMIC_MODULE_VOLTAGE;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size = TF_GET_PAYLOAD_SIZE(struct tf_rsp_pmic_module_voltage_t);

    if (0 == get_module_voltage(&module_voltage))
    {
        rsp.ddr_voltage = module_voltage.ddr;
        rsp.l2_cache_voltage = module_voltage.l2_cache;
        rsp.maxion_voltage = module_voltage.maxion;
        rsp.minion_voltage = module_voltage.minion;
        rsp.noc_voltage = module_voltage.noc;
        rsp.pcie_voltage = module_voltage.pcie;
        rsp.pcie_logic_voltage = module_voltage.pcie_logic;
        rsp.vddqlp_voltage = module_voltage.vddqlp;
        rsp.vddq_voltage = module_voltage.vddq;
    }

    return TF_Send_Response(&rsp, sizeof(struct tf_rsp_pmic_module_voltage_t));
}

int8_t PMIC_Module_Uptime_Cmd_Handler(const void *test_cmd)
{
    (void)test_cmd;
    struct module_uptime_t module_uptime = { 0 };
    struct tf_rsp_pmic_module_uptime_t rsp;

    rsp.rsp_hdr.id = TF_RSP_PMIC_MODULE_UPTIME;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size = TF_GET_PAYLOAD_SIZE(struct tf_rsp_pmic_module_uptime_t);

    if (0 == get_module_uptime(&module_uptime))
    {
        rsp.day = module_uptime.day;
        rsp.hour = module_uptime.hours;
        rsp.minute = module_uptime.mins;
    }

    return TF_Send_Response(&rsp, sizeof(struct tf_rsp_pmic_module_uptime_t));
}
