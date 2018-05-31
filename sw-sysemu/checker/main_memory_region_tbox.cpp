// Global
#include <string.h>
#include <dlfcn.h>

// Local
#include "main_memory_region_tbox.h"
#include "txs.h"

using namespace std;

// Creator
main_memory_region_tbox::main_memory_region_tbox(uint64 base, uint64 size, testLog & l, func_ptr_get_thread& get_thr)
    : main_memory_region(base, size, l, get_thr)
{
  // This is an object that resolves where the emulation functions are
  std::string repo = getenv("BEMU");
  repo += "/checker/build/emu.so";
  function_pointer_cache * func_cache = new function_pointer_cache(repo.c_str());

  init_txs_ptr = (func_init_txs_t) func_cache->get_function_ptr("init_txs");
  if(init_txs_ptr == NULL) 
    log << LOG_FTL <<"cannot find init_txs  in emu shared lib:" << dlerror() << endm;
}
 
// Destructor: free allocated mem
main_memory_region_tbox::~main_memory_region_tbox()
{
}

// Write to memory region
void main_memory_region_tbox::write(uint64 ad, int size, const void * data)
{
  log << LOG_DEBUG << "writing to tbox @=" <<hex<<ad<<dec<< endm;

  // If address is ImageInfoTable then call init_txs_ptr to init ImageInfoTable pointer. In tri_raster.cpp there can be fiund: #define TBOX_IMG_TABLE_BASE_PTR (TBOX_BASE + 0x40)
  if((ad-base_) == 0x40)
  {
      uint64 val = * ((uint64 *) data);
      init_txs_ptr(val);
  }
}


// Read from memory region
void main_memory_region_tbox::read(uint64 ad, int size, void * data)
{
  fdata result;
  log << LOG_DEBUG << "reading from tbox @=" <<hex<<ad<<dec<< endm;

  // the RTL has not yet computed the output, fill with zeros
  // the actual output will be set in checker.cpp, taking the value from the RTL
  bzero(data, size);
}
