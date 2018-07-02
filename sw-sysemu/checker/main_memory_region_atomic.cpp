// Global
#include <string.h>
#include <dlfcn.h>

// Local
#include "main_memory_region_atomic.h"
#include "txs.h"

using namespace std;

// Creator
main_memory_region_atomic::main_memory_region_atomic(uint64_t base, uint64_t size, testLog & l, func_ptr_get_thread& get_thr)
    : main_memory_region(base, size, l, get_thr)
{
  for(int i = 0; i < NUMBER_L2; i++)
  {
    l2_priv_data[i] = new char[size];
    bzero(l2_priv_data[i], size);
  }
}
 
// Destructor: free allocated mem
main_memory_region_atomic::~main_memory_region_atomic()
{
  for(int i = 0; i < NUMBER_L2; i++)
  {
    if(l2_priv_data[i] != NULL )
      delete[] l2_priv_data[i];
  }
}

// Write to memory region
void main_memory_region_atomic::write(uint64_t ad, int size, const void * data)
{
  uint32_t thread_id = (get_thread());
  uint32_t shire = thread_id / 128;
  if(shire >= NUMBER_L2)
  {
    log<<LOG_ERR<<"Not supporting more than 64 shires"<<endm;
    return;
  }
  if ( ( flags_ & MEM_REGION_WRITE_ALLOWED ) == 0) {
    log<<LOG_ERR<<"writing to address "<<hex<<ad<<dec<<" is not allowed"<<endm;
    return;
  }
  
  if(data_ != NULL)
    memcpy(l2_priv_data[shire] + (ad - base_), data, size);
}


// Read from memory region
void main_memory_region_atomic::read(uint64_t ad, int size, void * data)
{
  uint32_t thread_id = (get_thread());
  uint32_t shire = thread_id / 128;
  if(shire >= NUMBER_L2)
  {
    log<<LOG_ERR<<"Not supporting more than 64 shires"<<endm;
    return;
  }
  if ( ( flags_ & MEM_REGION_READ_ALLOWED ) == 0) {
    log<<LOG_ERR<<"reading from address "<<hex<<ad<<dec<<" is not allowed"<<endm;
    return;
  }
  
  if(l2_priv_data[shire] != NULL)
    memcpy(data, l2_priv_data[shire] + (ad - base_), size);

}
