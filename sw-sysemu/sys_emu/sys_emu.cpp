#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <list>
#include <exception>

#include "emu.h"
#include "emu_gio.h"
#include "insn.h"
#include "common/main_memory.h"
#include "log.h"
#include "net_emulator.h"
#include "api_communicate.h"

////////////////////////////////////////////////////////////////////////////////
// Defines
////////////////////////////////////////////////////////////////////////////////

#define RESET_PC    0x8000001000ULL
#define FCC_T0_ADDR 0x01003400C0ULL
#define FCC_T1_ADDR 0x01003400D0ULL
#define FLB_ADDR    0x0100340100ULL

////////////////////////////////////////////////////////////////////////////////
// Types
////////////////////////////////////////////////////////////////////////////////

// Reduce state
typedef enum
{
    Reduce_Idle,
    Reduce_Ready_To_Send,
    Reduce_Data_Consumed
} reduce_state;

////////////////////////////////////////////////////////////////////////////////
// Global variables
////////////////////////////////////////////////////////////////////////////////

uint64_t               emu_cycle = 0;
static std::list<int>  enabled_threads;                                               // List of enabled threads
static std::list<int>  fcc_wait_threads;                                              // List of threads waiting for an FCC
static std::list<int>  port_wait_threads;                                             // List of threads waiting for a port write
uint32_t               pending_fcc[EMU_NUM_THREADS][EMU_NUM_FCC_COUNTERS_PER_THREAD]; // Pending FastCreditCounter list
static uint64_t        current_pc[EMU_NUM_THREADS];                                   // PC for each thread
static reduce_state    reduce_state_array[EMU_NUM_MINIONS];                           // Reduce state
static uint32_t        reduce_pair_array[EMU_NUM_MINIONS];                            // Reduce pairing minion
static int             global_log_min;

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
// Instruction log
////////////////////////////////////////////////////////////////////////////////

// Returns current thread

static int32_t thread_id;

static uint32_t get_thread_emu()
{
    return thread_id;
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
";


static uint64_t minions_en = 1;
static uint64_t shires_en  = 1;

#ifdef SYSEMU_DEBUG
static int  steps          = 0;
static std::vector<uint64_t> pc_breakpoints;

static const char * help_dbg =
"help|h:\t\tPrint this message\n\
run|r:\t\tExecute until the end or a breakpoint is reached\n\
step|s [n]:\tExecute n cycles (or 1 if not specified)\n\
pc [N]:\t\tDump PC of thread N (0 <= N < 2048)\n\
xdump|x [N]:\tDump GPRs of thread N (0 <= N < 2048)\n\
fdump|f [N]:\tDump FPRs of thread N (0 <= N < 2048)\n\
csr [N] <off>:\tDump the CSR at offset \"off\" of thread N (0 <= N < 2048)\n\
break|b [PC]:\tSet a breakpoint for the provided PC\n\
list_breaks:\tList the currently active breakpoints\n\
clear_breaks:\tClear all the breakpoints previously set\n\
quit|q:\t\tTerminate the program\n\
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

bool process_dbg_cmd(std::string cmd) {
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
   } else if ((command[0] == "b") || (command[0] == "break")) {
      if (num_args > 1) {
         uint64_t pc_break;
         std::stringstream ss;
         bool found = false;
         ss << std::hex << command[1];
         ss >> pc_break;
         auto it = pc_breakpoints.begin();
         while (it != pc_breakpoints.end()) {
            if (*it == pc_break) {
               found = true;
               break;
            }
            it++;
         }
         if (!found) pc_breakpoints.push_back(pc_break);
         printf("Set breakpoint for PC 0x%lx", pc_break);
      }
   } else if ((command[0] == "list_breaks")) {
      auto it = pc_breakpoints.begin();
      while (it != pc_breakpoints.end()) {
         printf("Breakpoint set for PC 0x%lx\n", *it);
         it++;
      }
   } else if ((command[0] == "clear_breaks")) {
      pc_breakpoints.clear();
   // Architectural State Dumping
   } else if (command[0] == "pc") {
      uint32_t thid = (num_args > 1) ? std::stoi(command[1]) : 0;
      printf("PC[%d] = 0x%lx\n", thid, current_pc[thid]);
   } else if ((command[0] == "x") || (command[0] == "xdump")) {
      std::string str = dump_xregs((num_args > 1) ? std::stoi(command[1]) : 0).str();
      printf("%s\n", str.c_str());
   } else if ((command[0] == "f") || command[0] == "fdump") {
      std::string str = dump_fregs((num_args > 1) ? std::stoi(command[1]) : 0).str();
      printf("%s\n", str.c_str());
   } else if (command[0] == "csr") {
      uint32_t thid = 0, offset = 0;
      if (num_args > 2) {
        thid = std::stoi(command[1]);
        offset = std::stoul(command[2], nullptr, 0);
      } else if (num_args > 1) {
        offset = std::stoi(command[1], nullptr, 0);
      }
      csr c = get_csr_enum(offset);
      if (c == csr_unknown) {
        printf("Unrecognized CSR register\n");
      } else {
        printf("CSR[%d][0x%x] = 0x%lx\n", thid, offset, get_csr(thid, c));
      }
   } else {
      printf("Unknown command\n\n");
      printf("%s", help_dbg);
   }
   return prompt;
}

bool get_pc_break() {
   for (int s = 0; s < (EMU_NUM_MINIONS / EMU_MINIONS_PER_SHIRE); s++)
   {
      if (((shires_en >> s) & 1) == 0) continue;
      for (int m = 0; m < EMU_MINIONS_PER_SHIRE; m++)
      {
         if (((minions_en >> m) & 1) == 0) continue;
         for (int ii = 0; ii < EMU_THREADS_PER_MINION; ii++) {
            thread_id = (s * EMU_MINIONS_PER_SHIRE + m) * EMU_THREADS_PER_MINION + ii;
            if ( std::find(pc_breakpoints.begin(), pc_breakpoints.end(), current_pc[thread_id]) != pc_breakpoints.end() ) {
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

void fcc_to_threads(unsigned shire_id, unsigned thread_dest, uint64_t thread_mask, unsigned cnt_dest)
{
    for(int m = 0; m < EMU_MINIONS_PER_SHIRE; m++)
    {
        // Skip disabled minion
        if (((minions_en >> m) & 1) == 0) continue;

        if ((thread_mask >> m) & 1)
        {
            int thread_id = shire_id * EMU_THREADS_PER_SHIRE + m * EMU_THREADS_PER_MINION + thread_dest;
            LOG_OTHER(DEBUG, thread_id, "Receiving FCC on counter %u", cnt_dest);

            auto thread = std::find(fcc_wait_threads.begin(), fcc_wait_threads.end(), thread_id);
            // Checks if is already awaken
            if(thread == fcc_wait_threads.end())
            {
                // Pushes to pending FCC
                pending_fcc[thread_id][cnt_dest]++;
            }
            // Otherwise wakes up thread
            else
            {
                LOG_OTHER(DEBUG, thread_id, "Waking up due sent FCC on counter %u", cnt_dest);
                enabled_threads.push_back(thread_id);
                fcc_wait_threads.erase(thread);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// Sends an FCC to the desired minions specified in thread mask to the 1st or
// second thread (thread_dest), to the counter 0 or 1 (cnt_dest), inside the shire
// of thread_src
////////////////////////////////////////////////////////////////////////////////

void msg_to_thread(int thread_id)
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

void send_ipi_redirect_to_threads(unsigned shire_id, uint64_t thread_mask)
{
    // Get IPI_REDIRECT_FILTER ESR for the shire
    uint64_t ipi_redirect_filter;
    memory->read(ESR_SHIRE(3, shire_id, IPI_REDIRECT_FILTER), 8, &ipi_redirect_filter);

    unsigned thread0 =
        EMU_THREADS_PER_SHIRE * (shire_id == IO_SHIRE_ID ? EMU_IO_SHIRE_SP : shire_id);

    for(int t = 0; t < EMU_THREADS_PER_SHIRE; t++)
    {
        // If both IPI_REDIRECT_TRIGGER and IPI_REDIRECT_FILTER has bit set
        if(((thread_mask >> t) & 1) && ((ipi_redirect_filter >> t) & 1))
        {
            // Get PC
            uint64_t new_pc;
            uint64_t neigh_id;
            neigh_id = t / EMU_THREADS_PER_NEIGH;
            memory->read(ESR_NEIGH(0, shire_id, neigh_id, IPI_REDIRECT_PC), 8, &new_pc);
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

void raise_software_interrupt(unsigned shire_id, uint64_t thread_mask)
{
    unsigned thread0 =
        EMU_THREADS_PER_SHIRE
        * (shire_id == IO_SHIRE_ID ? EMU_IO_SHIRE_SP : shire_id);

    // Write mip.msip to all selected threads
    for (int t = 0; t < EMU_THREADS_PER_SHIRE; ++t)
    {
        if ((thread_mask >> t) & 1)
        {
            unsigned thread_id = thread0 + t;
            LOG_OTHER(DEBUG, thread_id, "%s", "Receiving IPI");
            raise_software_interrupt(thread_id);
            // If thread sleeping, wakes up
            if (std::find(enabled_threads.begin(), enabled_threads.end(), thread_id) == enabled_threads.end())
            {
                LOG_OTHER(DEBUG, thread_id, "%s", "Waking up due to IPI");
                enabled_threads.push_back(thread_id);
            }
        }
    }
}

void clear_software_interrupt(unsigned shire_id, uint64_t thread_mask)
{
    unsigned thread0 =
        EMU_THREADS_PER_SHIRE
        * (shire_id == IO_SHIRE_ID ? EMU_IO_SHIRE_SP : shire_id);

    // Clear mip.msip to all selected threads
    for (int t = 0; t < EMU_THREADS_PER_SHIRE; ++t)
    {
        if ((thread_mask >> t) & 1)
        {
            unsigned thread_id = thread0 + t;
            clear_software_interrupt(thread_id);
        }
    }
}


////////////////////////////////////////////////////////////////////////////////
// Main
////////////////////////////////////////////////////////////////////////////////

int main(int argc, char * argv[])
{
    char * elf_file      = NULL;
    char * mem_desc_file = NULL;
    char * net_desc_file = NULL;
    char * api_comm_path = NULL;
    bool elf             = false;
    bool mem_desc        = false;
    bool net_desc        = false;
    bool api_comm        = false;
    bool master_min      = false;
    bool minions         = false;
    bool second_thread   = true;
    bool shires          = false;
    bool log_en          = false;
    bool create_mem_at_runtime = false;
    int  log_min         = -1;
    char * dump_file     = NULL;
    int dump             = 0;
    uint64_t dump_addr   = 0;
    uint64_t dump_size   = 0;
    uint64_t reset_pc    = RESET_PC;
    bool reset_pc_flag   = false;
    bool debug           = false;
    uint64_t max_cycles  = 10000000;
    bool max_cycle       = false;
    bool mins_dis        = false;

    for(int i = 1; i < argc; i++)
    {
        if (max_cycle)
        {
            max_cycle = false;
            sscanf(argv[i], "%" SCNu64, &max_cycles);
        }
        else if (elf)
        {
            elf = false;
            elf_file = argv[i];
        }
        else if(mem_desc)
        {
            mem_desc = false;
            mem_desc_file = argv[i];
        }
        else if(net_desc)
        {
            net_desc = false;
            net_desc_file = argv[i];
        }
        else if(api_comm)
        {
            api_comm = false;
            api_comm_path = argv[i];
        }
        else if(minions)
        {
            sscanf(argv[i], "%" PRIx64, &minions_en);
            minions = 0;
        }
        else if(shires)
        {
            sscanf(argv[i], "%" PRIx64, &shires_en);
            shires = 0;
        }
        else if(reset_pc_flag)
        {
          sscanf(argv[i], "%" PRIx64, &reset_pc);
          reset_pc_flag = false;
        }
        else if(dump == 1)
        {
            sscanf(argv[i], "%" PRIx64, &dump_addr);
            dump = 0;
        }
        else if(dump == 2)
        {
            dump_size = atoi(argv[i]);
            dump = 0;
        }
        else if(dump == 3)
        {
            dump_file = argv[i];
            dump = 0;
        }
        else if(dump == 4)
        {
            log_min = atoi(argv[i]);
            dump = 0;
        }
        else if(strcmp(argv[i], "-max_cycles") == 0)
        {
            max_cycle = true;
        }
        else if(strcmp(argv[i], "-elf") == 0)
        {
            elf = true;
        }
        else if(strcmp(argv[i], "-mem_desc") == 0)
        {
            mem_desc = true;
        }
        else if(strcmp(argv[i], "-net_desc") == 0)
        {
            net_desc = true;
        }
        else if(strcmp(argv[i], "-api_comm") == 0)
        {
            api_comm = true;
        }
        else if(strcmp(argv[i], "-master_min") == 0)
        {
            master_min = true;
        }
        else if(strcmp(argv[i], "-minions") == 0)
        {
            minions = true;
        }
        else if(strcmp(argv[i], "-shires") == 0)
        {
            shires = true;
        }
        else if(strcmp(argv[i], "-reset_pc") == 0)
        {
            reset_pc_flag = true;
        }
        else if(strcmp(argv[i], "-dump_addr") == 0)
        {
            dump = 1;
        }
        else if(strcmp(argv[i], "-dump_size") == 0)
        {
            dump = 2;
        }
        else if(strcmp(argv[i], "-dump_file") == 0)
        {
            dump = 3;
        }
        else if(strcmp(argv[i], "-lm") == 0)
        {
            dump = 4;
        }
        else if(strcmp(argv[i], "-m") == 0)
        {
            create_mem_at_runtime = true;
        }
        else if(strcmp(argv[i], "-l") == 0)
        {
            log_en = true;
        }
        else if (  (strcmp(argv[i], "-h") == 0)
                 ||(strcmp(argv[i], "-help") == 0)
                 ||(strcmp(argv[i], "--help") == 0)) {
           printf("%s", help_msg);
           return 0;
        }
        else if (strcmp(argv[i], "-single_thread") == 0)
        {
            second_thread = false;
        }
        else if(strcmp(argv[i], "-d") == 0)
        {
            debug = true;
        }
        else if(strcmp(argv[i], "-mins_dis") == 0)
        {
            mins_dis = true;
        }
        else
        {
            LOG_NOTHREAD(FTL, "Unknown parameter %s", argv[i]);
        }
    }

    if ((elf_file == NULL) && (mem_desc_file == NULL) && (api_comm_path == NULL))
    {
        LOG_NOTHREAD(FTL, "%s", "Need an elf file or a mem_desc file or runtime API!");
    }

    uint64_t drivers_enabled = 0;
    if (net_desc_file != NULL) drivers_enabled++;
    if (master_min)            drivers_enabled++;

    if (drivers_enabled > 1)
    {
        LOG_NOTHREAD(FTL, "%s", "Can't have net_desc and master_min set at same time!");
    }

    if (debug == true) {
#ifdef SYSEMU_DEBUG
       LOG_NOTHREAD(INFO, "%s", "Starting in interactive mode.");
#else
       LOG_NOTHREAD(WARN, "%s", "Can't start interactive mode. SYSEMU hasn't been compiled with SYSEMU_DEBUG.");
#endif
    }

    emu::log.setLogLevel(log_en ? LOG_DEBUG : LOG_INFO);

    // Generates the main memory of the emulator
    memory = new main_memory(emu::log);
    memory->setGetThread(get_thread_emu);
    if (create_mem_at_runtime) {
       memory->create_mem_at_runtime();
    }

    // Init emu
    init_emu();
    log_only_minion(log_min);
    global_log_min = log_min;

    in_sysemu = true;

    // Log state (needed to know PC changes)
    inst_state_change emu_state_change;
    setlogstate(&emu_state_change); // This is done every time just in case we have several checkers

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
    set_msg_funcs((void *) msg_to_thread);

    // Parses the memory description
    if (elf_file != NULL) {
       memory->load_elf(elf_file);
    }
    if (mem_desc_file != NULL) {
       parse_mem_file(mem_desc_file, memory);
    }

    net_emulator net_emu(memory);
    // Parses the net description (it emulates a Maxion sending interrupts to minions)
    if(net_desc_file != NULL)
    {
        net_emu.set_file(net_desc_file);
    }

    api_communicate api_listener(memory);
    // Parses the net description (it emulates a Maxion sending interrupts to minions)
    if(api_comm_path != NULL)
    {
        api_listener.set_comm_path(api_comm_path);
    }

    bzero(pending_fcc, sizeof(pending_fcc));

    // initialize ports-----------------------------------

    // end initialize ports-------------------------------

    // Generates the mask of enabled minions
    // Setup for all minions

    // For all the shires
    for (int s = 0; s < (EMU_NUM_MINIONS / EMU_MINIONS_PER_SHIRE); s++)
    {
       // Skip disabled shire
       if (((shires_en >> s) & 1) == 0) continue;
       // Skip master shire if not enabled
       if ((master_min == 0) && (s >= EMU_NUM_COMPUTE_SHIRES)) continue;

       // For all the minions
       for (int m = 0; m < EMU_MINIONS_PER_SHIRE; m++)
       {
          // Skip disabled minion
          if (((minions_en >> m) & 1) == 0) continue;

          // Inits threads
          for (int ii = 0; ii < EMU_THREADS_PER_MINION; ii++) {
             thread_id = s * EMU_THREADS_PER_SHIRE + m * EMU_THREADS_PER_MINION + ii;
             LOG_OTHER(DEBUG, thread_id, "%s", "Resetting");
             current_pc[thread_id] = reset_pc;
             reduce_state_array[thread_id / EMU_THREADS_PER_MINION] = Reduce_Idle;
             set_thread(thread_id);
             init(x0, 0);
             minit(m0, 255);
             initcsr(thread_id);
             // Puts thread id in the active list
             if(!mins_dis) enabled_threads.push_back(thread_id);
             if(!second_thread) break; // single thread per minion
          }
       }
    }

    LOG_NOTHREAD(INFO, "%s", "Starting emulation");

    // While there are active threads or the network emulator is still not done
    while(  (emu_done() == false)
         && (   enabled_threads.size()
             || fcc_wait_threads.size()
             || port_wait_threads.size()
             || (net_emu.is_enabled() && !net_emu.done())
             || (api_listener.is_enabled() && !api_listener.is_done())
            )
         && (emu_cycle < max_cycles)
    )
    {
#ifdef SYSEMU_DEBUG
        if ((debug == true) && ((get_pc_break() == true) || (steps == 0))) {
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
            thread_id = * thread;

            // Try to execute one instruction, this may trap
            try
            {
                // Gets instruction and sets state
                insn_t inst;
                clearlogstate();
                set_thread(thread_id);
                set_pc(current_pc[thread_id]);
                check_pending_interrupts();
                inst.fetch_and_decode(current_pc[thread_id]);

                // In case of reduce, we need to make sure that the other minion is also in reduce state
                bool reduce_wait = false;
                if(inst.is_reduce())
                {
                    uint64_t other_min, action;
                    // Gets the source used for the reduce
                    uint64_t value = xget(inst.rs1());
                    get_reduce_info(value, &other_min, &action);
                    // Sender
                    if(action == 0)
                    {
                        // Moves to ready to send
                        reduce_state_array[thread_id / EMU_THREADS_PER_MINION] = Reduce_Ready_To_Send;
                        reduce_pair_array[thread_id / EMU_THREADS_PER_MINION]  = other_min;
                        // If the other minion hasn't arrived yet, wait
                        if((reduce_state_array[other_min] == Reduce_Idle) || (reduce_pair_array[other_min] != uint32_t(thread_id / EMU_THREADS_PER_MINION)))
                        {
                            reduce_wait = true;
                        }
                        // If it has consumed the data, move both threads to Idle
                        else if(reduce_state_array[other_min] == Reduce_Data_Consumed)
                        {
                            reduce_state_array[thread_id / EMU_THREADS_PER_MINION] = Reduce_Idle;
                            reduce_state_array[other_min]    = Reduce_Idle;
                        }
                        else
                        {
                            LOG(FTL, "Reduce error: Found pairing receiver minion: %" PRIu64 " in Reduce_Ready_To_Send!!", other_min);
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
                            LOG(FTL, "Reduce error: Found pairing sender minion: %" PRIu64 " in Reduce_Data_Consumed!!", other_min);
                        }
                    }
                }

                // Executes the instruction
                if(!reduce_wait)
                {
                    inst.execute();

                    if (get_msg_port_stall(thread_id, 0) ){
                        thread = enabled_threads.erase(thread);
                        port_wait_threads.push_back(thread_id);
                        if (thread == enabled_threads.end()) break;
                    }
                    else {
                        // Updates PC
                        if(emu_state_change.pc_mod) {
                            current_pc[thread_id] = emu_state_change.pc;
                        } else {
                            current_pc[thread_id] += inst.size();
                        }
                    }
                }

                // Thread is going to sleep
                if(inst.is_fcc() && !reduce_wait)
                {
                    unsigned cnt = get_fcc_cnt();
                    LOG(DEBUG, "Going to sleep (FCC) blocking on counter %u", cnt);
                    int old_thread = *thread;

                    // Checks if there's a pending FCC and wakes up thread again
                    if(pending_fcc[old_thread][cnt]==0)
                    {
                        thread = enabled_threads.erase(thread);
                        fcc_wait_threads.push_back(thread_id);
                    }
                    else
                    {
                        LOG(DEBUG, "Waking up due present FCC on counter %u", cnt);
                        pending_fcc[old_thread][cnt]--;
                    }
                }
                else if(inst.is_wfi() && !reduce_wait)
                {
                    LOG(DEBUG, "%s", "Going to sleep (WFI)");
                    thread = enabled_threads.erase(thread);
                }
                else if(inst.is_stall() && !reduce_wait)
                {
                    LOG(DEBUG, "%s", "Going to sleep (CSR STALL)");
                    thread = enabled_threads.erase(thread);
                }
                else
                {
                    thread++;
                }
            }
            catch (const trap_t& t)
            {
                take_trap(t);
                //LOG(DEBUG, "%s", "Taking a trap");
                if (current_pc[thread_id] == emu_state_change.pc)
                {
                    LOG(FTL, "Trapping to the same address that caused a trap (0x%" PRIx64 "). Avoiding infinite trap recursion.",
                        current_pc[thread_id]);
                }
                current_pc[thread_id] = emu_state_change.pc;
                thread++;
            }
            catch (const std::exception& e)
            {
                LOG(FTL, "%s", e.what());
            }
        }

        // Once all threads done this cycle, check for new IPIs from net emu/runtime API
        std::list<int> ipi_threads, ipi_threads_t1; // List of threads with an IPI with PC from network emu
        uint64_t       new_pc,      new_pc_t1;      // New PC for the IPI
        net_emu.get_new_ipi(&enabled_threads, &ipi_threads, &new_pc);
        api_listener.get_next_cmd(&enabled_threads, &ipi_threads, &new_pc, &ipi_threads_t1, &new_pc_t1);

        // Sets the PC for all the minions that got an IPI with PC
        auto it = ipi_threads.begin();
        while(it != ipi_threads.end())
        {
            LOG_OTHER(DEBUG, *it, "Waking up due IPI with PC 0x%" PRIx64, new_pc);
            if(new_pc != 0) current_pc[* it] = new_pc; // 0 means resume
            enabled_threads.push_back(* it);
            it++;
        }

        // Sets the PC for all the minions that got an IPI with PC
        it = ipi_threads_t1.begin();
        while(it != ipi_threads_t1.end())
        {
            LOG_OTHER(DEBUG, *it, "Waking up due IPI with PC 0x%" PRIx64, new_pc_t1);
            if(new_pc_t1 != 0) current_pc[* it] = new_pc_t1; // 0 means resume
            enabled_threads.push_back(* it);
            it++;
        }
        emu_cycle++;
    }
    if (emu_cycle == max_cycles) {
       LOG(ERR, "Error, max cycles reached (%" SCNd64 ")", max_cycles);
    }
    LOG_NOTHREAD(INFO, "%s", "Finishing emulation");

    // Dumping
    if(dump_file != NULL)
    {
        memory->dump_file(dump_file, dump_addr, dump_size);
    }

    return 0;
}

