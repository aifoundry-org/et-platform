/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef _EMU_H
#define _EMU_H

#ifndef SYS_EMU
#include <queue>
#endif

#include "emu_defines.h"
#include "memory/main_memory.h"

namespace bemu {


extern MainMemory memory;

// Configure the emulation environment
extern void init_emu(system_version_t);

// Reset state
extern void reset_esrs_for_shire(unsigned shireid);
extern void reset_hart(unsigned thread);

// Helpers
extern bool emu_done();
extern uint64_t get_csr(unsigned thread, uint16_t cnum);
extern void set_csr(unsigned thread, uint16_t cnum, uint64_t data);

// Hart state manipulation
extern bool thread_is_blocked(unsigned thread);

// Main memory accessors
extern void set_msg_funcs(void (*func_msg_to_thread) (unsigned));

// FIXME: This should be made internal to the checker
#ifndef SYS_EMU
std::queue<uint32_t>& get_minions_to_awake();
#endif


} // namespace bemu

#endif // _EMU_H
