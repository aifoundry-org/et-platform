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
#include "emu.h"
#include "emu_gio.h"
#include "esrs.h"
#include "insn.h"
#include "log.h"
#include "memory/dump_data.h"
#include "memory/load.h"
#include "memory/main_memory.h"
#include "mmu.h"
#include "net_emulator.h"
#include "api_communicate.h"
#include "processor.h"
#include "profiling.h"
#include "rvtimer.h"

extern std::array<Processor,EMU_NUM_THREADS> cpu;

////////////////////////////////////////////////////////////////////////////////
// Static Member variables
////////////////////////////////////////////////////////////////////////////////

uint64_t        sys_emu::emu_cycle = 0;
std::list<int>  sys_emu::enabled_threads;                                               // List of enabled threads
std::list<int>  sys_emu::fcc_wait_threads[2];                                           // List of threads waiting for an FCC
std::list<int>  sys_emu::port_wait_threads;                                             // List of threads waiting for a port write
std::bitset<EMU_NUM_THREADS> sys_emu::active_threads;                                   // List of threads being simulated
uint16_t        sys_emu::pending_fcc[EMU_NUM_THREADS][EMU_NUM_FCC_COUNTERS_PER_THREAD]; // Pending FastCreditCounter list
uint64_t        sys_emu::current_pc[EMU_NUM_THREADS];                                   // PC for each thread
int             sys_emu::global_log_min;
RVTimer         sys_emu::pu_rvtimer;
uint64_t        sys_emu::minions_en = 1;
uint64_t        sys_emu::shires_en  = 1;


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
// Parses a file that defines the memory regions plus contents to be
// loaded in the different regions
////////////////////////////////////////////////////////////////////////////////

static bool parse_mem_file(const char * filename)
{
    FILE * file = fopen(filename, "r");
    if (file == NULL)
    {
        LOG_NOTHREAD(FTL, "Parse Mem File Error -> Couldn't open file %s for reading!!", filename);
        return false;
    }

    // Parses the contents
    char buffer[1024];
    char * buf_ptr = (char *) buffer;
    size_t buf_size = 1024;
    while (getline(&buf_ptr, &buf_size, file) != -1)
    {
        uint64_t base_addr;
        uint64_t size;
        char str[1024];
        if(sscanf(buffer, "New Mem Region: 40'h%" PRIX64 ", 40'h%" PRIX64 ", %s", &base_addr, &size, str) == 3)
        {
            LOG_NOTHREAD(WARN, "Ignore: New Mem Region found: @ 0x%" PRIx64 ", size = 0x%" PRIu64, base_addr, size);
        }
        else if(sscanf(buffer, "File Load: 40'h%" PRIX64 ", %s", &base_addr, str) == 2)
        {
            LOG_NOTHREAD(INFO, "New File Load found: @ 0x%" PRIx64, base_addr);
            try
            {
                bemu::load_raw(bemu::memory, str, base_addr);
            }
            catch (...)
            {
                fclose(file);
                LOG_NOTHREAD(FTL, "Error loading file \"%s\"", str);
                return false;
            }
        }
        else if(sscanf(buffer, "ELF Load: %s", str) == 1)
        {
            LOG_NOTHREAD(INFO, "New ELF Load found: %s", str);
            try
            {
                bemu::load_elf(bemu::memory, str);
            }
            catch (...)
            {
                fclose(file);
                LOG_NOTHREAD(FTL, "Error loading ELF \"%s\"", str);
                return false;
            }
        }
    }
    // Closes the file
    fclose(file);
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// Help message
////////////////////////////////////////////////////////////////////////////////
static const char * help_msg =
"\n ET System Emulator\n\n\
     sys_emu [options]\n\n\
 Where options are one of:\n\
     -api_comm <path>         Path to socket that feeds runtime API commands.\n\
"
#ifdef SYSEMU_DEBUG
"     -d                      Start in interactive debug mode (must have been compiled with SYSEMU_DEBUG)\n"
#endif
"\
     -dump_addr <addr>        Address in memory at which to start dumping. Only valid if -dump_file is used\n\
     -dump_file <path>        Path to the file in which to dump the memory content at the end of the simulation\n\
     -dump_mem <path>         Path to the file in which to dump the memory content at the end of the simulation (dumps all the memory contents)\n\
"
#ifdef SYSEMU_PROFILING
"     -dump_prof <path>        Path to the file in which to dump the profiling content at the end of the simulation\n"
#endif
"\
     -dump_size <size>        Size of the memory to dump. Only valid if -dump_file is used\n\
     -elf <path>              Path to an ELF file to load.\n\
     -l                       Enable Logging\n\
     -lm <minion>             Log a given Minion ID only. (default: all)\n\
     -master_min              Enables master minion to send interrupts to compute minions.\n\
     -max_cycles <cycles>     Stops execution after provided number of cycles (default: 10M)\n\
     -mem_desc <path>         Path to a file describing the memory regions to create and what code to load there\n\
     -mem_reset <byte>        Reset value of main memory (default: 0)\n\
     -minions <mask>          A mask of Minions that should be enabled in each Shire (default: 1 Minion/Shire)\n\
     -mins_dis                Minions start disabled\n\
     -net_desc <path>         Path to a file describing emulation of a Maxion sending interrupts to minions.\n\
     -pu_uart_tx_file <path>  Path to the file in which to dump the contents of PU UART TX\n\
     -pu_uart1_tx_file <path> Path to the file in which to dump the contents of PU UART1 TX\n\
     -reset_pc <addr>         Sets boot program counter (default: 0x8000001000)\n\
     -shires <mask>           A mask of Shires that should be enabled. (default: 1 Shire)\n\
     -single_thread           Disable 2nd Minion thread\n\
     -sp_reset_pc <addr>      Sets Service Processor boot program counter (default: 0x40000000)\n\
";

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
                    enabled_threads.push_back(thread_id);
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
            enabled_threads.push_back(thread_id);
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
            if(std::find(enabled_threads.begin(), enabled_threads.end(), tid) == enabled_threads.end())
            {
                if (!thread_is_active(tid) || thread_is_disabled(tid)) {
                    LOG_OTHER(DEBUG, tid, "%s", "Disabled thread received IPI_REDIRECT");
                } else {
                    LOG_OTHER(DEBUG, tid, "%s", "Waking up due to IPI_REDIRECT");
                    enabled_threads.push_back(tid);
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
sys_emu::raise_timer_interrupt(uint64_t shire_mask)
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
                    if (thread_is_active(thread_id) && !thread_is_disabled(thread_id)) {
                        ::raise_timer_interrupt(thread_id);
                        if (std::find(enabled_threads.begin(), enabled_threads.end(), thread_id) == enabled_threads.end())
                            enabled_threads.push_back(thread_id);
                    }
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
    for (unsigned t = 0; t < shire_thread_count; ++t)
    {
        if (thread_mask & (1ull << t))
        {
            unsigned thread_id = thread0 + t;
            if (!thread_is_active(thread_id) || thread_is_disabled(thread_id)) {
                LOG_OTHER(DEBUG, thread_id, "%s", "Disabled thread received IPI");
            } else {
                LOG_OTHER(DEBUG, thread_id, "%s", "Receiving IPI");
                ::raise_software_interrupt(thread_id);
                // If thread sleeping, wakes up
                if (std::find(enabled_threads.begin(), enabled_threads.end(), thread_id) == enabled_threads.end())
                {
                    LOG_OTHER(DEBUG, thread_id, "%s", "Waking up due to IPI");
                    enabled_threads.push_back(thread_id);
                }
            }
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
        LOG_OTHER(DEBUG, thread_id, "%s", "Receiving External Interrupt");
        ::raise_external_machine_interrupt(thread_id);
        // If thread sleeping, wakes up
        if (std::find(enabled_threads.begin(), enabled_threads.end(), thread_id) == enabled_threads.end())
        {
            LOG_OTHER(DEBUG, thread_id, "%s", "Waking up due to External Interrupt");
            enabled_threads.push_back(thread_id);
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
        LOG_OTHER(DEBUG, thread_id, "%s", "Receiving External Supervisor Interrupt");
        ::raise_external_supervisor_interrupt(thread_id);
        // If thread sleeping, wakes up
        if (std::find(enabled_threads.begin(), enabled_threads.end(), thread_id) == enabled_threads.end())
        {
            LOG_OTHER(DEBUG, thread_id, "%s", "Waking up due to External Supervisor Interrupt");
            enabled_threads.push_back(thread_id);
        }
    }
}

bool sys_emu::init_api_listener(const char *communication_path, bemu::MainMemory* memory) {

    api_listener = std::unique_ptr<api_communicate>(new api_communicate(memory));

    api_listener->set_comm_path(communication_path);

    if(!api_listener->init())
    {
        throw std::runtime_error("Failed to initialize api listener");
    }
    return true;
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

std::tuple<bool, struct sys_emu_cmd_options>
sys_emu::parse_command_line_arguments(int argc, char* argv[])
{
    sys_emu_cmd_options cmd_options;

    for(int i = 1; i < argc; i++)
    {
        if (cmd_options.max_cycle)
        {
            cmd_options.max_cycle = false;
            sscanf(argv[i], "%" SCNu64, &cmd_options.max_cycles);
        }
        else if (cmd_options.elf)
        {
            cmd_options.elf = false;
            cmd_options.elf_file = argv[i];
        }
        else if(cmd_options.mem_desc)
        {
            cmd_options.mem_desc = false;
            cmd_options.mem_desc_file = argv[i];
        }
        else if(cmd_options.net_desc)
        {
            cmd_options.net_desc = false;
            cmd_options.net_desc_file = argv[i];
        }
        else if(cmd_options.api_comm)
        {
            cmd_options.api_comm = false;
            cmd_options.api_comm_path = argv[i];
        }
        else if(cmd_options.minions)
        {
            sscanf(argv[i], "%" PRIx64, &minions_en);
            cmd_options.minions = 0;
        }
        else if(cmd_options.shires)
        {
            sscanf(argv[i], "%" PRIx64, &shires_en);
            cmd_options.shires = 0;
        }
        else if(cmd_options.reset_pc_flag)
        {
          sscanf(argv[i], "%" PRIx64, &cmd_options.reset_pc);
          cmd_options.reset_pc_flag = false;
        }
        else if(cmd_options.sp_reset_pc_flag)
        {
          sscanf(argv[i], "%" PRIx64, &cmd_options.sp_reset_pc);
          cmd_options.sp_reset_pc_flag = false;
        }
        else if(cmd_options.dump == 1)
        {
            sscanf(argv[i], "%" PRIx64, &cmd_options.dump_addr);
            cmd_options.dump = 0;
        }
        else if(cmd_options.dump == 2)
        {
            cmd_options.dump_size = atoi(argv[i]);
            cmd_options.dump = 0;
        }
        else if(cmd_options.dump == 3)
        {
            cmd_options.dump_file = argv[i];
            cmd_options.dump = 0;
        }
        else if(cmd_options.dump == 4)
        {
            cmd_options.log_min = atoi(argv[i]);
            cmd_options.dump = 0;
        }
        else if(cmd_options.dump == 5)
        {
            cmd_options.dump_mem = argv[i];
            cmd_options.dump = 0;
        }
        else if(cmd_options.dump == 6)
        {
            cmd_options.pu_uart_tx_file = argv[i];
            cmd_options.dump = 0;
        }
        else if(cmd_options.dump == 7)
        {
            cmd_options.pu_uart1_tx_file = argv[i];
            cmd_options.dump = 0;
        }
        else if(cmd_options.mem_reset_flag)
        {
            cmd_options.mem_reset = atoi(argv[i]);
            cmd_options.mem_reset_flag = false;
        }
#ifdef SYSEMU_PROFILING
        else if(cmd_options.dump_prof == 1)
        {
            cmd_options.dump_prof_file = argv[i];
            cmd_options.dump_prof = 0;
        }
#endif
        else if(strcmp(argv[i], "-max_cycles") == 0)
        {
            cmd_options.max_cycle = true;
        }
        else if(strcmp(argv[i], "-elf") == 0)
        {
            cmd_options.elf = true;
        }
        else if(strcmp(argv[i], "-mem_desc") == 0)
        {
            cmd_options.mem_desc = true;
        }
        else if(strcmp(argv[i], "-net_desc") == 0)
        {
            cmd_options.net_desc = true;
        }
        else if(strcmp(argv[i], "-api_comm") == 0)
        {
            cmd_options.api_comm = true;
        }
        else if(strcmp(argv[i], "-master_min") == 0)
        {
            cmd_options.master_min = true;
        }
        else if(strcmp(argv[i], "-minions") == 0)
        {
            cmd_options.minions = true;
        }
        else if(strcmp(argv[i], "-shires") == 0)
        {
            cmd_options.shires = true;
        }
        else if(strcmp(argv[i], "-reset_pc") == 0)
        {
            cmd_options.reset_pc_flag = true;
        }
        else if(strcmp(argv[i], "-sp_reset_pc") == 0)
        {
            cmd_options.sp_reset_pc_flag = true;
        }
        else if(strcmp(argv[i], "-mem_reset") == 0)
        {
            cmd_options.mem_reset_flag = true;
        }
        else if(strcmp(argv[i], "-dump_addr") == 0)
        {
            cmd_options.dump = 1;
        }
        else if(strcmp(argv[i], "-dump_size") == 0)
        {
            cmd_options.dump = 2;
        }
        else if(strcmp(argv[i], "-dump_file") == 0)
        {
            cmd_options.dump = 3;
        }
        else if(strcmp(argv[i], "-lm") == 0)
        {
            cmd_options.dump = 4;
        }
        else if(strcmp(argv[i], "-dump_mem") == 0)
        {
            cmd_options.dump = 5;
        }
        else if(strcmp(argv[i], "-pu_uart_tx_file") == 0)
        {
            cmd_options.dump = 6;
        }
        else if(strcmp(argv[i], "-pu_uart1_tx_file") == 0)
        {
            cmd_options.dump = 7;
        }
        else if(strcmp(argv[i], "-m") == 0)
        {
            cmd_options.create_mem_at_runtime = true;
            LOG_NOTHREAD(WARN, "%s", "Ignoring deprecated option '-m'");
        }
        else if(strcmp(argv[i], "-l") == 0)
        {
            cmd_options.log_en = true;
        }
        else if (  (strcmp(argv[i], "-h") == 0)
                 ||(strcmp(argv[i], "-help") == 0)
                 ||(strcmp(argv[i], "--help") == 0)) {
           printf("%s", help_msg);
           std::tuple<bool, sys_emu_cmd_options> ret_value(false, sys_emu_cmd_options());
           return ret_value;
        }
        else if (strcmp(argv[i], "-single_thread") == 0)
        {
            cmd_options.second_thread = false;
        }
#ifdef SYSEMU_DEBUG
        else if(strcmp(argv[i], "-d") == 0)
        {
            cmd_options.debug = true;
        }
#endif
#ifdef SYSEMU_PROFILING
        else if(strcmp(argv[i], "-dump_prof") == 0)
        {
            cmd_options.dump_prof = 1;
        }
#endif
        else if(strcmp(argv[i], "-mins_dis") == 0)
        {
            cmd_options.mins_dis = true;
        }
        else
        {
            LOG_NOTHREAD(FTL, "Unknown parameter %s", argv[i]);
        }
    }

    std::tuple<bool, sys_emu_cmd_options> ret_value(true, cmd_options);
    return ret_value;
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
    if ((cmd_options.elf_file == NULL) && (cmd_options.mem_desc_file == NULL)
        && (cmd_options.api_comm_path == NULL))
    {
        LOG_NOTHREAD(FTL, "%s", "Need an elf file or a mem_desc file or runtime API!");
    }

    uint64_t drivers_enabled = 0;
    if (cmd_options.net_desc_file != NULL) drivers_enabled++;
    if (cmd_options.master_min)            drivers_enabled++;

    if (drivers_enabled > 1)
    {
        LOG_NOTHREAD(FTL, "%s", "Can't have net_desc and master_min set at same time!");
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
    if (cmd_options.elf_file != NULL) {
        try {
            bemu::load_elf(bemu::memory, cmd_options.elf_file);
        }
        catch (...) {
            LOG_NOTHREAD(FTL, "Error loading ELF \"%s\"", cmd_options.elf_file);
            return false;
        }
    }
    if (cmd_options.mem_desc_file != NULL) {
        if (!parse_mem_file(cmd_options.mem_desc_file))
            return false;
    }

    // Setup PU UART stream
    if (cmd_options.pu_uart_tx_file) {
        int fd = open(cmd_options.pu_uart_tx_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            LOG_NOTHREAD(FTL, "Error creating \"%s\"", cmd_options.pu_uart_tx_file);
            return false;
        }
        bemu::memory.pu_io_space.pu_uart.fd = fd;
    } else {
        bemu::memory.pu_io_space.pu_uart.fd = STDOUT_FILENO;
    }

    // Setup PU UART1 stream
    if (cmd_options.pu_uart1_tx_file) {
        int fd = open(cmd_options.pu_uart1_tx_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            LOG_NOTHREAD(FTL, "Error creating \"%s\"", cmd_options.pu_uart1_tx_file);
            return false;
        }
        bemu::memory.pu_io_space.pu_uart1.fd = fd;
    } else {
        bemu::memory.pu_io_space.pu_uart1.fd = STDOUT_FILENO;
    }

    // Initialize network
    net_emu = net_emulator(&bemu::memory);
    // Parses the net description (it emulates a Maxion sending interrupts to minions)
    if(cmd_options.net_desc_file != NULL)
    {
        net_emu.set_file(cmd_options.net_desc_file);
    }

    if (cmd_options.api_comm_path) {
        init_api_listener(cmd_options.api_comm_path, &bemu::memory);
    }

    // Reset the SoC

    enabled_threads.clear();
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
                    enabled_threads.push_back(tid);
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

    LOG_NOTHREAD(INFO, "%s", "Starting emulation");

    // While there are active threads or the network emulator is still not done
    while(  (emu_done() == false)
         && (   !enabled_threads.empty()
             || !fcc_wait_threads[0].empty()
             || !fcc_wait_threads[1].empty()
             || !port_wait_threads.empty()
             ||  pu_rvtimer.is_active()
             || (net_emu.is_enabled() && !net_emu.done())
             || (api_listener && api_listener->is_enabled() && !api_listener->is_done())
            )
         && (emu_cycle < cmd_options.max_cycles)
    )
    {
#ifdef SYSEMU_DEBUG
        if (cmd_options.debug)
            debug_check();
#endif

        // Update devices
        pu_rvtimer.update(emu_cycle);

        auto thread = enabled_threads.begin();

        while(thread != enabled_threads.end())
        {
            auto thread_id = * thread;

            // lazily erase disabled threads from the active thread list
            if (!thread_is_active(thread_id) || thread_is_disabled(thread_id))
            {
                thread = enabled_threads.erase(thread);
                LOG_OTHER(DEBUG, thread_id, "%s", "Disabling thread");
                continue;
            }

            if (thread_is_blocked(thread_id))
            {
                ++thread;
                continue;
            }

            // Try to execute one instruction, this may trap
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
                    insn_t inst = fetch_and_decode();
                    execute(inst);

                    if (get_msg_port_stall(thread_id, 0))
                    {
                        thread = enabled_threads.erase(thread);
                        port_wait_threads.push_back(thread_id);
                        if (thread == enabled_threads.end()) break;
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
                                thread = enabled_threads.erase(thread);
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
                            thread = enabled_threads.erase(thread);
                        }
                        else if (inst.is_stall())
                        {
                            LOG(DEBUG, "%s", "Going to sleep (STALL)");
                            thread = enabled_threads.erase(thread);
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
                insn_t inst = fetch_and_decode();
                current_pc[thread_id] += inst.size();
                raise_bus_error_interrupt(thread_id, e.addr);
                ++thread;
            }
            catch (const std::exception& e)
            {
                LOG_ALL_MINIONS(FTL, "%s", e.what());
            }
        }

        // Check interrupts from devices
        if (pu_rvtimer.interrupt_pending()) {
            raise_timer_interrupt((1ULL << EMU_NUM_SHIRES) - 1);
        } else if (pu_rvtimer.clear_pending()) {
            clear_timer_interrupt((1ULL << EMU_NUM_SHIRES) - 1);
        }

        // Net emu: check pending IPIs
        std::list<int> net_emu_ipi_threads;
        uint64_t net_emu_new_pc;
        net_emu.get_new_ipi(&enabled_threads, &net_emu_ipi_threads, &net_emu_new_pc);

        // Net emu: sets the PC for all the minions that got an IPI with PC
        for (auto it = net_emu_ipi_threads.begin(); it != net_emu_ipi_threads.end(); it++) {
            if(net_emu_new_pc != 0) current_pc[*it] = net_emu_new_pc; // 0 means resume
            if (thread_is_disabled(*it)) {
                LOG_OTHER(DEBUG, *it, "Disabled thread received IPI with PC 0x%" PRIx64, net_emu_new_pc);
            } else {
                enabled_threads.push_back(*it);
                LOG_OTHER(DEBUG, *it, "Waking up due IPI with PC 0x%" PRIx64, net_emu_new_pc);
            }
        }

        if (api_listener) {
            // Runtime API: check for new commands
            api_listener->get_next_cmd(&enabled_threads);
        }

        emu_cycle++;
    }

    if (emu_cycle == cmd_options.max_cycles)
    {
       // Dumps awaken threads
       LOG_NOTHREAD(INFO, "%s", "Dumping awaken threads:");
       auto thread = enabled_threads.begin();
       while(thread != enabled_threads.end())
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

    // Dumping
    if(cmd_options.dump_file != NULL)
        bemu::dump_data(bemu::memory, cmd_options.dump_file, cmd_options.dump_addr, cmd_options.dump_size);

    if(cmd_options.dump_mem)
        bemu::dump_data(bemu::memory, cmd_options.dump_mem, bemu::memory.first(), (bemu::memory.last() - bemu::memory.first()) + 1);

    if(cmd_options.pu_uart_tx_file != nullptr)
        close(bemu::memory.pu_io_space.pu_uart.fd);

    if(cmd_options.pu_uart1_tx_file != nullptr)
        close(bemu::memory.pu_io_space.pu_uart1.fd);

#ifdef SYSEMU_PROFILING
    if (cmd_options.dump_prof_file != NULL) {
        profiling_flush();
        profiling_dump(cmd_options.dump_prof_file);
    }
    profiling_fini();
#endif

    return 0;
}
