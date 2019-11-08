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
#include "devices/rvtimer.h"
#include "emu.h"
#include "emu_gio.h"
#include "esrs.h"
#include "gdbstub.h"
#include "insn.h"
#include "log.h"
#include "memory/dump_data.h"
#include "memory/load.h"
#include "memory/main_memory.h"
#include "mmu.h"
#include "processor.h"
#include "profiling.h"
#include "scp_directory.h"
#include "utils.h"

extern std::array<Processor,EMU_NUM_THREADS> cpu;
extern std::array<uint32_t,EMU_NUM_THREADS>  ext_seip;

////////////////////////////////////////////////////////////////////////////////
// Static Member variables
////////////////////////////////////////////////////////////////////////////////

uint64_t        sys_emu::emu_cycle = 0;
std::list<int>  sys_emu::running_threads;                                               // List of running threads
std::list<int>  sys_emu::wfi_wait_threads;                                              // List of threads waiting in a WFI
std::list<int>  sys_emu::fcc_wait_threads[EMU_NUM_FCC_COUNTERS_PER_THREAD];             // List of threads waiting for an FCC
std::list<int>  sys_emu::port_wait_threads;                                             // List of threads waiting for a port write
std::bitset<EMU_NUM_THREADS> sys_emu::active_threads;                                   // List of threads being simulated
uint16_t        sys_emu::pending_fcc[EMU_NUM_THREADS][EMU_NUM_FCC_COUNTERS_PER_THREAD]; // Pending FastCreditCounter list
uint64_t        sys_emu::current_pc[EMU_NUM_THREADS];                                   // PC for each thread
int             sys_emu::global_log_min;
RVTimer         sys_emu::pu_rvtimer;
uint64_t        sys_emu::minions_en = 1;
uint64_t        sys_emu::shires_en  = 1;
bool            sys_emu::coherency_check = false;
mem_directory   sys_emu::mem_dir;
bool            sys_emu::scp_check       = false;
scp_directory   sys_emu::scp_dir;
std::unique_ptr<api_communicate> sys_emu::api_listener;
sys_emu_cmd_options              sys_emu::cmd_options;
std::unordered_set<uint64_t> sys_emu::breakpoints;
std::bitset<EMU_NUM_THREADS> sys_emu::single_step;

////////////////////////////////////////////////////////////////////////////////
// Helper methods
////////////////////////////////////////////////////////////////////////////////

static inline bool multithreading_is_disabled(unsigned shire)
{
    return bemu::shire_other_esrs[shire].minion_feature & 0x10;
}

static inline bool thread_is_disabled(unsigned thread)
{
    return !cpu[thread].enabled;
}

////////////////////////////////////////////////////////////////////////////////
// Sends an FCC to the desired minions specified in thread mask to the 1st or
// second thread (thread_dest), to the counter 0 or 1 (cnt_dest), inside the shire
// of thread_src
////////////////////////////////////////////////////////////////////////////////

void
sys_emu::fcc_to_threads(unsigned shire_id, unsigned thread_dest, uint64_t thread_mask, unsigned cnt_dest)
{
    extern uint16_t fcc[EMU_NUM_THREADS][2];

    assert(thread_dest < 2);
    assert(cnt_dest < 2);
    for(int m = 0; m < EMU_MINIONS_PER_SHIRE; m++)
    {
        // Skip disabled minion
        if (((minions_en >> m) & 1) == 0) continue;

        if ((thread_mask >> m) & 1)
        {
            int thread_id = shire_id * EMU_THREADS_PER_SHIRE + m * EMU_THREADS_PER_MINION + thread_dest;
            assert(thread_id < EMU_NUM_THREADS);
            LOG_OTHER(DEBUG, thread_id, "Receiving FCC%u write", thread_dest*2 + cnt_dest);

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
                    LOG_OTHER(DEBUG, thread_id, "Disabled thread received FCC%u", thread_dest*2 + cnt_dest);
                } else {
                    LOG_OTHER(DEBUG, thread_id, "Waking up due to received FCC%u", thread_dest*2 + cnt_dest);
                    running_threads.push_back(thread_id);
                }
                fcc_wait_threads[cnt_dest].erase(thread);
                --fcc[thread_id][cnt_dest];
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// Sends an FCC to the desired minions specified in thread mask to the 1st or
// second thread (thread_dest), to the counter 0 or 1 (cnt_dest), inside the shire
// of thread_src
////////////////////////////////////////////////////////////////////////////////

void
sys_emu::msg_to_thread(int thread_id)
{
    auto thread = std::find(port_wait_threads.begin(), port_wait_threads.end(), thread_id);
    LOG_NOTHREAD(INFO, "Message to thread %i with log %i", thread_id, global_log_min);
    // Checks if in port wait state
    if(thread != port_wait_threads.end())
    {
        if (!thread_is_active(thread_id) || thread_is_disabled(thread_id)) {
            LOG_OTHER(DEBUG, thread_id, "%s", "Disabled thread received msg");
        } else {
            LOG_OTHER(DEBUG, thread_id, "%s", "Waking up due msg");
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
    uint64_t ipi_redirect_filter = bemu::shire_other_esrs[shire].ipi_redirect_filter;

    unsigned thread0 = EMU_THREADS_PER_SHIRE * shire;
    for(unsigned t = 0; t < EMU_THREADS_PER_SHIRE; t++)
    {
        // If both IPI_REDIRECT_TRIGGER and IPI_REDIRECT_FILTER has bit set
        if(((thread_mask >> t) & 1) && ((ipi_redirect_filter >> t) & 1))
        {
            // Get PC
            unsigned tid = thread0 + t;
            unsigned neigh = tid / EMU_THREADS_PER_NEIGH;
            uint64_t new_pc = bemu::neigh_esrs[neigh].ipi_redirect_pc;
            LOG_OTHER(DEBUG, tid, "Receiving IPI_REDIRECT to %llx", (long long unsigned int) new_pc);
            // If thread sleeping, wakes up and changes PC
            if(!thread_is_running(tid))
            {
                if (!thread_is_active(tid) || thread_is_disabled(tid)) {
                    LOG_OTHER(DEBUG, tid, "%s", "Disabled thread received IPI_REDIRECT");
                } else {
                    LOG_OTHER(DEBUG, tid, "%s", "Waking up due to IPI_REDIRECT");
                    running_threads.push_back(tid);
                    current_pc[tid] = new_pc;
                }
            }
            // Otherwise IPI is dropped
            else
            {
                LOG_OTHER(DEBUG, tid, "%s", "WARNING => IPI_REDIRECT dropped");
            }
        }
    }
}

void
sys_emu::raise_interrupt_wakeup_check(unsigned thread_id, uint64_t mask, const char *str)
{
    bool wakeup = false;

    // WFI doesn't require the global interrupt bits in mstatus to be enabled
    uint64_t xip = (cpu[thread_id].mip | ext_seip[thread_id]) & cpu[thread_id].mie;
    if (xip & mask) {
        wakeup = true;
    } else {
        // If there's a trap, catch it and wakeup the thread
        unsigned old_thread = get_thread();
        set_thread(thread_id);
        try {
            check_pending_interrupts();
        } catch (const trap_t& t) {
            wakeup = true;
        }
        set_thread(old_thread);
    }

    if (wakeup) {
        LOG_OTHER(DEBUG, thread_id, "Waking up due to %s", str);

        // Put the interrupted thread back to running
        running_threads.push_back(thread_id);

        // If it was waiting in a WFI, remove it
        auto wfi_it = std::find(wfi_wait_threads.begin(),
                                wfi_wait_threads.end(),
                                thread_id);
        if (wfi_it != wfi_wait_threads.end())
            wfi_wait_threads.erase(wfi_it);

        // If it was waiting for an FCC, remove it
        for (auto &fcc_wait_it: fcc_wait_threads) {
            auto th = std::find(fcc_wait_it.begin(),
                                fcc_wait_it.end(),
                                thread_id);
            if (th != fcc_wait_it.end())
                fcc_wait_it.erase(th);
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

        uint32_t target = bemu::shire_other_esrs[s].mtime_local_target;
        for (unsigned m = 0; m < shire_minion_count; m++) {
            if (!(target & (1ULL << m)))
                continue;

            for (unsigned ii = 0; ii < minion_thread_count; ii++) {
                int thread_id = s * EMU_THREADS_PER_SHIRE + m * EMU_THREADS_PER_MINION + ii;
                LOG_OTHER(DEBUG, thread_id, "%s", "Receiving Machine timer interrupt");
                ::raise_timer_interrupt(thread_id);

                if (!thread_is_active(thread_id) || thread_is_disabled(thread_id))
                    continue;

                // Check if the thread has to be awakened
                if (!thread_is_running(thread_id)) {
                    raise_interrupt_wakeup_check(thread_id,
                                                 1ull << MACHINE_TIMER_INTERRUPT,
                                                 "Machine timer interrupt");
                }
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

        uint32_t target = bemu::shire_other_esrs[s].mtime_local_target;
        for (unsigned m = 0; m < shire_minion_count; m++) {
            if (target & (1ULL << m)) {
                for (unsigned ii = 0; ii < minion_thread_count; ii++) {
                    int thread_id = s * EMU_THREADS_PER_SHIRE + m * EMU_THREADS_PER_MINION + ii;
                    ::clear_timer_interrupt(thread_id);
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
            LOG_OTHER(DEBUG, thread_id, "%s", "Disabled thread received IPI");
            continue;
        }

        LOG_OTHER(DEBUG, thread_id, "%s", "Receiving IPI");
        ::raise_software_interrupt(thread_id);

        // Check if the thread has to be awakened
        if (!thread_is_running(thread_id)) {
            raise_interrupt_wakeup_check(thread_id,
                                         1ull << MACHINE_SOFTWARE_INTERRUPT,
                                         "IPÌ");
        }
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
            ::clear_software_interrupt(thread_id);
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
        LOG_OTHER(DEBUG, thread_id, "%s", "Receiving Machine external interrupt");
        ::raise_external_machine_interrupt(thread_id);

        if (!thread_is_active(thread_id) || thread_is_disabled(thread_id))
            continue;

        // Check if the thread has to be awakened
        if (!thread_is_running(thread_id)) {
            raise_interrupt_wakeup_check(thread_id,
                                         1ull << MACHINE_EXTERNAL_INTERRUPT,
                                         "Machine external interrupt");
        }
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
        ::clear_external_machine_interrupt(thread_id);
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
        LOG_OTHER(DEBUG, thread_id, "%s", "Receiving Supervisor external interrupt");
        ::raise_external_supervisor_interrupt(thread_id);

        if (!thread_is_active(thread_id) || thread_is_disabled(thread_id))
            continue;

        // Check if the thread has to be awakened
        if (!thread_is_running(thread_id)) {
            raise_interrupt_wakeup_check(thread_id,
                                         1ull << SUPERVISOR_EXTERNAL_INTERRUPT,
                                         "Supervisor external interrupt");
        }
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
        ::clear_external_supervisor_interrupt(thread_id);
    }
}

bool sys_emu::init_api_listener(const char *communication_path, bemu::MainMemory* memory) {
    (void) communication_path;
    (void) memory;
    // Now api_communicate is an interface
    /*api_listener = std::unique_ptr<api_communicate>(new api_communicate(memory));

    api_listener->set_comm_path(communication_path);

    if(!api_listener->init())
    {
        throw std::runtime_error("Failed to initialize api listener");
    }*/
    return true;
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
sys_emu::shire_enable_threads(unsigned shire_id)
{
    if (shire_id == IO_SHIRE_ID)
        shire_id = EMU_IO_SHIRE_SP;

    unsigned thread0 = EMU_THREADS_PER_SHIRE * shire_id;
    unsigned shire_thread_count = (shire_id == EMU_IO_SHIRE_SP ? 1 : EMU_THREADS_PER_SHIRE);

    bemu::write_thread0_disable(shire_id, 0);
    bemu::write_thread1_disable(shire_id, 0);

    for (unsigned t = 0; t < shire_thread_count; ++t) {
        unsigned thread_id = thread0 + t;
        if (thread_is_active(thread_id) && !thread_is_disabled(thread_id))
            running_threads.push_back(thread_id);
    }
}

void
sys_emu::breakpoint_insert(uint64_t addr)
{
    LOG_NOTHREAD(INFO, "Inserting breakpoint at address 0x%" PRIx64 "", addr);
    breakpoints.insert(addr);
}

void
sys_emu::breakpoint_remove(uint64_t addr)
{
    LOG_NOTHREAD(INFO, "Removing breakpoint at address 0x%" PRIx64 "", addr);
    breakpoints.erase(addr);
}

bool
sys_emu::breakpoint_exists(uint64_t addr)
{
    return contains(breakpoints, addr);
}

////////////////////////////////////////////////////////////////////////////////
/// Main initialization function.
///
/// The initialization function is separate by the constructor because we need
/// to overwrite specific parts of the initialization in subclasses
////////////////////////////////////////////////////////////////////////////////

bool
sys_emu::init_simulator(const sys_emu_cmd_options& cmd_options)
{
    this->cmd_options = cmd_options;
    if (cmd_options.elf_file.empty()      &&
        cmd_options.mem_desc_file.empty() &&
        cmd_options.api_comm_path.empty())
    {
        LOG_NOTHREAD(FTL, "%s", "Need an elf file or a mem_desc file or runtime API!");
    }

#ifdef SYSEMU_DEBUG
    if (cmd_options.debug == true) {
       LOG_NOTHREAD(INFO, "%s", "Starting in interactive mode.");
       debug_init();
    }
#endif

    emu::log.setLogLevel(cmd_options.log_en ? LOG_DEBUG : LOG_INFO);

    // Init emu
    init_emu(system_version_t::ETSOC1_A0);
    log_only_minion(cmd_options.log_min);
    global_log_min = cmd_options.log_min;
    bemu::memory_reset_value = cmd_options.mem_reset;

    // Callbacks for port writes
    set_msg_funcs(msg_to_thread);

    // Parses the memory description
    if (!cmd_options.elf_file.empty()) {
        try {
            bemu::load_elf(bemu::memory, cmd_options.elf_file.c_str());
        }
        catch (...) {
            LOG_NOTHREAD(FTL, "Error loading ELF \"%s\"", cmd_options.elf_file.c_str());
            return false;
        }
    }
    if (!cmd_options.mem_desc_file.empty()) {
        if (!parse_mem_file(cmd_options.mem_desc_file.c_str()))
            return false;
    }

    // Setup PU UART stream
    if (!cmd_options.pu_uart_tx_file.empty()) {
        int fd = open(cmd_options.pu_uart_tx_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            LOG_NOTHREAD(FTL, "Error creating \"%s\"", cmd_options.pu_uart_tx_file.c_str());
            return false;
        }
        bemu::memory.pu_io_space.pu_uart.fd = fd;
    } else {
        bemu::memory.pu_io_space.pu_uart.fd = STDOUT_FILENO;
    }

    // Setup PU UART1 stream
    if (!cmd_options.pu_uart1_tx_file.empty()) {
        int fd = open(cmd_options.pu_uart1_tx_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            LOG_NOTHREAD(FTL, "Error creating \"%s\"", cmd_options.pu_uart1_tx_file.c_str());
            return false;
        }
        bemu::memory.pu_io_space.pu_uart1.fd = fd;
    } else {
        bemu::memory.pu_io_space.pu_uart1.fd = STDOUT_FILENO;
    }

    // Initialize Simulator API
    if (!cmd_options.api_comm_path.empty()) {
        init_api_listener(cmd_options.api_comm_path.c_str(), &bemu::memory);
    }

    // Reset the SoC

    running_threads.clear();
    fcc_wait_threads[0].clear();
    fcc_wait_threads[1].clear();
    port_wait_threads.clear();
    active_threads.reset();
    bzero(pending_fcc, sizeof(pending_fcc));

    for (unsigned s = 0; s < EMU_NUM_SHIRES; s++) {
        reset_esrs_for_shire(s);

        // Skip disabled shire
        if (((shires_en >> s) & 1) == 0)
            continue;

        // Skip master shire if not enabled
        if ((cmd_options.master_min == 0) && (s >= EMU_MASTER_SHIRE))
            continue;

        bool disable_multithreading =
                !cmd_options.second_thread || multithreading_is_disabled(s);

        unsigned minion_thread_count =
                disable_multithreading ? 1 : EMU_THREADS_PER_MINION;

        unsigned shire_minion_count =
                (s == EMU_IO_SHIRE_SP) ? 1 : EMU_MINIONS_PER_SHIRE;

        // Enable threads
        uint32_t minion_mask = minions_en & ((1ull << shire_minion_count) - 1);
        bemu::write_thread0_disable(s, ~minion_mask);
        if (disable_multithreading) {
            bemu::write_thread1_disable(s, 0xffffffff);
        } else {
            bemu::write_thread1_disable(s, ~minion_mask);
        }

        // For all the minions
        for (unsigned m = 0; m < shire_minion_count; m++) {
            // Skip disabled minion
            if (((minions_en >> m) & 1) == 0)
                continue;

            // Inits threads
            for (unsigned t = 0; t < minion_thread_count; t++) {
                unsigned tid = s * EMU_THREADS_PER_SHIRE + m * EMU_THREADS_PER_MINION + t;
                LOG_OTHER(DEBUG, tid, "%s", "Resetting");
                current_pc[tid] = (s == EMU_IO_SHIRE_SP) ? cmd_options.sp_reset_pc : cmd_options.reset_pc;

                reset_hart(tid);
                set_thread(tid);
                minit(m0, 255);

                // Puts thread id in the active list
                activate_thread(tid);
                if (!cmd_options.mins_dis) {
                    assert(!thread_is_disabled(tid));
                    running_threads.push_back(tid);
                }
            }
        }
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
// Main function implementation
////////////////////////////////////////////////////////////////////////////////

int
sys_emu::main_internal(int argc, char * argv[])
{
#if 0
    std::cout << "command:";
    for (int i = 0; i < argc; ++i)
        std::cout << " " << argv[i];
    std::cout << std::endl;
#endif
    auto result = parse_command_line_arguments(argc, argv);
    bool status = std::get<0>(result);
    sys_emu_cmd_options cmd_options = std::get<1>(result);

    if (!status) {
        return 0;
    }

    if (!init_simulator(cmd_options))
        return EXIT_FAILURE;

#ifdef SYSEMU_PROFILING
    profiling_init();
#endif

    if (cmd_options.gdb) {
        gdbstub_init();
        gdbstub_accept_client();
    }

    LOG_NOTHREAD(INFO, "%s", "Starting emulation");

    // While there are active threads or the network emulator is still not done
    while(  (emu_done() == false)
         && (   !running_threads.empty()
             || !fcc_wait_threads[0].empty()
             || !fcc_wait_threads[1].empty()
             || !port_wait_threads.empty()
             ||  pu_rvtimer.is_active()
             || (api_listener && api_listener->is_enabled())
             || (cmd_options.gdb && (gdbstub_get_status() != GDBSTUB_STATUS_NOT_INITIALIZED))
            )
         && (emu_cycle < cmd_options.max_cycles)
         && !(api_listener && api_listener->is_done())
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

        // Runtime API: check for new commands
        if (api_listener) {
            api_listener->get_next_cmd(&running_threads);
        }

        // Update devices
        pu_rvtimer.update(emu_cycle);

        auto thread = running_threads.begin();

        while(thread != running_threads.end())
        {
            auto thread_id = * thread;

            // lazily erase disabled threads from the active thread list
            if (!thread_is_active(thread_id) || thread_is_disabled(thread_id))
            {
                thread = running_threads.erase(thread);
                LOG_OTHER(DEBUG, thread_id, "%s", "Disabling thread");
                continue;
            }

            if (thread_is_blocked(thread_id))
            {
                ++thread;
                continue;
            }

            // Try to execute one instruction, this may trap
            insn_t inst {0, 0, nullptr};

            try
            {
                // Gets instruction and sets state
                clearlogstate();
                set_thread(thread_id);
                set_pc(current_pc[thread_id]);
                check_pending_interrupts();
                // In case of reduce, we need to make sure that the other
                // thread is also in reduce state before we complete execution
                if (cpu[thread_id].reduce.state == Processor::Reduce::State::Send)
                {
                    LOG(DEBUG, "Waiting to send data to H%u", cpu[thread_id].reduce.thread);
                    ++thread;
                }
                else if (cpu[thread_id].reduce.state == Processor::Reduce::State::Recv)
                {
                    unsigned other_thread = cpu[thread_id].reduce.thread;
                    // If pairing minion is in ready to send, consume the data
                    if ((cpu[other_thread].reduce.state == Processor::Reduce::State::Send) &&
                        (cpu[other_thread].reduce.thread == thread_id))
                    {
                        tensor_reduce_execute();
                    }
                    else
                    {
                        LOG(DEBUG, "Waiting to receive data from H%u", other_thread);
                    }
                    ++thread;
                }
                else
                {
                    // Executes the instruction
                    inst = fetch_and_decode();

                    // Check for breakpoints
                    if ((gdbstub_get_status() == GDBSTUB_STATUS_RUNNING) && breakpoint_exists(current_pc[thread_id])) {
                        LOG(DEBUG, "Hit breakpoint at address 0x%" PRIx64, current_pc[thread_id]);
                        gdbstub_signal(5);
                        running_threads.clear();
                        break;
                    }

                    // Dumping when M0:T0 reaches a PC
                    auto range = cmd_options.dump_at_pc.equal_range(current_pc[0]);
                    for (auto it = range.first; it != range.second; ++it) {
                            bemu::dump_data(bemu::memory, it->second.file.c_str(),
                                            it->second.addr, it->second.size);
                     }

                    // Logging
                    if (current_pc[0] == cmd_options.log_at_pc) {
                        emu::log.setLogLevel(LOG_DEBUG);
                    } else if (current_pc[0] == cmd_options.stop_log_at_pc) {
                       emu::log.setLogLevel(LOG_INFO);
                    }

                    execute(inst);

                    if (get_msg_port_stall(thread_id, 0))
                    {
                        thread = running_threads.erase(thread);
                        port_wait_threads.push_back(thread_id);
                        if (thread == running_threads.end()) break;
                    }
                    else
                    {
                        if (inst.is_fcc())
                        {
                            unsigned cnt = get_fcc_cnt();
                            int old_thread = *thread;

                            // Checks if there's a pending FCC and wakes up thread again
                            if (pending_fcc[old_thread][cnt]==0)
                            {
                                LOG(DEBUG, "Going to sleep (FCC%u)", 2*(thread_id & 1) + cnt);
                                thread = running_threads.erase(thread);
                                fcc_wait_threads[cnt].push_back(thread_id);
                            }
                            else
                            {
                                pending_fcc[old_thread][cnt]--;
                            }
                        }
                        else if (inst.is_wfi())
                        {
                            LOG(DEBUG, "%s", "Going to sleep (WFI)");
                            wfi_wait_threads.push_back(thread_id);
                            thread = running_threads.erase(thread);
                        }
                        else if (inst.is_stall())
                        {
                            LOG(DEBUG, "%s", "Going to sleep (STALL)");
                            thread = running_threads.erase(thread);
                        }
                        else
                        {
                            ++thread;
                        }
                        // Updates PC
                        if(emu_state_change.pc_mod) {
                            current_pc[thread_id] = emu_state_change.pc;
                        } else {
                            current_pc[thread_id] += inst.size();
                        }
                    }
                }
            }
            catch (const trap_t& t)
            {
                take_trap(t);
                //LOG(DEBUG, "%s", "Taking a trap");
                if (current_pc[thread_id] == emu_state_change.pc)
                {
                    LOG_ALL_MINIONS(FTL, "Trapping to the same address that caused a trap (0x%" PRIx64 "). Avoiding infinite trap recursion.",
                                    current_pc[thread_id]);
                }
                current_pc[thread_id] = emu_state_change.pc;
                ++thread;
            }
            catch (const bemu::memory_error& e)
            {
                current_pc[thread_id] += inst.size();
                raise_bus_error_interrupt(thread_id, e.addr);
                ++thread;
            }
            catch (const std::exception& e)
            {
                LOG_ALL_MINIONS(FTL, "%s", e.what());
            }

            // Check for single-step mode
            if ((gdbstub_get_status() == GDBSTUB_STATUS_RUNNING) && single_step[thread_id]) {
                LOG(DEBUG, "%s", "Single-step done");
                gdbstub_signal(5);
                single_step[thread_id] = false;
                running_threads.clear();
                break;
            }
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
          LOG_NOTHREAD(INFO, "\tThread %" SCNd32 ", PC: 0x%" PRIx64, thread_id, current_pc[thread_id]);
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
               LOG_NOTHREAD(INFO, "\tThread %" SCNd32 ", PC: 0x%" PRIx64, thread_id, current_pc[thread_id]);
               thread++;
           }
       }

       // Dumps Port wait threads
       LOG_NOTHREAD(INFO, "%s", "Dumping port wait threads:");
       thread = port_wait_threads.begin();
       while(thread != port_wait_threads.end())
       {
          auto thread_id = * thread;
          LOG_NOTHREAD(INFO, "\tThread %" SCNd32 ", PC: 0x%" PRIx64, thread_id, current_pc[thread_id]);
          thread++;
       }
       LOG_NOTHREAD(ERR, "Error, max cycles reached (%" SCNd64 ")", cmd_options.max_cycles);
    }
    LOG_NOTHREAD(INFO, "%s", "Finishing emulation");

    if (cmd_options.gdb)
        gdbstub_fini();

    // Dumping
    for (auto &dump: cmd_options.dump_at_end)
        bemu::dump_data(bemu::memory, dump.file.c_str(), dump.addr, dump.size);

    if(!cmd_options.dump_mem.empty())
        bemu::dump_data(bemu::memory, cmd_options.dump_mem.c_str(), bemu::memory.first(), (bemu::memory.last() - bemu::memory.first()) + 1);

    if(!cmd_options.pu_uart_tx_file.empty())
        close(bemu::memory.pu_io_space.pu_uart.fd);

    if(!cmd_options.pu_uart1_tx_file.empty())
        close(bemu::memory.pu_io_space.pu_uart1.fd);

#ifdef SYSEMU_PROFILING
    if (!cmd_options.dump_prof_file.empty()) {
        profiling_flush();
        profiling_dump(cmd_options.dump_prof_file.c_str());
    }
    profiling_fini();
#endif

    return 0;
}
