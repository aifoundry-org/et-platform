#ifndef _CHECKER_
#define _CHECKER_

// STD
#include <string>
#include <list>
#include <iostream>

// Local
#include "common/main_memory.h"
#include "log.h"

extern uint8_t checker_memread8(uint64_t addr);
extern void checker_memwrite64(uint64_t addr, uint64_t data);

// Checker results
typedef enum
{
    CHECKER_OK    = 0,
    CHECKER_ERROR = 1,
    CHECKER_WAIT  = 2,
    CHECKER_WARNING = 3
} checker_result;

typedef struct
{
    uint32_t banks;
    int      entry;
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

typedef struct
{
   uint64_t base;
   uint64_t top;
} ignored_mem_region;

class checker
{
 public:
        // Constructor and destructor
        checker(main_memory * memory_, enum logLevel emu_log_level, bool checker_en);
        ~checker();

        void set_et_core(int core_type);
        // Sets the PC
        void start_pc(uint32_t thread, uint64_t pc);
        void ipi_pc(uint32_t thread, uint64_t pc);
        // Emulates current instruction and compares the state changes
        checker_result emu_inst(uint32_t thread, inst_state_change * changes, int * wake_minion);
        // Decoder Helper functions
        void emu_disasm(char* str, size_t size, uint32_t bits);
        // Gets an error in string format
        std::string get_error_msg();

        // enable or disable 2nd thread
        void thread1_enabled ( unsigned minionId, uint64_t en, uint64_t pc);

        // Tensor operations
        void tensorload_write(uint32_t thread, uint32_t entry, uint64_t * data, uint32_t banks);
        void tensorfma_write(uint32_t thread, uint32_t entry, uint32_t * data, uint32_t tensorfma_regfile_wmask);
        void tensorquant_write(uint32_t thread, uint32_t entry, uint32_t * data, uint32_t tensorquant_regfile_wmask);
        void reduce_write(uint32_t thread, uint32_t entry, uint32_t * data);
        void aggregate_tl_data(uint32_t thread);

        void update_fcsr_flags(unsigned minionId, unsigned flags);

        // Register a region to ignore checks on
        void add_ignored_mem_region(uint64_t base, uint64_t top);
        bool address_is_in_ignored_region(uint64_t addr);

        // TBOX write to Minion Message Port
        void tbox_port_write(uint32_t thread, uint32_t port_id);

    private:
        checker_result do_reduce(uint32_t thread, uint64_t value, int * wake_thread);

        uint64_t                      current_pc[EMU_NUM_THREADS];         // Current PC
        reduce_state                  reduce_state_array[EMU_NUM_THREADS]; // Reduce state
        uint32_t                      reduce_pair_array[EMU_NUM_THREADS];  // Reduce pairing minion
        main_memory                 * memory;                              // Pointer to the memory of the simulation
        inst_state_change             emu_state_change;                    // Struct that holds the state change for the emu
        std::string                   error_msg;                           // Stores the error message
        testLog                       log;                                 // Logger
        uint64_t                      threadEnabled[EMU_NUM_THREADS];      // thread is enabled / disabled
        std::list<scratchpad_entry>   scp_entry_list[EMU_NUM_THREADS];     // List of RTL written scratchpad entries
        std::list<tensorfma_entry>    tensorfma_list[EMU_NUM_THREADS];     // List of RTL written tensorfma entries
        std::list<tensorfma_entry>    tensorquant_list[EMU_NUM_THREADS];   // List of RTL written tensorquant entries
        std::list<tensorfma_entry>    reduce_list[EMU_NUM_THREADS];        // List of RTL written reduce entries
        std::list<csr>                waived_csrs;                         // List of CSRs whose checking is waived
        std::list<ignored_mem_region> ignored_mem_regions;                 // List of memory regions whose data won't be checked
};

#endif // _CHECKER_

