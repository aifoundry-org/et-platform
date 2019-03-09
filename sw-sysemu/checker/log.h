#ifndef _LOG_H
#define _LOG_H

#include <cstdint>
#include "emu_defines.h"

// Instruction state change
struct inst_state_change {
    // Was any state modified?
    bool     pc_mod;              // Is a jump instruction
    bool     int_reg_mod;         // If an integer register was modified by instruction
    bool     fp_reg_mod;          // If a float register was modified by instruction
    bool     fflags_mod;          // Fcsr flags were updated by this instruction
    bool     m_reg_mod[MAXMREG];  // If a mask register was modified by instruction
    bool     mem_mod[VL];         // If a memory position was updated

    // Which state was modified?
    int      int_reg_rd;        // The destination integer register
    int      fp_reg_rd;         // The destination floating register
    int      mem_size[VL];      // Size of the memory update

    uint32_t inst_bits;         // Bits of instruction being executed

    // What are the new values?
    uint64_t pc;                      // PC of instruction being executed
    uint64_t int_reg_data;            // Value written to the integer register
    uint64_t fp_reg_data[VL/2];       // Value written to the floating register
    uint64_t fflags_value;            // Fcsr flags new value
    uint8_t  m_reg_data[MAXMREG][VL]; // Value written to the mask register
    uint64_t mem_addr[VL];            // Address being updated
    uint64_t mem_data[VL];            // New contents
    uint64_t mem_paddr;               // Physical address (used to check for ignored regions)

    // TensorLoad state changes
    bool      tensor_mod;    // Tensor load produced new values
    uint8_t   tl_transform;  // 0: No transform, 1: Interleave 2: Transpose
    uint8_t   tl_scp_first;
    uint8_t   tl_scp_count;
    uint16_t  tl_conv_skip;

    // TensorFMA state changes
    uint8_t  tensorfma_passes;
    uint32_t tensorfma_mod[TFMA_MAX_ACOLS];
    bool     tensorfma_mask[TFMA_MAX_ACOLS][MAXFREG][VL];

    // TensorQuant state changes
    uint8_t  tensorquant_trans;
    bool     tensorquant_skip_write[TQUANT_MAX_TRANS];
    uint32_t tensorquant_mod[TQUANT_MAX_TRANS];
    bool     tensorquant_mask[TQUANT_MAX_TRANS][MAXFREG][VL];

    // TensorReduce state changes
    uint8_t  tensorreduce_first;
    uint8_t  tensorreduce_count;

    // TensorLoad, TensorFMA, TensorQuant, and TensorReduce values
    uint32_t tensordata[TQUANT_MAX_TRANS > TFMA_MAX_ACOLS ? TQUANT_MAX_TRANS : TFMA_MAX_ACOLS][MAXFREG][VL];

    // Trap CSR changes
    bool     trap_mod;      // The instruction trap
    uint64_t mstatus;       // mstatus
    uint64_t cause;         // [m|s]cause
    uint64_t mip;           // mip
    uint64_t tval;          // [m|s]tval
    uint64_t epc;           // [m|s]epc
    uint64_t prv;           // prv
  //uint64_t tvec;          // vec
    bool     gsc_progress_mod;
    uint64_t gsc_progress;  // in case that we trap during a gather/scatter we need to save gsc_progress csr to check it
};

// Notify changes to state by an instruction
void log_pc_update(uint64_t new_pc);
void log_xreg_write(int xdst, const xdata& xval);
void log_freg_write(int fdst, const fdata& fval);
void log_mreg_write(int mdst, const mdata& mval);
void log_fflags_write(uint64_t new_fflags);
void log_mem_write(int pos, int size, uint64_t addr, uint64_t val);
void log_trap();
void log_gsc_progress(uint64_t gsc_progress, bool success = false);

// TensorLoad
void log_tensor_load(uint8_t trans, uint8_t first, uint8_t count, uint64_t conv_mask);
void log_tensor_load_scp_write(uint8_t line, const uint64_t* data);

// TensorFMA
void log_tensor_fma_new_pass();
void log_tensor_fma_write(int pass, int freg, int elem, uint32_t value);

// TensorQuant
void log_tensor_quant_new_transform(bool skip_write = false);
void log_tensor_quant_write(int trans, int freg, int elem, uint32_t value);

// TensorReduce
void log_tensor_reduce(uint8_t first, uint8_t count);
void log_tensor_reduce_write(uint8_t freg, uint8_t elem, uint32_t value);

// Support for log state
void setlogstate(inst_state_change * log_info_);
void clearlogstate();

#endif // _LOG_H
