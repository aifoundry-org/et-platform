// Global includes
#include <stdio.h>
#include <stdlib.h>

// STD
#include <list>

// Local includes
#include "emu.h"
#include "main_memory.h"
#include "function_pointer_cache.h"
#include "instruction_cache.h"
#include "instruction.h"
#include "testLog.h"
#include "net_emulator.h"
#include "rboxSysEmu.h"

////////////////////////////////////////////////////////////////////////////////
// Defines
////////////////////////////////////////////////////////////////////////////////

#define RESET_PC    0x00001000
#define IPI_T0_ADDR 0xFFF00040
#define IPI_T1_ADDR 0xFFF00048

#define PA_SIZE        40
#define PA_M           (((uint64)1 << PA_SIZE) - 1)
#define PG_OFFSET_SIZE 12
#define PG_OFFSET_M    (((uint64)1 << PG_OFFSET_SIZE) - 1)
#define PPN_SIZE       (PA_SIZE - PG_OFFSET_SIZE)
#define PPN_M          (((uint64)1 << PPN_SIZE) - 1)
#define PTE_V_OFFSET   0
#define PTE_R_OFFSET   1
#define PTE_W_OFFSET   2
#define PTE_X_OFFSET   3
#define PTE_U_OFFSET   4
#define PTE_G_OFFSET   5
#define PTE_A_OFFSET   6
#define PTE_D_OFFSET   7
#define PTE_PPN_OFFSET 10

////////////////////////////////////////////////////////////////////////////////
// Types
////////////////////////////////////////////////////////////////////////////////

typedef void   (*func_ptr_state)       (inst_state_change * log_info_);
typedef void   (*func_ptr_pc)          (uint64 pc);
typedef void   (*func_ptr_thread)      (uint32 thread);
typedef void   (*func_ptr_initcsr)     (uint32 thread);
typedef void   (*func_ptr_init)        (xreg dst, uint64 data);
typedef void   (*func_ptr_minit)       (mreg dst, uint64 data);
typedef void   (*func_ptr_debug)       (int debug, int fakesam);
typedef void   (*func_ptr_thread1_en)  (void *);
typedef void   (*func_ptr_reduce_info) (uint64 value, uint64 * other_min, uint64 * action);
typedef uint64 (*func_ptr_xget)        (uint64 src1);
typedef uint64 (*func_ptr_csrget)      (csr src1);
typedef void   (*func_ptr_write_msg_port_data) (uint32 thread, uint32 port_id, uint32 *data);
typedef void   (*func_ptr_set_msg_port_data_func) (void* f, void *g, void *h);
typedef bool   (*func_ptr_get_stall_msg_port) (uint32, uint32);

typedef void   (*func_ptr_mem)(
    void * func_memread8_,
    void * func_memread16_,
    void * func_memread32_,
    void * func_memread64_,
    void * func_memwrite8_,
    void * func_memwrite16_,
    void * func_memwrite32_,
    void * func_memwrite64_);

// Reduce state
typedef enum
{
    Reduce_Idle,
    Reduce_Ready_To_Send,
    Reduce_Data_Consumed
} reduce_state;

// Memory access type
typedef enum
{
    Mem_Access_Load,
    Mem_Access_Store,
    Mem_Access_Fetch
} mem_access_type;

// Global variables
std::list<int>           enabled_threads;          // List of enabled threads
std::list<int>           pending_ipi;              // Pending IPI list
uint64                   current_pc[4096*2];       // PC for each thread
reduce_state             reduce_state_array[4096]; // Reduce state
uint32                   reduce_pair_array[4096];  // Reduce pairing minion
function_pointer_cache * func_cache;

////////////////////////////////////////////////////////////////////////////////
// Functions to emulate the main memory
////////////////////////////////////////////////////////////////////////////////

main_memory * memory;

// Virtual to physical
uint64 virt_to_phys(uint64 addr, mem_access_type macc)
{
    // Resolve where csrget function is
    func_ptr_csrget csrget = (func_ptr_csrget) func_cache->get_function_ptr("csrget");

    // Read SATP, PRV and MSTATUS
    uint64 satp = (csrget(csr_satp));
    uint64 satp_mode = (satp >> 60) & 0xF;
    uint64 satp_ppn = satp & PPN_M;
    uint64 prv = (csrget(csr_prv));
    uint64 mstatus = (csrget(csr_mstatus));
    bool sum = (mstatus >> 18) & 0x1;
    bool mxr = (mstatus >> 19) & 0x1;

    bool vm_enabled = (satp_mode != 0) && (prv <= CSR_PRV_S);

    // Set for Sv48
    // TODO: Support Sv39
    int Num_Levels = 4;
    int PTE_Size = 8;
    int PTE_Idx_Size = 9;

    uint64 pte_idx_mask = ((uint64)1<<PTE_Idx_Size)-1;
    if (vm_enabled)
    {
        // Perform page walk
        int level;
        uint64 ppn, pte_addr, pte;
        bool pte_v, pte_r, pte_w, pte_x, pte_u, pte_a, pte_d;

        level = Num_Levels;
        ppn = satp_ppn;
        do {
          level--;
          if (level < 0)
            return -1;

          // Take VPN[level]
          uint64 vpn = (addr >> (PG_OFFSET_SIZE + PTE_Idx_Size*level)) & pte_idx_mask;
          // Read PTE
          pte_addr = (ppn << PG_OFFSET_SIZE) + vpn*PTE_Size;
          memory->read(pte_addr, PTE_Size, &pte);

          // Read PTE fields
          pte_v = (pte >> PTE_V_OFFSET) & 0x1;
          pte_r = (pte >> PTE_R_OFFSET) & 0x1;
          pte_w = (pte >> PTE_W_OFFSET) & 0x1;
          pte_x = (pte >> PTE_X_OFFSET) & 0x1;
          pte_u = (pte >> PTE_U_OFFSET) & 0x1;
          pte_a = (pte >> PTE_A_OFFSET) & 0x1;
          pte_d = (pte >> PTE_D_OFFSET) & 0x1;
          // Read PPN
          ppn = (pte >> PTE_PPN_OFFSET) & PPN_M;

          // Check invalid entry
          if (!pte_v || (!pte_r && pte_w))
            return -1;

          // Check if PTE is a pointer to next table level
        } while (!pte_r && !pte_x);

        // A leaf PTE has been found

        // Check permissions
        bool perm_ok = (macc == Mem_Access_Load)  ? pte_r || (mxr && pte_x) :
                       (macc == Mem_Access_Store) ? pte_w :
                                                    pte_x; // Mem_Access_Fetch
        if (!perm_ok)
          return -1;

        // Check privilege mode
        // If page is accessible to user mode, supervisor mode SW may also access it if sum bit from the sstatus is set
        // Otherwise, check that privilege mode is higher than user
        bool priv_ok = pte_u ? (prv == CSR_PRV_U) || sum : prv != CSR_PRV_U;
        if (!priv_ok)
          return -1;

        // Check if it is a misaligned superpage
        if ((level > 0) && ((ppn & ((1<<(PTE_Idx_Size*level))-1)) != 0))
          return -1;

        if (!pte_a || ((macc == Mem_Access_Store) && !pte_d))
        {
          // Set pte.a to 1 and, if the memory access is a store, also set pte.d to 1
          uint64 pte_write = pte;
          pte_write |= 1 << PTE_A_OFFSET;
          if (macc == Mem_Access_Store)
            pte_write |= 1 << PTE_D_OFFSET;

          // Write PTE
          memory->write(pte_addr, PTE_Size, &pte_write);
        }

        // Obtain physical address
        uint64 paddr;

        // Copy page offset
        paddr = addr & PG_OFFSET_M;

        for (int i = 0; i < Num_Levels-1; i++)
        {
          // If level > 0, this is a superpage translation so VPN[level-1:0] are part of the page offset
          if (i < level)
            paddr |= addr & (pte_idx_mask << (PG_OFFSET_SIZE + PTE_Idx_Size*i));
          else
            paddr |= (ppn & (pte_idx_mask << (PTE_Idx_Size*i))) << PG_OFFSET_SIZE;
        }
        // PPN[3] is 17 bits wide
        paddr |= ppn & ((((uint64)1<<17) - 1) << (PTE_Idx_Size*(Num_Levels-1)));
        // Final physical address only uses 40 bits
        paddr &= PA_M;

        return paddr;
    }
    else
    {
        // Direct mapping, physical address is 40 bits
        return addr & PA_M;
    }
}

// This functions are called by emu. We should clean this to a nicer way...
uint8 emu_memread8(uint64 addr)
{
    uint8 ret;
    memory->read(virt_to_phys(addr, Mem_Access_Load), 1, &ret);
    return ret;
}

uint16 emu_memread16(uint64 addr)
{
    uint16 ret;
    memory->read(virt_to_phys(addr, Mem_Access_Load), 2, &ret);
    return ret;
}

uint32 emu_memread32(uint64 addr)
{
    uint32 ret;
    memory->read(virt_to_phys(addr, Mem_Access_Load), 4, &ret);
    return ret;
}

uint64 emu_memread64(uint64 addr)
{
    uint64 ret;
    memory->read(virt_to_phys(addr, Mem_Access_Load), 8, &ret);
    return ret;
}

void emu_memwrite8(uint64 addr, uint8 data)
{
    memory->write(virt_to_phys(addr, Mem_Access_Store), 1, &data);
}

void emu_memwrite16(uint64 addr, uint16 data)
{
    memory->write(virt_to_phys(addr, Mem_Access_Store), 2, &data);
}

void emu_memwrite32(uint64 addr, uint32 data)
{
    memory->write(virt_to_phys(addr, Mem_Access_Store), 4, &data);
}

void emu_memwrite64(uint64 addr, uint64 data)
{
    memory->write(virt_to_phys(addr, Mem_Access_Store), 8, &data);
}

////////////////////////////////////////////////////////////////////////////////
// Dump or not log info
////////////////////////////////////////////////////////////////////////////////

bool dump_log(bool log_en, int log_min, int thread_id)
{
    if(log_min >= 4096) return log_en;
    return (((thread_id >> 1) == log_min) && log_en);
}

////////////////////////////////////////////////////////////////////////////////
// Second thread of a minion is enabled/disabled
////////////////////////////////////////////////////////////////////////////////

void thread1_enabled(unsigned minion_id, int en, uint64 pc, bool log_en)
{
    unsigned thread_id = minion_id + 1;
    if(en)
    {
        auto it = std::find(enabled_threads.begin(), enabled_threads.end(), thread_id);
        if(it == enabled_threads.end())
        {
            func_ptr_thread setthread = (func_ptr_thread) func_cache->get_function_ptr("set_thread");
            func_ptr_init initreg = (func_ptr_init) func_cache->get_function_ptr("init");
            func_ptr_minit minitreg = (func_ptr_minit) func_cache->get_function_ptr("minit");
            func_ptr_initcsr initcsr = (func_ptr_initcsr) func_cache->get_function_ptr("initcsr");

            if(log_en) { printf("Minion %i.%i.1: Resetting\n", minion_id / 128, (minion_id >> 1) & 0x3F); }
            current_pc[thread_id] = RESET_PC;
            (setthread(thread_id));
            (initreg(x0, 0));
            (minitreg(m0, 255));
            (initcsr(thread_id));
            enabled_threads.push_back(thread_id);
        }
    }
    else
    {
        auto it = std::find(enabled_threads.begin(), enabled_threads.end(), thread_id);
        if(it != enabled_threads.end())
        {
            printf("Minion %i.%i.1: Disabling\n", minion_id / 128, (minion_id >> 1) & 0x3F);
            enabled_threads.erase(it);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// Sends an IPI to the desired minions specified in thread mask to the 1st or
// second thread (thread_dest) inside the shire of thread_src
////////////////////////////////////////////////////////////////////////////////

void ipi_to_threads(unsigned thread_src, unsigned thread_dest, uint64_t thread_mask, bool log_en, int log_min)
{
    unsigned shire_id = thread_src / 128;
    for(int m = 0; m < 64; m++)
    {
        if((thread_mask >> m) & 1)
        {
            int thread_id = shire_id * 128 + m * 2 + thread_dest;
            if(dump_log(log_en, log_min, thread_id)) { printf("Minion %i.%i.%i: Receiving IPI\n", shire_id, m, thread_dest); }
            // Checks if is already awaken
            if(std::find(enabled_threads.begin(), enabled_threads.end(), thread_id) != enabled_threads.end())
            {
                // Pushes to pending IPI
                pending_ipi.push_back(thread_id);
            }
            // Otherwise wakes up thread
            else
            {
                if(dump_log(log_en, log_min, thread_id)) { printf("Minion %i.%i.%i: Waking up due IPI\n", shire_id, m, thread_dest); }
                enabled_threads.push_back(thread_id);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// functions to connect rbox emulation with emu
////////////////////////////////////////////////////////////////////////////////
rboxSysEmu *rbox[64];

void newMsgPortDataRequest(uint32_t current_thread,  uint32_t port_id) {
  unsigned shire = current_thread/128;
  unsigned thread = current_thread % 128;
  if (rbox[shire] != NULL)
    rbox[shire]->dataRequest(thread);
}

bool queryMsgPort(uint32_t current_thread,  uint32_t port_id) {
  unsigned shire = current_thread/128;
  unsigned thread = current_thread % 128;
  if (rbox[shire] != NULL)
    return rbox[shire]->dataQuery(thread);
  else return false;
}

////////////////////////////////////////////////////////////////////////////////
// Parses a file that defines the memory regions plus contents to be
// loaded in the different regions
////////////////////////////////////////////////////////////////////////////////

bool parse_mem_file(const char * filename, main_memory * memory, testLog& log)
{

    FILE * file = fopen(filename, "r");
    if(file == NULL)
    {
        log << LOG_FTL << "Parse Mem File Error -> Couldn't open file " << filename << " for reading!!" << endm;
    }

    // Parses the contents
    char buffer[1024];
    char * buf_ptr = (char *) buffer;
    size_t buf_size = 1024;
    while(getline(&buf_ptr, &buf_size, file) != -1)
    {
        uint64_t base_addr;
        uint64_t size;
        char str[1024];
        if(sscanf(buffer, "New Mem Region: 40'h%LX, 40'h%LX, %s", &base_addr, &size, str) == 3)
        {
            memory->new_region(base_addr, size);
            log << LOG_INFO << "New Mem Region found: @ 0x" << std::hex << base_addr << ", size = 0x" << size << std::dec << endm;
        }
        else if(sscanf(buffer, "File Load: 40'h%LX, %s", &base_addr, str) == 2)
        {
            memory->load_file(str, base_addr);
            log << LOG_INFO << "New File Load found: @ 0x" << std::hex << base_addr << std::dec << endm;
        }
    }
    // Closes the file
    fclose(file);
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// Instruction log
////////////////////////////////////////////////////////////////////////////////

void print_inst_log(instruction * inst, uint64 minion, uint64 current_pc, inst_state_change & emu_state_change)
{
    printf("Minion %i.%i.%i: PC %08x (inst bits %08x), mnemonic %s\n", minion / 128, (minion >> 1) & 0x3F, minion & 1, current_pc, inst->get_enc(), inst->get_mnemonic().c_str());
}

// Returns current thread

int32_t thread_id;

uint32_t get_thread()
{
    return thread_id;
}

////////////////////////////////////////////////////////////////////////////////
// Main
////////////////////////////////////////////////////////////////////////////////

int main(int argc, char * argv[])
{
    // Logger
    testLog log("System EMU", LOG_DEBUG);

    char * mem_desc_file = NULL;
    char * net_desc_file = NULL;
    bool mem_desc = false;
    bool net_desc = false;
    bool minions = false;
    bool shires = false;
    bool log_en = false;
    int  log_min = 4096;
    char * dump_file = NULL;
    int dump = 0;
    uint64 dump_addr = 0;
    uint64 dump_size = 0;
    uint64_t minions_en = 1;
    uint64_t shires_en = 1;
    bool use_rbox = false;

    bzero(rbox, sizeof(rbox));

    int i = 0;
    for(int i = 1; i < argc; i++)
    {
        if(mem_desc)
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
            sscanf(argv[i], "%llx", &minions_en);
            minions = 0;
        }
        else if(shires)
        {
            sscanf(argv[i], "%llx", &shires_en);
            shires = 0;
        }
        else if(dump == 1)
        {
            sscanf(argv[i], "%llx", &dump_addr);
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
        else if(strcmp(argv[i], "-l") == 0)
        {
            log_en = true;
        }
        else if (strcmp(argv[i], "-rbox") == 0) {
          use_rbox = true;
        }
        else
        {
            log << LOG_FTL << "Unkown parameter " << argv[i] << endm;
        }

        
    }

    if(mem_desc_file == NULL)
    {
        log << LOG_FTL << "Memory descriptor file not set!!" << endm;
    }

    // Generates the main memory of the emulator
    memory = new main_memory("checker main memory");
    memory->setGetThread(get_thread);

    // This is an object that resolves where the emulation functions are
    std::string repo = getenv("BEMU");
    repo += "/checker/build/emu.so";
    func_cache = new function_pointer_cache(repo.c_str());

    // Instruction cache    
    instruction_cache * inst_cache = new instruction_cache(memory, func_cache);

    // Resolves where some functions are
    func_ptr_pc setpc                = (func_ptr_pc) func_cache->get_function_ptr("set_pc");
    func_ptr_thread setthread        = (func_ptr_thread) func_cache->get_function_ptr("set_thread");
    func_ptr_mem setmemory           = (func_ptr_mem) func_cache->get_function_ptr("set_memory_funcs");
    func_ptr_init initreg            = (func_ptr_init) func_cache->get_function_ptr("init");
    func_ptr_minit minitreg          = (func_ptr_minit) func_cache->get_function_ptr("minit");
    func_ptr_initcsr initcsr         = (func_ptr_initcsr) func_cache->get_function_ptr("initcsr");
    func_ptr_state setlogstate       = (func_ptr_state) func_cache->get_function_ptr("setlogstate");
    func_ptr clearlogstate           = func_cache->get_function_ptr("clearlogstate");
    func_ptr_debug init_emu          = (func_ptr_debug) func_cache->get_function_ptr("init_emu");
    func_ptr_reduce_info reduce_info = (func_ptr_reduce_info) func_cache->get_function_ptr("get_reduce_info");
    func_ptr_xget xget               = (func_ptr_xget) func_cache->get_function_ptr("xget");
    func_ptr_write_msg_port_data write_msg_port = (func_ptr_write_msg_port_data)  func_cache->get_function_ptr("write_msg_port_data_");
    func_ptr_set_msg_port_data_func set_msg_port_data_func = (func_ptr_set_msg_port_data_func) func_cache->get_function_ptr("set_msg_port_data_func");
    func_ptr_get_stall_msg_port getStallMsgPort = (func_ptr_get_stall_msg_port) func_cache->get_function_ptr("get_msg_port_stall");
    // Init emu
    (init_emu(log_en, false));

    *( (bool*) func_cache->get_function_ptr("in_sysemu")) = true;

    // Log state (needed to know PC changes)
    inst_state_change emu_state_change;
    (setlogstate(&emu_state_change)); // This is done every time just in case we have several checkers

    // Defines the memory access functions
    (setmemory((void *) emu_memread8,  (void *) emu_memread16,  (void *) emu_memread32,  (void *) emu_memread64,
               (void *) emu_memwrite8, (void *) emu_memwrite16, (void *) emu_memwrite32, (void *) emu_memwrite64));

    // Sets callback function to know when second thread is enabled/disabled
    func_ptr_thread1_en set_thread1_en = (func_ptr_thread1_en) func_cache->get_function_ptr("set_thread1_enabled_func");
    set_thread1_en((void *) thread1_enabled);

    // Parses the memory description
    parse_mem_file(mem_desc_file, memory, log);

    net_emulator net_emu(memory);
    net_emu.set_log(&log);
    // Parses the net description (it emulates a Maxion sending interrupts to minions)
    if(net_desc_file != NULL)
    {
        net_emu.set_file(net_desc_file);
    }
    
    // initialize rboxes-----------------------------------
    if (use_rbox){ 
      for (int i = 0 ; i < 64; i++) 
        rbox[i]=new rboxSysEmu(i, memory, write_msg_port); 
      set_msg_port_data_func(NULL, (void * ) queryMsgPort, (void * ) newMsgPortDataRequest);
    }

    // Generates the mask of enabled minions
    // Setup for all minions

    // For all the shires
    for(int s = 0; s < 64; s++)
    {
        // If shire enabled
        if((shires_en >> s) & 1)
        {
            // For all the minions
            for(int m = 0; m < 64; m++)
            {
                if((minions_en >> m) & 1)
                {
                    // Inits minion
                    thread_id = (s * 64 + m) * 2;
                    if(dump_log(log_en, log_min, thread_id)) { printf("Minion %i.%i.0: Resetting\n", s, m); }
                    current_pc[thread_id] = RESET_PC;
                    reduce_state_array[thread_id>>1] = Reduce_Idle;
                    (setthread(thread_id));
                    (initreg(x0, 0));
                    (minitreg(m0, 255));
                    (initcsr(thread_id));
                    // Puts thread id in the active list
                    enabled_threads.push_back(thread_id);
                }
            }
        }
    }

    instruction * inst;
    uint64_t emu_cycle = 0;

    bool rboxes_done = false;
    // While there are active threads or the network emulator is still not done
    while(enabled_threads.size() || (net_emu.is_enabled() && !net_emu.done()) || (use_rbox && !rboxes_done))
    {

        // For every cycle execute rbox
        
        if (use_rbox) {
            rboxes_done = true;
            for ( int i = 0; i < 64; i++)
            {
                int newEnable = rbox[i]->tick();

                if (newEnable > 0)
                    enabled_threads.push_back(newEnable + 64*i);

                if (!rbox[i]->done())
                {
                    rboxes_done = false;
                    break;
                }
            }
        }

        if(log_en)
        {
            printf("Starting Emulation Cycle %lli\n", emu_cycle);
        }

        auto thread = enabled_threads.begin();

        while(thread != enabled_threads.end())
        {
            thread_id = * thread;

            // Computes logging for this thread
            bool do_log = dump_log(log_en, log_min, thread_id);
            (init_emu(do_log, false));
            if(do_log) { printf("Starting emu of thread %i\n", thread_id); }

            // Gets instruction and sets state
            inst = inst_cache->get_instruction(virt_to_phys(current_pc[thread_id], Mem_Access_Fetch));
            (setthread(thread_id));
            (setpc(current_pc[thread_id]));
            (clearlogstate());
            if(do_log)
                print_inst_log(inst, thread_id, current_pc[thread_id], emu_state_change);

            // In case of reduce, we need to make sure that the other minion is also in reduce state
            bool reduce_wait = false;
            if(inst->get_is_reduce())
            {
                uint64 other_min, action;
                // Gets the source used for the reduce
                uint64 src1 = (xreg) inst->get_param(2);
                uint64 value = (xget(src1));
                (reduce_info(value, &other_min, &action));
                // Sender
                if(action == 0)
                {
                    // Moves to ready to send
                    reduce_state_array[thread_id>>1] = Reduce_Ready_To_Send;
                    reduce_pair_array[thread_id>>1]  = other_min;
                    // If the other minion hasn't arrived yet, wait
                    if((reduce_state_array[other_min] == Reduce_Idle) || (reduce_pair_array[other_min] != (thread_id>>1)))
                    {
                        reduce_wait = true;
                    }
                    // If it has consumed the data, move both threads to Idle
                    else if(reduce_state_array[other_min] == Reduce_Data_Consumed)
                    {
                        reduce_state_array[thread_id>>1] = Reduce_Idle;
                        reduce_state_array[other_min]    = Reduce_Idle;
                    }
                    else
                    {
                        log << LOG_FTL << "Reduce error: Minion: " << (thread_id >> 1) << " found pairing receiver minion: " << other_min << " in Reduce_Ready_To_Send!!" << endm;
                    }
                }
                // Receiver
                else if(action == 1)
                {
                    reduce_pair_array[thread_id>>1] = other_min;
                    // If the other minion hasn't arrived yet, wait
                    if((reduce_state_array[other_min] == Reduce_Idle) || (reduce_pair_array[other_min] != (thread_id>>1)))
                    {
                        reduce_wait = true;
                    }
                    // If pairing minion is in ready to send, consume the data
                    else if(reduce_state_array[other_min] == Reduce_Ready_To_Send)
                    {
                        reduce_state_array[thread_id>>1] = Reduce_Data_Consumed;
                    }
                    else
                    {
                        log << LOG_FTL << "Reduce error: Minion: " << (thread_id >> 1) << " found pairing sender minion: " << other_min << " in Reduce_Data_Consumed!!" << endm;
                    }
                }
            }
           
            // Executes the instruction
            if(!reduce_wait)
            {
                inst->exec();

                if (getStallMsgPort(thread_id, 0) ){
                  thread = enabled_threads.erase(thread);
                  rbox[thread_id/128]->threadDisabled(thread_id%128);
                  if (thread == enabled_threads.end()) break;
                }
                else {
                  // Updates PC
                  if(emu_state_change.pc_mod)
                    current_pc[thread_id] = emu_state_change.pc;
                  else
                    current_pc[thread_id] = current_pc[thread_id] + 4;
                  
                  // Checks for IPI
                  if(emu_state_change.mem_mod[0] && (emu_state_change.mem_addr[0] == IPI_T0_ADDR))
                    ipi_to_threads(thread_id, 0, emu_state_change.mem_data[0], log_en, log_min);
                  if(emu_state_change.mem_mod[0] && (emu_state_change.mem_addr[0] == IPI_T1_ADDR))
                    ipi_to_threads(thread_id, 1, emu_state_change.mem_data[0], log_en, log_min);
                }
            }

            // Thread is going to sleep
            if(inst->get_is_wfi() && !reduce_wait)
            {
                if(do_log) { printf("Minion %i.%i.%i: Going to sleep\n", thread_id / 128, (thread_id / 2) & 0x3F, thread_id & 1); }
                auto old_thread = thread++;
                enabled_threads.erase(old_thread);
                // Checks if there's a pending IPI and wakes up thread again
                auto ipi = std::find(pending_ipi.begin(), pending_ipi.end(), * old_thread);
                if(ipi != pending_ipi.end())
                {
                    if(do_log) { printf("Minion %i.%i.%i: Waking up due IPI\n", (* old_thread) / 128, ((* old_thread) / 2) & 0x3F, (* old_thread) & 1); }
                    enabled_threads.push_back(* old_thread);
                    pending_ipi.erase(ipi);
                }
            }
            else
            {
                thread++;
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
            if(dump_log(log_en, log_min, * it)) { printf("Minion %i.%i.%i: Waking up due IPI with PC %llx\n", (* it) / 128, ((* it) / 2) & 0x3F, (* it) & 1, new_pc); }
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
      for (int i = 0 ; i < 64; i++)
        delete rbox[i];
    }

    return 0;
}

