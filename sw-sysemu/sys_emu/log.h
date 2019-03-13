#ifndef _LOG_H
#define _LOG_H

#include <cstdint>
#include <cassert>
#include "emu_defines.h"

struct inst_state_change {
    bool     pc_mod;    // Is a jump instruction
    uint64_t pc;        // PC of instruction being executed
};

extern inst_state_change emu_state_change;

// We need this to be functional so that we can emulate branches
inline void log_pc_update(uint64_t new_pc)
{
    assert(~new_pc & 1);
    emu_state_change.pc_mod = true;
    emu_state_change.pc = new_pc;
}

inline void clearlogstate()
{
    emu_state_change.pc_mod = false;
    emu_state_change.pc = 0ull;
}

// There is no checker so we make this an empty interface
inline void log_xreg_write(int, uint64_t) {}
inline void log_freg_write(int, const freg_t&) {}
inline void log_mreg_write(int, const mreg_t&) {}
inline void log_fflags_write(uint64_t) {}
inline void log_mem_write(int, int, uint64_t, uint64_t) {}
inline void log_trap() {}
inline void log_gsc_progress(uint64_t, bool = false) {}

// TensorLoad
inline void log_tensor_load(uint8_t, uint8_t, uint8_t, uint64_t) {}
inline void log_tensor_load_scp_write(uint8_t, const uint64_t*) {}

// TensorFMA
inline void log_tensor_fma_new_pass() {}
inline void log_tensor_fma_write(int, int, int, uint32_t) {}

// TensorQuant
inline void log_tensor_quant_new_transform(bool = false) {}
inline void log_tensor_quant_write(int, int, int, uint32_t) {}

// TensorReduce
inline void log_tensor_reduce(uint8_t, uint8_t) {}
inline void log_tensor_reduce_write(uint8_t, uint8_t, uint32_t) {}

#endif // _LOG_H
