#ifndef _LOG_H
#define _LOG_H

#include "emu_defines.h"
#include "inttypes.h"

// Instruction state change
typedef struct
{
    bool     pc_mod;            // Is a jump instruction
    uint64_t pc;                // PC of instruction being executed
    uint32_t inst_bits;         // Bits of instruction being executed
    bool     int_reg_mod;       // If an integer register was modified by instruction
    int      int_reg_rd;        // The destination integer register
    uint64_t int_reg_data;      // Value written to the integer register
    bool     fp_reg_mod;        // If a float register was modified by instruction
    int      fp_reg_rd;         // The destination floating register
    bool     fflags_mod;        // Fcsr flags were updated by this instruction
    uint64_t fflags_value;      // Fcsr flags new value
    uint64_t fp_reg_data[VL/2]; // Value written to the floating register
    bool     m_reg_mod[8];      // If a mask register was modified by instruction
    uint8_t  m_reg_data[8][VL]; // Value written to the mask register
    bool     mem_mod[VL];       // If a memory position was updated
    int      mem_size[VL];      // Size of the memory update
    uint64_t mem_addr[VL];      // Address being updated
    uint64_t mem_data[VL];      // New contents
    bool     tensor_mod;        // Tensor op produced new values
    int      tl_transform;      // 0: No transform, 1: Interleave 2: Transpose
} inst_state_change;

// Changes done by instruction
void logpcchange(uint64_t new_pc);
void logxregchange(int xdst);
void logfregchange(int fdst);
void logmregchange(int mdst);
void logmemwchange(int pos, int size, uint64_t addr, uint64_t val);
void logfflagschange(uint64_t new_fflags);
void logtensorchange(int trans);

// Support for log state
extern void setlogstate(inst_state_change * log_info_);
extern void clearlogstate();

#endif // _LOG_H
