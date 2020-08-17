
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

#include "esr_defines.h"
#include "hart.h"
#include "pmu.h"

//static int64_t configure_pmcs(uint64_t reset_counters);
//static int64_t sample_pmcs(uint64_t reset_counters);
//static int64_t reset_pmcs(void);

// Configure PMCs
// conf_area point to the beginning of the memory buffer where the configuration values are stored
// reset_counters is a boolean that determines whether we reset / start counters after the configuration
// return 0 id all configuration succeeds, or negative number equal to the number of failed configurations.
int64_t configure_pmcs(uint64_t reset_counters, uint64_t conf_buffer_addr)
{
    uint64_t hart_id = get_hart_id();
    uint64_t neigh_id = (hart_id >> 4) & 0x3;
    uint64_t shire_id = (hart_id >> 6) & 0x1F;
    uint64_t odd_hart = hart_id & 0x1;
    uint64_t program_neigh_harts = ((hart_id & 0xF) >= 0x8) && ((hart_id & 0xF) < 0xC);
    uint64_t program_sc_harts = ((hart_id & 0xF) == 0xC) || ((hart_id & 0xF) == 0xD);
    uint64_t program_ms_harts = ((hart_id & 0xF) == 0xF);
    int64_t ret = 0;

    // emizan:
    // We assume conf buffer is available -- hardcode it for now
    // It is not used so it does not matter
    // uint64_t conf_buffer_addr = 0x8280000000ULL;
    if (conf_buffer_addr == 0) {
        return -1;
    }
    uint64_t *conf_buffer = (uint64_t *)conf_buffer_addr;

    // minion counters: Each hart configures all counters so that we measure events for all the harts
    uint64_t *hart_minion_cfg_data =
        conf_buffer + PMU_EVENT_SHIRE_AREA * shire_id + PMU_MINION_COUNTERS_PER_HART * odd_hart;
    for (uint64_t i = 0; i < PMU_MINION_COUNTERS_PER_HART; i++) {
        ret = ret + configure_neigh_event(*hart_minion_cfg_data, PMU_MHPMEVENT3 + i);
    }

    // neigh counters: Only one minion per neigh needs to configure the counters
    uint64_t *hart_neigh_cfg_data = conf_buffer + PMU_EVENT_SHIRE_AREA * shire_id +
                                    PMU_MINION_COUNTERS_PER_HART * 2 +
                                    PMU_NEIGH_COUNTERS_PER_HART * odd_hart;
    if (program_neigh_harts) {
        ret = ret + configure_neigh_event(*hart_neigh_cfg_data, PMU_MHPMEVENT7);
        ret = ret + configure_neigh_event(*hart_neigh_cfg_data, PMU_MHPMEVENT8);
    }

    // sc location
    uint64_t *hart_sc_cfg_data = conf_buffer + PMU_EVENT_SHIRE_AREA * shire_id +
                                 PMU_MINION_COUNTERS_PER_HART * 2 +
                                 PMU_NEIGH_COUNTERS_PER_HART * 2 + odd_hart;
    if (program_sc_harts) {
        // One bank per neigh
        ret = ret + configure_sc_event(shire_id, neigh_id, odd_hart, *hart_sc_cfg_data);
    }

    // Shire id's 0-7 just to simplify code initialize ms-id's 0-7.
    // TBD: Use shires closer to memshires
    // Currently last hart on each neigh of shires 0-7 programs memshire pref ctrl registers,
    uint64_t *hart_ms_cfg_data =
        conf_buffer + PMU_EVENT_MEMSHIRE_OFFSET + shire_id * PMU_MS_COUNTERS_PER_MS;
    if (program_ms_harts) {
        ret = ret + configure_ms_event(shire_id, neigh_id, *hart_ms_cfg_data);
    }

    if (reset_counters) {
        ret = ret + reset_pmcs();
    }

    return ret;
}

// Reset PMCs
// Each of harts 0-11 reset one neigh counter
// Harts 12-13 reset SC bank PMCs
// Hart 15 of neigh 0 of shires 0-7 resets MS PMCs.
int64_t reset_pmcs(void)
{
    int64_t ret = 0;
    uint64_t hart_id = get_hart_id();
    uint64_t neigh_minion_id = (hart_id >> 1) & 0x7;
    uint64_t neigh_id = (hart_id >> 4) & 0x3;
    uint64_t shire_id = (hart_id >> 6) & 0x1F;
    uint64_t reset_sc_harts = ((hart_id & 0xF) == 0xC) || ((hart_id & 0xF) == 0xD);
    uint64_t reset_ms_harts = (shire_id < 8) && (neigh_id == 0) && ((hart_id & 0x1F) == 0x1F);

    if (neigh_minion_id < PMU_MINION_COUNTERS_PER_HART + PMU_NEIGH_COUNTERS_PER_HART) {
        ret = ret + reset_neigh_pmc(PMU_MHPMCOUNTER3 + neigh_minion_id);
    }

    if (reset_sc_harts) {
        reset_sc_pmcs(shire_id, neigh_id);
    }

    if (reset_ms_harts) {
        reset_ms_pmcs(shire_id);
    }

    return ret;
}

// Sample PMCs
// Each of harts 0-11 read one neigh PMC
// Harts 12-14 read shire cache PMCs
// Hart 15 of neighs 0-2, shires 0-7 read memshire PMCs
int64_t sample_pmcs(uint64_t reset_counters, uint64_t log_buffer_addr)
{
    int64_t ret = 0;
    uint64_t hart_id = get_hart_id();
    uint64_t neigh_id = (hart_id >> 4) & 0x7;
    uint64_t shire_id = (hart_id >> 6) & 0x1F;
    uint64_t read_sc_harts = (((hart_id & 0xF) >= 0xC) && ((hart_id & 0xF) <= 0xE));
    uint64_t read_ms_harts = ((hart_id & 0xF) == 0xF) && (neigh_id < 3) && (shire_id < 8);

    // We assume log buffer is available.
    // Here give it a hard coded value
    // Should it be: MRT_TRACE_CONTROL_BASE + MRT_TRACE_CONTROL_SIZE + (HART_ID * SIZE PER HART)
    // Is the buffer a unit64, or should it be a structure ?
    if (log_buffer_addr == 0) {
        return -1;
    }
    uint64_t *log_buffer = (uint64_t *)log_buffer_addr;
    uint64_t neigh_minion_id = (hart_id >> 1) & 0x7;

    // Minion and neigh PMCs
    if (neigh_minion_id < PMU_MINION_COUNTERS_PER_HART + PMU_NEIGH_COUNTERS_PER_HART) {
        uint64_t pmc_data = read_neigh_pmc(PMU_MHPMCOUNTER3 + neigh_minion_id);
        if (pmc_data == PMU_INCORRECT_COUNTER) {
            ret = ret - 1;
        }
        *(log_buffer + hart_id * 8) = pmc_data;
        //log_buffer++;
    }

    // SC PMCs
    if (read_sc_harts) {
        uint64_t pmc = (hart_id & 0xF) - 0xC;
        stop_sc_pmcs(shire_id, neigh_id);
        uint64_t pmc_data = sample_sc_pmcs(shire_id, neigh_id, pmc);
        if (pmc_data == PMU_INCORRECT_COUNTER) {
            ret = ret - 1;
        }
        *(log_buffer + hart_id * 8) = pmc_data;
        //log_buffer++;
    }

    // MS PMCs
    if (read_ms_harts) {
        stop_ms_pmcs(shire_id);
        uint64_t pmc_data = sample_ms_pmcs(shire_id, neigh_id);
        if (pmc_data == PMU_INCORRECT_COUNTER) {
            ret = ret - 1;
        }
        *(log_buffer + hart_id * 8) = pmc_data;
        //log_buffer++;
    }

    if (reset_counters) {
        ret = ret + reset_pmcs();
    }

    return ret;
}
