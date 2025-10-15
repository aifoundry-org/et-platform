/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#ifndef BEMU_PROFILING_H
#define BEMU_PROFILING_H

#include <cstdint>

class sys_emu;

void profiling_init(sys_emu* emu);
void profiling_fini();
void profiling_flush();
void profiling_dump(const char *filename);
void profiling_write_pc(int thread_id, uint64_t pc);

#endif // BEMU_PROFILING_H
