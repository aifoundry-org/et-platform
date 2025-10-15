/* Copyright (c) 2025 Ainekko, Co. */
/* SPDX-License-Identifier: Apache-2.0 */
/*                                                                         */
// From commit 5c81234d3c87419bf8724d27893dec5e86b9ff9c in esperanto-soc repository
#ifndef MINION_LVDPLL_PROGRAM_H
#define MINION_LVDPLL_PROGRAM_H


#include <stdbool.h>
#include "esr.h"
#include "hwinc/esr_region.h"
#include "hwinc/etsoc_shire_other_esr.h"
#include "hwinc/lvdpll_defines.h"
#include "hwinc/lvdpll_modes_config.h"
#include "hwinc/dvfs_lvdpll_modes_config.h"

#define NOP   __asm__ __volatile__ ("nop\n");

int update_minion_pll_freq_full(LVDPLL_MODE_e mode, uint64_t minion_shire_mask, uint8_t num_of_shires);
int update_minion_pll_freq_quick(LVDPLL_MODE_e mode, uint64_t minion_shire_mask, uint8_t num_of_shires);
int turn_off_pll_auto_normalization(uint64_t minion_shire_mask, uint8_t num_of_shires);
int update_self_pll_freq_full(LVDPLL_MODE_e mode);
int update_self_pll_freq_quick(LVDPLL_MODE_e mode);
LVDPLL_MODE_e freq_to_mode(uint16_t freq, uint8_t lvdpll_strap);
uint16_t mode_to_freq(LVDPLL_MODE_e mode, uint8_t lvdpll_strap);

inline void __attribute__ ((always_inline)) wait_for_busy(uint32_t num_reg)
{
    for(uint32_t i=1; i<=num_reg+1; i++)
       for(uint32_t j=0; j<LVDPLL_BUSY_TIME_WAIT; j++) NOP;
}

inline void __attribute__((always_inline)) pll_broadcast_req(esr_protection_t pp, esr_reg_t region, uint32_t address, uint64_t shire_mask, uint64_t value)
{
    volatile uint64_t * BC_ESR_ADDR = (uint64_t *) R_SHIRE_U_UC_Broadcast_Data;
    *BC_ESR_ADDR = value;
    volatile uint64_t * BC_REQ_ADDR = (uint64_t *) (R_SHIRE_U_UC_Broadcast_Control | ((uint64_t)(pp) << 30));
    *BC_REQ_ADDR =  (((uint64_t)(region & 0x03)     << 57) |
                    ((uint64_t)((0x8000 | (address >> 3)) & 0x1ffff) << 40) |
                    (shire_mask & 0xffffffffff));
}

static void __attribute__ ((noinline)) __attribute__ ((aligned (64))) wait_for_lock_loop(void)
{
    for(int i=0; i<LVDPLL_LOCK_TIME_WAIT; i++) NOP;
}

inline int __attribute__((always_inline)) pll_broadcast_register_write(uint32_t start_range, uint32_t num_reg,
                                                uint64_t minion_shire_mask, uint8_t num_of_shires, bool poll_for_busy)
{
    uint64_t reg_value;
    uint64_t minion_shire_mask_tmp;

    // Start PLL auto config
    reg_value = ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_PCLK_SEL_SET(2)
              | ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_REG_NUM_SET(num_reg)
              | ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_REG_FIRST_SET(start_range)
              | ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_WRITE_SET(0x1)
              | ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_RUN_SET(0x1)
              | ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_ENABLE_SET(0x1);
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_ADDRESS, minion_shire_mask, reg_value);

    if(poll_for_busy) {
        minion_shire_mask_tmp = minion_shire_mask;
        for ( uint8_t i = 0; i < num_of_shires; i++){
            if (minion_shire_mask_tmp & 1) {
                WAIT_BUSY(i)
            }
            minion_shire_mask_tmp >>=1;
        }
    }
    else {
        wait_for_busy(num_reg);
    }

    // Stop the PLL auto config, to be removed for next gen after RTLMIN-6209 is resolved
    reg_value = ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_PCLK_SEL_SET(2)
              | ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_REG_NUM_SET(num_reg)
              | ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_REG_FIRST_SET(start_range)
              | ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_ENABLE_SET(0x1);
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_ADDRESS, minion_shire_mask, reg_value);

    return 0;
}

inline static int __attribute__((always_inline)) wait_for_shire_pll_lock(uint8_t shire_id)
{
     WAIT_PLL_LOCK(shire_id)
}

inline static int __attribute__((always_inline)) wait_for_pll_lock(uint64_t minion_shire_mask, uint8_t num_of_shires, bool poll_for_lock)
{
     uint64_t minion_shire_mask_tmp;

     if(poll_for_lock) {
         minion_shire_mask_tmp = minion_shire_mask;
         for ( uint8_t i = 0; i < num_of_shires; i++){
             if (minion_shire_mask_tmp & 1) {
                 if(0 != wait_for_shire_pll_lock(i))
                 {
                     return -1;
                 }
             }
             minion_shire_mask_tmp >>=1;
         }
     }
     else {
         wait_for_lock_loop();
     }

     return 0;
}

inline static int __attribute__((always_inline)) dvfs_fcw_update_minion_pll_freq(uint16_t target_freq, uint64_t minion_shire_mask, uint8_t num_of_shires,
                                                        uint16_t current_freq, uint16_t norm_freq, uint16_t ref_clock_freq, bool poll_for_lock)
{

    //Reduce number of broadcast wites once RTLMIN-6209 is resolved

    uint64_t reg_value;
    uint16_t current_postdiv0;
    uint16_t new_postdiv0;
    uint16_t new_fcw_int;
    uint16_t new_fcw_frac;

    // Switch Mux to use Step Clock
    pll_broadcast_req(PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_CTRL_CLOCKMUX_ADDRESS, minion_shire_mask, SELECT_STEP_CLOCK);

    // Enable PLL auto config
    reg_value = ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_PCLK_SEL_SET(2)
              | ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_REG_NUM_SET(0x0)
              | ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_REG_FIRST_SET(0x2)
              | ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_ENABLE_SET(0x1);
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_ADDRESS, minion_shire_mask, reg_value);

    // Calculate postdivider according to OSC frequency at which normalization is done
    current_postdiv0 = (uint16_t)(norm_freq/current_freq);
    new_postdiv0 = (uint16_t)(norm_freq/target_freq);

    // Calculate new fcw_int and fcw_frac
    new_fcw_int = (uint16_t)((target_freq * new_postdiv0) / ref_clock_freq);
    new_fcw_frac = (uint16_t)(((65536 * ( target_freq * new_postdiv0)) - (65536 * (ref_clock_freq * new_fcw_int ))) / ref_clock_freq);

    // Program new values
    reg_value = (uint64_t)(new_fcw_frac << 16) | new_fcw_int;
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, minion_shire_mask, reg_value);
    if(0 != pll_broadcast_register_write(FCW_INT_OFFSET,0x1,minion_shire_mask,num_of_shires,DVFS_POLL_FOR_BUSY))
    {
        return -1;
    }

    if(new_postdiv0 != current_postdiv0) {
        reg_value = new_postdiv0;
        pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, minion_shire_mask, reg_value);
        if(0 != pll_broadcast_register_write(POSTDIV0_OFFSET,0x0,minion_shire_mask,num_of_shires,DVFS_POLL_FOR_BUSY))
        {
            return -1;
        }
    }

    // Write PLL reg strobe update register 0x38 to load previous configuration
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, minion_shire_mask, REG_STROBE_UPDATE);
    if(0 != pll_broadcast_register_write(REG_STROBE_UPDATE_OFFSET,0x0,minion_shire_mask,num_of_shires,DVFS_POLL_FOR_BUSY))
    {
        return -1;
    }

    __asm__ __volatile__ ("fence\n");

    //Dummy load in order fence to work before NOP loop
    __asm__ __volatile__ ("csrr x0,hpmcounter3\n");

    // Wait until PLL is locked to change clock mux
    if(0 != wait_for_pll_lock(minion_shire_mask, num_of_shires, poll_for_lock))
    {
        return -1;
    }

    // Switch Mux to use PLL output
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_CTRL_CLOCKMUX_ADDRESS, minion_shire_mask, SELECT_PLL_CLOCK_0);

    // Disable the PLL auto config
    reg_value = ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_PCLK_SEL_SET(2);
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_ADDRESS, minion_shire_mask, reg_value);

    return 0;

}

inline static int __attribute__((always_inline)) dvfs_update_minion_pll_mode(LVDPLL_MODE_e mode, uint64_t minion_shire_mask, uint8_t num_of_shires, bool poll_for_lock)
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
    reg_value = reg_value | et_dvfs_lvdpll_settings[(mode-1)].quick_values[1];
    reg_value = (reg_value << 16) | et_dvfs_lvdpll_settings[(mode-1)].quick_values[0];
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, minion_shire_mask, reg_value);
    if(0 != pll_broadcast_register_write(0x02,0x1,minion_shire_mask,num_of_shires,DVFS_POLL_FOR_BUSY))
    {
        return -1;
    }

    // Writing PLL register:
    //  0x0b  -  lock_threshold
    reg_value = 0;
    reg_value = reg_value | et_dvfs_lvdpll_settings[(mode-1)].quick_values[2];
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, minion_shire_mask, reg_value);
    if(0 != pll_broadcast_register_write(0x0b,0x0,minion_shire_mask,num_of_shires,DVFS_POLL_FOR_BUSY))
    {
        return -1;
    }

    // Writing PLL register:
    //  0x0e  -  postdiv0
    reg_value = 0;
    reg_value = reg_value | et_dvfs_lvdpll_settings[(mode-1)].quick_values[3];
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, minion_shire_mask, reg_value);
    if(0 != pll_broadcast_register_write(0x0e,0x0,minion_shire_mask,num_of_shires,DVFS_POLL_FOR_BUSY))
    {
        return -1;
    }

    // Writing PLL register:
    //  0x20  -  osc_cap_trim
    reg_value = 0;
    reg_value = reg_value | et_dvfs_lvdpll_settings[(mode-1)].quick_values[7];
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, minion_shire_mask, reg_value);
    if(0 != pll_broadcast_register_write(0x20,0x0,minion_shire_mask,num_of_shires,DVFS_POLL_FOR_BUSY))
    {
        return -1;
    }


    // Write PLL strobe register 0x38 to load previous configuration
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, minion_shire_mask, REG_STROBE_UPDATE);
    if(0 != pll_broadcast_register_write(REG_STROBE_UPDATE_OFFSET,0x0,minion_shire_mask,num_of_shires,DVFS_POLL_FOR_BUSY))
    {
        return -1;
    }


    // Wait until PLL is locked to change clock mux
    if(0 != wait_for_pll_lock(minion_shire_mask, num_of_shires, poll_for_lock))
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

inline static int __attribute__((always_inline)) dvfs_update_minion_pll_mode_with_normalization_on(LVDPLL_MODE_e mode, uint64_t minion_shire_mask, uint8_t num_of_shires, bool poll_for_lock)
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

    // Writing PLL register 0x0, enabling auto-normalization
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, minion_shire_mask, AUTO_NORMALIZATION_ON);
    if(0 != pll_broadcast_register_write(0x00,0x0,minion_shire_mask,num_of_shires,DVFS_POLL_FOR_BUSY))
    {
        return -1;
    }

    // Writing PLL registers:
    //  0x02  -  fcw_int
    //  0x03  -  fcw_frac
    reg_value = 0;
    reg_value = reg_value | et_dvfs_lvdpll_settings[(mode-1)].quick_values[1];
    reg_value = (reg_value << 16) | et_dvfs_lvdpll_settings[(mode-1)].quick_values[0];
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, minion_shire_mask, reg_value);
    if(0 != pll_broadcast_register_write(0x02,0x1,minion_shire_mask,num_of_shires,DVFS_POLL_FOR_BUSY))
    {
        return -1;
    }

    // Writing PLL register:
    //  0x0b  -  lock_threshold
    reg_value = 0;
    reg_value = reg_value | et_dvfs_lvdpll_settings[(mode-1)].quick_values[2];
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, minion_shire_mask, reg_value);
    if(0 != pll_broadcast_register_write(0x0b,0x0,minion_shire_mask,num_of_shires,DVFS_POLL_FOR_BUSY))
    {
        return -1;
    }

    // Writing PLL register:
    //  0x0e  -  postdiv0
    reg_value = 0;
    reg_value = reg_value | et_dvfs_lvdpll_settings[(mode-1)].quick_values[3];
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, minion_shire_mask, reg_value);
    if(0 != pll_broadcast_register_write(0x0e,0x0,minion_shire_mask,num_of_shires,DVFS_POLL_FOR_BUSY))
    {
        return -1;
    }

    // Writing PLL register:
    //  0x20  -  osc_cap_trim
    reg_value = 0;
    reg_value = reg_value | et_dvfs_lvdpll_settings[(mode-1)].quick_values[7];
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, minion_shire_mask, reg_value);
    if(0 != pll_broadcast_register_write(0x20,0x0,minion_shire_mask,num_of_shires,DVFS_POLL_FOR_BUSY))
    {
        return -1;
    }


    // Write PLL strobe register 0x38 to load previous configuration
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, minion_shire_mask, REG_STROBE_UPDATE);
    if(0 != pll_broadcast_register_write(REG_STROBE_UPDATE_OFFSET,0x0,minion_shire_mask,num_of_shires,DVFS_POLL_FOR_BUSY))
    {
        return -1;
    }


    // Wait until PLL is locked to change clock mux
    if(0 != wait_for_pll_lock(minion_shire_mask, num_of_shires, poll_for_lock))
    {
        return -1;
    }

    // Select PLL[0] output. Bits[2:0]=3'b100. Bit 3 to '1' to go with DLL output
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_CTRL_CLOCKMUX_ADDRESS, minion_shire_mask, SELECT_PLL_CLOCK_0);

    // Writing PLL register 0x0, disabling auto-normalization
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_CONFIG_DATA_0_ADDRESS, minion_shire_mask, AUTO_NORMALIZATION_OFF);
    if(0 != pll_broadcast_register_write(0x00,0x0,minion_shire_mask,num_of_shires,DVFS_POLL_FOR_BUSY))
    {
        return -1;
    }

    // Stop the PLL auto config
    reg_value = ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_PCLK_SEL_SET(2);
    pll_broadcast_req (PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_PLL_AUTO_CONFIG_ADDRESS, minion_shire_mask, reg_value);

    return 0;

}


#endif
