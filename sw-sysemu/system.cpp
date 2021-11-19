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
#include "insn_util.h"
#ifdef SYS_EMU
#include "sys_emu.h"
#endif

namespace bemu {


// Messaging extension (from msgport.cpp)
void configure_port(Hart&, unsigned, uint32_t);


void System::init(Stepping ver)
{
    stepping = ver;
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
        // Do this here so that logging messages can show the correct hartid
        cpu[tid].mhartid = (tid == EMU_IO_SHIRE_SP_THREAD)
            ? IO_SHIRE_SP_HARTID
            : tid;
    }
}


void System::reset_shire_state(unsigned shireid)
{
    unsigned shire = (shireid == IO_SHIRE_ID) ? EMU_IO_SHIRE_SP : shireid;
    unsigned neigh_count = (shire == EMU_IO_SHIRE_SP) ? 1 : EMU_NEIGH_PER_SHIRE;

    for (unsigned neigh = 0; neigh < neigh_count; ++neigh) {
        unsigned idx = EMU_NEIGH_PER_SHIRE*shire + neigh;
        neigh_esrs[idx].reset();
        coop_tloads[idx][0].fill(Coop_tload_state{});
        coop_tloads[idx][1].fill(Coop_tload_state{});
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
    // Waiting reasons
    cpu[thread].waits = Hart::Waiting::none;

    // Register files
    cpu[thread].xregs[x0] = 0;

    // PC
    cpu[thread].pc = 0;
    cpu[thread].npc = 0;

    // Currently executing instruction
    cpu[thread].inst = Instruction { 0, 0 };

    // Fetch buffer
    cpu[thread].fetch_pc = -1;

    // RISCV control and status registers
    cpu[thread].scounteren = 0;
    cpu[thread].mstatus = 0x0000000A00001800ULL; // mpp=11, sxl=uxl=10
    cpu[thread].medeleg = 0;
    cpu[thread].mideleg = 0;
    cpu[thread].mie = 0;
    cpu[thread].mcounteren = 0;
    for (auto counter = 0; counter < 6; ++counter) {
        neigh_pmu_events[thread / EMU_THREADS_PER_NEIGH][thread % EMU_THREADS_PER_NEIGH][counter] = PMU_MINION_EVENT_NONE;
    }
    cpu[thread].mcause = 0;
    cpu[thread].mip = 0;
    cpu[thread].tdata1 = 0x20C0000000000000ULL;
    // TODO: cpu[thread].dcsr <= xdebugver=1, prv=3;

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
    cpu[thread].prv = Privilege::M;
    cpu[thread].debug_mode = false;
    cpu[thread].ext_seip = 0;

    // Pre-computed state to improve simulation speed
    cpu[thread].break_on_load = false;
    cpu[thread].break_on_store = false;
    cpu[thread].break_on_fetch = false;

    // Reset core-shared state
    if ((thread % EMU_THREADS_PER_MINION) == 0) {
        cpu[thread].core->matp = 0;
        cpu[thread].core->menable_shadows = 0;
        cpu[thread].core->excl_mode = 0;
        cpu[thread].core->mcache_control = 0;
        cpu[thread].core->ucache_control = 0x200;
        for (auto& set : cpu[thread].core->scp_lock) {
            set.fill(false);
        }

        cpu[thread].core->reduce.state = TReduce::State::idle;
        cpu[thread].core->reduce.hart = &cpu[thread];
        cpu[thread].core->tmul.state = TMul::State::idle;
        cpu[thread].core->tquant.state = TQuant::State::idle;
        cpu[thread].core->tstore.state = TStore::State::idle;

        cpu[thread].core->tload_a[0].state = TLoad::State::idle;
        cpu[thread].core->tload_a[0].paired = false;
        cpu[thread].core->tload_a[1].state = TLoad::State::idle;
        cpu[thread].core->tload_a[1].paired = false;
        cpu[thread].core->tload_b.state = TLoad::State::idle;
        cpu[thread].core->tload_b.paired = false;

        cpu[thread].core->tqueue.clear();
    }
}


void System::raise_machine_timer_interrupt(unsigned shire)
{
#ifdef SYS_EMU
    if (shire == IO_SHIRE_ID) {
        shire = EMU_IO_SHIRE_SP;
    }

    unsigned begin_hart = shire * EMU_THREADS_PER_SHIRE;
    unsigned hart_count = (shire == EMU_IO_SHIRE_SP) ? 1 : EMU_THREADS_PER_SHIRE;
    unsigned end_hart   = begin_hart + hart_count;

    uint32_t mtime_target = (shire == EMU_IO_SHIRE_SP)
        ? 1
        : shire_other_esrs[shire].mtime_local_target;

    for (unsigned thread = begin_hart; thread < end_hart; ++thread) {
        if (!cpu[thread].is_nonexistent()) {
            unsigned minion = thread / EMU_THREADS_PER_MINION;
            if ((mtime_target >> minion) & 1) {
                cpu[thread].raise_interrupt(MACHINE_TIMER_INTERRUPT);
            }
        }
    }
#else
    (void) shire;
#endif // SYS_EMU
}


void System::clear_machine_timer_interrupt(unsigned shire)
{
#ifdef SYS_EMU
    if (shire == IO_SHIRE_ID) {
        shire = EMU_IO_SHIRE_SP;
    }

    unsigned begin_hart = shire * EMU_THREADS_PER_SHIRE;
    unsigned hart_count = (shire == EMU_IO_SHIRE_SP) ? 1 : EMU_THREADS_PER_SHIRE;
    unsigned end_hart   = begin_hart + hart_count;

    uint32_t mtime_target = (shire == EMU_IO_SHIRE_SP)
        ? 1
        : shire_other_esrs[shire].mtime_local_target;

    for (unsigned thread = begin_hart; thread < end_hart; ++thread) {
        if (!cpu[thread].is_nonexistent()) {
            unsigned minion = thread / EMU_THREADS_PER_MINION;
            if ((mtime_target >> minion) & 1) {
                cpu[thread].clear_interrupt(MACHINE_TIMER_INTERRUPT);
            }
        }
    }
#else
    (void) shire;
#endif // SYS_EMU
}


void System::raise_machine_external_interrupt(unsigned shire)
{
#ifdef SYS_EMU
    if (shire == IO_SHIRE_ID) {
        shire = EMU_IO_SHIRE_SP;
    }

    unsigned begin_hart = shire * EMU_THREADS_PER_SHIRE;
    unsigned hart_count = (shire == EMU_IO_SHIRE_SP) ? 1 : EMU_THREADS_PER_SHIRE;
    unsigned end_hart   = begin_hart + hart_count;

    for (unsigned thread = begin_hart; thread < end_hart; ++thread) {
        if (!cpu[thread].is_nonexistent()) {
            cpu[thread].raise_interrupt(MACHINE_EXTERNAL_INTERRUPT);
        }
    }
#else
    (void) shire;
#endif // SYS_EMU
}


void System::clear_machine_external_interrupt(unsigned shire)
{
#ifdef SYS_EMU
    if (shire == IO_SHIRE_ID) {
        shire = EMU_IO_SHIRE_SP;
    }

    unsigned begin_hart = shire * EMU_THREADS_PER_SHIRE;
    unsigned hart_count = (shire == EMU_IO_SHIRE_SP) ? 1 : EMU_THREADS_PER_SHIRE;
    unsigned end_hart   = begin_hart + hart_count;

    for (unsigned thread = begin_hart; thread < end_hart; ++thread) {
        if (!cpu[thread].is_nonexistent()) {
            cpu[thread].clear_interrupt(MACHINE_EXTERNAL_INTERRUPT);
        }
    }
#else
    (void) shire;
#endif // SYS_EMU
}


void System::raise_supervisor_external_interrupt(unsigned shire)
{
#ifdef SYS_EMU
    if (shire == IO_SHIRE_ID) {
        shire = EMU_IO_SHIRE_SP;
    }

    unsigned begin_hart = shire * EMU_THREADS_PER_SHIRE;
    unsigned hart_count = (shire == EMU_IO_SHIRE_SP) ? 1 : EMU_THREADS_PER_SHIRE;
    unsigned end_hart   = begin_hart + hart_count;

    for (unsigned thread = begin_hart; thread < end_hart; ++thread) {
        if (!cpu[thread].is_nonexistent()) {
            cpu[thread].raise_interrupt(SUPERVISOR_EXTERNAL_INTERRUPT);
        }
    }
#else
    (void) shire;
#endif // SYS_EMU
}


void System::clear_supervisor_external_interrupt(unsigned shire)
{
#ifdef SYS_EMU
    if (shire == IO_SHIRE_ID) {
        shire = EMU_IO_SHIRE_SP;
    }

    unsigned begin_hart = shire * EMU_THREADS_PER_SHIRE;
    unsigned hart_count = (shire == EMU_IO_SHIRE_SP) ? 1 : EMU_THREADS_PER_SHIRE;
    unsigned end_hart   = begin_hart + hart_count;

    for (unsigned thread = begin_hart; thread < end_hart; ++thread) {
        if (!cpu[thread].is_nonexistent()) {
            cpu[thread].clear_interrupt(SUPERVISOR_EXTERNAL_INTERRUPT);
        }
    }
#else
    (void) shire;
#endif // SYS_EMU
}


void System::raise_machine_software_interrupt(unsigned shire, uint64_t thread_mask)
{
#ifdef SYS_EMU
    if (shire == IO_SHIRE_ID) {
        shire = EMU_IO_SHIRE_SP;
    }

    unsigned begin_hart = shire * EMU_THREADS_PER_SHIRE;
    unsigned hart_count = (shire == EMU_IO_SHIRE_SP) ? 1 : EMU_THREADS_PER_SHIRE;
    unsigned end_hart   = begin_hart + hart_count;

    for (unsigned thread = begin_hart; thread < end_hart; ++thread) {
        if (((thread_mask >> thread) & 1) && !cpu[thread].is_nonexistent()) {
            cpu[thread].raise_interrupt(MACHINE_SOFTWARE_INTERRUPT);
        }
    }
#else
    (void) shire;
    (void) thread_mask;
#endif // SYS_EMU
}


void System::clear_machine_software_interrupt(unsigned shire, uint64_t thread_mask)
{
#ifdef SYS_EMU
    if (shire == IO_SHIRE_ID) {
        shire = EMU_IO_SHIRE_SP;
    }

    unsigned begin_hart = shire * EMU_THREADS_PER_SHIRE;
    unsigned hart_count = (shire == EMU_IO_SHIRE_SP) ? 1 : EMU_THREADS_PER_SHIRE;
    unsigned end_hart   = begin_hart + hart_count;

    for (unsigned thread = begin_hart; thread < end_hart; ++thread) {
        if (((thread_mask >> thread) & 1) && !cpu[thread].is_nonexistent()) {
            cpu[thread].clear_interrupt(MACHINE_SOFTWARE_INTERRUPT);
        }
    }
#else
    (void) shire;
    (void) thread_mask;
#endif // SYS_EMU
}


void System::send_ipi_redirect(unsigned shire, uint64_t thread_mask)
{
    if ((shire == IO_SHIRE_ID) || (shire == EMU_IO_SHIRE_SP)) {
        throw std::runtime_error("IPI redirect to Service Processor");
    }

    // Only harts enabled by IPI_REDIRECT_FILTER should receive the signal
    thread_mask &= shire_other_esrs[shire].ipi_redirect_filter;

    unsigned begin_hart = shire * EMU_THREADS_PER_SHIRE;

    for (unsigned t = 0; t < EMU_THREADS_PER_SHIRE; ++t) {
        if (((thread_mask >> t) & 1) == 0) {
            continue;
        }

        unsigned thread = begin_hart + t;
        if (cpu[thread].is_nonexistent() || cpu[thread].is_unavailable()) {
            continue;
        }

        if (cpu[thread].is_waiting(Hart::Waiting::interrupt)
            && cpu[thread].prv == Privilege::U)
        {
            unsigned neigh = thread / EMU_THREADS_PER_NEIGH;
            uint64_t target_pc = neigh_esrs[neigh].ipi_redirect_pc;
            cpu[thread].pc = cpu[thread].npc = target_pc;
            cpu[thread].stop_waiting(Hart::Waiting::interrupt);
        } else {
            cpu[thread].raise_interrupt(BAD_IPI_REDIRECT_INTERRUPT);
        }
    }
}


bool System::raise_host_interrupt(uint32_t bitmap)
{
#ifdef SYS_EMU
    if (bitmap != 0) {
        if (emu()->get_api_communicate()) {
            return emu()->get_api_communicate()->raise_host_interrupt(bitmap);
        }
        LOG_AGENT(WARN, noagent, "%s", "API Communicate is NULL!");
    }
#else
    (void) bitmap;
#endif
    return false;
}


void System::copy_memory_from_host_to_device(uint64_t from_addr, uint64_t to_addr, uint32_t size)
{
#ifdef SYS_EMU
    api_communicate *api_comm = emu()->get_api_communicate();
    if (api_comm) {
        uint8_t *buff = new uint8_t[size];
        api_comm->host_memory_read(from_addr, size, buff);
        memory.write(noagent, to_addr, size, buff);
        delete[] buff;
    } else {
        LOG_AGENT(WARN, noagent, "%s", "API Communicate is NULL!");
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
    api_communicate *api_comm = emu()->get_api_communicate();
    if (api_comm) {
        uint8_t *buff = new uint8_t[size];
        memory.read(noagent, from_addr, size, buff);
        api_comm->host_memory_write(to_addr, size, buff);
        delete[] buff;
    } else {
        LOG_AGENT(WARN, noagent, "%s", "API Communicate is NULL!");
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
    api_communicate *api_comm = emu()->get_api_communicate();
    if (api_comm) {
        api_comm->notify_iatu_ctrl_2_reg_write(pcie_id, iatu, value);
    } else {
        LOG_AGENT(WARN, noagent, "%s", "API Communicate is NULL!");
    }
#else
    (void) pcie_id;
    (void) iatu;
    (void) value;
#endif
}


void System::write_fcc_credinc(unsigned index, uint64_t shire, uint64_t minion_mask)
{
    if (shire == EMU_IO_SHIRE_SP) {
        throw std::runtime_error("write_fcc_credinc_N for IOShire");
    }

    const unsigned thread_in_minion = index / 2;
    const unsigned thread0 = thread_in_minion + shire * EMU_THREADS_PER_SHIRE;
    const unsigned counter = index % 2;

    for (int minion = 0; minion < EMU_MINIONS_PER_SHIRE; ++minion) {
        if (~minion_mask & (1ull << minion)) {
            continue;
        }
        unsigned thread = thread0 + minion * EMU_THREADS_PER_MINION;
        if (cpu[thread].is_nonexistent()) {
            continue;
        }
        // Increment credict counter and check for overflow
        cpu[thread].fcc[counter]++;
        if (cpu[thread].fcc[counter] == 0) {
            update_tensor_error(cpu[thread], 1 << 3);
        }
        LOG_HART(DEBUG, cpu[thread],
                 "\tReceiving credits: fcc0 = 0x%" PRIx16 ", fcc1 = 0x%" PRIx16,
                 cpu[thread].fcc[0], cpu[thread].fcc[1]);
        // Resume waiting harts
        if (counter == 0) {
            cpu[thread].stop_waiting(Hart::Waiting::credit0);
        } else  {
            cpu[thread].stop_waiting(Hart::Waiting::credit1);
        }
    }
}


void System::recalculate_thread0_enable(unsigned shire)
{
    uint32_t value = shire_other_esrs[shire].thread0_disable;

    unsigned mcount = (shire == EMU_IO_SHIRE_SP ? 1 : EMU_MINIONS_PER_SHIRE);
    for (unsigned m = 0; m < mcount; ++m) {
        unsigned thread = shire * EMU_THREADS_PER_SHIRE + m * EMU_THREADS_PER_MINION;
        if (!cpu[thread].is_nonexistent()) {
            if ((value >> m) & 1) {
                cpu[thread].become_unavailable();
            }
            else if (cpu[thread].is_unavailable()) {
                // FIXME: check if we should halt-on-reset!
                cpu[thread].start_running();
            }
        }
    }
}


void System::recalculate_thread1_enable(unsigned shire)
{
    if (shire == EMU_IO_SHIRE_SP) {
        return;
    }

    uint32_t value = (shire_other_esrs[shire].minion_feature & 0x10)
            ? 0xffffffff
            : shire_other_esrs[shire].thread1_disable;

    for (unsigned m = 0; m < EMU_MINIONS_PER_SHIRE; ++m) {
        unsigned thread = shire * EMU_THREADS_PER_SHIRE + m * EMU_THREADS_PER_MINION + 1;
        if (!cpu[thread].is_nonexistent()) {
            if ((value >> m) & 1) {
                cpu[thread].become_unavailable();
            }
            else if (cpu[thread].is_unavailable()) {
                // FIXME: check if we should halt-on-reset!
                cpu[thread].start_running();
            }
        }
    }
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

        LOG_AGENT(INFO, noagent, "Segment[%d] VA: 0x%" PRIx64 "\tType: 0x%" PRIx32 " (LOAD)",
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
            LOG_AGENT(INFO, noagent,
                         "Section[%d] %s\tVMA: 0x%" PRIx64 "\tLMA: 0x%" PRIx64 "\tSize: 0x%" PRIx64
                         "\tType: 0x%" PRIx32 "\tFlags: 0x%" PRIx64,
                         idx, sec->get_name().c_str(), vma, lma, sec->get_size(),
                         sec->get_type(), sec->get_flags());

            if (lma >= MainMemory::dram_base)
                lma &= ~0x4000000000ULL;

            memory.init(noagent, lma, sec->get_size(), sec->get_data());
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
        memory.init(noagent, addr, count, reinterpret_cast<MainMemory::const_pointer>(fbuf));
        addr += count;
    }
}

uint64_t System::emu_cycle() const noexcept
{
#ifdef SYS_EMU
    return m_emu ? m_emu->get_emu_cycle() : 0;
#else
    return 0;
#endif
}


} // bemu
