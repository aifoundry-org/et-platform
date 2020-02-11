/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_PROFILING_H
#define BEMU_PROFILING_H

#include <cstdint>

#define PROFILING_WRITE_PC(thread_id, pc) \
	profiling_write_pc(current_thread, pc)

void profiling_init();
void profiling_fini();
void profiling_flush();
void profiling_dump(const char *filename);
void profiling_write_pc(int thread_id, uint64_t pc);

#endif // BEMU_PROFILING_H
