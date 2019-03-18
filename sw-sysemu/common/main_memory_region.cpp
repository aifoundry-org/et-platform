// Global
#include <string.h>

// STD
#include <algorithm>
#include <fstream>

// Local
#include "emu_defines.h"
#include "main_memory_region.h"

// Namespaces
using namespace std;

// Creator
main_memory_region::main_memory_region(uint64_t base, uint64_t size, testLog & l,
                                       func_ptr_get_thread & get_thread, int flags,
                                       bool allocate_data)
    : base_(base), size_(size), data_(allocate_data ? new char[size]() : nullptr),
      flags_(flags), log(l), get_thread(get_thread)
{ }

// Destructor: free allocated mem
main_memory_region::~main_memory_region()
{
    if (data_)
        delete[] data_;
}

// Write to memory region
void main_memory_region::write(uint64_t ad, int size, const void * data)
{
    if ((flags_ & MEM_REGION_WRITE_ALLOWED) == 0) {
        log<<LOG_ERR<<"writing to address 0x"<<std::hex<<ad<<std::dec<<" is not allowed"<<endm;
        return;
    }
    if (data_)
        std::copy_n(reinterpret_cast<const char*>(data), size, data_ + (ad - base_));
}

// Read from memory region
void main_memory_region::read(uint64_t ad, int size, void * data)
{
    if ((flags_ & MEM_REGION_READ_ALLOWED) == 0) {
        log<<LOG_ERR<<"reading from address 0x"<<std::hex<<ad<<std::dec<<" is not allowed"<<endm;
        return;
    }
    if (data_)
        std::copy_n(data_ + (ad - base_), size, reinterpret_cast<char*>(data));
}

// Dumps the region
void main_memory_region::dump()
{
    log << LOG_DEBUG << "\tBase: 0x" << std::hex << base_ << ", Size: 0x" << size_ << std::dec << endm;
}

// Dumps the region contents in a file
void main_memory_region::dump_file(std::ofstream * f)
{
    if(size_ % 4)
    {
        log << LOG_FTL << "Can't dump with sizes not multiple of dword" << endm;
        return;
    }

    // Dump all the dwords
    uint64_t address = base_;
    char str[128];
    uint32_t * data = (uint32_t *) data_;
    for(uint64_t dword = 0; dword < (size_ / 4); dword++)
    {
        if((address >> 39) == 0) continue; // Only cacheable
        sprintf(str, "%010llX %08X\n", (long long unsigned int) address, data[dword]);
        f->write(str, strlen(str));
        address += 4;
    }
}

