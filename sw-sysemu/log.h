#ifndef _LOG_H
#define _LOG_H

#include "emu_defines.h"
#include "inttypes.h"

// Instruction state change
struct inst_state_change {
    // Was any state modified?
    bool     pc_mod;            // Is a jump instruction
    bool     int_reg_mod;       // If an integer register was modified by instruction
    bool     fp_reg_mod;        // If a float register was modified by instruction
    bool     fflags_mod;        // Fcsr flags were updated by this instruction
    bool     m_reg_mod[8];      // If a mask register was modified by instruction
    bool     mem_mod[VL];       // If a memory position was updated
    bool     tensor_mod;        // Tensor op produced new values

    // Which state was modified?
    int      int_reg_rd;        // The destination integer register
    int      fp_reg_rd;         // The destination floating register
    int      mem_size[VL];      // Size of the memory update
    int      tl_transform;      // 0: No transform, 1: Interleave 2: Transpose

    uint32_t inst_bits;         // Bits of instruction being executed

    // What are the new values?
    uint64_t pc;                // PC of instruction being executed
    uint64_t int_reg_data;      // Value written to the integer register
    uint64_t fp_reg_data[VL/2]; // Value written to the floating register
    uint64_t fflags_value;      // Fcsr flags new value
    uint8_t  m_reg_data[8][VL]; // Value written to the mask register
    uint64_t mem_addr[VL];      // Address being updated
    uint64_t mem_data[VL];      // New contents

    // TensorFMA state changes
    int      tensorfma_passes;
    uint32_t tensorfma_mod[TFMA_MAX_ACOLS];
    bool     tensorfma_mask[TFMA_MAX_ACOLS][MAXFREG][VL];
    uint32_t tensorfma_data[TFMA_MAX_ACOLS][MAXFREG][VL];
};

// Notify changes to state by an instruction
void log_pc_update(uint64_t new_pc);
void log_xreg_write(int xdst);
void log_freg_write(int fdst);
void log_mreg_write(int mdst);
void log_mem_write(int pos, int size, uint64_t addr, uint64_t val);
void log_fflags_write(uint64_t new_fflags);
void log_tensor_load(int trans);
void log_tensor_fma_new_pass();
void log_tensor_fma_write(int pass, int freg, int elem, uint32_t value);

// Support for log state
extern void setlogstate(inst_state_change * log_info_);
extern void clearlogstate();

#endif // _LOG_H
