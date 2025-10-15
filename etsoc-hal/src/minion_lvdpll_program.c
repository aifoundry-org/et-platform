/* Copyright (c) 2025 Ainekko, Co. */
/* SPDX-License-Identifier: Apache-2.0 */
/*                                                                         */
// From commit 3277180a59d39ed83be0987292829364bf5c1367 in esperanto-soc repository
/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------
*/


LVDPLL_MODE_e freq_to_mode(uint16_t freq, uint8_t lvdpll_strap)
{
    return (LVDPLL_MODE_e)(((freq - MODE_FREQUENCY_STEP_BASE) / MODE_FREQUENCY_STEP_SIZE) + (lvdpll_strap * MODE_NUMBER_OF_MODES));
}

uint16_t mode_to_freq(LVDPLL_MODE_e mode, uint8_t lvdpll_strap)
{
    return (uint16_t)(MODE_FREQUENCY_STEP_BASE + ((mode - (uint8_t)(lvdpll_strap * MODE_NUMBER_OF_MODES)) * MODE_FREQUENCY_STEP_SIZE));
}

static int pll_config_multiple_write(uint8_t shire_id, uint32_t reg_first, uint32_t reg_num)
{
    uint64_t reg_value;

    // Enable PLL auto config
    reg_value = ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_PCLK_SEL_SET(2)
              | ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_REG_NUM_SET(reg_num)
              | ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_REG_FIRST_SET(reg_first)
              | ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_ENABLE_SET(0x1);
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_ADDRESS, reg_value, 0);

    // Start PLL auto config
    reg_value = ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_PCLK_SEL_SET(2)
              | ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_REG_NUM_SET(reg_num)
              | ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_REG_FIRST_SET(reg_first)
              | ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_WRITE_SET(0x1)
              | ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_RUN_SET(0x1)
              | ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_ENABLE_SET(0x1);
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_ADDRESS, reg_value, 0);

    // Wait for the PLL configuration to finish
    WAIT_BUSY(shire_id)

    // Stop the PLL auto config
    reg_value = ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_PCLK_SEL_SET(2)
              | ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_REG_NUM_SET(reg_num)
              | ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_REG_FIRST_SET(reg_first);
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_ADDRESS, reg_value, 0);

    return 0;
}

int update_minion_pll_freq_full(LVDPLL_MODE_e mode, uint64_t minion_shire_mask, uint8_t num_of_shires)
{
    uint64_t reg_value;

    // Select 650MHz from step_clock, Bits[2:0] = 3'b011. Bit 3 to '1' to go with DLL output
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_CTRL_CLOCKMUX_ADDRESS, minion_shire_mask, SELECT_STEP_CLOCK);

    // Enable PLL auto config
    reg_value = ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_PCLK_SEL_SET(2)
              | ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_REG_NUM_SET(0x0)
              | ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_REG_FIRST_SET(0x2)
              | ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_ENABLE_SET(0x1);
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_ADDRESS, minion_shire_mask, reg_value);

    // Write PLL registers 0x00 to 0x0f. PLL registers can be downloaded in chunks of 16 registers
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[3];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[2];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[1];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[0];
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, minion_shire_mask, reg_value);
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[7];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[6];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[5];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[4];
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_1_ADDRESS, minion_shire_mask, reg_value);
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[11];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[10];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[9];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[8];
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_2_ADDRESS, minion_shire_mask, reg_value);
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[15];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[14];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[13];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[12];
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_3_ADDRESS, minion_shire_mask, reg_value);
    if(0 != pll_broadcast_register_write(0x00,0xf,minion_shire_mask,num_of_shires,true))
    {
        return -1;
    }

    // Write PLL registers 0x10 to 0x1a.
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[19];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[18];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[17];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[16];
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, minion_shire_mask, reg_value);
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[23];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[22];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[21];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[20];
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_1_ADDRESS, minion_shire_mask, reg_value);
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[26];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[25];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[24];
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_2_ADDRESS, minion_shire_mask, reg_value);
    if(0 != pll_broadcast_register_write(0x10,0xa,minion_shire_mask,num_of_shires,true))
    {
        return -1;
    }

    // Write PLL registers 0x20 to 0x24.
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[30];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[29];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[28];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[27];
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, minion_shire_mask, reg_value);
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[31];
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_1_ADDRESS, minion_shire_mask, reg_value);
    if(0 != pll_broadcast_register_write(0x20,0x4,minion_shire_mask,num_of_shires,true))
    {
        return -1;
    }

    // Write PLL registers 0x27 to 0x28.(DATA_1 and DATA_2 and DATA_3 not changed)
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[33];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[32];
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, minion_shire_mask, reg_value);
    if(0 != pll_broadcast_register_write(0x27,0x1,minion_shire_mask,num_of_shires,true))
    {
        return -1;
    }

    // Write PLL registers 0x38 to 0x39.(DATA_1 and DATA_2 and DATA_3 not changed)
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[35];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[34];
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, minion_shire_mask, reg_value);
    if(0 != pll_broadcast_register_write(0x38,0x1,minion_shire_mask,num_of_shires,true))
    {
        return -1;
    }

    // Write PLL strobe register 0x38 to load previous configuration
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, minion_shire_mask, REG_STROBE_UPDATE);
    if(0 != pll_broadcast_register_write(0x38,0x0,minion_shire_mask,num_of_shires,true))
    {
        return -1;
    }


    // Wait until PLL is locked to change clock mux
    if(0 != wait_for_pll_lock(minion_shire_mask, num_of_shires, true))
    {
        return -1;
    }

    // Select PLL[0] output. Bits[2:0]=3'b100. Bit 3 to '1' to go with DLL output
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_CTRL_CLOCKMUX_ADDRESS, minion_shire_mask, SELECT_PLL_CLOCK_0);

    // Disable the PLL auto config
    reg_value = ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_PCLK_SEL_SET(2);
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_ADDRESS, minion_shire_mask, reg_value);

    return 0;
}

int update_minion_pll_freq_quick(LVDPLL_MODE_e mode, uint64_t minion_shire_mask, uint8_t num_of_shires)
{
    uint64_t reg_value;

    // Select 1 GHz from step_clock, Bits[2:0] = 3'b011. Bit 3 to '1' to go with DLL output
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_CTRL_CLOCKMUX_ADDRESS, minion_shire_mask, SELECT_STEP_CLOCK);

    // Enable PLL auto config
    reg_value = ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_PCLK_SEL_SET(2)
              | ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_REG_NUM_SET(0x0)
              | ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_REG_FIRST_SET(0x2)
              | ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_ENABLE_SET(0x1);
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_ADDRESS, minion_shire_mask, reg_value);

    // Writing PLL registers:
    //  0x02  -  fcw_int
    //  0x03  -  fcw_frac
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[3];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[2];
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, minion_shire_mask, reg_value);
    if(0 != pll_broadcast_register_write(0x02,0x1,minion_shire_mask,num_of_shires,true))
    {
        return -1;
    }

    // Writing PLL register:
    //  0x0b  -  lock_threshold
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[11];
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, minion_shire_mask, reg_value);
    if(0 != pll_broadcast_register_write(0x0b,0x0,minion_shire_mask,num_of_shires,true))
    {
        return -1;
    }

    // Writing PLL register:
    //  0x0e  -  postdiv0
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[14];
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, minion_shire_mask, reg_value);
    if(0 != pll_broadcast_register_write(0x0e,0x0,minion_shire_mask,num_of_shires,true))
    {
        return -1;
    }

    // Writing PLL register:
    //  0x20  -  osc_cap_trim
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[32];
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, minion_shire_mask, reg_value);
    if(0 != pll_broadcast_register_write(0x20,0x0,minion_shire_mask,num_of_shires,true))
    {
        return -1;
    }


    // Write PLL strobe register 0x38 to load previous configuration
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, minion_shire_mask, REG_STROBE_UPDATE);
    if(0 != pll_broadcast_register_write(0x38,0x0,minion_shire_mask,num_of_shires,true))
    {
        return -1;
    }


    // Wait until PLL is locked to change clock mux
    if(0 != wait_for_pll_lock(minion_shire_mask, num_of_shires, true))
    {
        return -1;
    }

    // Select PLL[0] output. Bits[2:0]=3'b100. Bit 3 to '1' to go with DLL output
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_CTRL_CLOCKMUX_ADDRESS, minion_shire_mask, SELECT_PLL_CLOCK_0);

    // Stop the PLL auto config
    reg_value = ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_PCLK_SEL_SET(2);
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_ADDRESS, minion_shire_mask, reg_value);

    return 0;
}

int turn_off_pll_auto_normalization(uint64_t minion_shire_mask, uint8_t num_of_shires)
{
    uint64_t reg_value;

    // Enable PLL auto config
    reg_value = ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_PCLK_SEL_SET(2)
              | ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_REG_NUM_SET(0x0)
              | ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_REG_FIRST_SET(0x2)
              | ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_ENABLE_SET(0x1);
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_ADDRESS, minion_shire_mask, reg_value);

    // Turn of auto-normalization
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, minion_shire_mask, AUTO_NORMALIZATION_OFF);
    if(0 != pll_broadcast_register_write(0x00,0x0,minion_shire_mask,num_of_shires,true))
    {
        return -1;
    }

    // Disable the PLL auto config
    reg_value =  ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_PCLK_SEL_SET(2);
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_ADDRESS, minion_shire_mask, reg_value);

    return 0;
}

int update_self_pll_freq_full(LVDPLL_MODE_e mode)
{
    uint64_t reg_value;

    // Select 1 GHz from step_clock, Bits[2:0] = 3'b011. Bit 3 to '1' to go with DLL output
    write_esr_new(PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_CTRL_CLOCKMUX_ADDRESS, SELECT_STEP_CLOCK, 0);

    // Write PLL registers 0x00 to 0x0f. PLL registers can be downloaded in chunks of 16 registers
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[3];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[2];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[1];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[0];
    write_esr_new(PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, reg_value, 0);
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[7];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[6];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[5];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[4];
    write_esr_new(PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_1_ADDRESS, reg_value, 0);
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[11];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[10];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[9];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[8];
    write_esr_new(PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_2_ADDRESS, reg_value, 0);
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[15];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[14];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[13];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[12];
    write_esr_new(PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_3_ADDRESS, reg_value, 0);
    if(0 != pll_config_multiple_write(LOCAL_SHIRE_ID,0x00,0xf))
    {
        return -1;
    }

    // Write PLL registers 0x10 to 0x1a.
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[19];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[18];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[17];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[16];
    write_esr_new(PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, reg_value, 0);
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[23];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[22];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[21];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[20];
    write_esr_new(PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_1_ADDRESS, reg_value, 0);
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[26];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[25];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[24];
    write_esr_new(PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_2_ADDRESS, reg_value, 0);
    if(0 != pll_config_multiple_write(LOCAL_SHIRE_ID,0x10,0xa))
    {
        return -1;
    }

    // Write PLL registers 0x20 to 0x24.
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[30];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[29];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[28];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[27];
    write_esr_new(PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, reg_value, 0);
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[31];
    write_esr_new(PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_1_ADDRESS, reg_value, 0);
    if(0 != pll_config_multiple_write(LOCAL_SHIRE_ID,0x20,0x4))
    {
        return -1;
    }

    // Write PLL registers 0x27 to 0x28.(DATA_1 and DATA_2 and DATA_3 not changed)
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[33];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[32];
    write_esr_new(PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, reg_value, 0);
    if(0 != pll_config_multiple_write(LOCAL_SHIRE_ID,0x27,0x1))
    {
        return -1;
    }

    // Write PLL registers 0x38 to 0x39.(DATA_1 and DATA_2 and DATA_3 not changed)
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[35];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[34];
    write_esr_new(PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, reg_value, 0);
    if(0 != pll_config_multiple_write(LOCAL_SHIRE_ID,0x38,0x1))
    {
        return -1;
    }

    // Write PLL strobe register 0x38 to load previous configuration
    write_esr_new(PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, REG_STROBE_UPDATE, 0);
    if(0 != pll_config_multiple_write(LOCAL_SHIRE_ID,0x38,0x0))
    {
        return -1;
    }

    // Wait until PLL is locked to change clock mux
    int timeout = LOCK_TIMEOUT;
    while (timeout > 0) {
        if (ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_READ_DATA_LOCKED_GET(read_esr_new(PP_MACHINE, LOCAL_SHIRE_ID,
                REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_READ_DATA_ADDRESS, 0))) {
            break;
        }
        --timeout;
    }
    if(timeout == 0)
    {
        return -1;
    }

    // Select PLL[0] output. Bits[2:0]=3'b100. Bit 3 to '1' to go with DLL output
    write_esr_new(PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_CTRL_CLOCKMUX_ADDRESS, SELECT_PLL_CLOCK_0, 0);

    return 0;

}

int update_self_pll_freq_quick(LVDPLL_MODE_e mode)
{
    uint64_t reg_value;

    // Select 1 GHz from step_clock, Bits[2:0] = 3'b011. Bit 3 to '1' to go with DLL output
    write_esr_new (PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_CTRL_CLOCKMUX_ADDRESS, SELECT_STEP_CLOCK, 0);

    // Writing PLL registers:
    //  0x02  -  fcw_int
    //  0x03  -  fcw_frac
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[3];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[2];
    write_esr_new (PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, reg_value, 0);
    if(0 != pll_config_multiple_write(LOCAL_SHIRE_ID,0x02,0x1))
    {
        return -1;
    }

    // Writing PLL register:
    //  0x0b  -  lock_threshold
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[11];
    write_esr_new (PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, reg_value, 0);
    if(0 != pll_config_multiple_write(LOCAL_SHIRE_ID,0x0b,0x0))
    {
        return -1;
    }

    // Writing PLL register:
    //  0x0e  -  postdiv0
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[14];
    write_esr_new (PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, reg_value, 0);
    if(0 != pll_config_multiple_write(LOCAL_SHIRE_ID,0x0e,0x0))
    {
        return -1;
    }

    // Writing PLL register:
    //  0x20  -  osc_cap_trim
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[32];
    write_esr_new (PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, reg_value, 0);
    if(0 != pll_config_multiple_write(LOCAL_SHIRE_ID,0x20,0x0))
    {
        return -1;
    }

    // Write PLL strobe register 0x38 to load previous configuration
    write_esr_new (PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, REG_STROBE_UPDATE, 0);
    if(0 != pll_config_multiple_write(LOCAL_SHIRE_ID,0x38,0x0))
    {
        return -1;
    }

    // Wait until PLL is locked to change clock mux
    int timeout = LOCK_TIMEOUT;
    while (timeout > 0) {
        if (ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_READ_DATA_LOCKED_GET(read_esr_new(PP_MACHINE, LOCAL_SHIRE_ID,
                REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_READ_DATA_ADDRESS, 0))) {
            break;
        }
        --timeout;
    }
    if(timeout == 0)
    {
        return -1;
    }

    // Select PLL[0] output. Bits[2:0]=3'b100. Bit 3 to '1' to go with DLL output
    write_esr_new (PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_CTRL_CLOCKMUX_ADDRESS, SELECT_PLL_CLOCK_0, 0);

    return 0;

}
