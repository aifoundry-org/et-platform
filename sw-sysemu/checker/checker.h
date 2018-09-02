#ifndef _CHECKER_
#define _CHECKER_

// STD
#include <string>
#include <list>

// Local
#include "main_memory.h"
#include "instruction_cache.h"
#include "log.h"

// defined in emu.h
extern uint8_t emu_use_fake_txfma;

extern uint8_t checker_memread8(uint64_t addr);
extern void checker_memwrite64(uint64_t addr, uint64_t data);

// Checker results
typedef enum
{
    CHECKER_OK    = 0,
    CHECKER_ERROR = 1,
    CHECKER_WAIT  = 2
} checker_result;

typedef struct
{
    int    entry;
    uint64_t data[8];
} scratchpad_entry;

typedef struct
{
    int    entry;
    uint32_t data[VL];
    uint32_t tensorfma_regfile_wmask;
} tensorfma_entry;

// Reduce state
typedef enum
{
    Reduce_Idle,
    Reduce_Ready_To_Send,
    Reduce_Data_Consumed
} reduce_state;

class checker
{
    public:
        // Constructor and destructor
        checker(main_memory * memory_);
        ~checker();

        // Sets the PC
        void start_pc(uint32_t thread, uint64_t pc);
        void ipi_pc(uint32_t thread, uint64_t pc);
        // Emulates current instruction and compares the state changes
        checker_result emu_inst(uint32_t thread, inst_state_change * changes, uint32_t * wake_minion);
        // Gets an error in string format
        std::string get_error_msg();

        // Converts virtual address to physical
        uint64_t virt_to_phys(uint64_t addr, mem_access_type macc);

        // Gets mnemonic
        std::string get_mnemonic(uint64_t pc);

        // enable or disable 2nd thread
        void thread1_enabled ( unsigned minionId, uint64_t en, uint64_t pc);

        // Tensor operations
        void tensorload_write(uint32_t thread, uint32_t entry, uint64_t * data);
        void tensorfma_write(uint32_t thread, uint32_t entry, uint32_t * data, uint32_t tensorfma_regfile_wmask);
        void reduce_write(uint32_t thread, uint32_t entry, uint32_t * data);

        typedef void (*func_texrec_t) (unsigned minionId, unsigned thread_id, const uint8_t *data, unsigned wordIdx, uint32_t mask);
        void set_texrec_func(func_texrec_t func_ptr);

    private:
        checker_result do_reduce(uint32_t thread, instruction * inst, uint32_t * wake_thread);
        void texrec(unsigned minionId, unsigned thread_id, const uint8_t *data, unsigned wordIdx, uint32_t mask);

        uint64_t                      current_pc[EMU_NUM_THREADS];     // Current PC
        reduce_state                  reduce_state_array[4096];        // Reduce state
        uint32_t                      reduce_pair_array[4096];         // Reduce pairing minion
        main_memory                 * memory;                          // Pointer to the memory of the simulation
        instruction_cache           * inst_cache;                      // Pointer to the decoded instruction cache
        inst_state_change             emu_state_change;                // Struct that holds the state change for the emu
        std::string                   error_msg;                       // Stores the error message
        func_texrec_t                 texrec_func_ptr;                 // Emulates texrec
        testLog                       log;                             // Logger
        uint64_t                      threadEnabled[EMU_NUM_THREADS];  // thread is enabled / disabled
        std::list<scratchpad_entry>   scp_entry_list[EMU_NUM_THREADS]; // List of RTL written scratchpad entries
        std::list<tensorfma_entry>    tensorfma_list[EMU_NUM_THREADS]; // List of RTL written tensorfma entries
        std::list<tensorfma_entry>    reduce_list[EMU_NUM_THREADS];    // List of RTL written reduce entries
};

#endif // _CHECKER_

