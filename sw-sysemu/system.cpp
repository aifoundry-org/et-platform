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


void System::debug_reset(unsigned shire)
{
    unsigned s = (shire == IO_SHIRE_ID) ? EMU_IO_SHIRE_SP : shire;
    unsigned ncount = (s == EMU_IO_SHIRE_SP) ? 1 : EMU_NEIGH_PER_SHIRE;
    unsigned hcount = (s == EMU_IO_SHIRE_SP) ? 1 : EMU_THREADS_PER_SHIRE;
    unsigned hneigh = (s == EMU_IO_SHIRE_SP) ? 1 : EMU_THREADS_PER_NEIGH;

    for (unsigned n = 0; n < ncount; ++n) {
        auto& esrs = neigh_esrs[n + s * EMU_NEIGH_PER_SHIRE];
        esrs.debug_reset();
        for (unsigned h = 0; h < hneigh; ++h) {
            const auto& hart = cpu[h + n * EMU_THREADS_PER_NEIGH + s * EMU_THREADS_PER_SHIRE];
            esrs.hastatus0 |= ((uint64_t)hart.is_halted()) << h;
            esrs.hastatus0 |= ((uint64_t)hart.is_running()) << (h + 16);
        }
    }
    shire_cache_esrs[s].debug_reset();
    shire_other_esrs[s].debug_reset();
    broadcast_esrs[s].debug_reset();

    for (unsigned h = 0; h < hcount; ++h) {
        cpu[h + s * EMU_THREADS_PER_SHIRE].debug_reset();
    }
}


void System::begin_warm_reset(unsigned shire)
{
    unsigned s = (shire == IO_SHIRE_ID) ? EMU_IO_SHIRE_SP : shire;
    unsigned ncount = (s == EMU_IO_SHIRE_SP) ? 1 : EMU_NEIGH_PER_SHIRE;
    unsigned hcount = (s == EMU_IO_SHIRE_SP) ? 1 : EMU_THREADS_PER_SHIRE;

    for (unsigned n = 0; n < ncount; ++n) {
        unsigned neigh = n + s * EMU_NEIGH_PER_SHIRE;
        neigh_esrs[neigh].warm_reset();
        coop_tloads[neigh].tload_a[0].fill(Coop_tload_state{});
        coop_tloads[neigh].tload_a[1].fill(Coop_tload_state{});
        coop_tloads[neigh].tload_b.fill(Coop_tload_state{});
    }
    shire_cache_esrs[s].warm_reset();
    shire_other_esrs[s].warm_reset();
    broadcast_esrs[s].warm_reset();

    for (unsigned h = 0; h < hcount; ++h) {
        cpu[h + s * EMU_THREADS_PER_SHIRE].warm_reset();
    }
}


void System::end_warm_reset(unsigned shire)
{
    // We have already reset the Minions in begin_warm_reset(), where
    // they were set to 'unavailable', so now we just need to ensure that
    // enabled Minion become running or halted.
    recalculate_thread0_enable(shire);
    recalculate_thread1_enable(shire);
}


void System::cold_reset(unsigned shire)
{
    unsigned s = (shire == IO_SHIRE_ID) ? EMU_IO_SHIRE_SP : shire;
    unsigned ncount = (s == EMU_IO_SHIRE_SP) ? 1 : EMU_NEIGH_PER_SHIRE;
    unsigned hcount = (s == EMU_IO_SHIRE_SP) ? 1 : EMU_THREADS_PER_SHIRE;

    for (unsigned n = 0; n < ncount; ++n) {
        neigh_esrs[n + s * EMU_NEIGH_PER_SHIRE].cold_reset();
    }
    shire_cache_esrs[s].cold_reset();
    shire_other_esrs[s].cold_reset(s);
    broadcast_esrs[s].cold_reset();

    for (unsigned h = 0; h < hcount; ++h) {
        cpu[h + s * EMU_THREADS_PER_SHIRE].cold_reset();
    }
}


void System::cold_reset_mindm()
{
    for (unsigned s = 0; s < EMU_NUM_COMPUTE_SHIRES; ++s) {
        debug_reset(s);
    }
    dmctrl = 0;
}


void System::cold_reset_spdm()
{
    debug_reset(EMU_IO_SHIRE_SP);
    spdmctrl = 0;
}


void System::cold_reset_memshire()
{
    mem_shire_esrs.cold_reset();
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
        WARN_AGENT(other, noagent, "%s", "API Communicate is NULL!");
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
        WARN_AGENT(other, noagent, "%s", "API Communicate is NULL!");
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
        WARN_AGENT(other, noagent, "%s", "API Communicate is NULL!");
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
        WARN_AGENT(other, noagent, "%s", "API Communicate is NULL!");
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
                if (should_halt_on_reset(cpu[thread])) {
                    cpu[thread].enter_debug_mode(Debug_entry::Cause::haltreq);
                } else {
                    cpu[thread].start_running();
                }
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
                if (should_halt_on_reset(cpu[thread])) {
                    cpu[thread].enter_debug_mode(Debug_entry::Cause::haltreq);
                } else {
                    cpu[thread].start_running();
                }
            }
        }
    }
}


void System::config_reset_pc(unsigned neigh, uint64_t value)
{
    neigh_esrs[neigh].minion_boot = value & VA_M;
}


void System::config_simulated_harts(unsigned shire, uint32_t minionmask,
                                    bool multithreaded, bool enabled)
{
    static_assert(EMU_THREADS_PER_MINION == 2, "Wrong thread-per-minion count");

    if (shire == IO_SHIRE_ID) {
        shire = EMU_IO_SHIRE_SP;
    }
    if (shire == EMU_IO_SHIRE_SP) {
        multithreaded = false;
    }

    unsigned minion_count = (shire == EMU_IO_SHIRE_SP) ? 1 : EMU_MINIONS_PER_SHIRE;
    unsigned hart_count = (shire == EMU_IO_SHIRE_SP) ? 1 : EMU_THREADS_PER_MINION;

    uint32_t disabled[2];
    disabled[0] = (~minionmask) & ((1ul << minion_count) - 1);
    disabled[1] = multithreaded ? disabled[0] : ((1ul << minion_count) - 1);

    if (!multithreaded) {
        shire_other_esrs[shire].minion_feature |= 0x10;
    }
    for (unsigned m = 0; m < minion_count; ++m) {
        unsigned h = m * EMU_THREADS_PER_MINION + shire * EMU_THREADS_PER_SHIRE;
        for (unsigned t = 0; t < hart_count; ++t) {
            if (((disabled[t] >> m) & 1) == 0) {
                cpu[h + t].become_unavailable();
            } else {
                cpu[h + t].become_nonexistent();
            }
        }
    }

    // All harts will come out of reset as 'unavailable', but when
    // 'threadX_disable[n]' is set to 0 the corresponding hart will come out
    // of reset 'running' (or 'halted').
    if (enabled) {
        shire_other_esrs[shire].thread0_disable = disabled[0];
        shire_other_esrs[shire].thread1_disable = disabled[1];
    }
}


void System::load_elf(std::istream& stream)
{
    ELFIO::elfio elf;
    elf.load(stream);
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


void System::load_elf(const char* filename)
{
    std::ifstream file;

    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    file.open(filename, std::ios::in | std::ios::binary);
    load_elf(file);
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
