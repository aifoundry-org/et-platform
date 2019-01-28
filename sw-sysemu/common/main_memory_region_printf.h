#ifndef _MAIN_MEM_REGION_PRINTF_H_
#define _MAIN_MEM_REGION_PRINTF_H_

// Local
#include "main_memory_region.h"

// Global
#include <assert.h>

// Defines
#define PRINTF_THREADS 128

class main_memory_region_printf : main_memory_region
{
public:
  // Constructors and destructors
  main_memory_region_printf(uint64_t base, func_ptr_get_thread& get_thr)
    : main_memory_region(base, PRINTF_THREADS/2*64, log, get_thr, MEM_REGION_WO),
      log("rtlPrintf", LOG_DEBUG) { }

  ~main_memory_region_printf() {
    for(int i = 0; i< PRINTF_THREADS; i++){
      if (msg_[i].length()>0) log<<LOG_INFO<<(i>>1)<<"."<<(i&1)<<": (truncated msg) "<<msg_[i]<<endm;
    }
  }

  // read and write
  void write(uint64_t ad, int size, const void* data) {
    uint32_t thread = get_thread();
    assert(thread < PRINTF_THREADS);
    char c = * (char*) data;
    if ( c!=0) msg_[thread]+= c;
    else if (msg_[thread].length() > 0) {
      log<<LOG_INFO<<(thread>>1)<<"."<<(thread&1)<<": "<<msg_[thread]<<endm;
      msg_[thread]="";
    }
  }
private:
  std::string msg_[PRINTF_THREADS];
  testLog log;
};

#endif // _MAIN_MEMORY_REGION_PRINTF_

