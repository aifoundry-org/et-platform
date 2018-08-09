#ifndef _MAIN_MEMORY_REGION_TBOX_
#define _MAIN_MEMORY_REGION_TBOX_

// Local
#include "testLog.h"
#include "main_memory_region.h"

// This class is a memory region that behaves like a tbox:
//  - Gets the writes to know what job to do
//  - Once all the inputs are written, calls the tbox emulator
//  - Gets the reads and returns the output data

// memory map when writing:
// 00h: headerL (64b)
// 08h: headerH (64b)
// 10h: u  (128b)
// 20h: v  (128b)
// 30h: t   (128b)
// 40h: image information table base pointer

// Temporary: Add padding zeroes to the 256b of results to make a 512b object and then read as 4 chunks of 128b from the 
// following offsets. The padding adds 16 zeroes in between each of the 16b values.
// 00h: rec0    // red
// 10h: rec1    // green
// 20h: rec2    // blue
// 30h: rec3    // alpha
   
#define TBOX_MEM_REGION_SZ 128

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

