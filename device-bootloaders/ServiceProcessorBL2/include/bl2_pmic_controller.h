#ifndef __BL2_PMIC_CONTROLLER_H__
#define __BL2_PMIC_CONTROLLER_H__


#include "dm_event_def.h"

// Should come from the PMIC HAL 
#define PMIC_FIMRWARE_VERSION 0x1
#define GPIO_CONTROL 0x2
#define INT_CONTROL_CONFIG 0x3
#define INT_CONTROL_CAUSE 0x4
#define INPUT_VALUE 0x5
#define TEMP_ALARM_CONFIG_LO 0x6
#define TEMP_ALARM_CONFIG_HI 0x24
#define SYSTEM_TEMP 0x7
#define INPUT_POWER 0x8
#define RESET_CONTROL 0x9
#define RESET_CAUSATION 0xA
#define DDR_VOLTAGE 0xB
#define L2_CACHE_VOLTAGE 0xC
#define MAXION_VOLTAGE 0xD
#define MINION_SHIRE_ALL_VOLTAGE 0xE
#define MINION_SHIRE_GRP_1_VOLTAGE 0xF
#define MINION_SHIRE_GRP_2_VOLTAGE 0x10
#define MINION_SHIRE_GRP_3_VOLTAGE 0x11
#define MINION_SHIRE_GRP_4_VOLTAGE 0x12
#define MINION_SHIRE_GRP_5_VOLTAGE 0x13
#define MINION_SHIRE_GRP_6_VOLTAGE 0x14
#define MINION_SHIRE_GRP_7_VOLTAGE 0x15
#define MINION_SHIRE_GRP_8_VOLTAGE 0x16
#define MINION_SHIRE_GRP_9_VOLTAGE 0x17
#define MINION_SHIRE_GRP_10_VOLTAGE 0x18
#define MINION_SHIRE_GRP_11_VOLTAGE 0x19
#define MINION_SHIRE_GRP_12_VOLTAGE 0x1A
#define MINION_SHIRE_GRP_13_VOLTAGE 0x1B
#define MINION_SHIRE_GRP_14_VOLTAGE 0x1C
#define MINION_SHIRE_GRP_15_VOLTAGE 0x1D
#define MINION_SHIRE_GRP_16_VOLTAGE 0x1E
#define MINION_SHIRE_GRP_17_VOLTAGE 0x1F
#define NOC_VOLTAGE 0x20
#define PCIE_VOLTAGE 0x21
#define WDT_CONFIG 0x22
#define POWER_ALARM_SET_POINT 0x23

enum shire_type_t {
   DDR=0,
   L2CACHE,
   MAXION,
   MINION,
   PCIE,
   NOC
};

void setup_pmic(void);
uint8_t pmic_read_soc_power(void);
void pmic_set_temperature_threshold(_Bool reg, int limit);
void pmic_set_tdp_threshold(int limit);
uint8_t pmic_get_temperature(void);
uint8_t pmic_get_voltage(enum shire_type_t shire);
int32_t pmic_error_control_init(dm_event_isr_callback event_cb);
void pmic_error_isr(void);


#endif

