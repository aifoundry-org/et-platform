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
        MemShire_PLL_Program
        MemShire_Voltage_Update
        Memory_read
        Memory_write
*/
/***********************************************************************/
#include "bl2_ddr_init.h"
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

/*
** following DDR initialization flow from hardware team
*/
int ddr_config(void)
{
    // config parameter from hardware team
    // TODO confirm the right value
    uint32_t config_ecc = 0;
    uint32_t config_real_pll = 1;
    uint32_t config_800mhz = 0;
    uint32_t config_933mhz = 0;
    uint32_t config_auto_precharge = 0;
    uint32_t config_debug_level = 1;            
    uint32_t config_sim_only = 0;
    uint32_t config_disable_unused_clks = 0;
    uint32_t train_wait_time = 100;
    uint32_t train_cycle_count = 1;
    bool config_training = false;
    bool config_training_2d = true;

    uint32_t memshire;

    // figure out the correct config value here
    // e.g. if not real_pll, config_real_pll=0

    // only #0 and #4 has PLL, kick them off before phy init
    ms_pll_init(0x0, config_real_pll, 1, config_800mhz, config_933mhz);
    ms_pll_init(0x4, config_real_pll, 1, config_800mhz, config_933mhz);

    FOR_EACH_MEMSHIRE(
        ms_init_seq_phase1(memshire, config_ecc, config_real_pll, config_800mhz, config_933mhz, config_training)
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

        ms_init_seq_phase3_04_no_loop(memshire, train_wait_time, train_cycle_count);

        if(config_training_2d) {
            ms_init_seq_phase3_05_no_loop(memshire, config_800mhz, config_933mhz);

            FOR_EACH_MEMSHIRE_EVEN_FIRST(
                ms_init_seq_phase3_06 (memshire, config_debug_level, config_sim_only)
            );

            ms_init_seq_phase3_07_no_loop (memshire, train_wait_time, train_cycle_count);
        }

        FOR_EACH_MEMSHIRE(
            ms_init_seq_phase3_08 (memshire, config_ecc, config_800mhz, config_933mhz)
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
    strncpy(mem_detail, "Unknown", 8);
    return 0;
}

int ddr_get_memory_type(char *mem_type)
{
    strncpy(mem_type, "LPDDR4X", 8);
    return 0;
}

//////////////////////////////////////////////////////////////////////
// Following MemShire_x must go.  They belong to TF framework.

int MemShire_PLL_Program(uint8_t memshire, uint8_t frequency)
{
    switch(frequency) 
    {
        case MEMSHIRE_FREQUENCY_800:
            return (int) ms_config_795mhz(memshire);

        case MEMSHIRE_FREQUENCY_933:
            return (int) ms_config_933mhz(memshire);

        case MEMSHIRE_FREQUENCY_1067:
            return (int) ms_config_1067mhz(memshire);
        
        default:
            return -1;
    }

}
 
int Memory_read(uint8_t *address, uint8_t *rx_buffer, uint64_t size)
{
    /* check if address range is valid */
    if ((rx_buffer == NULL) || 
        (((uintptr_t)address < (uintptr_t)LOW_MEM_SUB_REGIONS_BASE) || 
         ((uintptr_t)address > (uintptr_t)(LOW_MEM_SUB_REGIONS_BASE + LOW_MEM_SUB_REGIONS_SIZE)))) 
    {
        
        return ERROR_INVALID_ARGUMENT;
    }

    /* read data into buffer */
    for (uint64_t i=0; i < size; i++)
    {
        *rx_buffer = *address;
        rx_buffer++;
        address++;
    }

    return 0;
}

int Memory_write(uint8_t *address, uint8_t* data_buf, uint64_t size)
{
    /* check if address range is valid */
    if ((data_buf == NULL) || 
        (((uintptr_t)address < (uintptr_t)LOW_MEM_SUB_REGIONS_BASE) || 
         ((uintptr_t)address > (uintptr_t)(LOW_MEM_SUB_REGIONS_BASE + LOW_MEM_SUB_REGIONS_SIZE)))) 
    {
        
        return ERROR_INVALID_ARGUMENT;
    }

    /* write data onto memory */
    for (uint64_t i=0; i < size; i++)
    {
        *address = *data_buf;
        data_buf++;
        address++;
    }

    return 0;
}
