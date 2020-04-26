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

#include <string>
#ifndef SYS_EMU
#include <queue>
#endif
#include "emu_defines.h"

#include "memory/main_memory.h"

// Processor configuration
namespace bemu {
    extern MainMemory memory;
}

// Configure the emulation environment
extern void init_emu(system_version_t);

// Reset state
extern void reset_esrs_for_shire(unsigned shireid);
extern void reset_hart(unsigned thread);

// Helpers
extern bool emu_done();
extern std::string dump_xregs(unsigned thread_id);
extern std::string dump_fregs(unsigned thread_id);
extern uint64_t get_csr(unsigned thread, uint16_t cnum);
extern void set_csr(unsigned thread, uint16_t cnum, uint64_t data);
extern void init(xreg dst, uint64_t val);         // init general purpose register
extern void fpinit(freg dst, uint64_t val[VL/2]); // init vector register
extern void minit(mreg dst, uint64_t val);        // init mask register

// Processor state manipulation
extern void set_thread(unsigned thread);
extern unsigned get_thread();
extern bool thread_is_blocked(unsigned thread);
extern uint32_t get_mask(unsigned maskNr);

// Main memory accessors
extern void set_msg_funcs(void (*func_msg_to_thread) (int));

// FIXME: This should be made internal to the checker
#ifndef SYS_EMU
std::queue<uint32_t>& get_minions_to_awake();
#endif

#endif // _EMU_H
