#ifndef _MAIN_MEMORY_REGION_
#define _MAIN_MEMORY_REGION_

#include <cstdint>

// Local
#define CHECKER
#include "testLog.h"

#define CACHE_LINE_MASK 0xFFFFFFFFC0ULL
#define CACHE_LINE_SIZE (64)

#define MEM_REGION_READ_ALLOWED 1
#define MEM_REGION_WRITE_ALLOWED 2

#define MEM_REGION_RO MEM_REGION_READ_ALLOWED
#define MEM_REGION_WO MEM_REGION_WRITE_ALLOWED
#define MEM_REGION_RW ( MEM_REGION_READ_ALLOWED | MEM_REGION_WRITE_ALLOWED )

// Memory region of the main memory
class main_memory_region
{
    public:
        typedef uint32_t (*func_ptr_get_thread) ();

        // Constructors and destructors
        main_memory_region(uint64_t base, uint64_t size, testLog & l,
                           func_ptr_get_thread & get_thread,
                           int flags = MEM_REGION_RW);
        virtual ~main_memory_region();

        // operators to compare, used when searching the correct region in a memory access
        bool operator==(uint64_t ad) const {
            return ad >= base_ && ad < base_ + size_;
        }
        bool operator!=(uint64_t ad) const {
            return !(*this == ad);
        }

        // read and write
        virtual void write(uint64_t ad, int size, const void* data);
        virtual void read(uint64_t ad, int size, void* data);

        // Dump
        void dump();

    protected:
        // members
        const uint64_t base_;
        const uint64_t size_;
        char * const data_;
        int  flags_;
        testLog& log;
        func_ptr_get_thread & get_thread;
};

#endif // _MAIN_MEMORY_REGION_
