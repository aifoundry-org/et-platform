#ifndef LOGH
#define LOGH

#include "emu.h"

// Instruction state change
typedef struct
{
    bool   pc_mod;           // Is a jump instruction
    uint64 pc;               // PC of instruction being executed
    bool   int_reg_mod;      // If an integer register was modified by instruction
    int    int_reg_rd;       // The destination integer register
    uint64 int_reg_data;     // Value written to the integer register
    bool   fp_reg_mod;       // If a float register was modified by instruction
    int    fp_reg_rd;        // The destination floating register
    uint64 fp_reg_data[2];   // Value written to the floating register
    bool   m_reg_mod[8];     // If a mask register was modified by instruction
    uint8  m_reg_data[8][8]; // Value written to the mask register
    bool   mem_mod[4];       // If a memory position was updated
    int    mem_size[4];      // Size of the memory update
    uint64 mem_addr[4];      // Address being updated
    uint64 mem_data[4];      // New contents
} inst_state_change;

// Changes done by instruction
void logpcchange(uint64 new_pc);
void logxregchange(int xdst);
void logfregchange(int fdst);
void logmregchange(int mdst);
void logmemwchange(int pos, int size, uint64 addr, uint64 val);

// Support for log state
extern "C" void setlogstate(inst_state_change * log_info_);
extern "C" void clearlogstate();

#endif

