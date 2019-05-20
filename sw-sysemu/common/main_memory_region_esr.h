#ifndef _MAIN_MEMORY_REGION_ESR_
#define _MAIN_MEMORY_REGION_ESR_

#include <cassert>
#include "main_memory.h"
#include "main_memory_region.h"

// Memory region used to implement the access to all ESRs defined in the memory map

class main_memory_region_esr : public main_memory_region
{
public:
    // Constructors and destructors
    main_memory_region_esr(main_memory* parent, uint64_t base, uint64_t size,
                           testLog & l, func_ptr_get_thread& get_th)
    : main_memory_region(base, size, l, get_th, MEM_REGION_RW, false),
      mem_(parent)
    {}

    ~main_memory_region_esr() {}

    // read and write
    void write(uint64_t ad, int size, const void* data) override;
    void read(uint64_t ad, int size, void* data) override;

    uint64_t read(uint64_t offset) const {
        assert(data_);
        return *reinterpret_cast<const uint64_t*>(data_ + offset);
    }

protected:
    main_memory* mem_;
};

#endif // _MAIN_MEMORY_REGION_ESR_

