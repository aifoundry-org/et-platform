#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <list>
#include <exception>

#include "emu.h"
#include "insn.h"
#include "common/main_memory.h"
#include "common/riscv_disasm.h"
#include "log.h"
#include "net_emulator.h"
#include "rboxSysEmu.h"

////////////////////////////////////////////////////////////////////////////////
// Defines
////////////////////////////////////////////////////////////////////////////////

#define RESET_PC    0x00001000
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

static std::list<int>  enabled_threads;                     // List of enabled threads
static std::list<int>  pending_fcc;                         // Pending FastCreditCounter list
static uint64_t        current_pc[EMU_NUM_THREADS];         // PC for each thread
static reduce_state    reduce_state_array[EMU_NUM_MINIONS]; // Reduce state
static uint32_t        reduce_pair_array[EMU_NUM_MINIONS];  // Reduce pairing minion

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
// Dump or not log info
////////////////////////////////////////////////////////////////////////////////

static bool dump_log(bool log_en, int log_min, int thread_id)
{
    if(log_min >= EMU_NUM_MINIONS) return log_en;
    return (((thread_id / EMU_THREADS_PER_MINION) == log_min) && log_en);
}

////////////////////////////////////////////////////////////////////////////////
// Sends an FCC to the desired minions specified in thread mask to the 1st or
// second thread (thread_dest) inside the shire of thread_src
////////////////////////////////////////////////////////////////////////////////

static void fcc_to_threads(unsigned shire_id, unsigned thread_dest, uint64_t thread_mask, bool log_en, int log_min)
{
    for(int m = 0; m < EMU_MINIONS_PER_SHIRE; m++)
    {
        if((thread_mask >> m) & 1)
        {
            int thread_id = shire_id * EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION + m * EMU_THREADS_PER_MINION  + thread_dest;
            if(dump_log(log_en, log_min, thread_id)) { printf("Minion %i.%i.%i: Receiving FCC\n", shire_id, m, thread_dest); }
            // Checks if is already awaken
            if(std::find(enabled_threads.begin(), enabled_threads.end(), thread_id) != enabled_threads.end())
            {
                // Pushes to pending FCC
                pending_fcc.push_back(thread_id);
            }
            // Otherwise wakes up thread
            else
            {
                if(dump_log(log_en, log_min, thread_id)) { printf("Minion %i.%i.%i: Waking up due sent FCC\n", shire_id, m, thread_dest); }
                enabled_threads.push_back(thread_id);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// functions to connect rbox emulation with emu
////////////////////////////////////////////////////////////////////////////////
static rboxSysEmu *rbox[EMU_NUM_SHIRES];

static void newMsgPortDataRequest(uint32_t current_thread,  uint32_t port_id)
{
    unsigned shire = current_thread/(EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION);
    unsigned thread = current_thread % (EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION);
    if (rbox[shire] != NULL)
        rbox[shire]->dataRequest(thread);
}

static bool queryMsgPort(uint32_t current_thread,  uint32_t port_id)
{
    unsigned shire = current_thread/(EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION);
    unsigned thread = current_thread % (EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION);
    if (rbox[shire] != NULL)
        return rbox[shire]->dataQuery(thread);
    else return false;
}

////////////////////////////////////////////////////////////////////////////////
// Parses a file that defines the memory regions plus contents to be
// loaded in the different regions
////////////////////////////////////////////////////////////////////////////////

static bool parse_mem_file(const char * filename, main_memory * memory, testLog& log)
{
    FILE * file = fopen(filename, "r");
    if (file == NULL)
    {
        log << LOG_FTL << "Parse Mem File Error -> Couldn't open file " << filename << " for reading!!" << endm;
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
            log << LOG_INFO << "New Mem Region found: @ 0x" << std::hex << base_addr << ", size = 0x" << size << std::dec << endm;
        }
        else if(sscanf(buffer, "File Load: 40'h%" PRIX64 ", %s", &base_addr, str) == 2)
        {
            memory->load_file(str, base_addr);
            log << LOG_INFO << "New File Load found: @ 0x" << std::hex << base_addr << std::dec << endm;
        }
        else if(sscanf(buffer, "ELF Load: %s", str) == 1)
        {
            memory->load_elf(str);
            log << LOG_INFO << "New ELF Load found: " << str << endm;
        }
    }
    // Closes the file
    fclose(file);
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// Instruction log
////////////////////////////////////////////////////////////////////////////////

static void print_inst_log(const insn_t& inst, uint64_t minion, uint64_t current_pc, inst_state_change & emu_state_change)
{
    char insn_disasm[128];
    riscv_disasm(insn_disasm, 128, inst.bits);
    printf("Minion %" PRIu64 ".%" PRIu64 ".%" PRIu64 ": PC %08" PRIx64 " (inst bits %08" PRIx32 "), mnemonic %s\n",
           minion / (EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION),
           (minion / EMU_THREADS_PER_MINION) % EMU_MINIONS_PER_SHIRE,
           minion % EMU_THREADS_PER_MINION, current_pc,
           inst.bits, insn_disasm);
}

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
     sys_emu <-mem_desc <file> | -elf <file>> [-net_desc <file>] [-minions <mask>] [-shires <mask>] [-dump_file <file_name> [-dump_addr <address>] [-dump_size <size>]] [-l] [-ll] [-lm <minion]> [-m] [-rbox] [-reset_pc <addr>] [-d]\n\n\
 -mem_desc    Path to a file describing the memory regions to create and what code to load there\n\
 -elf         Path to an ELF file to load.\n\
 -net_desc    Path to a file describing emulation of a Maxion sending interrupts to minions.\n\
 -minions     A mask of Minions that should be enabled in each Shire. Default: 1 Minion per shire\n\
 -shires      A mask of Shires that should be enabled. Default: 1 Shire\n\
 -dump_file   Path to the file in which to dump the memory content at the end of the simulation\n\
 -dump_addr   Address in memory at which to start dumping. Only valid if -dump_file is used\n\
 -dump_size   Size of the memory to dump. Only valid if -dump_file is used\n\
 -l           Enable Logging\n\
 -ll          Log memory accesses\n\
 -lm          Log a given Minion ID only. Default: all Minions\n\
 -m           Enable dynamic memory allocation. If a region of memory not specified in mem_desc is accessed, the model will create it instead of throwing an error.\n\
 -rbox        Enable RBOX emulation\n\
 -reset_pc    Sets boot program counter (default 0x1000) \n\
 -d           Start in interactive debug mode (must have been compiled with SYSEMU_DEBUG)\n\
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
xdump|x <N>:\tDump GPRs of thread N (0 <= N < 2048)\n\
fdump|f <N>:\tDump FPRs of thread N (0 <= N < 2048)\n\
break|b <PC>:\tSet a breakpoint for the provided PC\n\
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
   } else if ((command[0] == "x") || (command[0] == "xdump")) {
      std::string str = dump_xregs((num_args > 1) ? std::stoi(command[1]) : 0).str();
      printf("%s\n", str.c_str());
   } else if ((command[0] == "f") || command[0] == "fdump") {
      std::string str = dump_fregs((num_args > 1) ? std::stoi(command[1]) : 0).str();
      printf("%s\n", str.c_str());
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
// Main
////////////////////////////////////////////////////////////////////////////////

int main(int argc, char * argv[])
{

    // Logger
    testLog log("System EMU", LOG_DEBUG);

    char * elf_file      = NULL;
    char * mem_desc_file = NULL;
    char * net_desc_file = NULL;
    bool elf             = false;
    bool mem_desc        = false;
    bool net_desc        = false;
    bool minions         = false;
    bool second_thread   = true;
    bool shires          = false;
    bool log_en          = false;
    bool log_mem_en      = false;
    bool create_mem_at_runtime = false;
    int  log_min         = -1;
    char * dump_file     = NULL;
    int dump             = 0;
    uint64_t dump_addr   = 0;
    uint64_t dump_size   = 0;
    bool use_rbox        = false;
    uint64_t reset_pc    = RESET_PC;
    bool reset_pc_flag   = false;
    bool debug           = false;

    memset(rbox, 0, sizeof(rbox));

    for(int i = 1; i < argc; i++)
    {
        if (elf)
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
        else if(strcmp(argv[i], "-ll") == 0)
        {
            log_mem_en = true;
        }
        else if (strcmp(argv[i], "-rbox") == 0) {
           use_rbox = true;
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
        else
        {
            log << LOG_FTL << "Unkown parameter " << argv[i] << endm;
        }
    }

    if ((elf_file == NULL) && (mem_desc_file == NULL))
    {
        log << LOG_FTL << "Need an elf file or a mem_desc file!" << endm;
    }
    if (debug == true) {
#ifdef SYSEMU_DEBUG
       log << LOG_INFO << "Starting in interactive mode." << endm;
#else
       log << LOG_WARN << "Can't start interactive mode. SYSEMU hasn't been compiled with SYSEMU_DEBUG." << endm;
#endif
    }

    // Generates the main memory of the emulator
    memory = new main_memory("checker main memory", log_mem_en? LOG_DEBUG : LOG_INFO);
    memory->setGetThread(get_thread_emu);
    if (create_mem_at_runtime) {
       memory->create_mem_at_runtime();
    }

    // Init emu
    init_emu(log_en, false, log_en? LOG_DEBUG : LOG_INFO);
    log_only_minion(log_min);

    in_sysemu = true;

    // Log state (needed to know PC changes)
    inst_state_change emu_state_change;
    setlogstate(&emu_state_change); // This is done every time just in case we have several checkers

    // Defines the memory access functions
    set_memory_funcs((void *) emu_memread8,
                     (void *) emu_memread16,
                     (void *) emu_memread32,
                     (void *) emu_memread64,
                     (void *) emu_memwrite8,
                     (void *) emu_memwrite16,
                     (void *) emu_memwrite32,
                     (void *) emu_memwrite64);

    // Parses the memory description
    if (elf_file != NULL) {
       memory->load_elf(elf_file);
    }
    if (mem_desc_file != NULL) {
       parse_mem_file(mem_desc_file, memory, log);
    }

    net_emulator net_emu(memory);
    net_emu.set_log(&log);
    // Parses the net description (it emulates a Maxion sending interrupts to minions)
    if(net_desc_file != NULL)
    {
        net_emu.set_file(net_desc_file);
    }

    // initialize rboxes-----------------------------------
    if (use_rbox){
        for (int i = 0 ; i < EMU_NUM_SHIRES; i++)
            rbox[i] = new rboxSysEmu(i, memory, write_msg_port_data);
        set_msg_port_data_funcs(NULL, (void * ) queryMsgPort, (void * ) newMsgPortDataRequest);
    }
    // end initialize rboxes-------------------------------

    // initialize ports-----------------------------------

    // end initialize ports-------------------------------

    // Generates the mask of enabled minions
    // Setup for all minions

    // For all the shires
    for (int s = 0; s < (EMU_NUM_MINIONS / EMU_MINIONS_PER_SHIRE); s++)
    {
       // Skip disabled shire
       if (((shires_en >> s) & 1) == 0) continue;

       // For all the minions
       for (int m = 0; m < EMU_MINIONS_PER_SHIRE; m++)
       {
          // Skip disabled minion
          if (((minions_en >> m) & 1) == 0) continue;

          // Inits threads
          for (int ii = 0; ii < EMU_THREADS_PER_MINION; ii++) {
             thread_id = (s * EMU_MINIONS_PER_SHIRE + m) * EMU_THREADS_PER_MINION + ii;
             if (dump_log(log_en, log_min, thread_id)) { printf("Minion %i.%i.0: Resetting\n", s, m); }
             current_pc[thread_id] = reset_pc;
             reduce_state_array[thread_id / EMU_THREADS_PER_MINION] = Reduce_Idle;
             set_thread(thread_id);
             init(x0, 0);
             minit(m0, 255);
             initcsr(thread_id);
             // Puts thread id in the active list
             enabled_threads.push_back(thread_id);
             if(!second_thread) break; // single thread per minion
          }
       }
    }

    insn_t inst;
    uint64_t emu_cycle = 0;

    bool rboxes_done = false;
    // While there are active threads or the network emulator is still not done
    while((emu_done() == false) && (enabled_threads.size() || (net_emu.is_enabled() && !net_emu.done()) || (use_rbox && !rboxes_done)))
    {

        // For every cycle execute rbox

        if (use_rbox) {
            rboxes_done = true;
            for ( int i = 0; i < EMU_NUM_SHIRES; i++)
            {
                int newEnable = rbox[i]->tick();

                if (newEnable > 0)
                    enabled_threads.push_back(newEnable + EMU_MINIONS_PER_SHIRE*i);

                if (!rbox[i]->done())
                {
                    rboxes_done = false;
                    break;
                }
            }
        }

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

        if(log_en)
        {
            printf("Starting Emulation Cycle %" PRIu64 "\n", emu_cycle);
        }

        auto thread = enabled_threads.begin();

        while(thread != enabled_threads.end())
        {
            thread_id = * thread;

            // Computes logging for this thread
            bool do_log = dump_log(log_en, log_min, thread_id);
            if(do_log) { printf("Starting emu of thread %i\n", thread_id); }

            // Try to execute one instruction, this may trap
            try
            {
                // Gets instruction and sets state
                clearlogstate();
                set_thread(thread_id);
                set_pc(current_pc[thread_id]);
                inst.fetch_and_decode(current_pc[thread_id]);
                if(do_log)
                    print_inst_log(inst, thread_id, current_pc[thread_id], emu_state_change);

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
                            log << LOG_FTL << "Reduce error: Minion: " << (thread_id  / EMU_THREADS_PER_MINION) << " found pairing receiver minion: " << other_min << " in Reduce_Ready_To_Send!!" << endm;
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
                            log << LOG_FTL << "Reduce error: Minion: " << (thread_id  / EMU_THREADS_PER_MINION) << " found pairing sender minion: " << other_min << " in Reduce_Data_Consumed!!" << endm;
                        }
                    }
                }

                // Executes the instruction
                if(!reduce_wait)
                {
                    inst.execute();

                    if (get_msg_port_stall(thread_id, 0) ){
                        thread = enabled_threads.erase(thread);
                        if(use_rbox) rbox[thread_id/(EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION)]->threadDisabled(thread_id%(EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION));
                        if (thread == enabled_threads.end()) break;
                    }
                    else {
                        // Updates PC
                        if(emu_state_change.pc_mod) {
                            current_pc[thread_id] = emu_state_change.pc;
                        } else {
                            current_pc[thread_id] += inst.size();
                        }

                        // Checks for ESR writes
                        if((emu_state_change.mem_addr[0] & ESR_REGION_MASK) == ESR_REGION)
                        {
                            uint64_t paddr = emu_state_change.mem_addr[0];
                            unsigned shire_id = (paddr & ESR_REGION_LOCAL) >> ESR_REGION_SHIRE_SH;
                            // Check if doing a local access
                            if((paddr & ESR_REGION_LOCAL) == ESR_REGION_LOCAL)
                            {
                                // Set shire ID to the one of the thread
                                shire_id = thread_id / (EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION);
                                // Fix the final address
                                paddr = (paddr & ~ESR_REGION_LOCAL) + shire_id * ESR_REGION_OFFSET;
                            }

                            // Is it an FCC?
                            if(emu_state_change.mem_mod[0] && ((paddr & ~ESR_REGION_LOCAL) == (ESR_SHIRE_REGION + ESR_SHIRE_FCC0_OFFSET)))
                                fcc_to_threads(shire_id, 0, emu_state_change.mem_data[0], log_en, log_min);
                            if(emu_state_change.mem_mod[0] && ((paddr & ~ESR_REGION_LOCAL) == (ESR_SHIRE_REGION + ESR_SHIRE_FCC2_OFFSET)))
                                fcc_to_threads(shire_id, 1, emu_state_change.mem_data[0], log_en, log_min);
                        }
                    }
                }

                // Thread is going to sleep
                if(inst.is_fcc() && !reduce_wait)
                {
                    if(do_log) { printf("Minion %i.%i.%i: Going to sleep (FCC)\n", thread_id / (EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION), (thread_id / EMU_THREADS_PER_MINION) % EMU_MINIONS_PER_SHIRE, thread_id % EMU_THREADS_PER_MINION); }
                    int old_thread = *thread;
                    thread = enabled_threads.erase(thread);

                    // Checks if there's a pending FCC and wakes up thread again
                    auto fcc = std::find(pending_fcc.begin(), pending_fcc.end(), old_thread);
                    if(fcc != pending_fcc.end())
                    {
                        if(do_log) { printf("Minion %i.%i.%i: Waking up due present FCC\n", old_thread / (EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION), (old_thread / EMU_THREADS_PER_MINION) % EMU_MINIONS_PER_SHIRE, old_thread % EMU_THREADS_PER_MINION); }
                        enabled_threads.push_back(old_thread);
                        pending_fcc.erase(fcc);
                    }
                }
                else if(inst.is_wfi() && !reduce_wait)
                {
                    if(do_log) { printf("Minion %i.%i.%i: Going to sleep (WFI)\n", thread_id / (EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION), (thread_id / EMU_THREADS_PER_MINION) % EMU_MINIONS_PER_SHIRE, thread_id % EMU_THREADS_PER_MINION); }
                    thread = enabled_threads.erase(thread);
                }
                else if(inst.is_stall() && !reduce_wait)
                {
                    if(do_log) { printf("Minion %i.%i.%i: Going to sleep (CSR STALL)\n", thread_id / (EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION), (thread_id / EMU_THREADS_PER_MINION) % EMU_MINIONS_PER_SHIRE, thread_id % EMU_THREADS_PER_MINION); }
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
                //if(do_log) { printf("Minion %i.%i.%i: Taking a trap\n", thread_id / (EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION), (thread_id / EMU_THREADS_PER_MINION) % EMU_MINIONS_PER_SHIRE, thread_id % EMU_THREADS_PER_MINION); }
                if (current_pc[thread_id] == emu_state_change.pc) {
                   log << LOG_FTL << "Thread " << thread_id << " is trapping to the same address that caused a trap (0x" << std::hex << current_pc[thread_id] << "). Avoiding infinite trap recursion." << endm;
                }
                current_pc[thread_id] = emu_state_change.pc;
                thread++;
            }
            catch (const std::exception& e)
            {
                log << LOG_FTL << e.what() << endm;
            }
        }

        // Once all threads done this cycle, check for new IPIs from net emu
        std::list<int> ipi_threads; // List of threads with an IPI with PC from network emu
        uint64_t       new_pc;      // New PC for the IPI
        net_emu.get_new_ipi(&enabled_threads, &ipi_threads, &new_pc);

        // Sets the PC for all the minions that got an IPI with PC
        auto it = ipi_threads.begin();
        while(it != ipi_threads.end())
        {
            if(dump_log(log_en, log_min, * it)) { printf("Minion %i.%i.%i: Waking up due IPI with PC %" PRIx64 "\n", (* it) / (EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION), ((* it) / EMU_THREADS_PER_MINION) % EMU_MINIONS_PER_SHIRE, (* it) % EMU_THREADS_PER_MINION, new_pc); }
            current_pc[* it] = new_pc;
            enabled_threads.push_back(* it);
            it++;
        }
        emu_cycle++;
    }
    printf("Emulation done!!\n");

    // Dumping
    if(dump_file != NULL)
    {
        memory->dump_file(dump_file, dump_addr, dump_size);
    }

    if (use_rbox){
      for (int i = 0 ; i < EMU_NUM_SHIRES; i++)
        delete rbox[i];
    }

    return 0;
}

