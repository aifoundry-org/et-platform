/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#ifndef BEMU_MAIN_MEMORY_REGION_ESR_H
#define BEMU_MAIN_MEMORY_REGION_ESR_H

#include <cassert>
#include "esrs.h"
#include "main_memory.h"
#include "main_memory_region.h"

//namespace bemu {


struct main_memory_region_esr : public main_memory_region
{
    main_memory_region_esr()
    : main_memory_region(ESR_REGION_BASE, ESR_REGION_SIZE, false)
    {}

    ~main_memory_region_esr() {}

    void write(uint64_t addr, size_t n, const void* source) override {
        assert(n == 8);
        esr_write(addr, *reinterpret_cast<const uint64_t*>(source));
    }

    void read(uint64_t addr, size_t n, void* result) override {
        assert(n == 8);
        *reinterpret_cast<uint64_t*>(result) = esr_read(addr);
    }

    void dump_file(std::ofstream*) override {}
};


//} // namespace bemu

#endif // BEMU_MAIN_MEMORY_REGION_ESR_H
