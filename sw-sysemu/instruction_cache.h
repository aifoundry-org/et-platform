#ifndef _INSTRUCTION_CACHE_
#define _INSTRUCTION_CACHE_

// Local includes
#include "instruction.h"
#include "main_memory.h"

// STD includes
#include <unordered_map>
#include <string>

class instruction_cache
{
    public:
        instruction_cache(main_memory * memory_);
        ~instruction_cache();

        instruction * get_instruction(uint64_t pc);
    private:
        // Typedef for the hash of 
        typedef std::unordered_map<uint64_t, instruction *>           inst_cache_hash_t;
        typedef std::unordered_map<uint64_t, instruction *>::iterator inst_cache_hash_it_t;

        main_memory                * memory;     // Pointer to the main memory
        inst_cache_hash_t            cache;      // Hash of blocks
        testLog                      log;        // Logging

        void decode(uint64_t base_pc);
};

#endif // _INSTRUCTION_CACHE_

