#ifndef _CHECKER_
#define _CHECKER_

// STD
#include <string>
#include <list>

// Local
#include "checker_defines.h"
#include "main_memory.h"
#include "instruction_cache.h"
#include "function_pointer_cache.h"
#include "log.h"
#include "testLog.h"

typedef uint64_t (*func_ptr_csrget) (csr src1);

uint8_t checker_memread8(uint64_t addr);
void checker_memwrite64(uint64_t addr, uint64_t data);

typedef struct 
{
    int    entry;
    uint64_t data[8];
} scratchpad_entry;

typedef struct 
{
    int    entry;
    uint32_t data[4];
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
        checker(main_memory * memory_, function_pointer_cache * func_cache_);
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
        void tensorfma_write(uint32_t thread, uint32_t entry, uint32_t * data);
        void reduce_write(uint32_t thread, uint32_t entry, uint32_t * data);
    private:
        checker_result do_reduce(uint32_t thread, instruction * inst, uint32_t * wake_thread);

        typedef void (*func_ptr_state)  (inst_state_change * log_info_);
        typedef void (*func_ptr_pc)     (uint64_t pc);
        typedef void (*func_ptr_set_thread) (uint32_t thread);
        typedef uint32_t (*func_ptr_get_thread) ();
        typedef uint32_t (*func_ptr_get_mask) (unsigned maskNr);
        typedef uint64_t (*func_ptr_xget)     (uint64_t src1);
        typedef void (*func_ptr_update_msg_ports) ();
        typedef void (*func_ptr_initcsr)(uint32_t thread);
        typedef void (*func_ptr_init)   (xreg dst, uint64_t data);
        typedef void (*func_ptr_fpinit) (freg dst, uint64_t data[2]);
        typedef void (*func_ptr_debug)  (int debug, int fakesam);
        typedef void (*func_ptr_reduce_info) (uint64_t value, uint64_t * other_min, uint64_t * action);
        typedef uint64_t (*func_get_scratchpad_value) (int entry, int block, int * last_entry, int * size);
        typedef void (*func_get_scratchpad_conv_list) (std::list<bool> * list);
        typedef uint64_t (*func_get_tensorfma_value) (int entry, int pass, int block, int * size, int * passes, bool * conv_skip);
        typedef uint64_t (*func_get_reduce_value) (int entry, int block, int * size, int * start_entry);
        typedef uint64_t (*func_virt_to_phys) (uint64_t addr, mem_access_type macc);

        uint64_t                      current_pc[EMU_NUM_THREADS];     // Current PC
        reduce_state                  reduce_state_array[4096];        // Reduce state
        uint32_t                      reduce_pair_array[4096];         // Reduce pairing minion
        main_memory                 * memory;                          // Pointer to the memory of the simulation
        instruction_cache           * inst_cache;                      // Pointer to the decoded instruction cache
        function_pointer_cache      * func_cache;                      // Pointer to the emulation functions
        inst_state_change             emu_state_change;                // Struct that holds the state change for the emu
        std::string                   error_msg;                       // Stores the error message
        func_ptr                      clearlogstate;                   // Pointer to the function to clear the log state
        func_ptr_state                setlogstate;                     // Pointer to the function to set the log state
        func_ptr_pc                   setpc;                           // Pointer to the function to set the PC
        func_ptr_set_thread           set_thread;                      // Pointer to the function to set the Thread
        func_ptr_get_thread           get_thread;                      // Pointer to the function to get the Thread
        func_ptr_xget                 xget;                            // Gets an integer value
        func_ptr_csrget               csrget;                          // Pointer to the function to get CSRs
        func_ptr_update_msg_ports     update_msg_ports;                // 
        func_ptr_get_mask             get_mask;                        // Pointer to the function to get the mask
        func_ptr_init                 initreg;                         // Pointer to the function that writes a value to an int register
        func_ptr_fpinit               fpinitreg;                       // Pointer to the function that writes a value to a floating point reg
        func_ptr_reduce_info          reduce_info;                     // Pointer to the function that gets reduce info
        func_get_scratchpad_value     get_scratchpad_value;            // Pointer to the function to get a scratchpad value
        func_get_scratchpad_conv_list get_scratchpad_conv_list;        // Pointer to the function to get a scratchpad list of conv CSR results
        func_get_tensorfma_value      get_tensorfma_value;             // Pointer to the function to get a tensorfma value
        func_get_reduce_value         get_reduce_value;                // Pointer to the function to get a reduce value
        func_virt_to_phys             virt_to_phys_emu;                // Pointer to the function to do a virtual to physical translation
        testLog                       log;                             // Logger
        uint64_t                      threadEnabled[EMU_NUM_THREADS];  // thread is enabled / disabled
        std::list<scratchpad_entry>   spd_entry_list[EMU_NUM_THREADS]; // List of RTL written scratchpad entries
        std::list<tensorfma_entry>    tensorfma_list[EMU_NUM_THREADS]; // List of RTL written tensorfma entries
        std::list<tensorfma_entry>    reduce_list[EMU_NUM_THREADS];    // List of RTL written reduce entries
};

#endif // _CHECKER_

