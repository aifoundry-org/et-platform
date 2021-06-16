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
#include "layout.h"   // Memory_read()/Memory_write() dependent on this

#define NUMBER_OF_MEMSHIRE     8
#define FOR_EACH_MEMSHIRE(statement)                                               \
            {                                                                      \
                for(memshire = 0;memshire < NUMBER_OF_MEMSHIRE;++memshire)         \
                   statement;                                                      \
            }
#define FOR_EACH_MEMSHIRE_EVEN_FIRST(statement)                                    \
            {                                                                      \
                for(memshire = 0;memshire < NUMBER_OF_MEMSHIRE;memshire += 2)      \
                   statement;                                                      \
                for(memshire = 1;memshire < NUMBER_OF_MEMSHIRE;memshire += 2)      \
                   statement;                                                      \
            }

static int configure_memshire_plls(void)
{
    if (0 != program_memshire_pll(0, 19))
        return -1;

    if (0 != program_memshire_pll(4, 19))
        return -1;

    return 0;
}

/*
** following DDR initialization flow from hardware team
*/
int ddr_config(void)
{
    // algorithm/flow and config parameters are from hardware team
    uint32_t config_ecc = 0;
    uint32_t config_real_pll = 1;
    uint32_t config_800mhz = 1;   //TODO 800Mhz for first boot.  Production should have it as zero.
    uint32_t config_933mhz = 0;
    uint32_t config_auto_precharge = 0;
    uint32_t config_debug_level = 1;
    uint32_t config_sim_only = 0;
    uint32_t config_disable_unused_clks = 0;
    uint32_t config_train_poll_max_iterations = 50000;
    uint32_t config_train_poll_iteration_delay = 10000;
    uint32_t config_4gb = 0;
    uint32_t config_8gb = 0;
    uint32_t config_32gb = 0;
    bool config_training = false;
    bool config_training_2d = true;

    /* for simuation only
    config_real_pll = 0;
    config_training = false;
    config_sim_only = 1;
    config_train_poll_max_iterations = 50000;
    config_train_poll_iteration_delay = 10000;
    */

    uint32_t memshire;

    // only #0 and #4 has PLL, kick them off before phy init
    ms_pll_init(0x0, config_real_pll, 1, config_800mhz, config_933mhz);
    ms_pll_init(0x4, config_real_pll, 1, config_800mhz, config_933mhz);

    FOR_EACH_MEMSHIRE(
        ms_init_seq_phase1(memshire, config_ecc, config_real_pll, config_800mhz, config_933mhz,
          config_training, config_4gb, config_8gb, config_32gb)
    );

    FOR_EACH_MEMSHIRE(
        ms_init_seq_phase2(memshire, config_real_pll)
    );

    FOR_EACH_MEMSHIRE(
        ms_init_seq_phase3_01(memshire, config_800mhz, config_933mhz)
    );

    if(config_training) {

        ms_init_seq_phase3_02_no_loop(memshire, config_800mhz, config_933mhz);

        FOR_EACH_MEMSHIRE_EVEN_FIRST(
            ms_init_seq_phase3_03(memshire, config_debug_level, config_sim_only)
        );

        ms_init_seq_phase3_04_no_loop(memshire, config_train_poll_max_iterations, config_train_poll_iteration_delay);

        if(config_training_2d) {
            ms_init_seq_phase3_05_no_loop(memshire, config_800mhz, config_933mhz);

            FOR_EACH_MEMSHIRE_EVEN_FIRST(
                ms_init_seq_phase3_06 (memshire, config_debug_level, config_sim_only)
            );

            ms_init_seq_phase3_07_no_loop (memshire, config_train_poll_max_iterations, config_train_poll_iteration_delay);
        }

        FOR_EACH_MEMSHIRE(
            ms_init_seq_phase3_08 (memshire, config_ecc, config_800mhz, config_933mhz, config_4gb, config_8gb, config_32gb)
        );
    }
    FOR_EACH_MEMSHIRE(
        ms_init_seq_phase4_01(memshire, config_800mhz, config_933mhz)
    );

    FOR_EACH_MEMSHIRE(
        ms_init_seq_phase4_02(memshire, config_auto_precharge, config_disable_unused_clks)
    );

    return 0;
}

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
    // FIXME Program the DDR Voltage if required
    //pmic_get_voltage(DDR, voltage)

    if (0 != release_memshire_from_reset()) {
        Log_Write(LOG_LEVEL_ERROR, "release_memshire_from_reset() failed!\n");
        return MEMSHIRE_COLD_RESET_CONFIG_ERROR;
    }
    if (0 != configure_memshire_plls()) {
        Log_Write(LOG_LEVEL_ERROR, "configure_memshire_plls() failed!\n");
        return MEMSHIRE_PLL_CONFIG_ERROR;
    }
#if !FAST_BOOT
    if (0 != ddr_config()) {
        Log_Write(LOG_LEVEL_ERROR, "ddr_config() failed!\n");
        return MEMSHIRE_DDR_CONFIG_ERROR;
    }
    Log_Write(LOG_LEVEL_INFO, "DRAM ready.\n");
#endif
   return SUCCESS;
}
