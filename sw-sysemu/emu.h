#ifndef _EMU_H
#define _EMU_H

#include <queue>
#include <iomanip>

#include "emu_defines.h"
#include "testLog.h"

#include "emu_casts.h"
#include "emu_gio.h"
#include "emu_memop.h"

#include "fpu/fpu.h"
#include "fpu/fpu_casts.h"
#include "tbox_emu.h"
#include "rbox.h"

namespace TBOX { class TBOXEmu; }
namespace RBOX { class RBOXEmu; }

// Accelerators
#if (EMU_TBOXES_PER_SHIRE > 1)
extern TBOX::TBOXEmu tbox[EMU_NUM_COMPUTE_SHIRES][EMU_TBOXES_PER_SHIRE];
#define GET_TBOX(shire_id, rbox_id) tbox[shire_id][rbox_id]
#else
extern TBOX::TBOXEmu tbox[EMU_NUM_COMPUTE_SHIRES];
#define GET_TBOX(shire_id, tbox_id) tbox[shire_id]
#endif

#if (EMU_RBOXES_PER_SHIRE > 1)
extern RBOX::RBOXEmu rbox[EMU_NUM_COMPUTE_SHIRES][EMU_RBOXES_PER_SHIRE];
#define GET_RBOX(shire_id, rbox_id) rbox[shire_id][rbox_id]
#else
extern RBOX::RBOXEmu rbox[EMU_NUM_COMPUTE_SHIRES];
#define GET_RBOX(shire_id, rbox_id) rbox[shire_id]
#endif

// Processor configuration
extern uint8_t in_sysemu;
extern uint32_t current_thread;

// Configure the emulation environment
extern void init_emu();

// Helpers
extern bool emu_done();
extern std::string dump_xregs(uint32_t thread_id);
extern std::string dump_fregs(uint32_t thread_id);
extern void init_stack();
extern void initcsr(uint32_t thread);             // init all CSRs
extern uint64_t get_csr(uint32_t thread, uint16_t cnum);
extern uint64_t xget(uint64_t src1);
extern void init(xreg dst, uint64_t val);         // init general purpose register
extern void fpinit(freg dst, uint64_t val[VL/2]); // init vector register
extern void minit(mreg dst, uint64_t val);        // init mask register

// Processor state manipulation
extern void set_pc(uint64_t pc);
extern void set_thread(uint32_t thread);
extern uint32_t get_thread();
extern uint32_t get_mask(unsigned maskNr);

// Main memory accessors
extern void set_msg_funcs(void (*func_msg_to_thread) (int));

// Traps
extern void take_trap(const trap_t& t);

// Interrupts
extern void check_pending_interrupts();

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

extern void set_delayed_msg_port_write(bool f);
extern bool get_msg_port_stall(uint32_t target_thread, uint32_t port_id);
extern void read_msg_port_data(uint32_t target_thread, uint32_t port_id, uint32_t *data, uint8_t* oob);
extern void write_msg_port_data(uint32_t target_thread, uint32_t port_id, uint32_t *data, uint8_t oob);
extern void write_msg_port_data_from_tbox(uint32_t target_thread, uint32_t port_id, uint32_t tbox_id, uint32_t *data, uint8_t oob);
extern void write_msg_port_data_from_rbox(uint32_t target_thread, uint32_t port_id, uint32_t rbox_id, uint32_t *data, uint8_t oob);
extern void commit_msg_port_data(uint32_t target_thread, uint32_t port_id, uint32_t source_thread);
extern void commit_msg_port_data_from_tbox(uint32_t target_thread, uint32_t port_id, uint32_t tbox_id);
extern void commit_msg_port_data_from_rbox(uint32_t target_thread, uint32_t port_id, uint32_t rbox_id);

// ----- Esperanto tensor extension --------------------------------------------

// TensorReduce

extern void tensor_reduce_decode(uint64_t minion_id, uint64_t value, unsigned* other_min, unsigned* action);

// Shire cooperative mode

extern void write_shire_coop_mode(unsigned shire, uint64_t val);
extern uint64_t read_shire_coop_mode(unsigned shire);

// ----- Esperanto fast local barrier extension --------------------------------

// ----- Esperanto fast credit counter extension --------------------------------

extern uint64_t get_fcc_cnt();
extern void fcc_inc(uint64_t thread, uint64_t shire, uint64_t minion_mask, uint64_t fcc_id);
std::queue<uint32_t> &get_minions_to_awake();

// ----- Esperanto IPI extension ------------------------------------------------

extern void raise_interrupt(int thread, int cause);
extern void raise_software_interrupt(int thread);
extern void clear_software_interrupt(int thread);

extern void raise_timer_interrupt(int thread);
extern void clear_timer_interrupt(int thread);

extern void raise_external_interrupt(int thread);
extern void clear_external_interrupt(int thread);

// ----- Esperanto code prefetching extension -----------------------------------

extern void write_icache_prefetch(int privilege, unsigned shire, uint64_t val);

extern uint64_t read_icache_prefetch(int privilege, unsigned shire);

extern void finish_icache_prefetch(unsigned shire);

#endif // _EMU_H
