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

#include "dvfs_lvdpll_prog.h"


inline void __attribute__((always_inline)) update_minion_pll_freq_full(lvdpllMode mode, uint64_t minion_shire_mask)
{
   uint64_t reg_value;
   
   // Select 650MHz from step_clock, Bits[2:0] = 3'b011. Bit 3 to '1' to go with DLL output
   broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_CTRL_CLOCKMUX, minion_shire_mask, SELECT_STEP_CLOCK);

   // Enable PLL auto config
   reg_value =  SHIRE_OTHER_PLL_AUTO_CONFIG_PCLK_SEL
            | (0x0 << SHIRE_OTHER_PLL_AUTO_CONFIG_REG_NUM_OFF)
            | (0x02 << SHIRE_OTHER_PLL_AUTO_CONFIG_REG_FIRST_OFF)
            | (0x1 << SHIRE_OTHER_PLL_AUTO_CONFIG_ENABLE_OFF);
   broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_PLL_AUTO_CONFIG, minion_shire_mask, reg_value);

   // Write PLL registers 0x00 to 0x0f. PLL registers can be downloaded in chunks of 16 registers
   reg_value = 0;
   reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[3];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[2];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[1];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[0];
   broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, minion_shire_mask, reg_value);
   reg_value = 0;
   reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[7];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[6];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[5];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[4];
   broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_1, minion_shire_mask, reg_value);
   reg_value = 0;
   reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[11];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[10];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[9];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[8];
   broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_2, minion_shire_mask, reg_value);
   reg_value = 0;
   reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[15];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[14];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[13];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[12];
   broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_3, minion_shire_mask, reg_value);
   pll_broadcast_register_write(0x00,0xf,minion_shire_mask,true);
   
   // Write PLL registers 0x10 to 0x1a.
   reg_value = 0;
   reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[19];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[18];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[17];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[16];          
   broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, minion_shire_mask, reg_value);
   reg_value = 0;
   reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[23];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[22];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[21];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[20];
   broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_1, minion_shire_mask, reg_value);
   reg_value = 0;
   reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[26];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[25];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[24];
   broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_2, minion_shire_mask, reg_value);
   pll_broadcast_register_write(0x10,0xa,minion_shire_mask,true);

   // Write PLL registers 0x20 to 0x24.         
   reg_value = 0;
   reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[30];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[29];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[28];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[27];
   broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, minion_shire_mask, reg_value);
   reg_value = 0;
   reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[31];
   broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_1, minion_shire_mask, reg_value);
   pll_broadcast_register_write(0x20,0x4,minion_shire_mask,true);

   // Write PLL registers 0x27 to 0x28.(DATA_1 and DATA_2 and DATA_3 not changed)
   reg_value = 0;
   reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[33];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[32];
   broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, minion_shire_mask, reg_value);
   pll_broadcast_register_write(0x27,0x1,minion_shire_mask,true);

   // Write PLL registers 0x38 to 0x39.(DATA_1 and DATA_2 and DATA_3 not changed)
   reg_value = 0;
   reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[35];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[34];
   broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, minion_shire_mask, reg_value);
   pll_broadcast_register_write(0x38,0x1,minion_shire_mask,true);
   
   // Write PLL strobe register 0x38 to load previous configuration
   broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, minion_shire_mask, REG_STROBE_UPDATE);
   pll_broadcast_register_write(0x38,0x0,minion_shire_mask,true);
  
   
   // Wait until PLL is locked to change clock mux
   wait_for_pll_lock(1);
   
   // Select PLL[0] output. Bits[2:0]=3'b100. Bit 3 to '1' to go with DLL output
   broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_CTRL_CLOCKMUX, minion_shire_mask, SELECT_PLL_CLOCK_0);

   // Disable the PLL auto config
   reg_value =  SHIRE_OTHER_PLL_AUTO_CONFIG_PCLK_SEL;
   broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_PLL_AUTO_CONFIG, minion_shire_mask, reg_value);

   // Update current mode
   dvfs_lvdpll_config.dvfs_current_freq = gs_lvdpll_settings[(mode-1)].output_frequency / 1000000;

}

inline void __attribute__((always_inline)) update_minion_pll_freq_quick(lvdpllMode mode, uint64_t minion_shire_mask)
{
   uint64_t reg_value;
   uint8_t  reg_num = 0;
   
   // Select 1 GHz from step_clock, Bits[2:0] = 3'b011. Bit 3 to '1' to go with DLL output
   broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_CTRL_CLOCKMUX, minion_shire_mask, SELECT_STEP_CLOCK);

   // Enable PLL auto config
   reg_value =  SHIRE_OTHER_PLL_AUTO_CONFIG_PCLK_SEL
            | (0x0 << SHIRE_OTHER_PLL_AUTO_CONFIG_REG_NUM_OFF)
            | (0x02 << SHIRE_OTHER_PLL_AUTO_CONFIG_REG_FIRST_OFF)
            | (0x1 << SHIRE_OTHER_PLL_AUTO_CONFIG_ENABLE_OFF);
   broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_PLL_AUTO_CONFIG, minion_shire_mask, reg_value);

   reg_value = 0;
   reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[3];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[2];
   broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, minion_shire_mask, reg_value);
   pll_broadcast_register_write(0x02,0x1,minion_shire_mask,true);
   
   reg_value = 0;
   reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[11];
   broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, minion_shire_mask, reg_value);
   pll_broadcast_register_write(0x0b,0x0,minion_shire_mask,true);

   reg_value = 0;
   reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[14];
   broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, minion_shire_mask, reg_value);
   pll_broadcast_register_write(0x0e,0x0,minion_shire_mask,true);

   reg_value = 0;
   reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[32];
   broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, minion_shire_mask, reg_value);
   pll_broadcast_register_write(0x20,0x0,minion_shire_mask,true);
   
   
   // Write PLL strobe register 0x38 to load previous configuration
   broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, minion_shire_mask, REG_STROBE_UPDATE);
   pll_broadcast_register_write(0x38,0x0,minion_shire_mask,true);
  
   
   // Wait until PLL is locked to change clock mux
   wait_for_pll_lock(1);
   
   // Select PLL[0] output. Bits[2:0]=3'b100. Bit 3 to '1' to go with DLL output
   broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_CTRL_CLOCKMUX, minion_shire_mask, SELECT_PLL_CLOCK_0);

   // Stop the PLL auto config
   reg_value =  SHIRE_OTHER_PLL_AUTO_CONFIG_PCLK_SEL;
   broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_PLL_AUTO_CONFIG, minion_shire_mask, reg_value);

   // Update current mode
   dvfs_lvdpll_config.dvfs_current_freq = gs_lvdpll_settings[(mode-1)].output_frequency / 1000000;
 
}

inline void __attribute__((always_inline)) pll_normalize_and_turn_of_normalization()
{
   uint64_t reg_value;
   uint32_t minion_shire_mask;

   minion_shire_mask = dvfs_lvdpll_config.dvfs_minion_shire_mask;

   update_minion_pll_freq_full(LVDPLL_NORM_MODE, minion_shire_mask);
   // Enable PLL auto config
   reg_value =  SHIRE_OTHER_PLL_AUTO_CONFIG_PCLK_SEL
                 | (0x0 << SHIRE_OTHER_PLL_AUTO_CONFIG_REG_NUM_OFF)
                 | (0x02 << SHIRE_OTHER_PLL_AUTO_CONFIG_REG_FIRST_OFF)
                 | (0x1 << SHIRE_OTHER_PLL_AUTO_CONFIG_ENABLE_OFF);
   broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_PLL_AUTO_CONFIG, minion_shire_mask, reg_value);
   // Turn of auto-normalization 
   reg_value = 0x81b8;      
   broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, minion_shire_mask, reg_value);
   pll_broadcast_register_write(0x00,0x0,minion_shire_mask,true);

   // Disable the PLL auto config
   reg_value =  SHIRE_OTHER_PLL_AUTO_CONFIG_PCLK_SEL;
   broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_PLL_AUTO_CONFIG, minion_shire_mask, reg_value);
}

inline void __attribute__((always_inline)) config_compute_shire_dlls(uint64_t minion_shire_mask)
{
   uint64_t reg_value;
   uint8_t  num_of_shires;

   // Auto-config register set dll_enable and get reset deasserted of the DLL
   reg_value =  SHIRE_OTHER_DLL_AUTO_CONFIG_PCLK_SEL
             | (0x1 << SHIRE_OTHER_DLL_AUTO_CONFIG_DLL_EN_OFF);
   broadcast_req (PP_MACHINE, REGION_OTHER, SHIRE_OTHER_DLL_AUTO_CONFIG, minion_shire_mask, reg_value);

   num_of_shires = dvfs_lvdpll_config.dvfs_num_of_shires;
   for ( uint8_t i = 0; i < num_of_shires; i++){
      if (minion_shire_mask & 1) {
         while(!(read_esr(PP_MACHINE, i, REGION_OTHER, SHIRE_DLL_READ_DATA) & DLL_LOCK_MASK));
      }
      minion_shire_mask >>=1;
   }

}

inline void __attribute__((always_inline)) update_self_pll_freq_full(lvdpllMode mode)
{
   uint64_t reg_value;
   
   // Select 1 GHz from step_clock, Bits[2:0] = 3'b011. Bit 3 to '1' to go with DLL output
   write_esr(PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, SHIRE_OTHER_CTRL_CLOCKMUX, SELECT_STEP_CLOCK);

   // Write PLL registers 0x00 to 0x0f. PLL registers can be downloaded in chunks of 16 registers
   reg_value = 0;
   reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[3];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[2];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[1];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[0];
   write_esr(PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, reg_value);
   reg_value = 0;
   reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[7];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[6];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[5];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[4];
   write_esr(PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_1, reg_value); 
   reg_value = 0;
   reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[11];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[10];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[9];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[8];
   write_esr(PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_2, reg_value); 
   reg_value = 0;
   reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[15];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[14];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[13];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[12];
   write_esr(PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_3, reg_value); 
   pll_config_multiple_write(LOCAL_SHIRE_ID,0x00,0xf);
   
   // Write PLL registers 0x10 to 0x1a.
   reg_value = 0;
   reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[19];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[18];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[17];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[16];          
   write_esr(PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, reg_value);
   reg_value = 0;
   reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[23];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[22];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[21];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[20];
   write_esr(PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_1, reg_value);
   reg_value = 0;
   reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[26];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[25];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[24];
   write_esr(PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_2, reg_value);
   pll_config_multiple_write(LOCAL_SHIRE_ID,0x10,0xa);

   // Write PLL registers 0x20 to 0x24.         
   reg_value = 0;
   reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[30];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[29];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[28];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[27];
   write_esr(PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, reg_value);
   reg_value = 0;
   reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[31];
   write_esr(PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_1, reg_value);
   pll_config_multiple_write(LOCAL_SHIRE_ID,0x20,0x4);

   // Write PLL registers 0x27 to 0x28.(DATA_1 and DATA_2 and DATA_3 not changed)
   reg_value = 0;
   reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[33];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[32];
   write_esr(PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, reg_value);
   pll_config_multiple_write(LOCAL_SHIRE_ID,0x27,0x1);

   // Write PLL registers 0x38 to 0x39.(DATA_1 and DATA_2 and DATA_3 not changed)
   reg_value = 0;
   reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[35];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[34];
   write_esr(PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, reg_value);
   pll_config_multiple_write(LOCAL_SHIRE_ID,0x38,0x1);
   
   // Write PLL strobe register 0x38 to load previous configuration
   write_esr(PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, REG_STROBE_UPDATE);
   pll_config_multiple_write(LOCAL_SHIRE_ID,0x38,0x0);
  
   
   // Wait until PLL is locked to change clock mux
   while(!(read_esr(PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, SHIRE_PLL_READ_DATA) & PLL_LOCK_MASK));
   
   // Select PLL[0] output. Bits[2:0]=3'b100. Bit 3 to '1' to go with DLL output
   write_esr(PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, SHIRE_OTHER_CTRL_CLOCKMUX, SELECT_PLL_CLOCK_0);

}

inline void __attribute__((always_inline)) update_self_pll_freq_quick(lvdpllMode mode)
{
   uint64_t reg_value;
   uint8_t  reg_num = 0;
   
   // Select 1 GHz from step_clock, Bits[2:0] = 3'b011. Bit 3 to '1' to go with DLL output
   write_esr (PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, SHIRE_OTHER_CTRL_CLOCKMUX, SELECT_STEP_CLOCK);

   reg_value = 0;
   reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[3];
   reg_value = (reg_value << 16) | gs_lvdpll_settings[(mode-1)].values[2];
   write_esr (PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, reg_value);
   pll_config_multiple_write(LOCAL_SHIRE_ID,0x02,0x1);
   
   reg_value = 0;
   reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[11];
   write_esr (PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, reg_value);
   pll_config_multiple_write(LOCAL_SHIRE_ID,0x0b,0x0);

   reg_value = 0;
   reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[14];
   write_esr (PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, reg_value);
   pll_config_multiple_write(LOCAL_SHIRE_ID,0x0e,0x0);

   reg_value = 0;
   reg_value = reg_value | gs_lvdpll_settings[(mode-1)].values[32];
   write_esr (PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, reg_value);
   pll_config_multiple_write(LOCAL_SHIRE_ID,0x20,0x0);
   
   
   // Write PLL strobe register 0x38 to load previous configuration
   write_esr (PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, SHIRE_OTHER_PLL_CONFIG_DATA_0, REG_STROBE_UPDATE);
   pll_config_multiple_write(LOCAL_SHIRE_ID,0x38,0x0);
  
   
   // Wait until PLL is locked to change clock mux
   while(!(read_esr(PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, SHIRE_PLL_READ_DATA) & PLL_LOCK_MASK));
   
   // Select PLL[0] output. Bits[2:0]=3'b100. Bit 3 to '1' to go with DLL output
   write_esr (PP_MACHINE, LOCAL_SHIRE_ID, REGION_OTHER, SHIRE_OTHER_CTRL_CLOCKMUX, SELECT_PLL_CLOCK_0);
 
}

static inline void __attribute__((always_inline)) enable_minion_threads(uint64_t minion_shire_mask, uint64_t thread_mask)
{
   uint8_t  num_of_shires;
   uint8_t  enable_neigh;
   uint64_t reg_value;
   uint64_t thread_disable_mask;
   uint64_t thread0_mask = 0;
   uint64_t thread1_mask = 0;

   thread_disable_mask = thread_mask ^ 0xfffffffffffffffful;

   num_of_shires = dvfs_lvdpll_config.dvfs_num_of_shires;
   for ( uint8_t i = 0; i < num_of_shires; i++){
      if (minion_shire_mask & 1) {
         if(thread_mask && 0x0ffff) enable_neigh = 1;
         if((thread_mask >> 16) && 0x0ffff) enable_neigh = enable_neigh + 2;
         if((thread_mask >> 32) && 0x0ffff) enable_neigh = enable_neigh + 4;
         if((thread_mask >> 48) && 0x0ffff) enable_neigh = enable_neigh + 8;

         reg_value = (0x01000ul | (enable_neigh << 8)) + i;

         //enable neighs
         write_esr(PP_MACHINE, i, REGION_OTHER, SHIRE_OTHER_CONFIG, reg_value);

         //enable threads
         for(int j = 0; j < 64; j++) {
            if(j%2 == 0) thread0_mask |= thread_disable_mask & (1 << (j/2));
            else thread1_mask |= thread_disable_mask & (1 << (j/2));
         }
         write_esr(PP_MACHINE, i, REGION_OTHER, SHIRE_OTHER_THREAD0_DISABLE, thread0_mask);
         write_esr(PP_MACHINE, i, REGION_OTHER, SHIRE_OTHER_THREAD1_DISABLE, thread1_mask);

      }
      minion_shire_mask >>=1;
   }

}
