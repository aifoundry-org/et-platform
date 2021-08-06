#include "tf.h"
#include "dm.h"
#include "bl2_pmic_controller.h"
#include "thermal_pwr_mgmt.h"

int8_t PMIC_Module_Temperature_Cmd_Handler(void* test_cmd);
int8_t PMIC_Module_Power_Cmd_Handler(void* test_cmd);
int8_t PMIC_Module_Voltage_Cmd_Handler(void* test_cmd);
int8_t PMIC_Module_Uptime_Cmd_Handler(void* test_cmd);

int8_t PMIC_Send_TF(uint16_t id);

int8_t PMIC_Send_TF(uint16_t id)
{
    struct tf_pmic_rsp_t rsp;

    rsp.rsp_hdr.id = id;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size = TF_GET_PAYLOAD_SIZE(struct tf_pmic_rsp_t);

    rsp.value = 0;

    if (id == TF_RSP_PMIC_MODULE_TEMPERATURE)
        pmic_get_temperature(&rsp.value);
    else if (id == TF_RSP_PMIC_MODULE_POWER)
        pmic_read_soc_power(&rsp.value);
    else
        return -1;

    TF_Send_Response(&rsp, sizeof(struct tf_pmic_rsp_t));

    return 0;
}

int8_t PMIC_Module_Temperature_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    return PMIC_Send_TF(TF_RSP_PMIC_MODULE_TEMPERATURE);
}

int8_t PMIC_Module_Power_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    return PMIC_Send_TF(TF_RSP_PMIC_MODULE_POWER);
}

int8_t PMIC_Module_Voltage_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    struct module_voltage_t module_voltage ={0};;
    struct tf_pmic_module_voltage_rsp_t rsp;

    rsp.rsp_hdr.id = TF_RSP_PMIC_MODULE_VOLTAGE;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size = 
                TF_GET_PAYLOAD_SIZE(struct tf_pmic_module_voltage_rsp_t);

    if( 0 == get_module_voltage(&module_voltage))
    {
        rsp.ddr = module_voltage.ddr;
        rsp.l2_cache = module_voltage.l2_cache;
        rsp.maxion = module_voltage.maxion;
        rsp.minion = module_voltage.minion;
        rsp.noc = module_voltage.noc;
        rsp.pcie_logic = module_voltage.pcie_logic;
        rsp.vddqlp = module_voltage.vddqlp;
        rsp.vddq = module_voltage.vddq;
    }

    return TF_Send_Response(&rsp, sizeof(struct tf_pmic_module_voltage_rsp_t));
}

int8_t PMIC_Module_Uptime_Cmd_Handler(void* test_cmd)
{
    (void) test_cmd;
    struct module_uptime_t module_uptime = {0};
    struct tf_pmic_module_uptime_rsp_t rsp;

    rsp.rsp_hdr.id = TF_RSP_PMIC_MODULE_UPTIME;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size = 
                TF_GET_PAYLOAD_SIZE(struct tf_pmic_module_uptime_rsp_t);

    if(0 == get_module_uptime(&module_uptime))
    {
        rsp.day = module_uptime.day;
        rsp.hours = module_uptime.hours;
        rsp.mins = module_uptime.mins;
    }

    return TF_Send_Response(&rsp, sizeof(struct tf_pmic_module_uptime_rsp_t));
}

