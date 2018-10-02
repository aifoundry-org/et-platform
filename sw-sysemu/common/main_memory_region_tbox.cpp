// Global
#include <string.h>

// Local
#include "main_memory_region_tbox.h"
#include "txs.h"

using namespace std;

// Creator
main_memory_region_tbox::main_memory_region_tbox(uint64_t base, uint64_t size, testLog & l, func_ptr_get_thread& get_thr)
    : main_memory_region(base, size, l, get_thr)
{
}
 
// Destructor: free allocated mem
main_memory_region_tbox::~main_memory_region_tbox()
{
}

// Write to memory region
void main_memory_region_tbox::write(uint64_t ad, int size, const void * data)
{
  log << LOG_DEBUG << "writing to tbox @=" <<hex<<ad<<dec<< endm;

  // If address is ImageInfoTable then call init_txs_ptr to init ImageInfoTable pointer. In tri_raster.cpp there can be fiund: #define TBOX_IMG_TABLE_BASE_PTR (TBOX_BASE + 0x40)
  if((ad-base_) == 0x40)
  {
      uint64_t val = * ((uint64_t *) data);
      init_txs(val);
  }
}


// Read from memory region
void main_memory_region_tbox::read(uint64_t ad, int size, void * data)
{
  log << LOG_DEBUG << "reading from tbox @=" <<hex<<ad<<dec<< endm;

  // the RTL has not yet computed the output, fill with zeros
  // the actual output will be set in checker.cpp, taking the value from the RTL
  bzero(data, size);
}
