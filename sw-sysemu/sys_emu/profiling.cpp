/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include <unordered_map>
#include <iostream>
#include <fstream>
#include "emu_defines.h"
#include "emu_gio.h"
#include "sys_emu.h"
#include "profiling.h"

struct histogram_entry_t {
    uint64_t count;    // How manu times a BB has been entered
    uint64_t duration; // How long a BB lasts (cycles, retired instructions)
};

// Maps PCs (BBs) to histogram entries
static std::unordered_map<uint64_t, histogram_entry_t> histogram[EMU_NUM_THREADS];
static uint64_t last_event_pc[EMU_NUM_THREADS];
static uint64_t last_event_time[EMU_NUM_THREADS];

static sys_emu* g_sys_emu = nullptr;

void profiling_init(sys_emu* emu)
{
    g_sys_emu = emu;
    for (int i = 0; i < EMU_NUM_THREADS; i++)
        last_event_time[i] = UINT64_MAX;
}

void profiling_fini(void)
{
    for (int i = 0; i < EMU_NUM_THREADS; i++)
        histogram[i].clear();
}

void profiling_flush()
{
    uint64_t event_time = g_sys_emu->get_emu_cycle();

    for (int i = 0; i < EMU_NUM_THREADS; i++) {
        if (last_event_time[i] != UINT64_MAX) {
            uint64_t delta = event_time - last_event_time[i];
            histogram[i][last_event_pc[i]].duration += delta;
        }
    }
}

void profiling_dump(const char *filename)
{
    std::ofstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Profiling: Cannot open " << filename << std::endl;
        return;
    }

    for (int i = 0; i < EMU_NUM_THREADS; i++) {
        for (auto &entry: histogram[i]) {
            file << i << " " << std::hex << entry.first << " " << std::dec
                 << entry.second.count << " " << entry.second.duration << std::endl;
        }
    }
}

void profiling_write_pc(int thread_id, uint64_t pc)
{
    uint64_t event_time = g_sys_emu->get_emu_cycle();

    // If not the first event, accumulate the delta time to the previous BB
    if (last_event_time[thread_id] != UINT64_MAX) {
        uint64_t delta = event_time - last_event_time[thread_id];
        histogram[thread_id][last_event_pc[thread_id]].duration += delta;
    }

    histogram[thread_id][pc].count++;
    last_event_time[thread_id] = event_time;
    last_event_pc[thread_id] = pc;
}
