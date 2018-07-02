#ifndef _INSTRUCTION_CACHE_BLOCK_
#define _INSTRUCTION_CACHE_BLOCK_

// Local defines
#include "instruction.h"
#include "main_memory.h"
#include "function_pointer_cache.h"
#include "testLog.h"

// Defines
#define INSTRUCTION_CACHE_BLOCK_SHIFT 6                                                     // Shift for bytes in the block
#define INSTRUCTION_CACHE_BLOCK_SIZE   (1 << INSTRUCTION_CACHE_BLOCK_SHIFT)                 // Bytes per block
#define INSTRUCTION_CACHE_BLOCK_MASK  ((1 << INSTRUCTION_CACHE_BLOCK_SHIFT) - 1)            // Mask for bytes in block
#define INSTRUCTION_SHIFT 1                                                                 // At least 2 bytes per instruction
#define INSTRUCTION_CACHE_BLOCK_INSTR (INSTRUCTION_CACHE_BLOCK_SIZE >> INSTRUCTION_SHIFT)                   // Instruction per block, at lest 2 bytes per instruction
#define INSTRUCTION_CACHE_BLOCK_INSTR_SHIFT (INSTRUCTION_CACHE_BLOCK_SHIFT - INSTRUCTION_SHIFT)             // Shift for instructions in the block, at least 2 bytes per instruction
#define INSTRUCTION_CACHE_BLOCK_INSTR_MASK ((1 <<INSTRUCTION_CACHE_BLOCK_INSTR_SHIFT) - INSTRUCTION_SHIFT)  // Mask for instructions in the block, at least 2 bytes per instruction

// This class stores a block of INSTRUCTION_CACHE_BLOCK_SIZE consecutive instructions
// that have been decoded. We store instructions decoded to speed up performance of the
// checker
class instruction_cache_block
{
    public:
        instruction_cache_block(uint64_t base, main_memory * memory_, function_pointer_cache * func_cache_, testLog * log_);
        ~instruction_cache_block();

        instruction * get_instruction(uint64_t pc);
    private:
        void decode(); // Decodes the instructions using the main memory

        instruction                  instructions[INSTRUCTION_CACHE_BLOCK_SIZE / 2];    // Decoded instructions of this block
        uint64_t                     base_pc;                                           // Base PC for this block
        main_memory                * memory;                                            // Main memory to read the instruction bits and decode them
        function_pointer_cache     * func_cache;                                        // Pointer to the emulation functions
        testLog* log;                                                                   // Pointer to the logger
};

#endif // _INSTRUCTION_CACHE_BLOCK_

