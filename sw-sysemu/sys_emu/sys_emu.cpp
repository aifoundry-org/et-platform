/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include "sys_emu.h"

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <list>
#include <exception>
#include <algorithm>
#include <locale>
#include <tuple>

#include "api_communicate.h"
#include "checkers/l2_scp_checker.h"
#include "devices/rvtimer.h"
#include "emu_gio.h"
#include "esrs.h"
#include "gdbstub.h"
#include "insn.h"
#include "log.h"
#include "memory/dump_data.h"
#include "memory/main_memory.h"
#include "mmu.h"
#include "processor.h"
#include "profiling.h"
#include "sys_emu.h"
#include "tensor.h"
#ifdef HAVE_BACKTRACE
#include "crash_handler.h"
#endif


////////////////////////////////////////////////////////////////////////////////
// Static Member variables
////////////////////////////////////////////////////////////////////////////////

bemu::System    sys_emu::chip;
uint64_t        sys_emu::emu_cycle = 0;
std::list<int>  sys_emu::running_threads;                                               // List of running threads
std::list<int>  sys_emu::wfi_stall_wait_threads;                                        // List of threads waiting in a WFI or stall
std::list<int>  sys_emu::fcc_wait_threads[EMU_NUM_FCC_COUNTERS_PER_THREAD];             // List of threads waiting for an FCC
std::list<int>  sys_emu::port_wait_threads;                                             // List of threads waiting for a port write
std::bitset<EMU_NUM_THREADS> sys_emu::active_threads;                                   // List of threads being simulated
uint16_t        sys_emu::pending_fcc[EMU_NUM_THREADS][EMU_NUM_FCC_COUNTERS_PER_THREAD]; // Pending FastCreditCounter list
std::list<sys_emu_coop_tload>    sys_emu::coop_tload_pending_list[EMU_NUM_THREADS];                      // List of pending cooperative tloads per thread
bool            sys_emu::mem_check = false;
mem_checker     sys_emu::mem_checker_;
bool            sys_emu::l1_scp_check = false;
l1_scp_checker  sys_emu::l1_scp_checker_;
bool            sys_emu::l2_scp_check = false;
l2_scp_checker  sys_emu::l2_scp_checker_;
bool            sys_emu::flb_check = false;
flb_checker     sys_emu::flb_checker_;
api_communicate *sys_emu::api_listener;
sys_emu_cmd_options              sys_emu::cmd_options;
std::unordered_set<uint64_t>     sys_emu::breakpoints;
std::bitset<EMU_NUM_THREADS>     sys_emu::single_step;

////////////////////////////////////////////////////////////////////////////////
// Helper methods
////////////////////////////////////////////////////////////////////////////////

static inline bool multithreading_is_disabled(const bemu::System& soc, unsigned shire)
{
    return soc.shire_other_esrs[shire].minion_feature & 0x10;
}

static inline bool thread_in_reduce_send(const bemu::Hart& cpu)
{
    return ((cpu.mhartid % EMU_THREADS_PER_MINION) == 0)
        && (cpu.core->reduce.state == bemu::Core::Reduce::State::Send);
}

static inline bool thread_in_reduce_recv(const bemu::Hart& cpu)
{
    return ((cpu.mhartid % EMU_THREADS_PER_MINION) == 0)
        && (cpu.core->reduce.state == bemu::Core::Reduce::State::Recv);
}

////////////////////////////////////////////////////////////////////////////////
// Sends an FCC to the desired minions specified in thread mask to the 1st or
// second thread (thread_dest), to the counter 0 or 1 (cnt_dest), inside the shire
// of thread_src
////////////////////////////////////////////////////////////////////////////////

void
sys_emu::fcc_to_threads(unsigned shire_id, unsigned thread_dest, uint64_t thread_mask, unsigned cnt_dest)
{
    assert(thread_dest < 2);
    assert(cnt_dest < 2);
    for(int m = 0; m < EMU_MINIONS_PER_SHIRE; m++)
    {
        // Skip disabled minion
        if (((cmd_options.minions_en >> m) & 1) == 0) continue;

        if ((thread_mask >> m) & 1)
        {
            int thread_id = shire_id * EMU_THREADS_PER_SHIRE + m * EMU_THREADS_PER_MINION + thread_dest;
            assert(thread_id < EMU_NUM_THREADS);
            LOG_AGENT(DEBUG, chip.cpu[thread_id], "Receiving FCC%u write", thread_dest*2 + cnt_dest);

            auto thread = std::find(fcc_wait_threads[cnt_dest].begin(),
                                    fcc_wait_threads[cnt_dest].end(),
                                    thread_id);
            // Checks if is already awaken
            if(thread == fcc_wait_threads[cnt_dest].end())
            {
                // Pushes to pending FCC
                pending_fcc[thread_id][cnt_dest]++;
            }
            // Otherwise wakes up thread
            else
            {
                if (!thread_is_active(thread_id) || thread_is_disabled(thread_id)) {
                    LOG_AGENT(DEBUG, chip.cpu[thread_id], "Disabled thread received FCC%u", thread_dest*2 + cnt_dest);
                } else {
                    LOG_AGENT(DEBUG, chip.cpu[thread_id], "Waking up due to received FCC%u", thread_dest*2 + cnt_dest);
                    running_threads.push_back(thread_id);
                }
                fcc_wait_threads[cnt_dest].erase(thread);
                --chip.cpu[thread_id].fcc[cnt_dest];
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// Unblock a thread waiting for a message when the message is written to the
// thread's L1SCP
////////////////////////////////////////////////////////////////////////////////

void
sys_emu::msg_to_thread(unsigned thread_id)
{
    thread_id = bemu::hart_index(chip.cpu[thread_id]);
    auto thread = std::find(port_wait_threads.begin(), port_wait_threads.end(), thread_id);
    LOG_NOTHREAD(INFO, "Message to thread %i", thread_id);
    // Checks if in port wait state
    if(thread != port_wait_threads.end())
    {
        if (!thread_is_active(thread_id) || thread_is_disabled(thread_id)) {
            LOG_AGENT(DEBUG, chip.cpu[thread_id], "%s", "Disabled thread received msg");
        } else {
            LOG_AGENT(DEBUG, chip.cpu[thread_id], "%s", "Waking up due msg");
            running_threads.push_back(thread_id);
        }
        port_wait_threads.erase(thread);
    }
}

////////////////////////////////////////////////////////////////////////////////
// Send IPI_REDIRECT or IPI (or clear pending IPIs) to the minions specified in
// thread mask of the specified shire id.
////////////////////////////////////////////////////////////////////////////////

void
sys_emu::send_ipi_redirect_to_threads(unsigned shire, uint64_t thread_mask)
{
    if ((shire == IO_SHIRE_ID) || (shire == EMU_IO_SHIRE_SP))
        throw std::runtime_error("IPI_REDIRECT to SvcProc");

    // Get IPI_REDIRECT_FILTER ESR for the shire
    uint64_t ipi_redirect_filter = chip.shire_other_esrs[shire].ipi_redirect_filter;

    unsigned thread0 = EMU_THREADS_PER_SHIRE * shire;
    for(unsigned t = 0; t < EMU_THREADS_PER_SHIRE; t++)
    {
        // If both IPI_REDIRECT_TRIGGER and IPI_REDIRECT_FILTER has bit set
        if(((thread_mask >> t) & 1) && ((ipi_redirect_filter >> t) & 1))
        {
            // Get PC
            unsigned tid = thread0 + t;
            unsigned neigh = tid / EMU_THREADS_PER_NEIGH;
            uint64_t new_pc = chip.neigh_esrs[neigh].ipi_redirect_pc;
            LOG_AGENT(DEBUG, chip.cpu[tid], "Receiving IPI_REDIRECT to %llx", (long long unsigned int) new_pc);
            // If thread sleeping, wakes up and changes PC
            if(!thread_is_running(tid))
            {
                if (!thread_is_active(tid) || thread_is_disabled(tid)) {
                    LOG_AGENT(DEBUG, chip.cpu[tid], "%s", "Disabled thread received IPI_REDIRECT");
                } else {
                    LOG_AGENT(DEBUG, chip.cpu[tid], "%s", "Waking up due to IPI_REDIRECT");
                    running_threads.push_back(tid);
                    thread_set_pc(tid, new_pc);
                }
            }
            // Otherwise IPI is dropped
            else
            {
                LOG_AGENT(DEBUG, chip.cpu[tid], "%s", "WARNING => IPI_REDIRECT dropped");
            }
        }
    }
}

void
sys_emu::raise_interrupt_wakeup_check(unsigned thread_id)
{
    // WFI/stall don't require the global interrupt bits in mstatus to be enabled
    uint64_t xip = (chip.cpu[thread_id].mip | chip.cpu[thread_id].ext_seip) & chip.cpu[thread_id].mie;
    if (xip) {
        bool trap = false;

        // If it was waiting in a WFI or stall, wake it up
        auto wfi_stall_it = std::find(wfi_stall_wait_threads.begin(),
                                wfi_stall_wait_threads.end(),
                                thread_id);
        if (wfi_stall_it != wfi_stall_wait_threads.end()) {
            LOG_AGENT(DEBUG, chip.cpu[thread_id], "%s", "Waking up thread");
            wfi_stall_wait_threads.erase(wfi_stall_it);
            running_threads.push_back(thread_id);
            return;
        }

        // Otherwise, if there's a trap, catch it and wakeup the thread
        try {
            chip.cpu[thread_id].check_pending_interrupts();
        } catch (const bemu::trap_t& t) {
            trap = true;
        }

        if (trap) {
            LOG_AGENT(DEBUG, chip.cpu[thread_id], "%s", "Waking up thread");
            running_threads.push_back(thread_id);

            // If it was waiting for an FCC, remove it from the list
            for (auto &fcc_wait_it: fcc_wait_threads) {
                auto th = std::find(fcc_wait_it.begin(),
                                    fcc_wait_it.end(),
                                    thread_id);
                if (th != fcc_wait_it.end())
                    fcc_wait_it.erase(th);
            }
        }
    }
}

void
sys_emu::raise_timer_interrupt(uint64_t shire_mask)
{
    for (int s = 0; s < EMU_NUM_SHIRES; s++) {
        if (!(shire_mask & (1ULL << s)))
            continue;

        unsigned shire_minion_count = (s == EMU_IO_SHIRE_SP ? 1 : EMU_MINIONS_PER_SHIRE);
        unsigned minion_thread_count = (s == EMU_IO_SHIRE_SP ? 1 : EMU_THREADS_PER_MINION);
        uint32_t target = (s == EMU_IO_SHIRE_SP ? 1 : chip.shire_other_esrs[s].mtime_local_target);

        for (unsigned m = 0; m < shire_minion_count; m++) {
            if (!(target & (1ULL << m)))
                continue;

            for (unsigned ii = 0; ii < minion_thread_count; ii++) {
                int thread_id = s * EMU_THREADS_PER_SHIRE + m * EMU_THREADS_PER_MINION + ii;
                LOG_AGENT(DEBUG, chip.cpu[thread_id], "%s", "Receiving Machine timer interrupt");
                bemu::raise_timer_interrupt(chip.cpu[thread_id]);

                if (!thread_is_active(thread_id) || thread_is_disabled(thread_id))
                    continue;

                // Check if the thread has to be awakened
                if (!thread_is_running(thread_id))
                    raise_interrupt_wakeup_check(thread_id);
            }
        }
    }
}

void
sys_emu::clear_timer_interrupt(uint64_t shire_mask)
{
    for (int s = 0; s < EMU_NUM_SHIRES; s++) {
        if (!(shire_mask & (1ULL << s)))
            continue;

        unsigned shire_minion_count = (s == EMU_IO_SHIRE_SP ? 1 : EMU_MINIONS_PER_SHIRE);
        unsigned minion_thread_count = (s == EMU_IO_SHIRE_SP ? 1 : EMU_THREADS_PER_MINION);
        uint32_t target = (s == EMU_IO_SHIRE_SP ? 1 : chip.shire_other_esrs[s].mtime_local_target);

        for (unsigned m = 0; m < shire_minion_count; m++) {
            if (target & (1ULL << m)) {
                for (unsigned ii = 0; ii < minion_thread_count; ii++) {
                    int thread_id = s * EMU_THREADS_PER_SHIRE + m * EMU_THREADS_PER_MINION + ii;
                    bemu::clear_timer_interrupt(chip.cpu[thread_id]);
                }
            }
        }
    }
}

void
sys_emu::raise_software_interrupt(unsigned shire, uint64_t thread_mask)
{
    if (shire == IO_SHIRE_ID)
        shire = EMU_IO_SHIRE_SP;

    unsigned thread0 = EMU_THREADS_PER_SHIRE * shire;
    unsigned shire_thread_count = (shire == EMU_IO_SHIRE_SP ? 1 : EMU_THREADS_PER_SHIRE);

    // Write mip.msip to all selected threads
    for (unsigned t = 0; t < shire_thread_count; ++t) {
        if (!(thread_mask & (1ull << t)))
            continue;

        unsigned thread_id = thread0 + t;
        if (!thread_is_active(thread_id) || thread_is_disabled(thread_id)) {
            LOG_AGENT(DEBUG, chip.cpu[thread_id], "%s", "Disabled thread received IPI");
            continue;
        }

        LOG_AGENT(DEBUG, chip.cpu[thread_id], "%s", "Receiving IPI");
        bemu::raise_software_interrupt(chip.cpu[thread_id]);

        // Check if the thread has to be awakened
        if (!thread_is_running(thread_id))
            raise_interrupt_wakeup_check(thread_id);
    }
}

void
sys_emu::clear_software_interrupt(unsigned shire, uint64_t thread_mask)
{
    if (shire == IO_SHIRE_ID)
        shire = EMU_IO_SHIRE_SP;

    unsigned thread0 = EMU_THREADS_PER_SHIRE * shire;
    unsigned shire_thread_count = (shire == EMU_IO_SHIRE_SP ? 1 : EMU_THREADS_PER_SHIRE);

    // Clear mip.msip to all selected threads
    for (unsigned t = 0; t < shire_thread_count; ++t)
    {
        if ((thread_mask >> t) & 1)
        {
            unsigned thread_id = thread0 + t;
            bemu::clear_software_interrupt(chip.cpu[thread_id]);
        }
    }
}

void
sys_emu::raise_external_interrupt(unsigned shire)
{
    if (shire == IO_SHIRE_ID)
        shire = EMU_IO_SHIRE_SP;

    unsigned thread0 = EMU_THREADS_PER_SHIRE * shire;
    unsigned shire_thread_count = (shire == EMU_IO_SHIRE_SP ? 1 : EMU_THREADS_PER_SHIRE);

    // Write mip.meip to all the threads of the shire
    for (unsigned t = 0; t < shire_thread_count; ++t)
    {
        unsigned thread_id = thread0 + t;
        LOG_AGENT(DEBUG, chip.cpu[thread_id], "%s", "Receiving Machine external interrupt");
        bemu::raise_external_machine_interrupt(chip.cpu[thread_id]);

        if (!thread_is_active(thread_id) || thread_is_disabled(thread_id))
            continue;

        // Check if the thread has to be awakened
        if (!thread_is_running(thread_id))
            raise_interrupt_wakeup_check(thread_id);
    }
}

void
sys_emu::clear_external_interrupt(unsigned shire)
{
    if (shire == IO_SHIRE_ID)
        shire = EMU_IO_SHIRE_SP;

    unsigned thread0 = EMU_THREADS_PER_SHIRE * shire;
    unsigned shire_thread_count = (shire == EMU_IO_SHIRE_SP ? 1 : EMU_THREADS_PER_SHIRE);

    // Clear mip.meip to all the threads of the shire
    for (unsigned t = 0; t < shire_thread_count; ++t)
    {
        unsigned thread_id = thread0 + t;
        bemu::clear_external_machine_interrupt(chip.cpu[thread_id]);
    }
}

void
sys_emu::raise_external_supervisor_interrupt(unsigned shire_id)
{
    unsigned thread0 =
        EMU_THREADS_PER_SHIRE
        * (shire_id == IO_SHIRE_ID ? EMU_IO_SHIRE_SP : shire_id);

    unsigned shire_thread_count =
        (shire_id == IO_SHIRE_ID ? 1 : EMU_THREADS_PER_SHIRE);

    // Write external seip to all the threads of the shire
    for (unsigned t = 0; t < shire_thread_count; ++t)
    {
        unsigned thread_id = thread0 + t;
        LOG_AGENT(DEBUG, chip.cpu[thread_id], "%s", "Receiving Supervisor external interrupt");
        bemu::raise_external_supervisor_interrupt(chip.cpu[thread_id]);

        if (!thread_is_active(thread_id) || thread_is_disabled(thread_id))
            continue;

        // Check if the thread has to be awakened
        if (!thread_is_running(thread_id))
            raise_interrupt_wakeup_check(thread_id);
    }
}

void
sys_emu::clear_external_supervisor_interrupt(unsigned shire_id)
{
    unsigned thread0 =
        EMU_THREADS_PER_SHIRE
        * (shire_id == IO_SHIRE_ID ? EMU_IO_SHIRE_SP : shire_id);

    unsigned shire_thread_count =
        (shire_id == IO_SHIRE_ID ? 1 : EMU_THREADS_PER_SHIRE);

    // Clear external seip to all the threads of the shire
    for (unsigned t = 0; t < shire_thread_count; ++t)
    {
        unsigned thread_id = thread0 + t;
        bemu::clear_external_supervisor_interrupt(chip.cpu[thread_id]);
    }
}

void
sys_emu::evl_dv_handle_irq_inj(bool raise, uint64_t subopcode, uint64_t shire_mask)
{
    switch (subopcode)
    {
    case ET_DIAG_IRQ_INJ_MEI:
        for (unsigned s = 0; s < EMU_NUM_SHIRES; s++) {
            if (!(shire_mask & (1ULL << s)))
                continue;
            if (raise)
                sys_emu::raise_external_interrupt(s);
            else
                sys_emu::clear_external_interrupt(s);
        }
        break;
    case ET_DIAG_IRQ_INJ_SEI:
        for (unsigned s = 0; s < EMU_NUM_SHIRES; s++) {
            if (!(shire_mask & (1ULL << s)))
                continue;
            if (raise)
                sys_emu::raise_external_supervisor_interrupt(s);
            else
                sys_emu::clear_external_supervisor_interrupt(s);
        }
        break;
    case ET_DIAG_IRQ_INJ_TI:
        if (raise)
            sys_emu::raise_timer_interrupt(shire_mask);
        else
            sys_emu::clear_timer_interrupt(shire_mask);
        break;
    }
}

void
sys_emu::shire_enable_threads(unsigned shire_id, uint32_t thread0_disable, uint32_t thread1_disable)
{
    if (shire_id == IO_SHIRE_ID)
        shire_id = EMU_IO_SHIRE_SP;

    chip.write_thread0_disable(shire_id, thread0_disable);
    chip.write_thread1_disable(shire_id, thread1_disable);

    recalculate_thread_disable(shire_id);
}

void
sys_emu::recalculate_thread_disable(unsigned shire_id)
{
    if (shire_id == IO_SHIRE_ID)
        shire_id = EMU_IO_SHIRE_SP;

    unsigned thread_count = (shire_id == EMU_IO_SHIRE_SP ? 1 : EMU_THREADS_PER_SHIRE);

    for (unsigned t = 0; t < thread_count; ++t) {
        unsigned thread_id = t + EMU_THREADS_PER_SHIRE * shire_id;
        if (!thread_is_active(thread_id))
            continue;
        if (chip.cpu[thread_id].enabled && !thread_is_running(thread_id)) {
            auto wfi_stall_it = std::find(wfi_stall_wait_threads.begin(), wfi_stall_wait_threads.end(), thread_id);
            if (wfi_stall_it != wfi_stall_wait_threads.end())
                continue;

            auto port_it = std::find(port_wait_threads.begin(), port_wait_threads.end(), thread_id);
            if (port_it != port_wait_threads.end())
                continue;

            bool waits_for_fcc = false;
            for (auto &fcc_wait_it: fcc_wait_threads) {
                auto th = std::find(fcc_wait_it.begin(), fcc_wait_it.end(), thread_id);
                if (th != fcc_wait_it.end()) {
                    waits_for_fcc = true;
                    break;
                }
            }
            if (waits_for_fcc)
                continue;

            running_threads.push_back(thread_id);
        } else if (!chip.cpu[thread_id].enabled) {
            auto wfi_stall_it = std::find(wfi_stall_wait_threads.begin(), wfi_stall_wait_threads.end(), thread_id);
            if (wfi_stall_it != wfi_stall_wait_threads.end()) {
                wfi_stall_wait_threads.erase(wfi_stall_it);
                continue;
            }

            auto port_it = std::find(port_wait_threads.begin(), port_wait_threads.end(), thread_id);
            if (port_it != port_wait_threads.end()) {
                port_wait_threads.erase(port_it);
                continue;
            }

            bool waited_for_fcc = false;
            for (auto &fcc_wait_it: fcc_wait_threads) {
                auto th = std::find(fcc_wait_it.begin(), fcc_wait_it.end(), thread_id);
                if (th != fcc_wait_it.end()) {
                    fcc_wait_it.erase(th);
                    waited_for_fcc = true;
                    break;
                }
            }
            if (waited_for_fcc)
                continue;

            auto it = std::find(running_threads.begin(), running_threads.end(), thread_id);
            if (it != running_threads.end()) {
                running_threads.erase(it);
            }
        }
    }
}

void
sys_emu::coop_tload_add(uint32_t thread_id, bool tenb, uint32_t id, uint32_t coop_id, uint32_t min_mask, uint32_t neigh_mask)
{
    sys_emu_coop_tload coop_tload;

    coop_tload.tenb       = tenb;
    coop_tload.id         = id;
    coop_tload.coop_id    = coop_id;
    coop_tload.min_mask   = min_mask;
    coop_tload.neigh_mask = neigh_mask;
    coop_tload_pending_list[thread_id].push_back(coop_tload);
}

// Returns the thread id of the cooperating minion for a tensor load
uint32_t
sys_emu::coop_tload_get_thread_id(uint32_t thread_id, uint32_t neigh, uint32_t min)
{
    uint32_t other_thread_id = 0;
    uint32_t shire_id = thread_id / EMU_THREADS_PER_SHIRE;
    other_thread_id = shire_id * EMU_THREADS_PER_SHIRE + (neigh * EMU_MINIONS_PER_NEIGH + min) * EMU_THREADS_PER_MINION;
    return other_thread_id;
}

// Returns if the cooperative tloads are present in other threads
bool
sys_emu::coop_tload_all_present(uint32_t thread_id, const sys_emu_coop_tload & coop_tload, uint32_t & requested_mask, uint32_t & present_mask)
{
    bool all_present = true;

    // For all the neighs
    for(uint32_t neigh = 0; neigh < EMU_NEIGH_PER_SHIRE; neigh++)
    {
        // Skip check if not enabled
        if(((coop_tload.neigh_mask >> neigh) & 0x1) == 0) continue;

        // For all the minions
        for(uint32_t min = 0; min < EMU_MINIONS_PER_NEIGH; min++)
        {
            // Skip check if not enabled
            if(((coop_tload.min_mask >> min) & 0x1) == 0) continue;

            // Updates requested mask
            requested_mask |= 1 << (min + neigh * EMU_MINIONS_PER_NEIGH);

            // Gets the thread id of the cooperating thread
            uint32_t coop_thread_id = coop_tload_get_thread_id(thread_id, neigh, min);

            // Looks for first tload coop going to same tenb
            uint8_t state = 0;
            auto it_coop = coop_tload_pending_list[coop_thread_id].begin();
            while((state == 0) && (it_coop != coop_tload_pending_list[coop_thread_id].end()))
            {
                //printf("COOP: %i => checking id %i vs id %i of thread %i\n", thread_id, coop_tload.coop_id, it_coop->coop_id, coop_thread_id);
                // Coming from same TLoad FSM
                if(it_coop->tenb == coop_tload.tenb)
                {
                    if(it_coop->coop_id != coop_tload.coop_id) state = 2; // Found entry, different coop_id => TODO: this is a SW bug
                    else                                       state = 1; // Found entry, same coop_id
                }
                else it_coop++;
            }

            // If no tload found from same unit and same coop_id, then not ready
            if(state != 1)
            {
                all_present = false;
            }
            else
            {
                present_mask |= 1 << (min + neigh * EMU_MINIONS_PER_NEIGH);
            }
        }
    }

    return all_present;
}

// Marks as done the cooperating tensor loads
void
sys_emu::coop_tload_mark_done(uint32_t thread_id, const sys_emu_coop_tload & coop_tload)
{
    // For all the neighs
    for(uint32_t neigh = 0; neigh < EMU_NEIGH_PER_SHIRE; neigh++)
    {
        // Skip check if not enabled
        if(((coop_tload.neigh_mask >> neigh) & 0x1) == 0) continue;

        // For all the minions
        for(uint32_t min = 0; min < EMU_MINIONS_PER_NEIGH; min++)
        {
            // Skip check if not enabled
            if(((coop_tload.min_mask >> min) & 0x1) == 0) continue;

            // Gets the thread id of the cooperating thread
            uint32_t coop_thread_id = coop_tload_get_thread_id(thread_id, neigh, min);

            // Do not remove himself
            if(coop_thread_id == thread_id) continue;

            // Looks for first tload coop going to same tenb
            uint8_t state = 0;
            auto it_coop = coop_tload_pending_list[coop_thread_id].begin();
            while((state == 0) && (it_coop != coop_tload_pending_list[coop_thread_id].end()))
            {
                // Coming from same TLoad FSM
                if(it_coop->tenb == coop_tload.tenb)
                {
                    if(it_coop->coop_id != coop_tload.coop_id) state = 2; // Found entry, different coop_id => TODO: this is a SW bug
                    // Found entry, remove it
                    else
                    {
                        coop_tload_pending_list[coop_thread_id].erase(it_coop);
                        state = 1;
                    }
                }
                else it_coop++;
            }
        }
    }
}

// Checks if all cooperative tensor loads with id id for a specific thread id are done
bool
sys_emu::coop_tload_check(uint32_t thread_id, bool tenb, uint32_t id, uint32_t & requested_mask, uint32_t & present_mask)
{
    // Returns true if thread_id has all tloads resolved with id id
    bool resolved = true;

    // Clears the mask bits
    requested_mask = 0;
    present_mask   = 0;

    //printf("COOP: %i => coop_tload_check: tenb %i, id %i\n", thread_id, tenb, id);

    // First we need to distinguesh between tloads to TENB or not
    // Second, for non TENB, as there's a single FSM and it executes
    // in order, we need to check which is the last tload with the checked
    // id. Then, all the older tloads regardless of their id need to be
    // checked, as the FSM is in order

    auto last_tload = coop_tload_pending_list[thread_id].begin();
    if(tenb)
    {
        last_tload = coop_tload_pending_list[thread_id].end();
    }
    else
    {
        auto it = coop_tload_pending_list[thread_id].begin();
        while(it != coop_tload_pending_list[thread_id].end())
        {
            if(!it->tenb && (it->id == id))
            {
                last_tload = it;
            }
            it++;
        }
    }

    // We need to check all the tloads from the same FSM (tenb or not tenb)
    // For all the pending coop tloads
    bool last = false;
    auto it = coop_tload_pending_list[thread_id].begin();
    while(!last && (it != coop_tload_pending_list[thread_id].end()))
    {
        // Mark this is the last required checked
        last = (it == last_tload);
        // Ignores entry if different tenb
        if(it->tenb != tenb)
        {
            //printf("COOP: %i => coop_tload_check skip to check: id %i, coop_id %i, neigh %01x, min %02x\n", thread_id, it->id, it->coop_id, it->neigh_mask, it->min_mask);
            it++;
            continue;
        }
        //printf("COOP: %i => coop_tload_check need to check: id %i, neigh %01x, min %02x\n", thread_id, it->coop_id, it->neigh_mask, it->min_mask);

        // Gets if all tloads are present
        bool all_present = coop_tload_all_present(thread_id, * it, requested_mask, present_mask);
        if(all_present)
        {
            coop_tload_mark_done(thread_id, * it);
            it = coop_tload_pending_list[thread_id].erase(it);
        }
        else
        {
            // Not all done
            return false;
        }
    }

    return resolved;
}

void
sys_emu::breakpoint_insert(uint64_t addr)
{
    LOG_NOTHREAD(DEBUG, "Inserting breakpoint at address 0x%" PRIx64 "", addr);
    breakpoints.insert(addr);
}

void
sys_emu::breakpoint_remove(uint64_t addr)
{
    LOG_NOTHREAD(DEBUG, "Removing breakpoint at address 0x%" PRIx64 "", addr);
    breakpoints.erase(addr);
}

bool
sys_emu::breakpoint_exists(uint64_t addr)
{
    return contains(breakpoints, addr);
}

sys_emu::sys_emu(const sys_emu_cmd_options &cmd_options, api_communicate *api_comm)
{
    this->cmd_options = cmd_options;
    this->api_listener = api_comm;

    // Reset the SoC
    emu_cycle = 0;
    running_threads.clear();
    wfi_stall_wait_threads.clear();
    for (auto &item : fcc_wait_threads) {
        item.clear();
    }
    port_wait_threads.clear();
    active_threads.reset();
    bzero(pending_fcc, sizeof(pending_fcc));
    for (auto &item : coop_tload_pending_list) {
        item.clear();
    }
    mem_check = cmd_options.mem_check;
    mem_checker_ = mem_checker{};
    l1_scp_check = cmd_options.l1_scp_check;
    l1_scp_checker_ = l1_scp_checker{};
    l2_scp_check = cmd_options.l2_scp_check;
    new (&l2_scp_checker_) l2_scp_checker{};
    flb_check = cmd_options.flb_check;
    flb_checker_ = flb_checker{};
    breakpoints.clear();
    single_step.reset();

    if (cmd_options.elf_files.empty() && cmd_options.file_load_files.empty() &&
        cmd_options.mem_desc_file.empty() && cmd_options.api_comm_path.empty()) {
        LOG_NOTHREAD(FTL, "%s", "Need an ELF file, a file load, a mem_desc file or runtime API!");
    }

#ifdef SYSEMU_DEBUG
    if (cmd_options.debug == true) {
        LOG_NOTHREAD(INFO, "%s", "Starting in interactive mode.");
        debug_init();
    }
#endif

    bemu::log.setLogLevel(cmd_options.log_en ? LOG_DEBUG : LOG_INFO);

    // Init emu
    chip.init(bemu::system_version_t::ETSOC1_A0);
    bemu::log_set_threads(cmd_options.log_thread);
    memcpy(&chip.memory_reset_value, &cmd_options.mem_reset, MEM_RESET_PATTERN_SIZE);

    // Callbacks for port writes
    chip.set_msg_funcs(msg_to_thread);

    // Parses the ELF files and memory description
    for (const auto &elf: cmd_options.elf_files) {
        LOG_NOTHREAD(INFO, "Loading ELF: \"%s\"", elf.c_str());
        try {
            chip.load_elf(elf.c_str());
        }
        catch (...) {
            LOG_NOTHREAD(FTL, "Error loading ELF \"%s\"", elf.c_str());
        }
    }
    if (!cmd_options.mem_desc_file.empty()) {
        parse_mem_file(cmd_options.mem_desc_file.c_str());
    }

    // Load files
    for (const auto &info: cmd_options.file_load_files) {
        LOG_NOTHREAD(INFO, "Loading file @ 0x%" PRIx64 ": \"%s\"", info.addr, info.file.c_str());
        try {
            chip.load_raw(info.file.c_str(), info.addr);
        }
        catch (...) {
            LOG_NOTHREAD(FTL, "Error loading file \"%s\"", info.file.c_str());
        }
    }

    // Perform 32 bit writes
    for (const auto &info: cmd_options.mem_write32s) {
        LOG_NOTHREAD(INFO, "Writing 32-bit value 0x%" PRIx32 " to 0x%" PRIx64, info.value, info.addr);
        chip.memory.write(bemu::Noagent{&chip}, info.addr, sizeof(info.value),
                          reinterpret_cast<bemu::MainMemory::const_pointer>(&info.value));
    }

    // Setup PU UART0 RX stream
    if (!cmd_options.pu_uart0_rx_file.empty()) {
        int fd = open(cmd_options.pu_uart0_rx_file.c_str(), O_RDONLY, 0666);
        if (fd < 0) {
            LOG_NOTHREAD(FTL, "Error opening \"%s\"", cmd_options.pu_uart0_rx_file.c_str());
        }
        chip.pu_uart0_set_rx_fd(fd);
    } else {
        chip.pu_uart0_set_rx_fd(STDIN_FILENO);
    }

    // Setup PU UART1 RX stream
    if (!cmd_options.pu_uart1_rx_file.empty()) {
        int fd = open(cmd_options.pu_uart1_rx_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            LOG_NOTHREAD(FTL, "Error opening \"%s\"", cmd_options.pu_uart1_rx_file.c_str());
        }
        chip.pu_uart1_set_rx_fd(fd);
    } else {
        chip.pu_uart1_set_rx_fd(STDIN_FILENO);
    }

    // Setup SPIO UART0 RX stream
    if (!cmd_options.spio_uart0_rx_file.empty()) {
        int fd = open(cmd_options.spio_uart0_rx_file.c_str(), O_RDONLY | O_NONBLOCK | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            LOG_NOTHREAD(FTL, "Error opening \"%s\"", cmd_options.spio_uart0_rx_file.c_str());
        }
        chip.spio_uart0_set_rx_fd(fd);
    } else {
        chip.spio_uart0_set_rx_fd(STDIN_FILENO);
    }

    // Setup SPIO UART1 RX stream
    if (!cmd_options.spio_uart1_rx_file.empty()) {
        int fd = open(cmd_options.spio_uart1_rx_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            LOG_NOTHREAD(FTL, "Error opening \"%s\"", cmd_options.spio_uart1_rx_file.c_str());
        }
        chip.spio_uart1_set_rx_fd(fd);
    } else {
        chip.spio_uart1_set_rx_fd(STDIN_FILENO);
    }

    // Setup PU UART0 TX stream
    if (!cmd_options.pu_uart0_tx_file.empty()) {
        int fd = open(cmd_options.pu_uart0_tx_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            LOG_NOTHREAD(FTL, "Error creating \"%s\"", cmd_options.pu_uart0_tx_file.c_str());
        }
        chip.pu_uart0_set_tx_fd(fd);
    } else {
        chip.pu_uart0_set_tx_fd(STDOUT_FILENO);
    }

    // Setup PU UART1 TX stream
    if (!cmd_options.pu_uart1_tx_file.empty()) {
        int fd = open(cmd_options.pu_uart1_tx_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            LOG_NOTHREAD(FTL, "Error creating \"%s\"", cmd_options.pu_uart1_tx_file.c_str());
        }
        chip.pu_uart1_set_tx_fd(fd);
    } else {
        chip.pu_uart1_set_tx_fd(STDOUT_FILENO);
    }

    // Setup SPIO UART0 TX stream
    if (!cmd_options.spio_uart0_tx_file.empty()) {
        int fd = open(cmd_options.spio_uart0_tx_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            LOG_NOTHREAD(FTL, "Error creating \"%s\"", cmd_options.spio_uart0_tx_file.c_str());
        }
        chip.spio_uart0_set_tx_fd(fd);
    } else {
        chip.spio_uart0_set_tx_fd(STDOUT_FILENO);
    }

    // Setup SPIO UART1 TX stream
    if (!cmd_options.spio_uart1_tx_file.empty()) {
        int fd = open(cmd_options.spio_uart1_tx_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            LOG_NOTHREAD(FTL, "Error creating \"%s\"", cmd_options.spio_uart1_tx_file.c_str());
        }
        chip.spio_uart1_set_tx_fd(fd);
    } else {
        chip.spio_uart1_set_tx_fd(STDOUT_FILENO);
    }

    // Initialize Simulator API
    if (api_listener) {
        api_listener->set_system(&chip);
    }

    for (unsigned s = 0; s < EMU_NUM_SHIRES; s++) {
        chip.reset_esrs_for_shire(s);

        // Skip disabled shire
        if (((cmd_options.shires_en >> s) & 1) == 0)
            continue;

        bool disable_multithreading =
                !cmd_options.second_thread || multithreading_is_disabled(chip, s);

        unsigned minion_thread_count =
                disable_multithreading ? 1 : EMU_THREADS_PER_MINION;

        unsigned shire_minion_count =
                (s == EMU_IO_SHIRE_SP) ? 1 : EMU_MINIONS_PER_SHIRE;

        // Calculate mask of which Minions of the Shire to enable
        uint32_t enable_minion_mask = cmd_options.minions_en & ((1ull << shire_minion_count) - 1);
        if (((s == EMU_IO_SHIRE_SP) && cmd_options.sp_dis) ||
            ((s != EMU_IO_SHIRE_SP) && cmd_options.mins_dis)) {
            enable_minion_mask = 0;
        }

        chip.write_thread0_disable(s, ~enable_minion_mask);
        if (disable_multithreading) {
            chip.write_thread1_disable(s, 0xffffffff);
        } else {
            chip.write_thread1_disable(s, ~enable_minion_mask);
        }

        // For all the minions
        for (unsigned m = 0; m < shire_minion_count; m++) {
            // Skip disabled minion
            if (((cmd_options.minions_en >> m) & 1) == 0)
                continue;

            // Inits threads
            for (unsigned t = 0; t < minion_thread_count; t++) {
                unsigned tid = s * EMU_THREADS_PER_SHIRE + m * EMU_THREADS_PER_MINION + t;
                chip.reset_hart(tid);
                chip.cpu[tid].mregs[bemu::m0].set();
                thread_set_pc(tid, (s == EMU_IO_SHIRE_SP) ? cmd_options.sp_reset_pc : cmd_options.reset_pc);

                // Puts thread id in the active list
                activate_thread(tid);
                if (((s == EMU_IO_SHIRE_SP) && !cmd_options.sp_dis) ||
                    ((s != EMU_IO_SHIRE_SP) && !cmd_options.mins_dis)) {
                    LOG_AGENT(DEBUG, chip.cpu[tid], "%s", "Resetting");
                    assert(!thread_is_disabled(tid));
                    running_threads.push_back(tid);
                }
            }
        }
    }

    // Initialize xregs passed to command line
    for (auto &info: cmd_options.set_xreg) {
        chip.cpu[info.thread].xregs[info.xreg] = info.value;
    }
}

////////////////////////////////////////////////////////////////////////////////
// Main function implementation
////////////////////////////////////////////////////////////////////////////////

int sys_emu::main_internal() {
#ifdef HAVE_BACKTRACE
    Crash_handler __crash_handler;
#endif

#ifdef SYSEMU_PROFILING
    profiling_init();
#endif

    if (cmd_options.gdb) {
        gdbstub_init();
        gdbstub_accept_client();
    }

    LOG_NOTHREAD(INFO, "%s", "Starting emulation");

    // While there are active threads or the network emulator is still not done
    while(  (chip.emu_done() == false)
         && (   !running_threads.empty()
             || !fcc_wait_threads[0].empty()
             || !fcc_wait_threads[1].empty()
             || !port_wait_threads.empty()
             ||  chip.pu_rvtimer_is_active()
             ||  chip.spio_rvtimer_is_active()
             || (api_listener != nullptr)
             || (cmd_options.gdb && (gdbstub_get_status() != GDBSTUB_STATUS_NOT_INITIALIZED))
            )
         && (emu_cycle < cmd_options.max_cycles)
    )
    {
#ifdef SYSEMU_DEBUG
        if (cmd_options.debug)
            debug_check();
#endif

        if (cmd_options.gdb) {
            switch (gdbstub_get_status()) {
            case GDBSTUB_STATUS_WAITING_CLIENT:
                gdbstub_accept_client();
                break;
            case GDBSTUB_STATUS_RUNNING:
                /* Non-blocking, consumes all the pending GDB commands */
                gdbstub_io();
                break;
            default:
                break;
            }
        }

        // Runtime API: Process new commands
        if (api_listener) {
            api_listener->process();
        }

        // Update peripherals/devices
        chip.tick_peripherals(emu_cycle);

        auto thread = running_threads.begin();

        while(thread != running_threads.end())
        {
            auto thread_id = * thread;

            // lazily erase disabled threads from the active thread list
            if (!thread_is_active(thread_id) || thread_is_disabled(thread_id))
            {
                thread = running_threads.erase(thread);
                LOG_AGENT(DEBUG, chip.cpu[thread_id], "%s", "Disabling thread");
                continue;
            }

            if (chip.thread_is_blocked(thread_id))
            {
                ++thread;
                continue;
            }

            try
            {
                // Gets instruction and sets state
                clearlogstate();
                chip.cpu[thread_id].check_pending_interrupts();
                // In case of reduce, we need to make sure that the other
                // thread is also in reduce state before we complete execution
                if (thread_in_reduce_send(chip.cpu[thread_id]))
                {
                    LOG_AGENT(DEBUG, chip.cpu[thread_id], "Waiting to send data to H%u", chip.cpu[thread_id].core->reduce.thread);
                    ++thread;
                }
                else if (thread_in_reduce_recv(chip.cpu[thread_id]))
                {
                    unsigned other_thread = chip.cpu[thread_id].core->reduce.thread;
                    // If pairing minion is in ready to send, consume the data
                    if (thread_in_reduce_send(chip.cpu[other_thread]) && (chip.cpu[other_thread].core->reduce.thread == thread_id))
                    {
                        bemu::tensor_reduce_execute(chip.cpu[thread_id]);
                    }
                    else
                    {
                        LOG_AGENT(DEBUG, chip.cpu[thread_id], "Waiting to receive data from H%u", other_thread);
                    }
                    ++thread;
                }
                else if (chip.cpu[thread_id].wait.state != bemu::Hart::Wait::State::Idle)
                {
                    if (chip.cpu[thread_id].wait.state == bemu::Hart::Wait::State::WaitReady)
                    {
                        bemu::tensor_wait_execute(chip.cpu[thread_id]);
                    }
                    else if (chip.cpu[thread_id].wait.state == bemu::Hart::Wait::State::Wait)
                    {
                        LOG_AGENT(DEBUG, chip.cpu[thread_id], "%s", "Rechecking TensorWait state");
                        bemu::tensor_wait_start(chip.cpu[thread_id], chip.cpu[thread_id].wait.value);
                    }
                    else if (chip.cpu[thread_id].wait.state == bemu::Hart::Wait::State::TxFMA)
                    {
                        LOG_AGENT(DEBUG, chip.cpu[thread_id], "%s", "Rechecking TensorFMA state");
                        bemu::tensor_fma_execute(chip.cpu[thread_id]);
                    }
                    ++thread;
                }
                else
                {
                    // Executes the instruction
                    chip.cpu[thread_id].fetch();

                    // Check for breakpoints
                    if ((gdbstub_get_status() == GDBSTUB_STATUS_RUNNING) && breakpoint_exists(thread_get_pc(thread_id))) {
                        LOG_AGENT(DEBUG, chip.cpu[thread_id], "Hit breakpoint at address 0x%" PRIx64, thread_get_pc(thread_id));
                        gdbstub_signal_break(thread_id);
                        running_threads.clear();
                        break;
                    }

                    // Dumping when M0:T0 reaches a PC
                    auto range = cmd_options.dump_at_pc.equal_range(thread_get_pc(0));
                    for (auto it = range.first; it != range.second; ++it) {
                        bemu::dump_data(chip.memory, bemu::Noagent{&chip},
                                        it->second.file.c_str(), it->second.addr, it->second.size);
                    }

                    // Logging
                    if (thread_get_pc(0) == cmd_options.log_at_pc) {
                        bemu::log.setLogLevel(LOG_DEBUG);
                    } else if (thread_get_pc(0) == cmd_options.stop_log_at_pc) {
                        bemu::log.setLogLevel(LOG_INFO);
                    }

                    chip.cpu[thread_id].execute();

                    chip.cpu[thread_id].notify_pmu_minion_event(PMU_MINION_EVENT_RETIRED_INST0 + (thread_id & 1));

                    if (chip.cpu[thread_id].get_msg_port_stall(0))
                    {
                        thread = running_threads.erase(thread);
                        port_wait_threads.push_back(thread_id);
                        if (thread == running_threads.end()) break;
                    }
                    else
                    {
                        if (chip.cpu[thread_id].inst.is_fcc_write())
                        {
                            unsigned cnt = chip.cpu[thread_id].fcc_cnt;
                            int old_thread = *thread;

                            // Checks if there's a pending FCC and wakes up thread again
                            if (pending_fcc[old_thread][cnt]==0)
                            {
                                LOG_AGENT(DEBUG, chip.cpu[thread_id], "Going to sleep (FCC%u)", 2*(thread_id & 1) + cnt);
                                thread = running_threads.erase(thread);
                                fcc_wait_threads[cnt].push_back(thread_id);
                            }
                            else
                            {
                                pending_fcc[old_thread][cnt]--;
                            }
                        }
                        else if (chip.cpu[thread_id].inst.is_wfi())
                        {
                            if (chip.cpu[thread_id].core->excl_mode) {
                                LOG_AGENT(DEBUG, chip.cpu[thread_id], "%s", "Not going to sleep (WFI) because exclusive mode");
                            } else {
                                LOG_AGENT(DEBUG, chip.cpu[thread_id], "%s", "Going to sleep (WFI)");
                                wfi_stall_wait_threads.push_back(thread_id);
                                thread = running_threads.erase(thread);
                                raise_interrupt_wakeup_check(thread_id);
                            }
                        }
                        else if (chip.cpu[thread_id].inst.is_stall_write())
                        {
                            if (chip.cpu[thread_id].core->excl_mode) {
                                LOG_AGENT(DEBUG, chip.cpu[thread_id], "%s", "Not going to sleep (STALL) because exclusive mode");
                            } else {
                                LOG_AGENT(DEBUG, chip.cpu[thread_id], "%s", "Going to sleep (STALL)");
                                wfi_stall_wait_threads.push_back(thread_id);
                                thread = running_threads.erase(thread);
                                raise_interrupt_wakeup_check(thread_id);
                            }
                        }
                        else
                        {
                            ++thread;
                        }
                        chip.cpu[thread_id].advance_pc();
                    }
                }
            }
            catch (const bemu::trap_t& t)
            {
                uint64_t old_pc = thread_get_pc(thread_id);
                chip.cpu[thread_id].take_trap(t);
                chip.cpu[thread_id].advance_pc();
                if (thread_get_pc(thread_id) == old_pc)
                {
                    LOG_AGENT(FTL, chip.cpu[thread_id],
                              "Trapping to the same address that caused a trap (0x%" PRIx64
                              "). Avoiding infinite trap recursion.",
                              thread_get_pc(thread_id));
                }
                ++thread;
            }
            catch (const bemu::memory_error& e)
            {
                chip.cpu[thread_id].advance_pc();
                bemu::raise_bus_error_interrupt(chip.cpu[thread_id], e.addr);
                ++thread;
            }
            catch (const std::exception& e)
            {
                LOG_AGENT(FTL, chip.cpu[thread_id], "%s", e.what());
            }

            // Check for single-step mode
            if ((gdbstub_get_status() == GDBSTUB_STATUS_RUNNING) && single_step[thread_id]) {
                LOG_AGENT(DEBUG, chip.cpu[thread_id], "%s", "Single-step done");
                gdbstub_signal_break(thread_id);
                single_step[thread_id] = false;
                running_threads.clear();
                break;
            }

            chip.cpu[thread_id].notify_pmu_minion_event(PMU_MINION_EVENT_CYCLES);
        }

        emu_cycle++;
    }

    if (emu_cycle == cmd_options.max_cycles)
    {
       // Dumps awaken threads
       LOG_NOTHREAD(INFO, "%s", "Dumping awaken threads:");
       auto thread = running_threads.begin();
       while(thread != running_threads.end())
       {
          auto thread_id = * thread;
          LOG_NOTHREAD(INFO, "\tThread %" SCNd32 ", PC: 0x%" PRIx64, thread_id, thread_get_pc(thread_id));
          thread++;
       }

       // Dumps FCC wait threads
       for (int i = 0; i < 2; ++i)
       {
           LOG_NOTHREAD(INFO, "Dumping FCC%d wait threads:", i);
           thread = fcc_wait_threads[i].begin();
           while(thread != fcc_wait_threads[i].end())
           {
               auto thread_id = * thread;
               LOG_NOTHREAD(INFO, "\tThread %" SCNd32 ", PC: 0x%" PRIx64, thread_id, thread_get_pc(thread_id));
               thread++;
           }
       }

       // Dumps Port wait threads
       LOG_NOTHREAD(INFO, "%s", "Dumping port wait threads:");
       thread = port_wait_threads.begin();
       while(thread != port_wait_threads.end())
       {
          auto thread_id = * thread;
          LOG_NOTHREAD(INFO, "\tThread %" SCNd32 ", PC: 0x%" PRIx64, thread_id, thread_get_pc(thread_id));
          thread++;
       }
       LOG_NOTHREAD(ERR, "Error, max cycles reached (%" SCNd64 ")", cmd_options.max_cycles);
    }
    LOG_NOTHREAD(INFO, "%s", "Finishing emulation");

    if (cmd_options.gdb)
        gdbstub_fini();

    // Dumping
    for (auto &dump: cmd_options.dump_at_end)
        bemu::dump_data(chip.memory, bemu::Noagent{&chip}, dump.file.c_str(), dump.addr, dump.size);

    if(!cmd_options.dump_mem.empty())
        bemu::dump_data(chip.memory, bemu::Noagent{&chip}, cmd_options.dump_mem.c_str(), chip.memory.first(), (chip.memory.last() - chip.memory.first()) + 1);

    if(!cmd_options.pu_uart0_rx_file.empty())
        close(chip.pu_uart0_get_rx_fd());

    if(!cmd_options.pu_uart1_rx_file.empty())
        close(chip.pu_uart1_get_rx_fd());

    if(!cmd_options.spio_uart0_rx_file.empty())
        close(chip.spio_uart0_get_rx_fd());

    if(!cmd_options.spio_uart1_rx_file.empty())
        close(chip.spio_uart1_get_rx_fd());

    if(!cmd_options.pu_uart0_tx_file.empty())
        close(chip.pu_uart0_get_tx_fd());

    if(!cmd_options.pu_uart1_tx_file.empty())
        close(chip.pu_uart1_get_tx_fd());

    if(!cmd_options.spio_uart0_tx_file.empty())
        close(chip.spio_uart0_get_tx_fd());

    if(!cmd_options.spio_uart1_tx_file.empty())
        close(chip.spio_uart1_get_tx_fd());

#ifdef SYSEMU_PROFILING
    if (!cmd_options.dump_prof_file.empty()) {
        profiling_flush();
        profiling_dump(cmd_options.dump_prof_file.c_str());
    }
    profiling_fini();
#endif

    return 0;
}
