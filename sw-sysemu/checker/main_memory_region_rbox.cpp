// Global
#include <string.h>
#include <dlfcn.h>

// Local
#include "main_memory_region_rbox.h"
#include "txs.h"

using namespace std;

// Creator
main_memory_region_rbox::main_memory_region_rbox(uint64_t base, uint64_t size, testLog & l, func_ptr_get_thread& get_thr)
    : main_memory_region(base, size, l, get_thr)
{
  bzero(credits_, sizeof(credits_));
}
 
// Destructor: free allocated mem
main_memory_region_rbox::~main_memory_region_rbox()
{
}

// Write to memory region
void main_memory_region_rbox::write(uint64_t ad, int size, const void * data)
{
  log << LOG_DEBUG << "writing to rbox @=" <<hex<<ad<<dec<< endm;
  uint16_t thread;
  memcpy( &thread, data, sizeof(uint16_t));
  incCredit(thread);
}


// Read from memory region
void main_memory_region_rbox::read(uint64_t ad, int size, void * data)
{
  log << LOG_ERR << "not expecting to read from rbox memory map"<<endm;
}

void main_memory_region_rbox::decCredit(uint16_t thread){
  credits_[thread]--;
}
void main_memory_region_rbox::incCredit(uint16_t thread){
  credits_[thread]++;
}

uint16_t main_memory_region_rbox::getCredit(uint16_t thread){
  return credits_[thread];
}
