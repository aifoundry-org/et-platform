/***********************************************************************
*
* Copyright (C) 2021 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/

// exporting functions from ddr_init.c

// From DDR PHY training application note.  These is used in config_debug_level
#define PHY_MSG_VERBOSITY_MORE_DETAILED_DEBUG   0x04
#define PHY_MSG_VERBOSITY_DETAILED_DEBUG        0x05
#define PHY_MSG_VERBOSITY_COARSE_DEBUG          0x0a
#define PHY_MSG_VERBOSITY_STAGE_COMPLETION      0xc8
#define PHY_MSG_VERBOSITY_ASSERTION_MESSAGES    0xc9
#define PHY_MSG_VERBOSITY_FIRMWARE_COMPLETE     0xff

// Used by hardware code for mem_config in ms_ddr_phy_xx_train_from_file()
#define DDR_1067MHZ       0
#define DDR_933MHZ        1
#define DDR_800MHZ        2

// Defined Synopsys DDR Controller PMU SRAM address in the system
#define DDRC_PMU_SRAM  0x62140000

uint32_t ms_init_seq_phase1 (uint32_t memshire, uint32_t config_ecc, uint32_t config_real_pll, uint32_t config_800mhz, uint32_t config_933mhz, uint32_t config_training,
  uint32_t config_4gb, uint32_t config_8gb, uint32_t config_32gb, uint32_t config_sim_only);
uint32_t ms_init_seq_phase2 (uint32_t memshire, uint32_t config_real_pll);
uint32_t ms_init_seq_phase3_01 (uint32_t memshire, uint32_t config_800mhz, uint32_t config_933mhz);
uint32_t ms_init_seq_phase3_01_skiptrain(uint32_t memshire, uint32_t config_800mhz, uint32_t config_933mhz);
uint32_t ms_init_swizle_ca_bub (uint32_t memshire0, uint32_t memshire1, uint32_t memshire2, uint32_t memshire3,
  uint32_t memshire4, uint32_t memshire5, uint32_t memshire6, uint32_t memshire7);
uint32_t ms_init_swizle_dq_bub (uint32_t memshire0, uint32_t memshire1, uint32_t memshire2, uint32_t memshire3,
  uint32_t memshire4, uint32_t memshire5, uint32_t memshire6, uint32_t memshire7);
uint32_t ms_init_seq_phase3_02_no_loop (uint32_t memshire, uint32_t config_800mhz, uint32_t config_933mhz);
uint32_t ms_init_seq_phase3_03 (uint32_t memshire, uint32_t config_debug_level, uint32_t config_sim_only);
uint32_t ms_init_seq_phase3_04_no_loop (uint32_t memshire, uint32_t train_poll_max_iterations, uint32_t train_poll_iteration_delay);
uint32_t ms_init_seq_phase3_05_no_loop (uint32_t memshire, uint32_t config_800mhz, uint32_t config_933mhz);
uint32_t ms_init_seq_phase3_06 (uint32_t memshire, uint32_t config_debug_level, uint32_t config_sim_only);
uint32_t ms_init_seq_phase3_07_no_loop (uint32_t memshire, uint32_t train_poll_max_iterations, uint32_t train_poll_iteration_delay);
uint32_t ms_init_seq_phase3_08 (uint32_t memshire, uint32_t config_ecc, uint32_t config_800mhz, uint32_t config_933mhz,
  uint32_t config_4gb, uint32_t config_8gb, uint32_t config_32gb);
uint32_t ms_init_seq_phase4_01 (uint32_t memshire, uint32_t config_800mhz, uint32_t config_933mhz);
uint32_t ms_init_seq_phase4_01_skiptrain (uint32_t memshire, uint32_t config_800mhz, uint32_t config_933mhz);
uint32_t ms_init_seq_phase4_02 (uint32_t memshire, uint32_t config_auto_precharge, uint32_t config_disable_unused_clks, uint32_t config_training);
uint32_t ms_init_clear_ddr (uint32_t memshire, uint32_t config_disable_unused_clks, uint32_t config_4gb, uint32_t config_8gb, uint32_t config_32gb, uint32_t init_pattern);

// helper functions required from consumer code-base
uint64_t ms_read_esr(uint32_t memshire, uint64_t reg);
void ms_write_esr(uint32_t memshire, uint64_t reg, uint64_t value);
uint32_t ms_read_reg(uint32_t memshire, uint64_t reg);
void ms_write_reg(uint32_t memshire, uint64_t reg, uint32_t value);
uint32_t ms_read_ddrc_reg(uint32_t memshire, uint32_t blk, uint64_t reg);
void ms_write_ddrc_reg(uint32_t memshire, uint32_t blk, uint64_t reg, uint32_t value);
void ms_write_both_ddrc_reg(uint32_t memshire, uint64_t reg, uint32_t value);
void ms_write_ddrc_addr(uint32_t memshire, uint64_t addr_in, uint32_t value);
uint32_t ms_poll_pll_reg(uint32_t memshire, uint64_t reg, uint32_t wait_value, uint32_t wait_mask, uint32_t timeout_tries);
uint32_t ms_poll_ddrc_reg(uint32_t memshire, uint32_t blk, uint64_t reg, uint32_t wait_value, uint32_t wait_mask, uint32_t timeout_tries, uint32_t wait_count);
void ms_write_phy_ram(uint32_t memshire, const uint64_t addr, const uint32_t value);
void ms_ddr_phy_1d_train_from_file(uint32_t mem_config, uint32_t memshire);
void ms_wait_for_training(uint32_t memshire, uint32_t train_poll_max_iterations, uint32_t train_poll_iteration_delay);
void ms_ddr_phy_2d_train_from_file(uint32_t mem_config, uint32_t memshire);
void ms_wait_for_training_2d(uint32_t memshire, uint32_t train_poll_max_iterations, uint32_t train_poll_iteration_delay);

void post_train_update_regs(uint32_t memshire);
