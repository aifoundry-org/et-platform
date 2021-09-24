
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
#include "etsoc/isa/esr_defines.h"
#include "transports/mm_cm_iface/broadcast.h"

/* etsoc_hal */
#include <hwinc/etsoc_shire_other_esr.h>
#include <hwinc/minion_lvdpll_program.h>
#include "esr.h"

static uint64_t booted_shire_mask = 0;
static uint64_t minion_current_freq = 0;

// Configure Minion PLL to specific mode. This uses the broadcast mechanism hence all Minions
// will be programmed to the same frequency.
static int64_t minion_configure_cold_boot_pll(uint64_t shire_mask, uint64_t pll_mode)
{
#if MM_DVFS_NORMALIZATION_OFF == 1
    int64_t status;
    status = pll_normalize_and_turn_of_normalization(shire_mask);
    if (status != 0)
    {
        return status;
    }

    status = update_minion_pll_freq_quick(pll_mode, shire_mask);
    if (status != 0)
    {
        return status;
    }
    minion_current_freq = (uint64_t)mode_to_freq(pll_mode);
#else
    (void)pll_mode;
    /* Switch to LVDPLL output */
    broadcast(0xc, shire_mask, PRV_M, ESR_SHIRE_REGION, ESR_SHIRE_SHIRE_CTRL_CLOCKMUX_REGNO);
#endif

    booted_shire_mask = shire_mask;
    minion_current_freq = INITIAL_MINION_FREQ;

    return 0;
}

// Update Minion PLL to specific frequency. This uses the broadcast mechanism hence all Minions
// will be programmed to the same frequency.
int64_t dynamic_minion_pll_frequency_update(uint64_t freq)
{
    int64_t status;

#if MM_DVFS_USE_FCW == 1
    status = dvfs_fcw_update_minion_pll_freq((uint16_t)freq,
                booted_shire_mask, (uint16_t)minion_current_freq, MM_DVFS_POLL_FOR_LOCK);
    if (status != 0)
    {
        return status;
    }
#else
    status = dvfs_update_minion_pll_mode(freq_to_mode((uint32_t)freq),
                booted_shire_mask, MM_DVFS_POLL_FOR_LOCK);
    if (status != 0)
    {
        return status;
    }
#endif

    minion_current_freq = freq;

    return 0;
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
    write_esr_new(PP_MACHINE, MM_SHIRE_ID, REGION_OTHER, 2, ETSOC_SHIRE_OTHER_ESR_THREAD0_DISABLE_BYTE_ADDRESS,
                    ~(MM_HART_MASK), 0);
    write_esr_new(PP_MACHINE, MM_SHIRE_ID, REGION_OTHER, 2, ETSOC_SHIRE_OTHER_ESR_THREAD1_DISABLE_BYTE_ADDRESS,
                    ~(MM_HART_MASK), 0);


    return 0;
}

int64_t configure_compute_minion(uint64_t shire_mask, uint64_t pll_mode)
{
    int64_t status;
    uint64_t cm_shire_mask = (shire_mask & CM_SHIRE_ID_MASK);

    status = minion_configure_cold_boot_pll(cm_shire_mask, pll_mode);
    if (status != 0)
        return status;

    status = enable_compute_threads(cm_shire_mask);

    return status;
}
