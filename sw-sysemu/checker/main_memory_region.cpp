// Global
#include <string.h>

// Local
#include "main_memory_region.h"

using namespace std;

// Creator
main_memory_region::main_memory_region(uint64 base, uint64 size, testLog & l, func_ptr_get_thread & get_thread, int flags)
  : base_(base), size_(size), data_(new char[size]), log(l),
    flags_(flags),
    get_thread(get_thread)
{
    bzero(data_, size);
}
 
// Destructor: free allocated mem
main_memory_region::~main_memory_region()
{
    if(data_ != NULL )
        delete[] data_;
}

// operators to compare, used when searching the correct main_memory_region in a memory access
bool main_memory_region::operator==(uint64 ad)
{ 
    return ad >= base_ && ad < base_ + size_;
}

// operators to compare, used when searching the correct main_memory_region in a memory access
bool main_memory_region::operator!=(uint64 ad)
{
    return !(*this == ad);
}
    
// Write to memory region
void main_memory_region::write(uint64 ad, int size, const void * data)
{
  if ( ( flags_ & MEM_REGION_WRITE_ALLOWED ) == 0) {
    log<<LOG_ERR<<"writing to address "<<hex<<ad<<dec<<" is not allowed"<<endm;
    return;
  }
  
  if(data_ != NULL)
    memcpy(data_ + (ad - base_), data, size);
}

// Read from memory region
void main_memory_region::read(uint64 ad, int size, void * data)
{
  if ( ( flags_ & MEM_REGION_READ_ALLOWED ) == 0) {
    log<<LOG_ERR<<"reading from address "<<hex<<ad<<dec<<" is not allowed"<<endm;
    return;
  }
  
  if(data_ != NULL)
    memcpy(data, data_ + (ad - base_), size);
}

// Dumps the region
void main_memory_region::dump()
{
    log << LOG_DEBUG << "\tBase: 0x" << hex << base_ << ", Size: 0x" << size_ << dec << endm;
}

