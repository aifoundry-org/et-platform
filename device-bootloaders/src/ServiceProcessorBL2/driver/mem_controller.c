/***********************************************************************
*
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
/*! \file ddr_controller.c
    \brief A C module that implements the DDR memory subsystem.

    Public interfaces:
        ddr_config
        ddr_error_control_init
        ddr_error_control_deinit
        ddr_enable_uce_interrupt
        ddr_disable_ce_interrupt
        ddr_disable_uce_interrupt
        ddr_set_ce_threshold
        ddr_get_ce_count
        ddr_get_uce_count
        ddr_error_threshold_isr
        ddr_get_memory_details
        ddr_get_memory_type
*/
/***********************************************************************/
#include "mem_controller.h"
#include "bl2_sp_memshire_pll.h"
#include "dm_event_control.h"
#include "hal_ddr_init.h"

static uint32_t memshire_frequency;
static uint32_t ddr_frequency;
/* MEMSHIRE PLL frequency modes (795MHz) for different ref clocks, 100MHz, 24Mhz and 40MHz */
static uint8_t min_lvdpll_mode_795MHz[3] = {50, 51, 52};
/* MEMSHIRE PLL frequency modes (933MHz) for different ref clocks, 100MHz, 24Mhz and 40MHz */
static uint8_t min_lvdpll_mode_933MHz[3] = {28, 29, 30};
/* MEMSHIRE PLL frequency modes (1066MHz) for different ref clocks, 100MHz, 24Mhz and 40MHz */
static uint8_t min_lvdpll_mode_1066MHz[3] = {19, 20, 21};

int configure_memshire_plls(const DDR_MODE *ddr_mode)
{
    uint8_t pll_mode;
    uint8_t hpdpll_strap_pins;

    hpdpll_strap_pins = get_hpdpll_strap_value();

    /* [PLL Mode Spreadsheet]
    https://docs.google.com/spreadsheets/d/0B45kZDfsf1VrbE5QOW1LZ1Zoc0VmWXRyMDJQMDViLUM2NGMw/ */

    if(ddr_mode->frequency == DDR_FREQUENCY_800MHZ) {
        pll_mode = min_lvdpll_mode_795MHz[hpdpll_strap_pins];
    }
    else if(ddr_mode->frequency == DDR_FREQUENCY_933MHZ) {
        pll_mode = min_lvdpll_mode_933MHz[hpdpll_strap_pins];
    }
    else if(ddr_mode->frequency == DDR_FREQUENCY_1066MHZ) {
        pll_mode = min_lvdpll_mode_1066MHz[hpdpll_strap_pins];
    }
    else {
        return -1;
    }

    if (0 != program_memshire_pll(0, pll_mode, &memshire_frequency))
        return -1;

    if (0 != program_memshire_pll(4, pll_mode, &memshire_frequency))
        return -1;

    return 0;
}

#if !FAST_BOOT
/*
** following DDR initialization flow from hardware team
*/
int ddr_config(const DDR_MODE *ddr_mode)
{
    // algorithm/flow and config parameters are from hardware team
    uint32_t config_ecc;
    uint32_t config_real_pll;
    uint32_t config_800mhz;
    uint32_t config_933mhz;
    uint32_t config_auto_precharge;
    uint32_t config_debug_level;    // one of PHY_MSG_VERBOSITY_xxxx
    uint32_t config_sim_only;
    uint32_t config_disable_unused_clks;
    uint32_t config_train_poll_max_iterations;
    uint32_t config_train_poll_iteration_delay;
    uint32_t config_4gb;
    uint32_t config_8gb;
    uint32_t config_32gb;
    bool config_training;
    bool config_training_2d;

    // local variables
    uint32_t memshire;

   // decide frequency paramters
    if(ddr_mode->frequency == DDR_FREQUENCY_800MHZ) {
        config_800mhz = 1;
        config_933mhz = 0;
    }
    else if(ddr_mode->frequency == DDR_FREQUENCY_933MHZ) {
        config_800mhz = 0;
        config_933mhz = 1;
    }
    else if(ddr_mode->frequency == DDR_FREQUENCY_1066MHZ) {
        config_800mhz = 0;
        config_933mhz = 0;
    }
    else {
        return -1;
    }

    // decide capacity parameters
    if(ddr_mode->capacity == DDR_CAPACITY_4GB) {
        config_4gb  = 1;
        config_8gb  = 0;
        config_32gb = 0;
    }
    else if(ddr_mode->capacity == DDR_CAPACITY_8GB) {
        config_4gb  = 0;
        config_8gb  = 1;
        config_32gb = 0;
    }
    else if(ddr_mode->capacity == DDR_CAPACITY_16GB) {
        config_4gb  = 0;
        config_8gb  = 0;
        config_32gb = 0;
    }
    else if(ddr_mode->capacity == DDR_CAPACITY_32GB) {
        config_4gb  = 0;
        config_8gb  = 0;
        config_32gb = 1;
    }
    else {
        return -1;
    }

    config_ecc       = ddr_mode->ecc ? 1 : 0;
    config_training  = ddr_mode->training ? 1 : 0;
    config_sim_only  = ddr_mode->sim_only ? 1 : 0;

    config_real_pll = 1;
    config_auto_precharge = 0;
    config_debug_level = PHY_MSG_VERBOSITY_MORE_DETAILED_DEBUG;
    config_disable_unused_clks = 0;
    config_train_poll_max_iterations = 50000;
    config_train_poll_iteration_delay = 1000;   // unit in ns.  less than 1000ns will simply do task yield
    config_training_2d = true;

    FOR_EACH_MEMSHIRE(
        CHECK_MEMSHIRE_ID(memshire);
    )

    FOR_EACH_MEMSHIRE(
        ms_init_seq_phase1(memshire, config_ecc, config_real_pll, config_800mhz, config_933mhz,
          config_training, config_4gb, config_8gb, config_32gb, config_sim_only);
    )

    FOR_EACH_MEMSHIRE(
        ms_init_seq_phase2(memshire, config_real_pll);
    )

    FOR_EACH_MEMSHIRE(
        if(config_sim_only)
            ms_init_seq_phase3_01_skiptrain(memshire, config_800mhz, config_933mhz);
        else
            ms_init_seq_phase3_01(memshire, config_800mhz, config_933mhz);
    )

    if(config_training) {

        Log_Write(LOG_LEVEL_INFO, "DDR:[%d][txt]config_debug_level = 0x%02x\n", 0, config_debug_level);

        ms_init_seq_phase3_02_no_loop(memshire, config_800mhz, config_933mhz);

        FOR_EACH_MEMSHIRE_EVEN_FIRST(
            Log_Write(LOG_LEVEL_INFO, "DDR:[%d][txt]Training 1D starts\n", memshire);
            ms_init_seq_phase3_03(memshire, config_debug_level, config_sim_only);
        )

        ms_init_seq_phase3_04_no_loop(memshire, config_train_poll_max_iterations, config_train_poll_iteration_delay);

        if(config_training_2d) {
            ms_init_seq_phase3_05_no_loop(memshire, config_800mhz, config_933mhz);

            FOR_EACH_MEMSHIRE_EVEN_FIRST(
                Log_Write(LOG_LEVEL_INFO, "DDR:[%d][txt]Training 2D starts\n", memshire);
                ms_init_seq_phase3_06 (memshire, config_debug_level, config_sim_only);
            )

            ms_init_seq_phase3_07_no_loop (memshire, config_train_poll_max_iterations, config_train_poll_iteration_delay);
        }

        FOR_EACH_MEMSHIRE(
            ms_init_seq_phase3_08 (memshire, config_ecc, config_800mhz, config_933mhz, config_4gb, config_8gb, config_32gb);
        )
    }
    FOR_EACH_MEMSHIRE(
        if(config_sim_only)
            ms_init_seq_phase4_01_skiptrain(memshire, config_800mhz, config_933mhz);
        else
            ms_init_seq_phase4_01(memshire, config_800mhz, config_933mhz);
    )

    FOR_EACH_MEMSHIRE(
        ms_init_seq_phase4_02(memshire, config_auto_precharge, config_disable_unused_clks, config_training);
    )

    return 0;
}
#endif //!FAST_BOOT

static struct ddr_event_control_block  event_control_block __attribute__((section(".data")));

int32_t ddr_error_control_init(dm_event_isr_callback event_cb)
{
    event_control_block.event_cb = event_cb;
    return  0;
}
int32_t ddr_error_control_deinit(void)
{
    return  0;
}

int32_t ddr_enable_uce_interrupt(void)
{
    return  0;
}

int32_t ddr_disable_ce_interrupt(void)
{
    return  0;
}

int32_t ddr_disable_uce_interrupt(void)
{
    return  0;
}

int32_t ddr_set_ce_threshold(uint32_t ce_threshold)
{
    (void)ce_threshold;
    return  0;
}

int32_t ddr_get_ce_count(uint32_t *ce_count)
{
    *ce_count = 0;
    return  0;
}

int32_t ddr_get_uce_count(uint32_t *uce_count)
{
    *uce_count = 0;
    return  0;
}

void ddr_error_threshold_isr(void)
{

}

int ddr_get_memory_details(char *mem_detail)
{
    char name[] = "Unknown";
    snprintf(mem_detail, 8, "%s", name);

    return 0;
}

int ddr_get_memory_type(char *mem_type)
{
    char name[] = "LPDDR4X";
    snprintf(mem_type, 8, "%s", name);

    return 0;
}

int32_t configure_memshire(void)
{
    memshire_frequency = 0;
    ddr_frequency = 0;

    DDR_MODE ddr_mode = {
        .frequency = DDR_FREQUENCY_800MHZ,  // First boot on 800Mhz, production should be 1066Mhz
        .capacity = DDR_CAPACITY_16GB,
        .ecc = false,
        .training = false,
        .sim_only = false
    };

    //TODO: decide ddr_mode based on, e.g. from storage

    if (0 != release_memshire_from_reset()) {
        Log_Write(LOG_LEVEL_ERROR, "release_memshire_from_reset() failed!\n");
        return MEMSHIRE_COLD_RESET_CONFIG_ERROR;
    }
    if (0 != configure_memshire_plls(&ddr_mode)) {
        Log_Write(LOG_LEVEL_ERROR, "configure_memshire_plls() failed!\n");
        return MEMSHIRE_PLL_CONFIG_ERROR;
    }
#if !FAST_BOOT
    if (0 != ddr_config(&ddr_mode)) {
        Log_Write(LOG_LEVEL_ERROR, "ddr_config() failed!\n");
        return MEMSHIRE_DDR_CONFIG_ERROR;
    }
    Log_Write(LOG_LEVEL_INFO, "DRAM ready.\n");
#endif
    if(ddr_mode.frequency == DDR_FREQUENCY_800MHZ) {
        ddr_frequency = 800;
    }
    else if(ddr_mode.frequency == DDR_FREQUENCY_933MHZ) {
        ddr_frequency = 933;
    }
    else if(ddr_mode.frequency == DDR_FREQUENCY_1066MHZ) {
        ddr_frequency = 1066;
    }

   return SUCCESS;
}

uint32_t get_memshire_frequency( void )
{
   return memshire_frequency;
}

uint32_t get_ddr_frequency( void )
{
   return ddr_frequency;
}
