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
#include "device-common/hart.h"
#include "pmu.h"

// Configure PMCs
// reset_counters is a boolean that determines whether we reset / start counters after the configuration
// conf_buffer_addr points to the beginning of the memory buffer where the configuration values are stored
// return 0 id all configuration succeeds, or negative number equal to the number of failed configurations.
int64_t configure_pmcs(uint64_t reset_counters, uint64_t conf_buffer_addr)
{
    uint64_t hart_id = get_hart_id();
    uint64_t neigh_id = (hart_id >> 4) & 0x3;
    uint64_t shire_id = (hart_id >> 6) & 0x1F;
    uint64_t odd_hart = hart_id & 0x1;
    uint64_t program_neigh_harts = ((hart_id & 0xF) == 0x8) || ((hart_id & 0xF) == 0x9);
    uint64_t program_sc_harts = ((hart_id & 0xF) == NEIGH_HART_SC);
    uint64_t program_ms_harts = (((hart_id & 0xF) == NEIGH_HART_MS) && (shire_id < 8) && (neigh_id == 3));
    int64_t ret = 0;

    // If the conf buffer has not been set, do not do anything
    if (conf_buffer_addr == 0) {
        return -1;
    }
    uint64_t *conf_buffer = (uint64_t *)conf_buffer_addr;

    // minion counters: Each hart configures all counters so that we measure events for all the harts
    // Since we reserve PMC3 to be used as a timer for tracing, we start from PMC4
    for (uint64_t i = 1; i < PMU_MINION_COUNTERS_PER_HART; i++) {
        uint64_t *hart_minion_cfg_data =
            conf_buffer + PMU_EVENT_SHIRE_AREA * shire_id + PMU_MINION_COUNTERS_PER_HART * odd_hart + i;
        ret = ret + pmu_core_event_configure(PMU_MHPMEVENT3 + i, *hart_minion_cfg_data);
    }

    // neigh counters: Only one minion per neigh needs to configure the counters (8 or 9)
    if (program_neigh_harts) {
        for (uint64_t i = 0; i < PMU_NEIGH_COUNTERS_PER_HART; i++) {
            uint64_t *hart_neigh_cfg_data = conf_buffer + PMU_EVENT_SHIRE_AREA * shire_id +
                                            PMU_MINION_COUNTERS_PER_HART * 2 +
                                            PMU_NEIGH_COUNTERS_PER_HART * odd_hart + i;
            ret = ret + pmu_core_event_configure(PMU_MHPMEVENT7+i, *hart_neigh_cfg_data);
        }
    }

    // sc location
    uint64_t *hart_sc_cfg_data = conf_buffer + PMU_EVENT_SHIRE_AREA * shire_id +
                                 PMU_MINION_COUNTERS_PER_HART * 2 +
                                 PMU_NEIGH_COUNTERS_PER_HART * 2;
    // We use 1 hart so that there is no race between configuration / resetting and sampling
    if (program_sc_harts) {
        // Turn off clock gating so you count cycles and related events properly
        volatile uint64_t *sc_reqq_ctl_esr = (uint64_t *)ESR_CACHE(shire_id, neigh_id, SC_REQQ_CTL);
        *sc_reqq_ctl_esr |= (0x1ULL << 22);

        uint64_t ctl_status_cfg = *hart_sc_cfg_data;
        uint64_t pmc0_cfg = *(hart_sc_cfg_data+1);
        uint64_t pmc1_cfg = *(hart_sc_cfg_data+2);
        ret = ret + pmu_shire_cache_event_configure(shire_id, neigh_id, 0, pmc0_cfg);
        ret = ret + pmu_shire_cache_event_configure(shire_id, neigh_id, 1, pmc1_cfg);
        // Set bits in ctl_status register that show whether we monitor events or resources.
        pmu_shire_cache_counter_set(shire_id, neigh_id, ctl_status_cfg);
    }

    // Shire id's 0-7 just to simplify code initialize ms-id's 0-7.
    // TBD: Use shires closer to memshires
    // Currently last hart on neigh 3 of shires 0-7 programs memshire pref ctrl registers,
    // Use one hart to avoid races
    if (program_ms_harts) {

        // First program the ctl_status register to the events you are going to measure
        uint64_t *hart_ms_cfg_data = conf_buffer + PMU_EVENT_MEMSHIRE_OFFSET + shire_id * PMU_MS_COUNTERS_PER_MS;
        pmu_memshire_event_set(shire_id, *hart_ms_cfg_data);

        // Now set the PMC qual registers -- we can probably save 2 of the 4 writes in most cases
        for (uint64_t i=0; i < 4; i++) {
            hart_ms_cfg_data = conf_buffer + PMU_EVENT_MEMSHIRE_OFFSET + shire_id * PMU_MS_COUNTERS_PER_MS + i + 1;
            ret = ret + pmu_memshire_event_configure(shire_id, i, *hart_ms_cfg_data);
        }
    }

    if (reset_counters) {
        ret = ret + reset_pmcs();
    }

    return ret;
}

// Reset PMCs
// Each of harts 0-11 reset one neigh counter
// Hart 13 resets SC bank PMCs
// Hart 15 of neigh 3 of shires 0-7 resets MS PMCs.
int64_t reset_pmcs(void)
{
    int64_t ret = 0;
    uint64_t hart_id = get_hart_id();
    uint64_t neigh_minion_id = (hart_id >> 1) & 0x7;
    uint64_t neigh_id = (hart_id >> 4) & 0x3;
    uint64_t shire_id = (hart_id >> 6) & 0x1F;
    uint64_t reset_sc_harts = ((hart_id & 0xF) == NEIGH_HART_SC);
    uint64_t reset_ms_harts = (shire_id < 8) && (neigh_id == 3) && ((hart_id & 0xF) == NEIGH_HART_MS);

    // To avoid reseting PMC3 that is used for timestamp, neigh_minion_id should be > 0
    if (neigh_minion_id < PMU_MINION_COUNTERS_PER_HART + PMU_NEIGH_COUNTERS_PER_HART &&
        neigh_minion_id > 0) {
        ret = ret + pmu_core_counter_reset(PMU_MHPMCOUNTER3 + neigh_minion_id);
    }

    if (reset_sc_harts) {
        pmu_shire_cache_counter_reset(shire_id, neigh_id);
    }

    if (reset_ms_harts) {
        pmu_memshire_event_reset(shire_id);
    }

    return ret;
}

// Sample PMCs
// Each of harts 0-11 read one neigh PMC
// Hart 13 reads shire cache PMCs
// Hart 15 of neigh 3, shires 0-7 read memshire PMCs
int64_t sample_pmcs(uint64_t reset_counters, uint64_t log_tensor_addr)
{
    int64_t ret = 0;
    uint64_t hart_id = get_hart_id();
    uint64_t neigh_id = (hart_id >> 4) & 0x3;
    uint64_t shire_id = (hart_id >> 6) & 0x1F;
    uint64_t read_sc_harts = ((hart_id & 0xF) == NEIGH_HART_SC);
    uint64_t read_ms_harts = ((hart_id & 0xF) == NEIGH_HART_MS) && (neigh_id == 3) && (shire_id < 8);

    // Use log buffer back door
    //if (log_buffer_addr == 0) {
    //    return -1;
    //}
    uint64_t *log_tensor = (uint64_t *)log_tensor_addr;
    uint64_t neigh_minion_id = (hart_id >> 1) & 0x7;

    // Minion and neigh PMCs
    // TBD: To avoid reseting PMC3 that is used for timestamp, neigh_minion_id should be > 0
    // Right now we keep on reading these PMCs, but we do not have to
    if (neigh_minion_id < PMU_MINION_COUNTERS_PER_HART + PMU_NEIGH_COUNTERS_PER_HART) {
        uint64_t pmc_data = pmu_core_counter_read(PMU_MHPMCOUNTER3 + neigh_minion_id);
        if (pmc_data == PMU_INCORRECT_COUNTER) {
            ret = ret - 1;
        }
        if (log_tensor) {
            // emizan: Given changes in hart assignment after this was first put
            // this needs to be revised. Data out of log_tensor may be off
            *(log_tensor + hart_id * 8) = pmc_data;
        }
    }

    // SC PMCs
    if (read_sc_harts) {
        pmu_shire_cache_counter_stop(shire_id, neigh_id);
        // Read 3 SC bank counters, probably we can save the clock counter.
        for (uint64_t i=0; i < 3; i++) {
            uint64_t pmc_data = pmu_shire_cache_counter_sample(shire_id, neigh_id, i);
            if (pmc_data == PMU_INCORRECT_COUNTER) {
               ret = ret - 1;
            }
            // emizan: Given changes in hart assignment after this was first put
            // this needs to be revised. Data out of log_tensor may be off
            if (log_tensor) {
                *(log_tensor + (shire_id * 64 + neigh_id * 16 + NEIGH_HART_SC + i- 1)* 8) = pmc_data;
            }
        }
    }

    // MS PMCs
    if (read_ms_harts) {
        pmu_memshire_event_stop(shire_id);
        // Read the 3 MS pef counters -- probably we can save the clock counter read
        for (uint64_t i=0; i < 3; i++) {
            uint64_t pmc_data = pmu_memshire_event_sample(shire_id, i);
            if (pmc_data == PMU_INCORRECT_COUNTER) {
                ret = ret - 1;
            }
            // emizan: Given changes in hart assignment after this was first put
            // this needs to be revised. Data out of log_tensor may be off
            if (log_tensor) {
                *(log_tensor + (shire_id * 64 + (neigh_id - 3 + i)* 16 + (hart_id & 0xF)) * 8) = pmc_data;
            }
        }
    }

    if (reset_counters) {
        ret = ret + reset_pmcs();
    }

    return ret;
}
