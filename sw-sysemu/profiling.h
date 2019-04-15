/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */
#ifndef BEMU_PROFILING_H
#define BEMU_PROFILING_H

#include <cstdint>

#ifdef BEMU_PROFILING
#define PROFILING_WRITE_PC(thread_id, pc) profiling_write_pc(current_thread, pc)
#else
#define PROFILING_WRITE_PC(thread_id, pc) do {} while(0)
#endif

void profiling_init();
void profiling_fini();
void profiling_dump(const char *filename);
void profiling_write_pc(int thread_id, uint64_t pc);


#endif // BEMU_PROFILING_H
