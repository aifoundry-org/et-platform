
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

#include "device-common/esr_defines.h"
#include "etsoc_hal/inc/esr.h"
#include "etsoc_hal/inc/etsoc_shire_other_esr.h"
#include "config/mm_config.h"
#include "broadcast.h"
#include "minion_cfg.h"

// Configure Minion PLL to specific mode. This uses the broadcast mechanism hence all Minions
// will be programmed to the same frequency.
static int64_t minion_configure_pll(uint64_t shire_mask, uint64_t pll_mode)
{
    // TODO : To implement full cold boot PLL programming sequence as per
    // Details covered: https://esperantotech.atlassian.net/wiki/spaces/SW/pages/836337737/Power+Management+System+Software+Specification#Cold-Reset-Sequence
    if (pll_mode == 10)
        return -1;

    broadcast(0xb, shire_mask, PRV_M, ESR_SHIRE_REGION, ESR_SHIRE_SHIRE_CTRL_CLOCKMUX_REGNO);

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

    status = minion_configure_pll(cm_shire_mask, pll_mode);
    if (status != 0)
        return status;

    status = enable_compute_threads(cm_shire_mask);

    return status;
}
