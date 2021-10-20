
/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

/* machine minion specific headers */
#include "minion_cfg.h"
#include "config/mm_config.h"

/* minion_bl */
#include <etsoc/isa/esr_defines.h>
#include <transports/mm_cm_iface/broadcast.h>

/* etsoc_hal */
#include <hwinc/etsoc_shire_other_esr.h>
#include <hwinc/minion_lvdpll_program.h>
#include "esr.h"

struct pll_conf_reg_t pll_conf_reg __attribute__((section(".data")));

struct pll_conf_reg_t {
    uint64_t booted_shire_mask;
    uint16_t minion_current_freq;
    uint16_t minion_norm_freq;
    uint8_t minion_lvdpll_strap;
    uint8_t num_shires;
};

volatile struct pll_conf_reg_t *get_pll_conf_reg(void)
{
    return &pll_conf_reg;
}

// get_highest_set_bit_offset
// This function returns highest set bit offset
static uint8_t get_highest_set_bit_offset(uint64_t shire_mask)
{
    return (uint8_t)(64 - __builtin_clzl(shire_mask));
}

// Configure Minion PLL to specific mode. This uses the broadcast mechanism hence all Minions
// will be programmed to the same frequency.
static int64_t minion_configure_cold_boot_pll(uint64_t shire_mask, uint8_t lvdpll_strap)
{
    get_pll_conf_reg()->minion_lvdpll_strap = lvdpll_strap;

    if (0 != shire_mask)
    {
        get_pll_conf_reg()->num_shires = get_highest_set_bit_offset(shire_mask);
    }

#if DVFS_NORMALIZATION_OFF == 1
    int64_t status;
    uint8_t lvdpll_mode;

    // Disable auto-normalization
    turn_off_pll_auto_normalization(shire_mask, get_pll_conf_reg()->num_shires);

    lvdpll_mode = freq_to_mode(INITIAL_MINION_FREQ, lvdpll_strap);
    // Postdiv times normalization frequency gives oscilator frequency
    get_pll_conf_reg()->minion_norm_freq =
        gs_lvdpll_settings[(lvdpll_mode - 1)].values[14] * INITIAL_MINION_FREQ;
    get_pll_conf_reg()->minion_current_freq = INITIAL_MINION_FREQ;
#else
    /* Switch to LVDPLL output */
    pll_broadcast_req(PP_MACHINE, REGION_OTHER, ETSOC_SHIRE_OTHER_ESR_SHIRE_CTRL_CLOCKMUX_ADDRESS,
        shire_mask, SELECT_PLL_CLOCK_0);
    get_pll_conf_reg()->minion_current_freq = INITIAL_MINION_FREQ;
#endif

    get_pll_conf_reg()->booted_shire_mask = shire_mask;

    return 0;
}

// Update Minion PLL to specific frequency. This uses the broadcast mechanism hence all Minions
// will be programmed to the same frequency.
int64_t dynamic_minion_pll_frequency_update(uint64_t freq)
{
    int64_t status;

#if DVFS_USE_FCW == 1
    uint16_t ref_clock = (lvdpll_strap == 0) ? 100 : (lvdpll_strap == 1) ? 24 : 40;
    status = dvfs_fcw_update_minion_pll_freq(freq, booted_shire_mask, num_shires,
        minion_current_freq, minion_norm_freq, ref_clock, DVFS_POLL_FOR_LOCK);
#else
    status = dvfs_update_minion_pll_mode(
        freq_to_mode((uint16_t)freq, get_pll_conf_reg()->minion_lvdpll_strap),
        get_pll_conf_reg()->booted_shire_mask, get_pll_conf_reg()->num_shires, DVFS_POLL_FOR_LOCK);
#endif

    get_pll_conf_reg()->minion_current_freq = (uint16_t)freq;

    return status;
}

// Enable all Minion Threads which participate in Kernel Compute Execution
static int64_t enable_compute_threads(uint64_t shire_mask)
{
    // Enable all Threads which will participate in Compute Kernel Execution
    // within Compute Shire Minion
    broadcast(0x0, shire_mask, PRV_M, ESR_SHIRE_REGION, ESR_SHIRE_THREAD0_DISABLE_REGNO);
    broadcast(0x0, shire_mask, PRV_M, ESR_SHIRE_REGION, ESR_SHIRE_THREAD1_DISABLE_REGNO);

    // Enable parts of the Master Shire Threads which also participate in Compute Kernel Exection
    // Note the rests of the MM threads has been enabled during BL2 phase hence keep mask to all threads
    // to avoid disabling the rest of the threads
    write_esr_new(PP_MACHINE, MM_SHIRE_ID, REGION_OTHER, 2,
        ETSOC_SHIRE_OTHER_ESR_THREAD0_DISABLE_BYTE_ADDRESS, ~(MM_HART_MASK), 0);
    write_esr_new(PP_MACHINE, MM_SHIRE_ID, REGION_OTHER, 2,
        ETSOC_SHIRE_OTHER_ESR_THREAD1_DISABLE_BYTE_ADDRESS, ~(MM_HART_MASK), 0);

    return 0;
}

int64_t configure_compute_minion(uint64_t shire_mask, uint64_t lvdpll_strap)
{
    int64_t status;
    uint64_t cm_shire_mask = (shire_mask & CM_SHIRE_ID_MASK);

    status = minion_configure_cold_boot_pll(cm_shire_mask, (uint8_t)lvdpll_strap);
    if (status != 0)
        return status;

    status = enable_compute_threads(cm_shire_mask);

    return status;
}
