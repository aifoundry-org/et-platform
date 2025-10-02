/*-------------------------------------------------------------------------
* Copyright (C) 2018, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies or
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

#include "et_test_common.h"
#include "minion_esr_defines.h"
#include "pll_config.h"
#include "lvdpll_defines.h"
#include "movellus_lvdpll_modes_config.h"

typedef struct
{
   uint32_t        dvfs_minion_shire_mask;
   uint16_t        dvfs_current_freq;
   uint8_t         dvfs_num_of_shires;
}dvfs_lvdpll_config_t;

static dvfs_lvdpll_config_t dvfs_lvdpll_config = {
                                                   0xffffffff,        //dvfs_minion_shire_mask
                                                   750,               //dvfs_current_mode
                                                   32                 //dvfs_num_of_shires
                                                 };

inline void __attribute__((always_inline)) update_dvfs_lvdpll_config(
                                                                     uint32_t dvfs_minion_shire_mask,
                                                                     uint16_t dvfs_current_freq,
                                                                     uint8_t dvfs_num_of_shires)
{
   dvfs_lvdpll_config = {
                           dvfs_minion_shire_mask,
                           dvfs_current_freq,
                           dvfs_num_of_shires
   };
}

//void __attribute__ ((noinline)) __attribute__ ((aligned (64))) wait_for_busy(uint32_t num_reg)
//{
//   for(int i=1; i<=num_reg+1; i++)
//      for(int j=0; j<LVDPLL_BUSY_TIME_WAIT; j++) NOP;
//}

static inline void __attribute__((always_inline)) pll_broadcast_register_write(uint32_t start_range, uint32_t num_reg, uint64_t minion_shire_mask, bool poll_for_busy)
{
  uint64_t reg_value;
  uint64_t minion_shire_mask_tmp;
  uint8_t  num_of_shires;
  
  // Start PLL auto config
  reg_value =  SHIRE_OTHER_PLL_AUTO_CONFIG_PCLK_SEL
            | (num_reg << SHIRE_OTHER_PLL_AUTO_CONFIG_REG_NUM_OFF)
            | (start_range << SHIRE_OTHER_PLL_AUTO_CONFIG_REG_FIRST_OFF)
            | (0x1 << SHIRE_OTHER_PLL_AUTO_CONFIG_WRITE_OFF)
            | (0x1 << SHIRE_OTHER_PLL_AUTO_CONFIG_RUN_OFF)
            | (0x1 << SHIRE_OTHER_PLL_AUTO_CONFIG_ENABLE_OFF);
  broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_PLL_AUTO_CONFIG, minion_shire_mask, reg_value);

  if(poll_for_busy) {
      minion_shire_mask_tmp = minion_shire_mask;
      num_of_shires = dvfs_lvdpll_config.dvfs_num_of_shires;
      for ( uint8_t i = 0; i < num_of_shires; i++){
         if (minion_shire_mask_tmp & 1) {
            while((read_esr(PP_MACHINE, i, REGION_OTHER, SHIRE_PLL_READ_DATA) & PLL_BUSY_MASK));
         }
         minion_shire_mask_tmp >>=1;
      }
   }
   //else {
   //   wait_for_busy(num_reg);
   //}
  
  // Stop the PLL auto config, to be removed for next gen after RTLMIN-6209 is resolved
  reg_value =  SHIRE_OTHER_PLL_AUTO_CONFIG_PCLK_SEL
            | (num_reg << SHIRE_OTHER_PLL_AUTO_CONFIG_REG_NUM_OFF)
            | (start_range << SHIRE_OTHER_PLL_AUTO_CONFIG_REG_FIRST_OFF)
            | (0x1 << SHIRE_OTHER_PLL_AUTO_CONFIG_ENABLE_OFF);
  broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_PLL_AUTO_CONFIG, minion_shire_mask, reg_value);
}

void __attribute__ ((noinline)) __attribute__ ((aligned (64))) wait_for_lock_loop()
{
   for(int i=0; i<LVDPLL_LOCK_TIME_WAIT; i++) NOP;
}

static inline void __attribute__((always_inline)) wait_for_pll_lock(bool poll_for_lock)
{
   uint64_t minion_shire_mask;
   uint32_t wait_time;
   uint8_t  num_of_shires;

   if(poll_for_lock) {
      minion_shire_mask = dvfs_lvdpll_config.dvfs_minion_shire_mask;
      num_of_shires = dvfs_lvdpll_config.dvfs_num_of_shires;
      for ( uint8_t i = 0; i < num_of_shires; i++){
         if (minion_shire_mask & 1) {
            while(!(read_esr(PP_MACHINE, i, REGION_OTHER, SHIRE_PLL_READ_DATA) & PLL_LOCK_MASK));
         }
         minion_shire_mask >>=1;
      }
   }
   else {
      wait_for_lock_loop();
   }
}

inline void __attribute__((always_inline)) update_minion_pll_freq(uint16_t target_freq, bool poll_for_lock)
{

   //Reduce number of broadcasr wites once RTLMIN-6209 is resolved

   uint64_t reg_value;
   uint32_t minion_shire_mask;
   uint16_t current_freq;
   uint16_t current_postdiv0;
   uint16_t new_postdiv0;
   uint16_t new_fcw_int;
   uint16_t new_fcw_frac;

   current_freq = dvfs_lvdpll_config.dvfs_current_freq;
   minion_shire_mask = dvfs_lvdpll_config.dvfs_minion_shire_mask;
   
   // Switch Mux to use Step Clock
   broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_CTRL_CLOCKMUX, minion_shire_mask, SELECT_STEP_CLOCK);

   // Enable PLL auto config
   reg_value =  SHIRE_OTHER_PLL_AUTO_CONFIG_PCLK_SEL
            | (0x0 << SHIRE_OTHER_PLL_AUTO_CONFIG_REG_NUM_OFF)
            | (0x02 << SHIRE_OTHER_PLL_AUTO_CONFIG_REG_FIRST_OFF)
            | (0x1 << SHIRE_OTHER_PLL_AUTO_CONFIG_ENABLE_OFF);
   broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_PLL_AUTO_CONFIG, minion_shire_mask, reg_value);

   // Calculate postdivider according to OSC frequency at which normalization is done
   current_postdiv0 = LVDPLL_OSC_NORM_FREQ/current_freq;
   new_postdiv0 = LVDPLL_OSC_NORM_FREQ/target_freq;

   // Calculate new fcw_int and fcw_frac
   new_fcw_int = (target_freq * new_postdiv0) / 100;
   new_fcw_frac = ((65536 * ( target_freq * new_postdiv0)) - (65536 * (100 * new_fcw_int ))) / 100;

   // Program new values
   reg_value = (new_fcw_frac << 16) | new_fcw_int;
   broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, minion_shire_mask, reg_value);
   pll_broadcast_register_write(FCW_INT_OFFSET,0x1,minion_shire_mask,false);

   if(new_postdiv0 != current_postdiv0) {
      reg_value = new_postdiv0;
      broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, minion_shire_mask, reg_value);
      pll_broadcast_register_write(POSTDIV0_OFFSET,0x0,minion_shire_mask,false);
   }   
   
   // Write PLL reg strobe update register 0x38 to load previous configuration
   broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, minion_shire_mask, REG_STROBE_UPDATE);
   pll_broadcast_register_write(REG_STROBE_UPDATE_OFFSET,0x0,minion_shire_mask,false);
  
   __asm__ __volatile__ ("fence\n");

   //Dummy load in order fence to work before NOP loop
   __asm__ __volatile__ ("csrr x0,hpmcounter3\n");
   
   // Wait until PLL is locked to change clock mux
   wait_for_pll_lock(poll_for_lock);
   
   // Switch Mux to use PLL output
   broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_CTRL_CLOCKMUX, minion_shire_mask, SELECT_PLL_CLOCK_0);

   // Disable the PLL auto config
   reg_value =  SHIRE_OTHER_PLL_AUTO_CONFIG_PCLK_SEL;
   broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_PLL_AUTO_CONFIG, minion_shire_mask, reg_value);

   // Update current mode
   dvfs_lvdpll_config.dvfs_current_freq = target_freq;
 
}

