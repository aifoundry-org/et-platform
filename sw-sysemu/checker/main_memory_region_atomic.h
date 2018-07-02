#ifndef _MAIN_MEMORY_REGION_ATOMIC_
#define _MAIN_MEMORY_REGION_ATOMIC_

// Local
#include "checker_defines.h"
#include "testLog.h"
#include "main_memory_region.h"
#include "emu.h"

// This class behaves as an atomic region, that is private per L2
#define NUMBER_L2 64

class main_memory_region_atomic : main_memory_region
{
public:
  // Constructors and destructors
  main_memory_region_atomic(uint64_t base, uint64_t size, testLog & l, func_ptr_get_thread& get_th);
  ~main_memory_region_atomic();

  // read and write
  void write(uint64_t ad, int size, const void* data);
  void read(uint64_t ad, int size, void* data);

private:
  char * l2_priv_data[NUMBER_L2];
};

#endif // _MAIN_MEMORY_REGION_ATOMIC_

