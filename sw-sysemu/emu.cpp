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

// Memory state
MainMemory memory{};
typename MemoryRegion::reset_value_type memory_reset_value = {0};

// Hart state
std::array<Hart,EMU_NUM_THREADS>  cpu;
std::array<Core,EMU_NUM_MINIONS>  core;

system_version_t sysver = system_version_t::UNKNOWN;

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

   for (unsigned tid = 0; tid < EMU_NUM_THREADS; ++tid) {
       unsigned cid = tid / EMU_THREADS_PER_MINION;
       cpu[tid].core = &core[cid];
   }
}

void reset_esrs_for_shire(unsigned shireid)
{
    unsigned shire = shire_index(shireid);
    unsigned neigh_count = (shire == EMU_IO_SHIRE_SP) ? 1 : EMU_NEIGH_PER_SHIRE;

    for (unsigned neigh = 0; neigh < neigh_count; ++neigh) {
        unsigned idx = EMU_NEIGH_PER_SHIRE*shire + neigh;
        neigh_esrs[idx].reset();
    }
    shire_cache_esrs[shire].reset();
    shire_other_esrs[shire].reset(shireid);
    broadcast_esrs[shire].reset();

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
void configure_port(Hart&, unsigned, uint32_t);

////////////////////////////////////////////////////////////////////////////////
//
// Helper routines
//
////////////////////////////////////////////////////////////////////////////////

void reset_hart(unsigned thread)
{
    // Register files
    cpu[thread].xregs[x0] = 0;

    // PC
    cpu[thread].pc = 0;
    cpu[thread].npc = 0;

    // Currently executing instruction
    cpu[thread].inst = insn_t { 0, 0 };

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
    cpu[thread].mhartid = (thread == EMU_IO_SHIRE_SP_THREAD)
                        ? IO_SHIRE_SP_HARTID
                        : thread;

    // Esperanto control and status registers
    cpu[thread].minstmask = 0;
    cpu[thread].minstmatch = 0;
    // TODO: cpu[thread].amofence_ctrl <= ...
    cpu[thread].gsc_progress = 0;

    // Port control
    for (unsigned p = 0; p < cpu[thread].portctrl.size(); ++p) {
        configure_port(cpu[thread], p, 0);
    }

    // Other hart internal (microarchitectural or hidden) state
    cpu[thread].prv = PRV_M;
    cpu[thread].debug_mode = false;
    cpu[thread].ext_seip = 0;
    cpu[thread].mtvec_is_set = false;
    cpu[thread].stvec_is_set = false;
    cpu[thread].fcc_wait = false;
    cpu[thread].fcc_cnt = 0;

    // Pre-computed state to improve simulation speed
    cpu[thread].break_on_load = false;
    cpu[thread].break_on_store = false;
    cpu[thread].break_on_fetch = false;

    // Reset tensor operation state machines
    cpu[thread].wait.state = Hart::Wait::State::Idle;
    cpu[thread].txload.fill(0xFFFFFFFFFFFFFFFFULL);
    cpu[thread].shadow_txload.fill(0xFFFFFFFFFFFFFFFFULL);

    // Reset core-shared state
    cpu[thread].core->matp = 0;
    cpu[thread].core->menable_shadows = 0;
    cpu[thread].core->excl_mode = 0;
    cpu[thread].core->mcache_control = 0;
    cpu[thread].core->ucache_control = 0x200;
    for (auto& set : cpu[thread].core->scp_lock) {
        set.fill(false);
    }
 
    // Reset core-shared tensor operation state machines
    cpu[thread].core->reduce.count = 0;
    cpu[thread].core->reduce.state = Core::Reduce::State::Idle;
    cpu[thread].core->txquant = 0xFFFFFFFFFFFFFFFFULL;
    cpu[thread].core->shadow_txquant = 0xFFFFFFFFFFFFFFFFULL;
    cpu[thread].core->txfma = 0xFFFFFFFFFFFFFFFFULL;
    cpu[thread].core->shadow_txfma = 0xFFFFFFFFFFFFFFFFULL;
    cpu[thread].core->tensor_op.fill(Core::Tensor::None);
    cpu[thread].core->tensorload_setupb_topair = false;
}

bool thread_is_blocked(unsigned thread)
{
    unsigned other_excl = 1 + ((~thread & 1) << 1);
    return cpu[thread].core->excl_mode == other_excl;
}

void pu_plic_interrupt_pending_set(uint32_t source_id)
{
    memory.pu_io_space.pu_plic.interrupt_pending_set(source_id);
}

void pu_plic_interrupt_pending_clear(uint32_t source_id)
{
    memory.pu_io_space.pu_plic.interrupt_pending_clear(source_id);
}

// LCOV_EXCL_STOP


} // namespace bemu
