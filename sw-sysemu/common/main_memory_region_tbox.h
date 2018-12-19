#ifndef _MAIN_MEMORY_REGION_TBOX_
#define _MAIN_MEMORY_REGION_TBOX_

#include "main_memory_region.h"

// Fake TBOX memory region to capture a single fake ESR that defines the
// Image Descriptor Table Pointer.

class main_memory_region_tbox : main_memory_region
{
public:
  // Constructors and destructors
  main_memory_region_tbox(uint64_t base, uint64_t size, testLog & l, func_ptr_get_thread& get_th);
  ~main_memory_region_tbox();

  // read and write
  void write(uint64_t ad, int size, const void* data);
  void read(uint64_t ad, int size, void* data);
};

#endif // _MAIN_MEMORY_REGION_TBOX_

