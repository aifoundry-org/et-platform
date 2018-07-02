#ifndef _MAIN_MEMORY_REGION_RBOX_
#define _MAIN_MEMORY_REGION_RBOX_

// Local
#include "checker_defines.h"
#include "testLog.h"
#include "main_memory_region.h"
#include "emu.h"
#include "function_pointer_cache.h"
   

class main_memory_region_rbox : main_memory_region
{
public:
  // Constructors and destructors
  main_memory_region_rbox(uint64_t base, uint64_t size, testLog & l, func_ptr_get_thread& get_th);
  ~main_memory_region_rbox();

  // read and write
  void write(uint64_t ad, int size, const void* data);
  void read(uint64_t ad, int size, void* data);

  void decCredit(uint16_t thread);
  void incCredit(uint16_t thread);
  uint16_t getCredit(uint16_t thread);
private:
  uint16_t credits_[EMU_NUM_THREADS];
};

#endif // _MAIN_MEMORY_REGION_RBOX_

