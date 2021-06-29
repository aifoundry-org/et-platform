#include "tf.h"
#include "bl2_pmic_controller.h"

int8_t PMIC_Module_Temperature_Cmd_Handler(void* test_cmd);
int8_t PMIC_Module_Power_Cmd_Handler(void* test_cmd);
int8_t PMIC_Send_TF(uint16_t id);

int8_t PMIC_Send_TF(uint16_t id)
{
    struct tf_pmic_rsp_t rsp;

    rsp.rsp_hdr.id = id;
    rsp.rsp_hdr.flags = TF_RSP_WITH_PAYLOAD;
    rsp.rsp_hdr.payload_size = TF_GET_PAYLOAD_SIZE(struct tf_pmic_rsp_t);

    rsp.value = 0;

    if (rsp.rsp_hdr.id == TF_RSP_PMIC_MODULE_TEMPERATURE)
        pmic_get_temperature(&rsp.value);
    else if (rsp.rsp_hdr.id == TF_RSP_PMIC_MODULE_POWER)
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
