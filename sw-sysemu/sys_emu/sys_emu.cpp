#include "sys_emu.h"

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <list>
#include <exception>
#include <algorithm>
#include <locale>
#include <tuple>

#include "emu.h"
#include "emu_gio.h"
#include "emu_memop.h"
#include "esrs.h"
#include "mmu.h"
#include "insn.h"
#include "common/main_memory.h"
#include "log.h"
#include "profiling.h"
#include "net_emulator.h"
#include "api_communicate.h"

////////////////////////////////////////////////////////////////////////////////
// Static Member variables
////////////////////////////////////////////////////////////////////////////////

uint64_t        sys_emu::emu_cycle = 0;
std::list<int>  sys_emu::enabled_threads;                                               // List of enabled threads
std::list<int>  sys_emu::fcc_wait_threads[2];                                           // List of threads waiting for an FCC
std::list<int>  sys_emu::port_wait_threads;                                             // List of threads waiting for a port write
uint32_t        sys_emu::pending_fcc[EMU_NUM_THREADS][EMU_NUM_FCC_COUNTERS_PER_THREAD]; // Pending FastCreditCounter list
uint64_t        sys_emu::current_pc[EMU_NUM_THREADS];                                   // PC for each thread
reduce_state    sys_emu::reduce_state_array[EMU_NUM_MINIONS];                           // Reduce state
uint32_t        sys_emu::reduce_pair_array[EMU_NUM_MINIONS];                            // Reduce pairing minion
int             sys_emu::global_log_min;


////////////////////////////////////////////////////////////////////////////////
// Functions to emulate the main memory
////////////////////////////////////////////////////////////////////////////////

static main_memory* memory;

// This functions are called by emu. We should clean this to a nicer way...
static uint8_t emu_memread8(uint64_t addr)
{
    uint8_t ret;
    memory->read(addr, 1, &ret);
    return ret;
}

static uint16_t emu_memread16(uint64_t addr)
{
    uint16_t ret;
    memory->read(addr, 2, &ret);
    return ret;
}

static uint32_t emu_memread32(uint64_t addr)
{
    uint32_t ret;
    memory->read(addr, 4, &ret);
    return ret;
}

static uint64_t emu_memread64(uint64_t addr)
{
    uint64_t ret;
    memory->read(addr, 8, &ret);
    return ret;
}

static void emu_memwrite8(uint64_t addr, uint8_t data)
{
    memory->write(addr, 1, &data);
}

static void emu_memwrite16(uint64_t addr, uint16_t data)
{
    memory->write(addr, 2, &data);
}

static void emu_memwrite32(uint64_t addr, uint32_t data)
{
    memory->write(addr, 4, &data);
}

static void emu_memwrite64(uint64_t addr, uint64_t data)
{
    memory->write(addr, 8, &data);
}

////////////////////////////////////////////////////////////////////////////////
// Parses a file that defines the memory regions plus contents to be
// loaded in the different regions
////////////////////////////////////////////////////////////////////////////////

static bool parse_mem_file(const char * filename, main_memory * memory)
{
    FILE * file = fopen(filename, "r");
    if (file == NULL)
    {
        LOG_NOTHREAD(FTL, "Parse Mem File Error -> Couldn't open file %s for reading!!", filename);
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
            memory->new_region(base_addr, size);
            LOG_NOTHREAD(INFO, "New Mem Region found: @ 0x%" PRIx64 ", size = 0x%" PRIu64, base_addr, size);
        }
        else if(sscanf(buffer, "File Load: 40'h%" PRIX64 ", %s", &base_addr, str) == 2)
        {
            memory->load_file(str, base_addr);
            LOG_NOTHREAD(INFO, "New File Load found: @ 0x%" PRIx64, base_addr);
        }
        else if(sscanf(buffer, "ELF Load: %s", str) == 1)
        {
            memory->load_elf(str);
            LOG_NOTHREAD(INFO, "New ELF Load found: %s", str);
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
     sys_emu <-mem_desc <file> | -elf <file>> [-net_desc <file>] [-api_comm <path>] [-master_min] [-minions <mask>] [-shires <mask>] [-dump_file <file_name> [-dump_addr <address>] [-dump_size <size>]] [-l] [-lm <minion]> [-m] [-reset_pc <addr>] [-d] [-max_cycles <cycles>]\n\n\
 -mem_desc    Path to a file describing the memory regions to create and what code to load there\n\
 -elf         Path to an ELF file to load.\n\
 -net_desc    Path to a file describing emulation of a Maxion sending interrupts to minions.\n\
 -api_comm    Path to socket that feeds runtime API commands.\n\
 -master_min  Enables master minion to send interrupts to compute minions.\n\
 -minions     A mask of Minions that should be enabled in each Shire. Default: 1 Minion per shire\n\
 -shires      A mask of Shires that should be enabled. Default: 1 Shire\n\
 -dump_file   Path to the file in which to dump the memory content at the end of the simulation\n\
 -dump_addr   Address in memory at which to start dumping. Only valid if -dump_file is used\n\
 -dump_size   Size of the memory to dump. Only valid if -dump_file is used\n\
 -l           Enable Logging\n\
 -lm          Log a given Minion ID only. Default: all Minions\n\
 -m           Enable dynamic memory allocation. If a region of memory not specified in mem_desc is accessed, the model will create it instead of throwing an error.\n\
 -reset_pc    Sets boot program counter (default 0x8000001000) \n\
 -d           Start in interactive debug mode (must have been compiled with SYSEMU_DEBUG)\n\
 -max_cycles  Stops execution after provided number of cycles (default: 10M)\n\
 -mins_dis    Minions start disabled\n\
 -dump_mem    Path to the file in which to dump the memory content at the end of the simulation (dumps all the memory contents)\n"
#ifdef SYSEMU_PROFILING
"\
 -dump_prof   Path to the file in which to dump the profiling content at the end of the simulation\n"
#endif
;


static uint64_t minions_en = 1;
static uint64_t shires_en  = 1;

#ifdef SYSEMU_DEBUG
static int  steps          = 0;

struct pc_breakpoint_t {
    uint64_t pc;
    int      thread; // -1 == all threads
};

static std::list<pc_breakpoint_t> pc_breakpoints;

bool pc_breakpoints_exists(uint64_t pc, int thread)
{
    return pc_breakpoints.end() !=
        std::find_if(pc_breakpoints.begin(), pc_breakpoints.end(),
            [&](const pc_breakpoint_t &b) {
                return (b.pc == pc) && ((b.thread == -1) || b.thread == thread);
            }
        );
}

bool pc_breakpoints_add(uint64_t pc, int thread)
{
    if (pc_breakpoints_exists(pc, thread))
        return false;

    // If the breakpoint we are adding is global, remove all the local
    // breakpoints with the same pc
    if (thread == -1) {
        pc_breakpoints.remove_if(
            [&](const pc_breakpoint_t &b) {
                return b.pc == pc;
            }
        );
    }

    pc_breakpoints.push_back({pc, thread});
    return true;
}

void pc_breakpoints_dump(int thread)
{
    for (auto &it: pc_breakpoints) {
        if (it.thread == -1) // Global breakpoint
            printf("Breakpoint set for all threads at PC 0x%lx\n", it.pc);
        else if ((thread == -1) || (thread == it.thread))
            printf("Breakpoint set for thread %d at PC 0x%lx\n", it.thread, it.pc);
    }
}

void pc_breakpoints_clear_for_thread(int thread)
{
    pc_breakpoints.remove_if(
        [&](const pc_breakpoint_t &b) {
            return b.thread == thread;
        }
    );
}

void pc_breakpoints_clear(void)
{
    pc_breakpoints.clear();
}

static const char * help_dbg =
"\
help|h:                Print this message\n\
run|r:                 Execute until the end or a breakpoint is reached\n\
step|s [n]:            Execute n cycles (or 1 if not specified)\n\
pc [N]:                Dump PC of thread N (0 <= N < 2048)\n\
xdump|x [N]:           Dump GPRs of thread N (0 <= N < 2048)\n\
fdump|f [N]:           Dump FPRs of thread N (0 <= N < 2048)\n\
csr [N] <off>:         Dump the CSR at offset \"off\" of thread N (0 <= N < 2048)\n\
mdump|m <addr> <size>: Dump size bytes of memory at addr\n\
break|b [N] <PC>:      Set a breakpoint for the provided PC and thread N\n\
list_breaks [N]:       List the currently active breakpoints for a given thread N, or all if N == 0.\n\
clear_breaks [N]:      Clear all the breakpoints previously set if no N, or for thread N\n\
quit|q:                Terminate the program\n\
";

size_t split(const std::string &txt, std::vector<std::string> &strs, char ch = ' ')
{
   size_t pos = txt.find( ch );
   size_t initialPos = 0;
   strs.clear();

   // Decompose statement
   while( pos != std::string::npos ) {
      strs.push_back( txt.substr( initialPos, pos - initialPos ) );
      initialPos = pos + 1;

      pos = txt.find( ch, initialPos );
   }

   // Add the last one
   strs.push_back( txt.substr( initialPos, std::min( pos, txt.size() ) - initialPos + 1 ) );

   return strs.size();
}

static void memdump(uint64_t addr, uint64_t size)
{
    char ascii[17] = {0};
    for (uint64_t i = 0; i < size; i++) {
        uint8_t data = pmemread8(vmemtranslate(addr + i, 1, Mem_Access_Load));
        printf("%02X ", data);
        ascii[i % 16] = std::isprint(data) ? data : '.';
        if ((i + 1) % 8 == 0 || (i + 1) == size) {
            printf(" ");
            if ((i + 1) % 16 == 0) {
                printf("|  %s \n", ascii);
            } else if (i+1 == size) {
                ascii[(i+1) % 16] = '\0';
                if ((i + 1) % 16 <= 8)
                    printf(" ");
                for (uint64_t j = (i+1) % 16; j < 16; j++)
                    printf("   ");
                printf("|  %s \n", ascii);
            }
        }
    }
}

bool sys_emu::process_dbg_cmd(std::string cmd) {
   bool prompt = true;
   std::vector<std::string> command;
   size_t num_args = split(cmd, command);
   steps = -1;
   // Miscellaneous
   if ((cmd == "h") || (cmd == "help")) {
      printf("%s", help_dbg);
   } else if ((cmd == "q") || (cmd == "quit")) {
      exit(0);
   // Simulation control
   } else if ((command[0] == "r") || (command[0] == "run")) {
      prompt = false;
   } else if ((command[0] == "") || (command[0] == "s") || (command[0] == "step")) {
      steps = (num_args > 1) ? std::stoi(command[1]) : 1;
      prompt = false;
   // Breakpoints
   } else if ((command[0] == "b") || (command[0] == "break")) {
      uint64_t pc_break = current_pc[0];
      int thread = -1;
      if (num_args == 2) {
        pc_break = std::stoull(command[1], nullptr, 0);
      } else if (num_args > 2) {
        thread = std::stoi(command[1]);
        pc_break = std::stoull(command[2], nullptr, 0);
      }
      if (pc_breakpoints_add(pc_break, thread)) {
        if (thread == -1)
          printf("Set breakpoint for all threads at PC 0x%lx\n", pc_break);
        else
          printf("Set breakpoint for thread %d at PC 0x%lx\n", thread, pc_break);
     }
   } else if ((command[0] == "list_breaks")) {
      int thread = -1;
      if (num_args > 1)
        thread = std::stoi(command[1]);
      pc_breakpoints_dump(thread);
   } else if ((command[0] == "clear_breaks")) {
      if (num_args > 1)
        pc_breakpoints_clear_for_thread(std::stoi(command[1]));
      else
        pc_breakpoints_clear();
   // Architectural State Dumping
   } else if (command[0] == "pc") {
      uint32_t thid = (num_args > 1) ? std::stoi(command[1]) : 0;
      printf("PC[%d] = 0x%lx\n", thid, current_pc[thid]);
   } else if ((command[0] == "x") || (command[0] == "xdump")) {
      std::string str = dump_xregs((num_args > 1) ? std::stoi(command[1]) : 0);
      printf("%s\n", str.c_str());
   } else if ((command[0] == "f") || command[0] == "fdump") {
      std::string str = dump_fregs((num_args > 1) ? std::stoi(command[1]) : 0);
      printf("%s\n", str.c_str());
   } else if (command[0] == "csr") {
      uint32_t thid = 0;
      uint16_t offset = 0;
      if (num_args > 2) {
        thid = std::stoi(command[1]);
        offset = std::stoul(command[2], nullptr, 0);
      } else if (num_args > 1) {
        offset = std::stoul(command[1], nullptr, 0);
      }
      try {
        printf("CSR[%d][0x%x] = 0x%lx\n", thid, offset & 0xfff, get_csr(thid, offset & 0xfff));
      }
      catch (const trap_t&) {
        printf("Unrecognized CSR register\n");
      }
   } else if ((command[0] == "m") || (command[0] == "mdump")) {
      if (num_args > 2) {
          uint64_t addr = std::stoull(command[1], nullptr, 0);
          uint64_t size = std::stoull(command[2], nullptr, 0);
          memdump(addr, size);
      }
   } else {
      printf("Unknown command\n\n");
      printf("%s", help_dbg);
   }
   return prompt;
}

bool sys_emu::get_pc_break(uint64_t &pc, int &thread) {
   for (int s = 0; s < EMU_NUM_SHIRES; s++)
   {
      if (((shires_en >> s) & 1) == 0) continue;

      unsigned shire_minion_count = (s == EMU_IO_SHIRE_SP ? 1 : EMU_MINIONS_PER_SHIRE);
      unsigned minion_thread_count = (s == EMU_IO_SHIRE_SP ? 1 : EMU_THREADS_PER_MINION);

      for (int m = 0; m < shire_minion_count; m++)
      {
         if (((minions_en >> m) & 1) == 0) continue;
         for (int ii = 0; ii < minion_thread_count; ii++) {
            int thread_id = s * EMU_THREADS_PER_SHIRE + m * EMU_THREADS_PER_MINION + ii;
            if ( pc_breakpoints_exists(current_pc[thread_id], thread_id)) {
               pc = current_pc[thread_id];
               thread = thread_id;
               return true;
            }
         }
      }
   }
   return false;
}
#endif


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
                LOG_OTHER(DEBUG, thread_id, "Waking up due to received FCC%u", thread_dest*2 + cnt_dest);
                enabled_threads.push_back(thread_id);
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
        LOG_OTHER(DEBUG, thread_id, "%s", "Waking up due msg");
        enabled_threads.push_back(thread_id);
        port_wait_threads.erase(thread);
    }
}

////////////////////////////////////////////////////////////////////////////////
// Send IPI_REDIRECT or IPI (or clear pending IPIs) to the minions specified in
// thread mask of the specified shire id.
////////////////////////////////////////////////////////////////////////////////

void
sys_emu::send_ipi_redirect_to_threads(unsigned shire_id, uint64_t thread_mask)
{
    if (shire_id == IO_SHIRE_ID)
        throw std::runtime_error("IPI_REDIRECT to SvcProc");

    // Get IPI_REDIRECT_FILTER ESR for the shire
    uint64_t ipi_redirect_filter;
    memory->read(ESR_SHIRE(shire_id, IPI_REDIRECT_FILTER), 8, &ipi_redirect_filter);

    unsigned thread0 = EMU_THREADS_PER_SHIRE * shire_id;
    for(int t = 0; t < EMU_THREADS_PER_SHIRE; t++)
    {
        // If both IPI_REDIRECT_TRIGGER and IPI_REDIRECT_FILTER has bit set
        if(((thread_mask >> t) & 1) && ((ipi_redirect_filter >> t) & 1))
        {
            // Get PC
            uint64_t new_pc;
            uint64_t neigh_id;
            neigh_id = t / EMU_THREADS_PER_NEIGH;
            memory->read(ESR_NEIGH(shire_id, neigh_id, IPI_REDIRECT_PC), 8, &new_pc);
            int thread_id = thread0 + t;
            LOG_OTHER(DEBUG, thread_id, "Receiving IPI_REDIRECT to %llx", (long long unsigned int) new_pc);
            // If thread sleeping, wakes up and changes PC
            if(std::find(enabled_threads.begin(), enabled_threads.end(), thread_id) == enabled_threads.end())
            {
                LOG_OTHER(DEBUG, thread_id, "%s", "Waking up due to IPI_REDIRECT");
                enabled_threads.push_back(thread_id);
                current_pc[thread_id] = new_pc;
            }
            // Otherwise IPI is dropped
            else
            {
                LOG_OTHER(DEBUG, thread_id, "%s", "WARNING => IPI_REDIRECT dropped");
            }
        }
    }
}

void
sys_emu::raise_software_interrupt(unsigned shire_id, uint64_t thread_mask)
{
    unsigned thread0 =
        EMU_THREADS_PER_SHIRE
        * (shire_id == IO_SHIRE_ID ? EMU_IO_SHIRE_SP : shire_id);

    unsigned shire_thread_count =
        (shire_id == IO_SHIRE_ID ? 1 : EMU_THREADS_PER_SHIRE);

    // Write mip.msip to all selected threads
    for (unsigned t = 0; t < shire_thread_count; ++t)
    {
        if (thread_mask & (1ull << t))
        {
            unsigned thread_id = thread0 + t;
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

void
sys_emu::clear_software_interrupt(unsigned shire_id, uint64_t thread_mask)
{
    unsigned thread0 =
        EMU_THREADS_PER_SHIRE
        * (shire_id == IO_SHIRE_ID ? EMU_IO_SHIRE_SP : shire_id);

    unsigned shire_thread_count =
        (shire_id == IO_SHIRE_ID ? 1 : EMU_THREADS_PER_SHIRE);

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

std::tuple<bool, struct sys_emu_cmd_options>
parse_command_line_arguments(int argc, char* argv[])
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
#ifdef SYSEMU_PROFILING
        else if(strcmp(argv[i], "-dump_prof") == 0)
        {
            cmd_options.dump_prof = 1;
        }
#endif
        else if(strcmp(argv[i], "-lm") == 0)
        {
            cmd_options.dump = 4;
        }
        else if(strcmp(argv[i], "-dump_mem") == 0)
        {
            cmd_options.dump = 5;
        }
        else if(strcmp(argv[i], "-m") == 0)
        {
            cmd_options.create_mem_at_runtime = true;
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
        else if(strcmp(argv[i], "-d") == 0)
        {
            cmd_options.debug = true;
        }
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

void
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

    if (cmd_options.debug == true) {
#ifdef SYSEMU_DEBUG
       LOG_NOTHREAD(INFO, "%s", "Starting in interactive mode.");
#else
       LOG_NOTHREAD(WARN, "%s", "Can't start interactive mode. SYSEMU hasn't been compiled with SYSEMU_DEBUG.");
#endif
    }

    emu::log.setLogLevel(cmd_options.log_en ? LOG_DEBUG : LOG_INFO);

    // Generates the main memory of the emulator
    memory = new main_memory();
    if (cmd_options.create_mem_at_runtime) {
       memory->create_mem_at_runtime();
    }

    // Init emu
    init_emu(system_version_t::ETSOC1_A0);
    log_only_minion(cmd_options.log_min);
    global_log_min = cmd_options.log_min;

    // Defines the memory access functions
    set_memory_funcs(emu_memread8,
                     emu_memread16,
                     emu_memread32,
                     emu_memread64,
                     emu_memwrite8,
                     emu_memwrite16,
                     emu_memwrite32,
                     emu_memwrite64);

    // Callbacks for port writes
    set_msg_funcs(msg_to_thread);

    // Parses the memory description
    if (cmd_options.elf_file != NULL) {
       memory->load_elf(cmd_options.elf_file);
    }
    if (cmd_options.mem_desc_file != NULL) {
       parse_mem_file(cmd_options.mem_desc_file, memory);
    }

    // Initialize network
    net_emu = net_emulator(memory);
    // Parses the net description (it emulates a Maxion sending interrupts to minions)
    if(cmd_options.net_desc_file != NULL)
    {
        net_emu.set_file(cmd_options.net_desc_file);
    }

    api_listener = allocate_api_listener(memory);
    // Parses the net description (it emulates a Maxion sending interrupts to minions)
    if(cmd_options.api_comm_path != NULL)
    {
        api_listener->set_comm_path(cmd_options.api_comm_path);
    }

    bzero(pending_fcc, sizeof(pending_fcc));

    // initialize ports-----------------------------------

    // end initialize ports-------------------------------

    // Generates the mask of enabled minions
    // Setup for all minions

    // For all the shires
    for (unsigned s = 0; s < EMU_NUM_SHIRES; s++)
    {
       reset_esrs_for_shire(s);

       // Skip disabled shire
       if (((shires_en >> s) & 1) == 0) continue;

       // Skip master shire if not enabled
       if ((cmd_options.master_min == 0) && (s >= EMU_MASTER_SHIRE)) continue;

       unsigned shire_minion_count = (s == EMU_IO_SHIRE_SP ? 1 : EMU_MINIONS_PER_SHIRE);
       unsigned minion_thread_count = (s == EMU_IO_SHIRE_SP ? 1 : EMU_THREADS_PER_MINION);

       LOG_NOTHREAD(DEBUG, "s: %u, m: %u, t: %u", s, shire_minion_count, minion_thread_count);

       // For all the minions
       for (unsigned m = 0; m < shire_minion_count; m++)
       {
          // Skip disabled minion
          if (((minions_en >> m) & 1) == 0) continue;

          // Inits threads
          for (unsigned ii = 0; ii < minion_thread_count; ii++) {
             unsigned t = s * EMU_THREADS_PER_SHIRE + m * EMU_THREADS_PER_MINION + ii;
             LOG_OTHER(DEBUG, t, "%s", "Resetting");
             current_pc[t] = cmd_options.reset_pc;
             reduce_state_array[t / EMU_THREADS_PER_MINION] = Reduce_Idle;
             reset_hart(t);
             set_thread(t);
             minit(m0, 255);
             // Puts thread id in the active list
             if(!cmd_options.mins_dis) enabled_threads.push_back(t);
             if(!cmd_options.second_thread) break; // single thread per minion
          }
       }
    }
}

////////////////////////////////////////////////////////////////////////////////
// Main function implementation
////////////////////////////////////////////////////////////////////////////////

int
sys_emu::main_internal(int argc, char * argv[])
{
    auto result = parse_command_line_arguments(argc, argv);
    bool status = std::get<0>(result);
    sys_emu_cmd_options cmd_options = std::get<1>(result);

    if (!status) {
        return 0;
    }

    init_simulator(cmd_options);

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
             || (net_emu.is_enabled() && !net_emu.done())
             || (api_listener->is_enabled() && !api_listener->is_done())
            )
         && (emu_cycle < cmd_options.max_cycles)
    )
    {
#ifdef SYSEMU_DEBUG
        // Check if any thread has reached a breakpoint
        int break_thread;
        uint64_t break_pc;
        bool break_reached = get_pc_break(break_pc, break_thread);
        if (break_reached)
            printf("Thread %d reached breakpoint at PC 0x%lx\n", break_thread, break_pc);

        if ((cmd_options.debug == true) && ((break_reached == true) || (steps == 0))) {
           bool retry = false;
           bool prompt = true;
           std::string line;
           do {
              printf("\n$ ");
              std::getline(std::cin, line);
              try {
                 prompt = process_dbg_cmd(line);
                 retry = false;
              }
              catch (const std::exception& e)
              {
                 printf("\nError parsing command. Please retry\n\n");
                 printf("%s", help_dbg);
                 retry = true;
              }
           } while (prompt || retry);
        }
        if (steps > 0) steps--;
#endif

        auto thread = enabled_threads.begin();

        while(thread != enabled_threads.end())
        {
            auto thread_id = * thread;

            // Try to execute one instruction, this may trap
            try
            {
                // Gets instruction and sets state
                clearlogstate();
                set_thread(thread_id);
                set_pc(current_pc[thread_id]);
                check_pending_interrupts();
                insn_t inst = fetch_and_decode(current_pc[thread_id]);

                // In case of reduce, we need to make sure that the other minion is also in reduce state
                bool reduce_wait = false;

                // FIXME: This is a hack, because we do not call inst.execute() until the tensor_reduce
                // has synchronized; but we should not wait if we are going to raise an exception
                if(inst.is_reduce() && ((thread_id % EMU_THREADS_PER_MINION) == 0))
                {
                    unsigned other_min, action;
                    // Gets the source used for the reduce
                    uint64_t value = xget(inst.rs1());
                    tensor_reduce_decode(thread_id / EMU_THREADS_PER_MINION, value, &other_min, &action);
                    // Sender
                    if(action == 0)
                    {
                        // Moves to ready to send
                        reduce_state_array[thread_id / EMU_THREADS_PER_MINION] = Reduce_Ready_To_Send;
                        reduce_pair_array[thread_id / EMU_THREADS_PER_MINION] = other_min;
                        // If the other minion hasn't arrived yet, wait
                        if((reduce_state_array[other_min] == Reduce_Idle) || (reduce_pair_array[other_min] != uint32_t(thread_id / EMU_THREADS_PER_MINION)))
                        {
                            reduce_wait = true;
                        }
                        // If it has consumed the data, move both threads to Idle
                        else if(reduce_state_array[other_min] == Reduce_Data_Consumed)
                        {
                            reduce_state_array[thread_id / EMU_THREADS_PER_MINION] = Reduce_Idle;
                            reduce_state_array[other_min] = Reduce_Idle;
                        }
                        else
                        {
                            LOG_ALL_MINIONS(FTL, "Reduce error: Found pairing receiver minion: %u in Reduce_Ready_To_Send!!", other_min);
                        }
                    }
                    // Receiver
                    else if(action == 1)
                    {
                        reduce_pair_array[thread_id / EMU_THREADS_PER_MINION] = other_min;
                        // If the other minion hasn't arrived yet, wait
                        if((reduce_state_array[other_min] == Reduce_Idle) || (reduce_pair_array[other_min] != uint32_t(thread_id / EMU_THREADS_PER_MINION)))
                        {
                            reduce_wait = true;
                        }
                        // If pairing minion is in ready to send, consume the data
                        else if(reduce_state_array[other_min] == Reduce_Ready_To_Send)
                        {
                            reduce_state_array[thread_id / EMU_THREADS_PER_MINION] = Reduce_Data_Consumed;
                        }
                        else
                        {
                            LOG_ALL_MINIONS(FTL, "Reduce error: Found pairing sender minion: %u in Reduce_Data_Consumed!!", other_min);
                        }
                    }
                }

                // Executes the instruction
                if(!reduce_wait)
                {
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
                else
                {
                    ++thread;
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
                thread++;
            }
            catch (const std::exception& e)
            {
                LOG_ALL_MINIONS(FTL, "%s", e.what());
            }
        }

        // Net emu: check pending IPIs
        std::list<int> net_emu_ipi_threads;
        uint64_t net_emu_new_pc;
        net_emu.get_new_ipi(&enabled_threads, &net_emu_ipi_threads, &net_emu_new_pc);

        // Net emu: sets the PC for all the minions that got an IPI with PC
        for (auto it = net_emu_ipi_threads.begin(); it != net_emu_ipi_threads.end(); it++) {
            LOG_OTHER(DEBUG, *it, "Waking up due IPI with PC 0x%" PRIx64, net_emu_new_pc);
            if(net_emu_new_pc != 0) current_pc[*it] = net_emu_new_pc; // 0 means resume
            enabled_threads.push_back(*it);

        }

        // Runtime API: check for new commands
        api_listener->get_next_cmd(&enabled_threads);

        emu_cycle++;
    }
    if (emu_cycle == cmd_options.max_cycles)
    {
       LOG(ERR, "Error, max cycles reached (%" SCNd64 ")", cmd_options.max_cycles);
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
        memory->dump_file(cmd_options.dump_file, cmd_options.dump_addr,
                          cmd_options.dump_size);

    if(cmd_options.dump_mem)
        memory->dump_file(cmd_options.dump_mem);

#ifdef SYSEMU_PROFILING
    if (cmd_options.dump_prof_file != NULL) {
        profiling_flush();
        profiling_dump(cmd_options.dump_prof_file);
    }
    profiling_fini();
#endif

    return 0;
}
