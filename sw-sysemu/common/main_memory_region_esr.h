/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#ifndef BEMU_MAIN_MEMORY_REGION_ESR_H
#define BEMU_MAIN_MEMORY_REGION_ESR_H

#include "main_memory.h"
#include "main_memory_region.h"

//namespace bemu {


struct main_memory_region_esr : public main_memory_region
{
    main_memory_region_esr(main_memory* parent, uint64_t base, uint64_t size)
    : main_memory_region(base, size, false), mem(parent)
    {}

    ~main_memory_region_esr() {}

    void write(uint64_t addr, size_t n, const void* source) override;
    void read(uint64_t addr, size_t n, void* result) override;
    void dump_file(std::ofstream*) override {}

    // for exposition only
    main_memory* const mem;
};


//} // namespace bemu

#endif // BEMU_MAIN_MEMORY_REGION_ESR_H
