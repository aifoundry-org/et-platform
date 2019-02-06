#ifndef _MAIN_MEMORY_REGION_RBOX_
#define _MAIN_MEMORY_REGION_RBOX_

#include "main_memory_region.h"
#include "emu_defines.h"

class main_memory_region_rbox : main_memory_region
{
public:
    // Constructors and destructors
    main_memory_region_rbox(uint64_t base, uint64_t size, testLog & l, func_ptr_get_thread& get_th);
    ~main_memory_region_rbox();

    // read and write
    void write(uint64_t ad, int size, const void* data);
    void read(uint64_t ad, int size, void* data);

private:

    void decode_esr(uint64_t ad, uint32_t &rbox_id, uint32_t &reg_id);
};

#endif // _MAIN_MEMORY_REGION_RBOX_

