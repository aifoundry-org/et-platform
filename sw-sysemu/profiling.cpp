#include <unordered_map>
#include <iostream>
#include <fstream>
#include "emu_defines.h"
#include "emu_gio.h"
#include "profiling.h"

static std::unordered_map<uint64_t, uint64_t> histogram[EMU_NUM_THREADS];

void profiling_init(void)
{
}

void profiling_fini(void)
{
}

void profiling_dump(const char *filename)
{
    std::ofstream file(filename);

    if (!file.is_open()) {
        emu::log << LOG_ERR << "cannot open " << filename << endm;
        return;
    }

    for (int i = 0; i < EMU_NUM_THREADS; i++) {
        for (auto &entry: histogram[i]) {
            file << i << " " << std::hex << entry.first << " " << std::dec << entry.second << std::endl;
        }
    }
}

void profiling_write_pc(int thread_id, uint64_t pc)
{
	histogram[thread_id][pc]++;
}
