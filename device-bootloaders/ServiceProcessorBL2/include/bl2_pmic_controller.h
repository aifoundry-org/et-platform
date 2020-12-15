#ifndef __BL2_PMIC_CONTROLLER_H__
#define __BL2_PMIC_CONTROLLER_H__

void setup_pmic(void);
uint8_t pmic_read_soc_power(void);
void pmic_set_temperature_threshold(uint8_t reg, uint8_t limit);
void pmic_set_tdp_threshold(uint8_t limit);
uint8_t pmic_get_temperature(void);
uint8_t pmic_get_voltage(uint8_t shire);

#endif

