/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

// LCOV_EXCL_START
#include <array>
#include <cassert>
#include <cfenv>        // FIXME: remove this when we purge std::fesetround() from the code!

//#include "decode.h"
#include "emu.h"
#include "emu_gio.h"
#include "esrs.h"
#include "log.h"
#include "memory/main_memory.h"
#include "processor.h"
#ifdef SYS_EMU
#include "sys_emu.h"
#endif
#include "traps.h"
#include "utility.h"

namespace bemu {

extern void reset_msg_ports(unsigned);

// Memory state
MainMemory memory{};
typename MemoryRegion::reset_value_type memory_reset_value = {0};

// Hart state
std::array<Processor,EMU_NUM_THREADS>  cpu;

system_version_t sysver = system_version_t::UNKNOWN;

uint32_t current_inst = 0;
unsigned current_thread = 0;

bool m_emu_done = false;

bool emu_done()
{
   return m_emu_done;
}

void init_emu(system_version_t ver)
{
    sysver = ver;
   // FIXME: remove '#include <cfenv>' when we purge this function from the code
   std::fesetround(FE_TONEAREST); // set rne for host
}

void reset_esrs_for_shire(unsigned shireid)
{
    unsigned shire = (shireid == IO_SHIRE_ID) ? EMU_IO_SHIRE_SP : shireid;
    unsigned neigh_count = (shire == EMU_IO_SHIRE_SP) ? 1 : EMU_NEIGH_PER_SHIRE;

    for (unsigned neigh = 0; neigh < neigh_count; ++neigh) {
        unsigned idx = EMU_NEIGH_PER_SHIRE*shire + neigh;
        bemu::neigh_esrs[idx].reset();
    }
    bemu::shire_cache_esrs[shire].reset();
    bemu::shire_other_esrs[shire].reset(shireid);
    bemu::broadcast_esrs[shire].reset();

    // reset FCC for all threads in shire
    unsigned thread0 = shire * EMU_THREADS_PER_SHIRE;
    unsigned threadN = thread0 + (shire == EMU_IO_SHIRE_SP ? 1 : EMU_THREADS_PER_SHIRE);
    for (unsigned thread = thread0; thread < threadN; ++thread) {
        cpu[thread].fcc[0] = 0;
        cpu[thread].fcc[1] = 0;
    }
}

// forward declarations

// Messaging extension
int64_t port_get(unsigned id, bool block);
uint32_t legalize_portctrl(uint32_t wdata);
void configure_port(unsigned id, uint32_t wdata);
uint64_t read_port_base_address(unsigned thread, unsigned id);

////////////////////////////////////////////////////////////////////////////////
//
// Helper routines
//
////////////////////////////////////////////////////////////////////////////////

void init(xreg dst, uint64_t val)
{
    if (dst != x0)
    {
       cpu[current_thread].xregs[dst] = val;
       LOG(DEBUG, "init x%d <-- 0x%016" PRIx64, dst, val);
    }
}

void fpinit(freg dst, uint64_t val[VLEND])
{
    for (size_t i = 0; i < VLEND; ++i)
        cpu[current_thread].fregs[dst].u64[i] = val[i];
}

void reset_hart(unsigned thread)
{
    // Register files
    cpu[thread].xregs[0] = 0;

    // PC
    cpu[thread].pc = 0;
    cpu[thread].npc = 0;

    // RISCV control and status registers
    cpu[thread].scounteren = 0;
    cpu[thread].mstatus = 0x0000000A00001800ULL; // mpp=11, sxl=uxl=10
    cpu[thread].medeleg = 0;
    cpu[thread].mideleg = 0;
    cpu[thread].mie = 0;
    cpu[thread].mcounteren = 0;
    for (auto &elem : cpu[thread].mhpmevent) {
        elem = 0;
    }
    cpu[thread].mcause = 0;
    cpu[thread].mip = 0;
    cpu[thread].tdata1 = 0x20C0000000000000ULL;
    // TODO: cpu[thread].dcsr <= xdebugver=1, prv=3;
    cpu[thread].mhartid = (thread == (EMU_IO_SHIRE_SP*EMU_THREADS_PER_SHIRE))
                        ? (IO_SHIRE_ID*EMU_THREADS_PER_SHIRE)
                        : thread;

    // Esperanto control and status registers
    cpu[thread].matp = 0;
    cpu[thread].minstmask = 0;
    cpu[thread].minstmatch = 0;
    // TODO: cpu[thread].amofence_ctrl <= ...
    cpu[thread].menable_shadows = 0;
    cpu[thread].excl_mode = 0;
    cpu[thread].mcache_control = 0;
    cpu[thread].ucache_control = 0x200;
    cpu[thread].gsc_progress = 0;
    for (auto &elem : cpu[thread].portctrl) {
        elem = 0x8000;
    }

    // Other hart internal (microarchitectural or hidden) state
    cpu[thread].prv = PRV_M;

    // Pre-computed state to improve simulation speed
    cpu[thread].break_on_load = false;
    cpu[thread].break_on_store = false;
    cpu[thread].break_on_fetch = false;

    // Reset tensor operation state machines
    cpu[thread].reduce.count = 0;
    cpu[thread].reduce.state = Processor::Reduce::State::Idle;
    cpu[thread].wait.state = Processor::Wait::State::Idle;
    cpu[thread].txquant = 0xFFFFFFFFFFFFFFFFULL;
    cpu[thread].shadow_txquant = 0xFFFFFFFFFFFFFFFFULL;
    cpu[thread].txfma = 0xFFFFFFFFFFFFFFFFULL;
    cpu[thread].shadow_txfma = 0xFFFFFFFFFFFFFFFFULL;
    cpu[thread].txload.fill(0xFFFFFFFFFFFFFFFFULL);
    cpu[thread].shadow_txload.fill(0xFFFFFFFFFFFFFFFFULL);
    cpu[thread].tensor_op.fill(Processor::Tensor::None);

    // Other processor state outside of cpu[thread]
    reset_msg_ports(thread);
    cpu[thread].debug_mode = false;
    cpu[thread].ext_seip = 0;
    cpu[thread].mtvec_is_set = false;
    cpu[thread].stvec_is_set = false;
    cpu[thread].fcc_wait = false;
    cpu[thread].fcc_cnt = 0;
    cpu[thread].tensorload_setupb_topair = false;
}

void minit(mreg dst, uint64_t val)
{
    cpu[current_thread].mregs[dst] = val;
    LOG(DEBUG, "init m[%d] <-- 0x%02lx", int(dst), cpu[current_thread].mregs[dst].to_ulong());
}

void set_pc(uint64_t pc)
{
    cpu[current_thread].pc = sextVA(pc);
}

void set_thread(unsigned thread)
{
    current_thread = thread;
}

unsigned get_thread()
{
    return current_thread;
}

bool thread_is_blocked(unsigned thread)
{
    unsigned other_excl = 1 + ((~thread & 1) << 1);
    return cpu[thread].excl_mode == other_excl;
}

uint32_t get_mask(unsigned maskNr)
{
    return cpu[current_thread].mregs[maskNr].to_ulong();
}

void pu_plic_interrupt_pending_set(uint32_t source_id)
{
    bemu::memory.pu_io_space.pu_plic.interrupt_pending_set(source_id);
}

void pu_plic_interrupt_pending_clear(uint32_t source_id)
{
    bemu::memory.pu_io_space.pu_plic.interrupt_pending_clear(source_id);
}

// LCOV_EXCL_STOP


} // namespace bemu
