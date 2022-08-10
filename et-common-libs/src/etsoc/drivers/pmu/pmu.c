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

#include "etsoc/isa/esr_defines.h"
#include "etsoc/isa/hart.h"
#include "etsoc/drivers/pmu/pmu.h"

// Must be called by only one hart in a neighborhood
int64_t configure_sc_pmcs(uint64_t ctl_status_cfg, uint64_t pmc0_cfg, uint64_t pmc1_cfg)
{
    int64_t ret;
    uint64_t hart_id = get_hart_id();
    uint64_t neigh_id = (hart_id >> 4) & 0x3;
    uint64_t shire_id = (hart_id >> 6) & 0x1F;

    // Turn off clock gating so you count cycles and related events properly
    volatile uint64_t *sc_reqq_ctl_esr = (uint64_t *)ESR_CACHE(shire_id, neigh_id, SC_REQQ_CTL);
    *sc_reqq_ctl_esr |= (0x1ULL << 22);

    ret = pmu_shire_cache_event_configure(shire_id, neigh_id, 0, pmc0_cfg);
    ret = ret + pmu_shire_cache_event_configure(shire_id, neigh_id, 1, pmc1_cfg);
    // Set bits in ctl_status register that show whether we monitor events or resources.
    pmu_shire_cache_counter_set(shire_id, neigh_id, ctl_status_cfg);

    return ret;
}

int64_t configure_ms_pmcs(uint64_t ms_id, uint64_t ctl_status_cfg, uint64_t ddrc_perfmon_p0_qual,
    uint64_t ddrc_perfmon_p1_qual, uint64_t ddrc_perfmon_p0_qual2, uint64_t ddrc_perfmon_p1_qual2)
{
    int64_t ret;

    // First program the ctl_status register to the events you are going to measure
    pmu_memshire_event_set(ms_id, ctl_status_cfg);

    // Now set the PMC qual registers
    ret = pmu_memshire_event_configure(ms_id, 0, ddrc_perfmon_p0_qual);
    ret = ret + pmu_memshire_event_configure(ms_id, 1, ddrc_perfmon_p1_qual);
    ret = ret + pmu_memshire_event_configure(ms_id, 2, ddrc_perfmon_p0_qual2);
    ret = ret + pmu_memshire_event_configure(ms_id, 3, ddrc_perfmon_p1_qual2);

    return ret;
}

uint64_t sample_sc_pmcs(uint64_t shire_id, uint64_t neigh_id, uint64_t pmc)
{
    uint64_t pmc_data = 0;

    /* Stop the counter */
    pmu_shire_cache_counter_stop(shire_id, neigh_id, pmc);

    /* Sample the counter */
    pmc_data = pmu_shire_cache_counter_sample(shire_id, neigh_id, pmc);

    /* Start the counter */
    pmu_shire_cache_counter_start(shire_id, neigh_id, pmc);

    return pmc_data;
}

uint64_t sample_ms_pmcs(uint64_t ms_id, uint64_t pmc)
{
    uint64_t pmc_data = 0;

    if (ms_id < PMU_MEM_SHIRE_COUNT)
    {
        /* Stop the counter */
        pmu_memshire_event_stop(ms_id, pmc);

        /* Sample the counter */
        pmc_data = pmu_memshire_event_sample(ms_id, pmc);

        /* Start the counter */
        pmu_memshire_event_start(ms_id, pmc);
    }

    return pmc_data;
}

/* Each of harts 0-11 reset one neigh counter */
int64_t reset_minion_neigh_pmcs_all(void)
{
    int64_t ret = 0;
    uint64_t neigh_minion_id = (get_hart_id() >> 1) & 0x7;

    /* To avoid reseting PMC3 that is used for timestamp, neigh_minion_id should be > 0 */
    if (neigh_minion_id < PMU_MINION_COUNTERS_PER_HART + PMU_NEIGH_COUNTERS_PER_HART &&
        neigh_minion_id > 0)
    {
        ret = ret + pmu_core_counter_reset(PMU_MHPMCOUNTER3 + neigh_minion_id);
    }

    return ret;
}

/* Hart no. 13 in each neighborhood resets SC bank PMCs */
int64_t reset_sc_pmcs_all(void)
{
    int64_t ret = 0;
    uint64_t hart_id = get_hart_id();
    uint64_t neigh_id = (hart_id >> 4) & 0x3;
    uint64_t shire_id = (hart_id >> 6) & 0x1F;
    uint64_t reset_sc_harts = ((hart_id & 0xF) == NEIGH_HART_SC);

    if (reset_sc_harts)
    {
        for (uint8_t pmc = 0; pmc < PMU_SC_COUNTERS_PER_BANK; pmc++)
        {
            pmu_shire_cache_counter_reset(shire_id, neigh_id, pmc);
        }
    }

    return ret;
}

/* Shire 0 Hart no. 15 of neigh 3 resets all MS PMCs. */
int64_t reset_ms_pmcs_all(void)
{
    int64_t ret = 0;
    uint64_t hart_id = get_hart_id();
    uint64_t neigh_id = (hart_id >> 4) & 0x3;
    uint64_t shire_id = (hart_id >> 6) & 0x1F;
    uint64_t reset_ms_harts = (shire_id == PMU_MS_COUNTERS_CONTROL_SHIRE) && (neigh_id == 3) &&
                              ((hart_id & 0xF) == NEIGH_HART_MS);

    if (reset_ms_harts)
    {
        /* Reset all memshires */
        for (uint8_t ms_idx = 0; ms_idx < PMU_MEM_SHIRE_COUNT; ms_idx++)
        {
            /* Reset each MS PMC */
            for (uint8_t pmc = 0; pmc < PMU_MS_COUNTERS_PER_MS; pmc++)
            {
                pmu_memshire_event_reset(ms_idx, pmc);
            }
        }
    }

    return ret;
}

int32_t sample_sc_pmcs_all(uint64_t shire_id, uint64_t bank_id, shire_pmc_cnt_t *val)
{
    int32_t status;

    /* Stop the counter */
    pmu_shire_cache_counter_stop(shire_id, bank_id, PMU_SC_ALL);

    /* Sample the counter */
    status = pmu_shire_cache_counter_sample_all(shire_id, bank_id, val);

    /* Start the counter */
    pmu_shire_cache_counter_start(shire_id, bank_id, PMU_SC_ALL);

    return status;
}

int32_t sample_ms_pmcs_all(uint64_t ms_id, shire_pmc_cnt_t *val)
{
    int32_t status = -1;

    if (ms_id < PMU_MEM_SHIRE_COUNT)
    {
        /* Stop the counter */
        pmu_memshire_event_stop(ms_id, PMU_MS_ALL);

        /* Sample the counter */
        status = pmu_memshire_event_sample_all(ms_id, val);

        /* Start the counter */
        pmu_memshire_event_start(ms_id, PMU_MS_ALL);
    }

    return status;
}
