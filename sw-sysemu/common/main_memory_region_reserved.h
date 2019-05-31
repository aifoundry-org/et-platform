/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#ifndef BEMU_MAIN_MEMORY_REGION_RESERVED_H
#define BEMU_MAIN_MEMORY_REGION_RESERVED_H

#include "main_memory_region.h"
#include "traps.h"

//namespace bemu {


struct main_memory_region_reserved : public main_memory_region
{
    main_memory_region_reserved(uint64_t base, uint64_t size)
    : main_memory_region(base, size, false)
    {}

    ~main_memory_region_reserved() {}

    void write(uint64_t addr, size_t, const void*) override {
        throw trap_bus_error(addr);
    }

    void read(uint64_t addr, size_t, void*) override {
        throw trap_bus_error(addr);
    }

    void dump_file(std::ofstream*) override {}
};


//} // namespace bemu

#endif // BEMU_MAIN_MEMORY_REGION_RESERVED_H
