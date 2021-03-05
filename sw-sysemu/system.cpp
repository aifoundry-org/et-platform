/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include <algorithm>
#include <cfenv>        // FIXME: remove this when we purge std::fesetround() from the code!
#include <cstring>
#include <fstream>

#include "elfio/elfio.hpp"
#include "emu_gio.h"
#include "system.h"
#include "insns/tensor_error.h"
#ifdef SYS_EMU
#include "sys_emu.h"
#endif

namespace bemu {


// Messaging extension (from msgport.cpp)
void configure_port(Hart&, unsigned, uint32_t);


void System::init(system_version_t ver)
{
    sysver = ver;
    m_emu_done = false;

    // Init memory
    memory.reset();

    // FIXME: remove '#include <cfenv>' when we purge this function from the code
    std::fesetround(FE_TONEAREST);  // set rne for host

    // Init harts & cores
    for (unsigned tid = 0; tid < EMU_NUM_THREADS; ++tid) {
        unsigned cid = tid / EMU_THREADS_PER_MINION;
        cpu[tid].core = &core[cid];
        cpu[tid].chip = this;
    }
}


void System::reset_esrs_for_shire(unsigned shireid)
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

    recalculate_thread0_enable(shire);
    recalculate_thread1_enable(shire);

    // reset FCC for all threads in shire
    unsigned thread0 = shire * EMU_THREADS_PER_SHIRE;
    unsigned threadN = thread0 + (shire == EMU_IO_SHIRE_SP ? 1 : EMU_THREADS_PER_SHIRE);
    for (unsigned thread = thread0; thread < threadN; ++thread) {
        cpu[thread].fcc[0] = 0;
        cpu[thread].fcc[1] = 0;
    }
}


void System::reset_hart(unsigned thread)
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


void System::raise_timer_interrupt(uint64_t shire_mask)
{
#ifdef SYS_EMU
    sys_emu::raise_timer_interrupt(shire_mask);
#else
    (void) shire_mask;
#endif
}


void System::clear_timer_interrupt(uint64_t shire_mask)
{
#ifdef SYS_EMU
    sys_emu::clear_timer_interrupt(shire_mask);
#else
    (void) shire_mask;
#endif
}


void System::raise_external_interrupt(unsigned shire)
{
#ifdef SYS_EMU
    sys_emu::raise_external_interrupt(shire);
#else
    (void) shire;
#endif
}


void System::clear_external_interrupt(unsigned shire)
{
#ifdef SYS_EMU
    sys_emu::clear_external_interrupt(shire);
#else
    (void) shire;
#endif
}


void System::raise_external_supervisor_interrupt(unsigned shire)
{
#ifdef SYS_EMU
    sys_emu::raise_external_supervisor_interrupt(shire);
#else
    (void) shire;
#endif
}


void System::clear_external_supervisor_interrupt(unsigned shire)
{
#ifdef SYS_EMU
    sys_emu::clear_external_supervisor_interrupt(shire);
#else
    (void) shire;
#endif
}


void System::raise_software_interrupt(unsigned shire, uint64_t thread_mask)
{
#ifdef SYS_EMU
    sys_emu::raise_software_interrupt(shire, thread_mask);
#else
    (void) shire;
    (void) thread_mask;
#endif
}


void System::clear_software_interrupt(unsigned shire, uint64_t thread_mask)
{
#ifdef SYS_EMU
    sys_emu::clear_software_interrupt(shire, thread_mask);
#else
    (void) shire;
    (void) thread_mask;
#endif
}


void System::send_ipi_redirect_to_threads(unsigned shire, uint64_t thread_mask)
{
#ifdef SYS_EMU
    sys_emu::send_ipi_redirect_to_threads(shire, thread_mask);
#else
    (void) shire;
    (void) thread_mask;
#endif
}


bool System::raise_host_interrupt(uint32_t bitmap)
{
#ifdef SYS_EMU
    if (bitmap != 0) {
        if (sys_emu::get_api_communicate()) {
            return sys_emu::get_api_communicate()->raise_host_interrupt(bitmap);
        }
        LOG_NOTHREAD(WARN, "%s", "API Communicate is NULL!");
    }
#else
    (void) bitmap;
#endif
    return false;
}


void System::copy_memory_from_host_to_device(uint64_t from_addr, uint64_t to_addr, uint32_t size)
{
#ifdef SYS_EMU
    api_communicate *api_comm = sys_emu::get_api_communicate();
    if (api_comm) {
        uint8_t *buff = new uint8_t[size];
        api_comm->host_memory_read(from_addr, size, buff);
        memory.write(Noagent{this}, to_addr, size, buff);
        delete[] buff;
    } else {
        LOG_NOTHREAD(WARN, "%s", "API Communicate is NULL!");
    }
#else
    (void) from_addr;
    (void) to_addr;
    (void) size;
#endif
}


void System::copy_memory_from_device_to_host(uint64_t from_addr, uint64_t to_addr, uint32_t size)
{
#ifdef SYS_EMU
    api_communicate *api_comm = sys_emu::get_api_communicate();
    if (api_comm) {
        uint8_t *buff = new uint8_t[size];
        memory.read(Noagent{this}, from_addr, size, buff);
        api_comm->host_memory_write(to_addr, size, buff);
        delete[] buff;
    } else {
        LOG_NOTHREAD(WARN, "%s", "API Communicate is NULL!");
    }
#else
    (void) from_addr;
    (void) to_addr;
    (void) size;
#endif
}


void System::notify_iatu_ctrl_2_reg_write(int pcie_id, uint32_t iatu, uint32_t value)
{
#ifdef SYS_EMU
    api_communicate *api_comm = sys_emu::get_api_communicate();
    if (api_comm) {
        api_comm->notify_iatu_ctrl_2_reg_write(pcie_id, iatu, value);
    } else {
        LOG_NOTHREAD(WARN, "%s", "API Communicate is NULL!");
    }
#else
    (void) pcie_id;
    (void) iatu;
    (void) value;
#endif
}


void System::write_fcc_credinc(int index, uint64_t shire, uint64_t minion_mask)
{
    if (shire == EMU_IO_SHIRE_SP)
        throw std::runtime_error("write_fcc_credinc_N for IOShire");

    const unsigned thread_in_minion = index / 2;
    const unsigned thread0 = thread_in_minion + shire * EMU_THREADS_PER_SHIRE;
    const unsigned counter = index % 2;

    for (int minion = 0; minion < EMU_MINIONS_PER_SHIRE; ++minion) {
        if (~minion_mask & (1ull << minion))
            continue;

        unsigned thread = thread0 + minion * EMU_THREADS_PER_MINION;
        cpu[thread].fcc[counter]++;
        LOG_HART(DEBUG, cpu[thread], "\tfcc = 0x%" PRIx64,
                 (uint64_t(cpu[thread].fcc[1]) << 16) + uint64_t(cpu[thread].fcc[0]));
#ifndef SYS_EMU
        // wake up waiting threads (only for checker, not sysemu)
        if (cpu[thread].fcc_wait) {
            cpu[thread].fcc_wait = false;
            m_minions_to_awake.push(thread / EMU_THREADS_PER_MINION);
        }
#endif
        //check for overflow
        if (cpu[thread].fcc[counter] == 0)
            update_tensor_error(cpu[thread], 1 << 3);
    }

#ifdef SYS_EMU
    sys_emu::fcc_to_threads(shire, thread_in_minion, minion_mask, counter);
#endif
}


void System::recalculate_thread0_enable(unsigned shire)
{
    uint32_t value = shire_other_esrs[shire].thread0_disable;
    unsigned mcount = (shire == EMU_IO_SHIRE_SP ? 1 : EMU_MINIONS_PER_SHIRE);
    for (unsigned m = 0; m < mcount; ++m) {
        unsigned thread = shire * EMU_THREADS_PER_SHIRE + m * EMU_THREADS_PER_MINION;
        cpu[thread].enabled = !((value >> m) & 1);
    }
#ifdef SYS_EMU
    sys_emu::recalculate_thread_disable(shire);
#endif
}


void System::recalculate_thread1_enable(unsigned shire)
{
    if (shire == EMU_IO_SHIRE_SP)
        return;

    uint32_t value = (shire_other_esrs[shire].minion_feature & 0x10)
            ? 0xffffffff
            : shire_other_esrs[shire].thread1_disable;
    for (unsigned m = 0; m < EMU_MINIONS_PER_SHIRE; ++m) {
        unsigned thread = shire * EMU_THREADS_PER_SHIRE + m * EMU_THREADS_PER_MINION + 1;
        cpu[thread].enabled = !((value >> m) & 1);
    }
#ifdef SYS_EMU
    sys_emu::recalculate_thread_disable(shire);
#endif
}


void System::load_elf(const char* filename)
{
    std::ifstream file;
    ELFIO::elfio elf;

    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    file.open(filename, std::ios::in | std::ios::binary);
    elf.load(file);
    for (const ELFIO::segment* seg : elf.segments) {
        if (!(seg->get_type() & PT_LOAD))
            continue;

        LOG_NOTHREAD(INFO, "Segment[%d] VA: 0x%" PRIx64 "\tType: 0x%" PRIx32 " (LOAD)",
                     seg->get_index(), seg->get_virtual_address(), seg->get_type());

        uint64_t vma_offset = seg->get_virtual_address() - seg->get_physical_address();

        for (ELFIO::Elf_Half idx = 0; idx < seg->get_sections_num(); ++idx) {
            const ELFIO::section* sec =
                    elf.sections[seg->get_section_index_at(idx)];

            if (!sec->get_size())
                continue;

            if (sec->get_type() == SHT_NOBITS)
                continue;

            uint64_t vma = sec->get_address();
            uint64_t lma = vma - vma_offset;
            LOG_NOTHREAD(INFO, "Section[%d] %s\tVMA: 0x%" PRIx64 "\tLMA: 0x%" PRIx64 "\tSize: 0x%" PRIx64
                         "\tType: 0x%" PRIx32 "\tFlags: 0x%" PRIx64,
                         idx, sec->get_name().c_str(), vma, lma, sec->get_size(),
                         sec->get_type(), sec->get_flags());

            if (lma >= MainMemory::dram_base)
                lma &= ~0x4000000000ULL;

            memory.init(Noagent{this}, lma, sec->get_size(), sec->get_data());
        }
    }
}


void System::load_raw(const char* filename, unsigned long long addr)
{
    std::ifstream file;
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    file.open(filename, std::ios::in | std::ios::binary);
    file.exceptions(std::ifstream::badbit);

    char fbuf[65536];
    while (true) {
        file.read(fbuf, 65536);
        std::streamsize count = file.gcount();
        if (count <= 0)
            break;
        if (addr >= MainMemory::dram_base)
            addr &= ~0x4000000000ULL;
        memory.init(Noagent{this}, addr, count, reinterpret_cast<MainMemory::const_pointer>(fbuf));
        addr += count;
    }
}


} // bemu
