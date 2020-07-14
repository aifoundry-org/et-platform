#include <stdint.h>

#include "ms_reg_def.h"
#include "ddrc_reg_def.h"

#define MODE_NUMBER 6

void ddr_init(uint8_t memshire_id);
int ddr_config(void);
uint8_t ms_init_ddr_phy_1067 (uint8_t memshire);
uint8_t ms_init_seq_phase1 (uint8_t memshire, uint8_t config_ecc, uint8_t config_real_pll);
uint8_t ms_init_seq_phase2 (uint8_t memshire, uint8_t config_real_pll);
uint8_t ms_init_seq_phase3 (uint8_t memshire); 
uint8_t ms_init_seq_phase4 (uint8_t memshire);


