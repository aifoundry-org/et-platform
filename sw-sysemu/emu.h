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

#include <queue>

#include "memory/main_memory.h"

// Processor configuration
extern unsigned current_thread;
namespace bemu {
    extern MainMemory memory;
    extern typename MemoryRegion::reset_value_type memory_reset_value;
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
extern void init_stack();
extern uint64_t get_csr(unsigned thread, uint16_t cnum);
extern void set_csr(unsigned thread, uint16_t cnum, uint64_t data);
extern uint64_t xget(xreg src1);
extern void init(xreg dst, uint64_t val);         // init general purpose register
extern void fpinit(freg dst, uint64_t val[VL/2]); // init vector register
extern void minit(mreg dst, uint64_t val);        // init mask register

// Processor state manipulation
extern void set_pc(uint64_t pc);
extern void set_thread(unsigned thread);
extern unsigned get_thread();
extern bool thread_is_blocked(unsigned thread);
extern uint32_t get_mask(unsigned maskNr);

// Main memory accessors
extern void set_msg_funcs(void (*func_msg_to_thread) (int));

// Traps
extern void take_trap(const trap_t& t);

// Interrupts
extern void check_pending_interrupts();
extern void raise_interrupt(int thread, int cause, uint64_t mip, uint64_t mbusaddr=0);
extern void raise_software_interrupt(int thread);
extern void clear_software_interrupt(int thread);
extern void raise_timer_interrupt(int thread);
extern void clear_timer_interrupt(int thread);
extern void raise_external_machine_interrupt(int thread);
extern void clear_external_machine_interrupt(int thread);
extern void raise_external_supervisor_interrupt(int thread);
extern void clear_external_supervisor_interrupt(int thread);
extern void raise_bus_error_interrupt(int thread, uint64_t busaddr);
extern void clear_bus_error_interrupt(int thread);
extern void pu_plic_interrupt_pending_set(uint32_t source_id);
extern void pu_plic_interrupt_pending_clear(uint32_t source_id);

// Illegal instruction encodings will execute this
extern void unknown(const char* comm = 0);

// Instruction encodings that match minstmatch/minstmask will execute this
extern void check_minst_match(uint32_t bits);

// ----- SYSTEM emulation ------------------------------------------------------

extern void ecall(const char* comm = 0);
extern void ebreak(const char* comm = 0);
extern void sret(const char* comm = 0);
extern void mret(const char* comm = 0);
extern void wfi(const char* comm = 0);
extern void sfence_vma(xreg src1, xreg src2, const char* comm = 0);
extern void csrrw(xreg dst, uint16_t src1, xreg src2, const char* comm = 0);
extern void csrrs(xreg dst, uint16_t src1, xreg src2, const char* comm = 0);
extern void csrrc(xreg dst, uint16_t src1, xreg src2, const char* comm = 0);
extern void csrrwi(xreg dst, uint16_t src1, uint64_t imm, const char* comm = 0);
extern void csrrsi(xreg dst, uint16_t src1, uint64_t imm, const char* comm = 0);
extern void csrrci(xreg dst, uint16_t src1, uint64_t imm, const char* comm = 0);

// ----- Esperanto atomic extension --------------------------------------------

// ----- Esperanto cache control extension -------------------------------------

// ----- Esperanto messaging extension -----------------------------------------

extern unsigned get_msg_port_write_width(unsigned thread, unsigned port);

extern void set_delayed_msg_port_write(bool f);
extern bool get_msg_port_stall(unsigned target_thread, unsigned port_id);
extern void write_msg_port_data(unsigned target_thread, unsigned port_id, uint32_t *data, uint8_t oob);
extern void write_msg_port_data_from_tbox(unsigned target_thread, unsigned port_id, unsigned tbox_id, uint32_t *data, uint8_t oob);
extern void write_msg_port_data_from_rbox(unsigned target_thread, unsigned port_id, unsigned rbox_id, uint32_t *data, uint8_t oob);
extern void commit_msg_port_data(unsigned target_thread, unsigned port_id, unsigned source_thread);
extern void commit_msg_port_data_from_tbox(unsigned target_thread, unsigned port_id, unsigned tbox_id);
extern void commit_msg_port_data_from_rbox(unsigned target_thread, unsigned port_id, unsigned rbox_id);

// ----- Esperanto tensor extension --------------------------------------------

// TensorFMA

extern void tensor_fma_execute();

// TensorLoad

extern void tensor_load_execute(bool tenb);

// TensorQuant

extern void tensor_quant_execute();

// TensorReduce

extern void tensor_reduce_step(unsigned thread);
extern void tensor_reduce_execute();

// TensorWait

extern void tensor_wait_start(uint64_t value);
extern void tensor_wait_execute();

// Shire cooperative mode

extern void write_shire_coop_mode(unsigned shire, uint64_t val);
extern uint64_t read_shire_coop_mode(unsigned shire);

// ----- Esperanto fast local barrier extension --------------------------------

// ----- Esperanto fast credit counter extension --------------------------------

extern uint64_t get_fcc_cnt();
extern void fcc_inc(uint64_t thread, uint64_t shire, uint64_t minion_mask, uint64_t fcc_id);
std::queue<uint32_t> &get_minions_to_awake();

// ----- Esperanto IPI extension ------------------------------------------------

// ----- Esperanto code prefetching extension -----------------------------------

extern void write_icache_prefetch(int privilege, unsigned shire, uint64_t val);

extern uint64_t read_icache_prefetch(int privilege, unsigned shire);

extern void finish_icache_prefetch(unsigned shire);

#endif // _EMU_H
