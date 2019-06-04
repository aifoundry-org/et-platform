/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#ifndef BEMU_MAIN_MEMORY_REGION_SCP_LINEAR_H
#define BEMU_MAIN_MEMORY_REGION_SCP_LINEAR_H

#include "emu_defines.h"
#include "main_memory.h"
#include "main_memory_region.h"
#include "traps.h"

//namespace bemu {


struct main_memory_region_scp_linear : public main_memory_region
{
    main_memory_region_scp_linear(main_memory* m, uint64_t base, uint64_t size)
    : main_memory_region(base, size, false), mem(m)
    {}

    ~main_memory_region_scp_linear() {}

    // read and write
    void write(uint64_t addr, size_t n, const void* source) override {
        addr = l2_scp_addr(addr);
        auto scp = mem->find_region_containing(addr);
        if (!scp)
            throw trap_bus_error(addr);
        scp->write(addr, n, source);
    }

    void read(uint64_t addr, size_t n, void* result) override {
        uint64_t addr2 = l2_scp_addr(addr);
        auto scp = mem->find_region_containing(addr2);
        if (!scp)
            throw trap_bus_error(addr);
        scp->read(addr2, n, result);
    }

    uint64_t l2_scp_addr(uint64_t addr) const {
        uint64_t offset = (((addr >> 11) & 0x1FFFF) << 6) | (addr & 0x3F);
        uint64_t shire = (((addr >> 28) & 0x3) << 5) | ((addr >> 6) & 0x1F);
        return L2_SCP_BASE | (shire << 23) | offset;
    }

    // for exposition only
    main_memory* const mem;
};


//} // namespace bemu

#endif // BEMU_MAIN_MEMORY_REGION_SCP_LINEAR_H
