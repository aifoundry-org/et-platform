// Global includes
#include <stdio.h>
#include <stdlib.h>

// STD
#include <list>

// Local includes
#include "emu.h"
#include "emu_defines.h"
#include "main_memory.h"
#include "instruction_cache.h"
#include "instruction.h"
#include "log.h"
#include "net_emulator.h"
#include "rboxSysEmu.h"
#include "sys_emu.h"

////////////////////////////////////////////////////////////////////////////////
// Global variables
////////////////////////////////////////////////////////////////////////////////

std::list<int>  enabled_threads;                     // List of enabled threads
std::list<int>  pending_ipi;                         // Pending IPI list
static uint64_t current_pc[EMU_NUM_THREADS];         // PC for each thread
reduce_state    reduce_state_array[EMU_NUM_MINIONS]; // Reduce state
uint32_t        reduce_pair_array[EMU_NUM_MINIONS];  // Reduce pairing minion

////////////////////////////////////////////////////////////////////////////////
// Functions to emulate the main memory
////////////////////////////////////////////////////////////////////////////////

main_memory * memory;

// This functions are called by emu. We should clean this to a nicer way...
uint8_t emu_memread8(uint64_t addr)
{
    uint8_t ret;
    memory->read(addr, 1, &ret);
    return ret;
}

uint16_t emu_memread16(uint64_t addr)
{
    uint16_t ret;
    memory->read(addr, 2, &ret);
    return ret;
}

uint32_t emu_memread32(uint64_t addr)
{
    uint32_t ret;
    memory->read(addr, 4, &ret);
    return ret;
}

uint64_t emu_memread64(uint64_t addr)
{
    uint64_t ret;
    memory->read(addr, 8, &ret);
    return ret;
}

void emu_memwrite8(uint64_t addr, uint8_t data)
{
    memory->write(addr, 1, &data);
}

void emu_memwrite16(uint64_t addr, uint16_t data)
{
    memory->write(addr, 2, &data);
}

void emu_memwrite32(uint64_t addr, uint32_t data)
{
    memory->write(addr, 4, &data);
}

void emu_memwrite64(uint64_t addr, uint64_t data)
{
    memory->write(addr, 8, &data);
}

////////////////////////////////////////////////////////////////////////////////
// Dump or not log info
////////////////////////////////////////////////////////////////////////////////

bool dump_log(bool log_en, int log_min, int thread_id)
{
    if(log_min >= EMU_NUM_MINIONS) return log_en;
    return (((thread_id / EMU_THREADS_PER_MINION) == log_min) && log_en);
}

////////////////////////////////////////////////////////////////////////////////
// Sends an IPI to the desired minions specified in thread mask to the 1st or
// second thread (thread_dest) inside the shire of thread_src
////////////////////////////////////////////////////////////////////////////////

void ipi_to_threads(unsigned thread_src, unsigned thread_dest, uint64_t thread_mask, bool log_en, int log_min)
{
    unsigned shire_id = thread_src / (EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION);
    for(int m = 0; m < EMU_MINIONS_PER_SHIRE; m++)
    {
        if((thread_mask >> m) & 1)
        {
            int thread_id = shire_id * EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION + m * EMU_THREADS_PER_MINION  + thread_dest;
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
                if(dump_log(log_en, log_min, thread_id)) { printf("Minion %i.%i.%i: Waking up due sent IPI\n", shire_id, m, thread_dest); }
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
  unsigned shire = current_thread/(EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION);
  unsigned thread = current_thread % (EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION);
  if (rbox[shire] != NULL)
    rbox[shire]->dataRequest(thread);
}

bool queryMsgPort(uint32_t current_thread,  uint32_t port_id) {
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
    }
    // Closes the file
    fclose(file);
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// Instruction log
////////////////////////////////////////////////////////////////////////////////

void print_inst_log(instruction * inst, uint64_t minion, uint64_t current_pc, inst_state_change & emu_state_change)
{
    printf("Minion %" PRIu64 ".%" PRIu64 ".%" PRIu64 ": PC %08" PRIx64 " (inst bits %08" PRIx32 "), mnemonic %s\n", minion / (EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION), (minion / EMU_THREADS_PER_MINION) % EMU_MINIONS_PER_SHIRE, minion % EMU_THREADS_PER_MINION, current_pc, inst->get_enc(), inst->get_mnemonic().c_str());
}

// Returns current thread

int32_t thread_id;

static uint32_t get_thread_emu()
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
    bool mem_desc        = false;
    bool net_desc        = false;
    bool minions         = false;
    bool shires          = false;
    bool log_en          = false;
    bool create_mem_at_runtime = false;
    int  log_min         = EMU_NUM_MINIONS;
    char * dump_file     = NULL;
    int dump             = 0;
    uint64_t dump_addr   = 0;
    uint64_t dump_size   = 0;
    uint64_t minions_en  = 1;
    uint64_t shires_en   = 1;
    bool use_rbox        = false;

    bzero(rbox, sizeof(rbox));

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
            sscanf(argv[i], "%" PRIx64, &minions_en);
            minions = 0;
        }
        else if(shires)
        {
            sscanf(argv[i], "%" PRIx64, &shires_en);
            shires = 0;
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
        else if(strcmp(argv[i], "-m") == 0)
        {
            create_mem_at_runtime = true;
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
    memory->setGetThread(get_thread_emu);
    if (create_mem_at_runtime) {
       memory->create_mem_at_runtime();
    }

    // Instruction cache
    instruction_cache * inst_cache = new instruction_cache(memory);

    // Init emu
    init_emu(log_en, false);

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
        rbox[i] = new rboxSysEmu(i, memory, write_msg_port_data_);
      set_msg_port_data_func(NULL, (void * ) queryMsgPort, (void * ) newMsgPortDataRequest);
    }

    // Generates the mask of enabled minions
    // Setup for all minions

    // For all the shires
    for(int s = 0; s < (EMU_NUM_MINIONS / EMU_MINIONS_PER_SHIRE); s++)
    {
        // If shire enabled
        if((shires_en >> s) & 1)
        {
            // For all the minions
            for(int m = 0; m < EMU_MINIONS_PER_SHIRE; m++)
            {
                if((minions_en >> m) & 1)
                {
                    // Inits minion
                    thread_id = (s * EMU_MINIONS_PER_SHIRE + m) * EMU_THREADS_PER_MINION;
                    if(dump_log(log_en, log_min, thread_id)) { printf("Minion %i.%i.0: Resetting\n", s, m); }
                    current_pc[thread_id] = RESET_PC;
                    reduce_state_array[thread_id / EMU_THREADS_PER_MINION] = Reduce_Idle;
                    set_thread(thread_id);
                    init(x0, 0);
                    minit(m0, 255);
                    initcsr(thread_id);
                    // Puts thread id in the active list
                    enabled_threads.push_back(thread_id);

                    // Inits minion
                    thread_id = (s * EMU_MINIONS_PER_SHIRE + m) * EMU_THREADS_PER_MINION + 1;
                    if(dump_log(log_en, log_min, thread_id)) { printf("Minion %i.%i.0: Resetting\n", s, m); }
                    current_pc[thread_id] = RESET_PC;
                    reduce_state_array[thread_id / EMU_THREADS_PER_MINION] = Reduce_Idle;
                    set_thread(thread_id);
                    init(x0, 0);
                    minit(m0, 255);
                    initcsr(thread_id);
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
            printf("Starting Emulation Cycle %" PRIu64 "\n", emu_cycle);
        }

        auto thread = enabled_threads.begin();

        while(thread != enabled_threads.end())
        {
            thread_id = * thread;

            // Computes logging for this thread
            bool do_log = dump_log(log_en, log_min, thread_id);
            init_emu(do_log, false);
            if(do_log) { printf("Starting emu of thread %i\n", thread_id); }

            // Try to execute one instruction, this may trap
            try
            {
                // Gets instruction and sets state
                clearlogstate();
                set_thread(thread_id);
                set_pc(current_pc[thread_id]);
                inst = inst_cache->get_instruction(virt_to_phys_emu(current_pc[thread_id], Mem_Access_Fetch));
                if(do_log)
                    print_inst_log(inst, thread_id, current_pc[thread_id], emu_state_change);

                // Check if a trap is forced for this instruction
                emu_mcode_insn(inst->get_emu_func());

                // In case of reduce, we need to make sure that the other minion is also in reduce state
                bool reduce_wait = false;
                if(inst->get_is_reduce())
                {
                    uint64_t other_min, action;
                    // Gets the source used for the reduce
                    uint64_t src1 = (xreg) inst->get_param(2);
                    uint64_t value = xget(src1);
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
                    inst->exec();

                    if (get_msg_port_stall(thread_id, 0) ){
                        thread = enabled_threads.erase(thread);
                        rbox[thread_id/(EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION)]->threadDisabled(thread_id%(EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION));
                        if (thread == enabled_threads.end()) break;
                    }
                    else {
                        // Updates PC
                        if(emu_state_change.pc_mod) {
                            current_pc[thread_id] = emu_state_change.pc;
                        } else {
                            current_pc[thread_id] += inst->get_size();
                        }

                        // Checks for IPI
                        if(emu_state_change.mem_mod[0] && ((emu_state_change.mem_addr[0] & 0xFFFFC1FF) == IPI_T0_ADDR))
                            ipi_to_threads(thread_id, 0, emu_state_change.mem_data[0], log_en, log_min);
                        if(emu_state_change.mem_mod[0] && ((emu_state_change.mem_addr[0] & 0xFFFFC1FF) == IPI_T1_ADDR))
                            ipi_to_threads(thread_id, 1, emu_state_change.mem_data[0], log_en, log_min);
                    }
                }

                // Thread is going to sleep
                if(inst->get_is_wfi() && !reduce_wait)
                {
                    if(do_log) { printf("Minion %i.%i.%i: Going to sleep\n", thread_id / (EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION), (thread_id / EMU_THREADS_PER_MINION) % EMU_MINIONS_PER_SHIRE, thread_id % EMU_THREADS_PER_MINION); }
                    int old_thread = *thread;
                    thread = enabled_threads.erase(thread);

                    // Checks if there's a pending IPI and wakes up thread again
                    auto ipi = std::find(pending_ipi.begin(), pending_ipi.end(), old_thread);
                    if(ipi != pending_ipi.end())
                    {
                        if(do_log) { printf("Minion %i.%i.%i: Waking up due present IPI\n", old_thread / (EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION), (old_thread / EMU_THREADS_PER_MINION) % EMU_MINIONS_PER_SHIRE, old_thread % EMU_THREADS_PER_MINION); }
                        enabled_threads.push_back(old_thread);
                        pending_ipi.erase(ipi);
                    }
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
                current_pc[thread_id] = emu_state_change.pc;
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
      for (int i = 0 ; i < 64; i++)
        delete rbox[i];
    }

    return 0;
}

