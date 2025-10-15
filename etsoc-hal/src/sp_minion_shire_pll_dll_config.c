/* Copyright (c) 2025 Ainekko, Co. */
/* SPDX-License-Identifier: Apache-2.0 */
/*                                                                         */
// From commit 6b2223de8e142bc49820a0b3b81badfa05f5b67b in esperanto-soc repository

int dll_config(uint8_t shire_id)
{
    uint64_t reg_value;

    /* Select ref clock for DLL input */
    SWITCH_CLOCK_MUX(shire_id, SELECT_REF_CLOCK)

    /* Auto-config register set dll_enable and get reset deasserted of the DLL */
    reg_value = ETSOC_SHIRE_OTHER_ESR_SHIRE_DLL_AUTO_CONFIG_PCLK_SEL_SET(2) |
                ETSOC_SHIRE_OTHER_ESR_SHIRE_DLL_AUTO_CONFIG_DLL_ENABLE_SET(0x1);
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_DLL_AUTO_CONFIG_ADDRESS, reg_value, 0);

    /* Wait for the DLL to lock within a given timeout */
    WAIT_DLL_LOCK(shire_id)
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

int lvdpll_register_read(uint8_t shire_id, uint32_t reg_num, uint16_t* read_value)
{
  uint64_t reg_value;

  // Enable PLL auto config
  reg_value = ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_PCLK_SEL_SET(2)
            | ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_REG_FIRST_SET(reg_num)
            | ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_ENABLE_SET(0x1);
  write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_ADDRESS, reg_value, 0);

  // Start PLL auto config read
  reg_value = ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_PCLK_SEL_SET(2)
            | ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_REG_FIRST_SET(reg_num)
            | ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_RUN_SET(0x1)
            | ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_ENABLE_SET(0x1);
  write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_ADDRESS, reg_value, 0);

  // Wait for the PLL read to finish
  WAIT_BUSY(shire_id)
  *read_value = (uint16_t)(ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_READ_DATA_READ_DATA_GET(read_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_READ_DATA_ADDRESS, 0)));

  // Stop the PLL auto config
  reg_value = ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_PCLK_SEL_SET(2)
            | ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_REG_FIRST_SET(reg_num);
  write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_ADDRESS, reg_value, 0);

  return 0;
}

int lvdpll_clear_lock_monitor(uint8_t shire_id)
{
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, 0x3, 0);
    if(0 != pll_config_multiple_write(shire_id,0x19,0x0))
    {
        return -1;
    }

    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, 0x0, 0);
    if(0 != pll_config_multiple_write(shire_id,0x19,0x0))
    {
        return -1;
    }

    return 0;
}

int lvdpll_read_lock_monitor(uint8_t shire_id, uint16_t* lock_monitor_value)
{
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, 0x1, 0);
    if(0 != pll_config_multiple_write(shire_id,0x19,0x0))
    {
        return -1;
    }

    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, 0x0, 0);
    if(0 != pll_config_multiple_write(shire_id,0x19,0x0))
    {
        return -1;
    }

    if(0 != lvdpll_register_read(shire_id,0x30,lock_monitor_value))
    {
        return -1;
    }

    return 0;
}

int lvdpll_disable(uint8_t shire_id)
{
    uint16_t reg0;

    if(0 != lvdpll_register_read(shire_id,0x0,&reg0))
    {
        return -1;
    }

    reg0 &= (uint16_t)(~(0x0008)); // Disable PLL
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, reg0, 0);
    if(0 != pll_config_multiple_write(shire_id,0x00,0x0))
    {
        return -1;
    }

    // Write PLL strobe register 0x38 to load previous configuration
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, REG_STROBE_UPDATE, 0);
    if(0 != pll_config_multiple_write(shire_id,0x38,0x0))
    {
        return -1;
    }

    return 0;
}

int config_lvdpll_freq_full(uint8_t shire_id, LVDPLL_MODE_e mode)
{
    uint64_t reg_value;

    // Write PLL registers 0x00 to 0x0f. PLL registers can be downloaded in chunks of 16 registers
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[3];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[2];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[1];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[0];
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, reg_value, 0);
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[7];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[6];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[5];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[4];
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_1_ADDRESS, reg_value, 0);
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[11];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[10];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[9];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[8];
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_2_ADDRESS, reg_value, 0);
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[15];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[14];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[13];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[12];
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_3_ADDRESS, reg_value, 0);
    if(0 != pll_config_multiple_write(shire_id,0x00,0xf))
    {
        return -1;
    }

    // Write PLL registers 0x10 to 0x1a.
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[19];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[18];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[17];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[16];
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, reg_value, 0);
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[23];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[22];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[21];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[20];
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_1_ADDRESS, reg_value, 0);
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[26];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[25];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[24];
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_2_ADDRESS, reg_value, 0);
    if(0 != pll_config_multiple_write(shire_id,0x10,0xa))
    {
        return -1;
    }

    // Write PLL registers 0x20 to 0x24.
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[30];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[29];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[28];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[27];
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, reg_value, 0);
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[31];
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_1_ADDRESS, reg_value, 0);
    if(0 != pll_config_multiple_write(shire_id,0x20,0x4))
    {
        return -1;
    }

    // Write PLL registers 0x27 to 0x28.(DATA_1 and DATA_2 and DATA_3 not changed)
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[33];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[32];
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, reg_value, 0);
    if(0 != pll_config_multiple_write(shire_id,0x27,0x1))
    {
        return -1;
    }

    // Write PLL registers 0x38 to 0x39.(DATA_1 and DATA_2 and DATA_3 not changed)
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[35];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[34];
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, reg_value, 0);
    if(0 != pll_config_multiple_write(shire_id,0x38,0x1))
    {
        return -1;
    }

    // Write PLL strobe register 0x38 to load previous configuration
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, REG_STROBE_UPDATE, 0);
    if(0 != pll_config_multiple_write(shire_id,0x38,0x0))
    {
        return -1;
    }

    /* Wait for the PLL to lock within a given timeout */
    WAIT_PLL_LOCK(shire_id)

}

int config_lvdpll_freq_full_ldo_update(uint8_t shire_id, LVDPLL_MODE_e mode, LVDPLL_LDO_UPDATE_t ldo_update, uint16_t ldo_ref_trim, uint32_t threshold_multiplier)
{
    uint64_t reg_value;
    uint16_t ldo_reg;
    uint16_t reg0;
    uint16_t threshold_reg;
    uint16_t osc_reg;

    // Write PLL registers 0x00 to 0x0f. PLL registers can be downloaded in chunks of 16 registers
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[3];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[2];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[1];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[0];
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, reg_value, 0);
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[7];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[6];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[5];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[4];
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_1_ADDRESS, reg_value, 0);
    reg_value = 0;
    threshold_reg = gs_lvdpll_settings[(mode-1)].values[11];
    if ((uint32_t)(threshold_reg * threshold_multiplier) > 0x0000FFFF)
    {
        threshold_reg = (uint16_t)(0xFFFF);
    }
    else
    {
        threshold_reg = (uint16_t)(threshold_reg * threshold_multiplier);
    }
    reg_value = reg_value | threshold_reg;
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[10];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[9];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[8];
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_2_ADDRESS, reg_value, 0);
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[15];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[14];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[13];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[12];
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_3_ADDRESS, reg_value, 0);
    if(0 != pll_config_multiple_write(shire_id,0x00,0xf))
    {
        return -1;
    }

    // Write PLL registers 0x10 to 0x1a.
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[19];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[18];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[17];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[16];
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, reg_value, 0);
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[23];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[22];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[21];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[20];
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_1_ADDRESS, reg_value, 0);
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[26];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[25];
    ldo_reg = gs_lvdpll_settings[(mode-1)].values[24];
    ldo_reg &= (uint16_t)(~(0x3C)); // Clear Trim
    ldo_reg |= (uint16_t)(ldo_ref_trim << 2); // Update trim
    switch (ldo_update)
    {
        case LVDPLL_LDO_DISABLE:
        case LVDPLL_LDO_KICK:
            ldo_reg &= (uint16_t)(~(0x1));  // Clear bypass
            ldo_reg |= 0x2;    // Turn off LDO
            break;
        case LVDPLL_LDO_ENABLE:
            ldo_reg &= (uint16_t)(~(0x3)); // Turn on LDO and remove LDO bypass
            break;
        case LVDPLL_LDO_BYPASS:
            ldo_reg |= 0x1;    // Bypass LDO
            break;
        case LVDPLL_LDO_NO_UPDATE:
        default:
            break;
    }
    reg_value = (reg_value << 16) | ldo_reg;
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_2_ADDRESS, reg_value, 0);
    if(0 != pll_config_multiple_write(shire_id,0x10,0xa))
    {
        return -1;
    }

    // Write PLL registers 0x20 to 0x24.
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[30];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[29];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[28];
    osc_reg = gs_lvdpll_settings[(mode-1)].values[27];
    if (ldo_update == LVDPLL_LDO_KICK || ldo_update == LVDPLL_LDO_ENABLE)
    {
        osc_reg = 0x4;
    }
    reg_value = (reg_value << 16) | osc_reg;
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, reg_value, 0);
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[31];
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_1_ADDRESS, reg_value, 0);
    if(0 != pll_config_multiple_write(shire_id,0x20,0x4))
    {
        return -1;
    }

    // Write PLL registers 0x27 to 0x28.(DATA_1 and DATA_2 and DATA_3 not changed)
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[33];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[32];
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, reg_value, 0);
    if(0 != pll_config_multiple_write(shire_id,0x27,0x1))
    {
        return -1;
    }

    // Write PLL registers 0x38 to 0x39.(DATA_1 and DATA_2 and DATA_3 not changed)
    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[35];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[34];
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, reg_value, 0);
    if(0 != pll_config_multiple_write(shire_id,0x38,0x1))
    {
        return -1;
    }

    // Write PLL strobe register 0x38 to load previous configuration
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, REG_STROBE_UPDATE, 0);
    if(0 != pll_config_multiple_write(shire_id,0x38,0x0))
    {
        return -1;
    }

    for (int i=0; i<2000; i++)
        {
            __asm volatile ( " nop " );
        }

    /* Wait for the PLL to lock within a given timeout */
    int time_out = LOCK_TIMEOUT;
    while (time_out > 0) {
        if (ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_READ_DATA_LOCKED_GET(read_esr_new(PP_MACHINE, shire_id,
                REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_READ_DATA_ADDRESS, 0))) {
            break;
        }
        --time_out;
    }
    if(time_out == 0) return -1;

    if (ldo_update == LVDPLL_LDO_KICK)
    {
        ldo_reg &= (uint16_t)(~(0x2));  // Turn on LDO
        // Write ldo reg
        write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, ldo_reg, 0);
        if(0 != pll_config_multiple_write(shire_id,0x18,0x0))
        {
            return -1;
        }

        // Write PLL strobe register 0x38 to load previous configuration
        write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, REG_STROBE_UPDATE, 0);
        if(0 != pll_config_multiple_write(shire_id,0x38,0x0))
        {
            return -1;
        }

        for (int i=0; i<2000; i++)
        {
            __asm volatile ( " nop " );
        }

        reg0 = gs_lvdpll_settings[(mode-1)].values[0];
        reg0 &= (uint16_t)(~(0x8)); // Disable PLL
        write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, reg0, 0);
        if(0 != pll_config_multiple_write(shire_id,0x0,0x0))
        {
            return -1;
        }

        // Write PLL strobe register 0x38 to load previous configuration
        write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, REG_STROBE_UPDATE, 0);
        if(0 != pll_config_multiple_write(shire_id,0x38,0x0))
        {
            return -1;
        }

        reg0 |= 0x8; // Enable PLL
        write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, reg0, 0);
        if(0 != pll_config_multiple_write(shire_id,0x0,0x0))
        {
            return -1;
        }

        // Write PLL strobe register 0x38 to load previous configuration
        write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, REG_STROBE_UPDATE, 0);
        if(0 != pll_config_multiple_write(shire_id,0x38,0x0))
        {
            return -1;
        }

        for (int i=0; i<2000; i++)
        {
            __asm volatile ( " nop " );
        }

        /* Wait for the PLL to lock within a given timeout */
        WAIT_PLL_LOCK(shire_id)
    }

    return 0;

}

int config_lvdpll_freq_quick(uint8_t shire_id, LVDPLL_MODE_e mode)
{
    uint64_t reg_value;

    // Select 1 GHz from step_clock, Bits[2:0] = 3'b011. Bit 3 to '1' to go with DLL output
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_CTRL_CLOCKMUX_ADDRESS, SELECT_STEP_CLOCK, 0);

    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[3];
    reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[2];
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, reg_value, 0);
    if(0 != pll_config_multiple_write(shire_id,0x02,0x1))
    {
        return -1;
    }

    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[11];
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, reg_value, 0);
    if(0 != pll_config_multiple_write(shire_id,0x0b,0x0))
    {
        return -1;
    }

    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[14];
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, reg_value, 0);
    if(0 != pll_config_multiple_write(shire_id,0x0e,0x0))
    {
        return -1;
    }

    reg_value = 0;
    reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[32];
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, reg_value, 0);
    if(0 != pll_config_multiple_write(shire_id,0x20,0x0))
    {
        return -1;
    }


    // Write PLL strobe register 0x38 to load previous configuration
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, REG_STROBE_UPDATE, 0);
    if(0 != pll_config_multiple_write(shire_id,0x38,0x0))
    {
        return -1;
    }

    /* Wait for the PLL to lock within a given timeout */
    WAIT_PLL_LOCK(shire_id)

}

int lvdpll_get_dco_filter_value(uint8_t shire_id, uint16_t* dco_filter_value)
{
    // PLL_REG_INDEX_REG_LOCK_MONITOR_CONTROL[0]: sample_strobe
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, 0x1, 0);
    if(0 != pll_config_multiple_write(shire_id,0x19,0x0))
    {
        return -1;
    }

    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, 0x0, 0);
    if(0 != pll_config_multiple_write(shire_id,0x19,0x0))
    {
        return -1;
    }

    if(0 != lvdpll_register_read(shire_id,0x32,dco_filter_value))
    {
        return -1;
    }

    return 0;
}

int lvdpll_get_dco_frequency(uint8_t shire_id, uint32_t* dco_frequency)
{
    uint16_t freq_monitor_clock_count = 0;

    // PLL_REG_INDEX_REG_FREQ_MONITOR_CONTROL[0]: freq_monitor_enable
    // PLL_REG_INDEX_REG_FREQ_MONITOR_CONTROL[5:1]: freq_monitor_clkref_div_code
    // PLL_REG_INDEX_REG_FREQ_MONITOR_CONTROL[10:6]: freq_monitor_clk_div_code
    // PLL_REG_INDEX_REG_FREQ_MONITOR_CONTROL[15:11]: freq_monitor_clock_sel
    // PLL_REG_INDEX_REG_FREQ_MONITOR_CLK_REF_COUNT[15:0]: freq_monitor_clkref_count
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, 0x000A280B, 0);
    if(0 != pll_config_multiple_write(shire_id,0x23,0x1))
    {
        return -1;
    }

    // Write PLL strobe register 0x38 to load previous configuration
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, REG_STROBE_UPDATE, 0);
    if(0 != pll_config_multiple_write(shire_id,0x38,0x0))
    {
        return -1;
    }

    //usdelay(30);
    for (int i=0; i<10000; i++)
    {
        __asm volatile ( " nop " );
    }

    // Read freq monitor clock count
    if(0 != lvdpll_register_read(shire_id,0x25,&freq_monitor_clock_count))
    {
        return -1;
    }

    // PLL_REG_INDEX_REG_FREQ_MONITOR_CONTROL[0]: freq_monitor_enable
    // PLL_REG_INDEX_REG_FREQ_MONITOR_CONTROL[5:1]: freq_monitor_clkref_div_code
    // PLL_REG_INDEX_REG_FREQ_MONITOR_CONTROL[10:6]: freq_monitor_clk_div_code
    // PLL_REG_INDEX_REG_FREQ_MONITOR_CONTROL[15:11]: freq_monitor_clock_sel
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, 0x2800, 0);
    if(0 != pll_config_multiple_write(shire_id,0x23,0x0))
    {
        return -1;
    }

    // Write PLL strobe register 0x38 to load previous configuration
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, REG_STROBE_UPDATE, 0);
    if(0 != pll_config_multiple_write(shire_id,0x38,0x0))
    {
        return -1;
    }

    *dco_frequency = (uint32_t)((freq_monitor_clock_count * 100) / 320);

    return 0;
}

int lvdpll_ldo_update(uint8_t shire_id, LVDPLL_LDO_UPDATE_t ldo_update)
{
    uint16_t ldo_reg;

    // Read ldo reg
    if(0 != lvdpll_register_read(shire_id,0x18,&ldo_reg))
    {
        return -1;
    }

    ldo_reg &= (uint16_t)(~(0x3F)); // Clear Trim and LDO bypass
    if (ldo_update == LVDPLL_LDO_DISABLE || ldo_update == LVDPLL_LDO_KICK)
    {
        ldo_reg |= 0x2;     // Turn off LDO
    }
    else
    {
        if (ldo_update == LVDPLL_LDO_BYPASS)
        {
            ldo_reg |= 0x1;     // Bypass LDO
        }
    }

    ldo_reg |= 0x34;    // Set LDO Trim 13

    // Write ldo reg
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, ldo_reg, 0);
    if(0 != pll_config_multiple_write(shire_id,0x18,0x0))
    {
        return -1;
    }

    // Write PLL strobe register 0x38 to load previous configuration
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, REG_STROBE_UPDATE, 0);
    if(0 != pll_config_multiple_write(shire_id,0x38,0x0))
    {
        return -1;
    }

    //usdelay(30);

    if (ldo_update == LVDPLL_LDO_KICK)
    {
        ldo_reg &= (uint16_t)(~(0x2));  // Turn on LDO
        // Write ldo reg
        write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, ldo_reg, 0);
        if(0 != pll_config_multiple_write(shire_id,0x18,0x0))
        {
            return -1;
        }

        // Write PLL strobe register 0x38 to load previous configuration
        write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, REG_STROBE_UPDATE, 0);
        if(0 != pll_config_multiple_write(shire_id,0x38,0x0))
        {
            return -1;
        }
    }

    /* Wait for the PLL to lock within a given timeout */
    WAIT_PLL_LOCK(shire_id)

    return 0;
}

int lvdpll_increase_threshold(uint8_t shire_id, uint32_t threshold_multiplier)
{
    uint16_t threshold_reg;

    // Read threshold reg
    if(0 != lvdpll_register_read(shire_id,0x0B,&threshold_reg))
    {
        return -1;
    }

    if ((uint32_t)(threshold_reg * threshold_multiplier) > 0x0000FFFF)
    {
        threshold_reg = (uint16_t)(0xFFFF);
    }
    else
    {
        threshold_reg = (uint16_t)(threshold_reg * threshold_multiplier);
    }

    // Write ldo reg
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, threshold_reg, 0);
    if(0 != pll_config_multiple_write(shire_id,0x0B,0x0))
    {
        return -1;
    }

    // Write PLL strobe register 0x38 to load previous configuration
    write_esr_new(PP_MACHINE, shire_id, REGION_OTHER, ESR_OTHER_SUBREGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, REG_STROBE_UPDATE, 0);
    if(0 != pll_config_multiple_write(shire_id,0x38,0x0))
    {
        return -1;
    }

    /* Wait for the PLL to lock within a given timeout */
    WAIT_PLL_LOCK(shire_id)

    return 0;
}