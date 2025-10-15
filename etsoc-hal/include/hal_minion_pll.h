/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

// exporting functions from sp_minion_shire_pll_dll_config.c
#include "hwinc/lvdpll_defines.h"

int dll_config(uint8_t shire_id);
int config_lvdpll_freq_full(uint8_t shire_id, LVDPLL_MODE_e mode);
int config_lvdpll_freq_full_ldo_update(uint8_t shire_id, LVDPLL_MODE_e mode,
	    LVDPLL_LDO_UPDATE_t ldo_update, uint16_t ldo_ref_trim, uint32_t threshold_multiplier);
int config_lvdpll_freq_quick(uint8_t shire_id, LVDPLL_MODE_e mode);
int lvdpll_register_read(uint8_t shire_id, uint32_t reg_num, uint16_t* read_value);
int lvdpll_clear_lock_monitor(uint8_t shire_id);
int lvdpll_read_lock_monitor(uint8_t shire_id, uint16_t* lock_monitor_value);
int lvdpll_disable(uint8_t shire_id);

// helper functions required from consumer code-base