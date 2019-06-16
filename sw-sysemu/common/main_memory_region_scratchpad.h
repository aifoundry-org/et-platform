/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#ifndef BEMU_MAIN_MEMORY_REGION_SCRATCHPAD_H
#define BEMU_MAIN_MEMORY_REGION_SCRATCHPAD_H

#include "emu_defines.h"
#include "esrs.h"
#include "main_memory_region.h"

//namespace bemu {


struct main_memory_region_scratchpad : public main_memory_region
{
    main_memory_region_scratchpad(uint64_t base, uint64_t size)
    : main_memory_region(base, size, false)
    {}

    ~main_memory_region_scratchpad() {}

    // read and write
    void write(uint64_t addr, size_t n, const void* source) override {
        uint64_t addr2 = normalize(addr);
        unsigned shire = addr_to_shire(addr2);
        if (shire >= EMU_NUM_SHIRES)
            throw trap_bus_error(addr);
        unsigned bank = addr_to_bank(addr2);
        unsigned pos = addr_to_offset(addr2);
        if (((pos + n) >> 12) > num_scp_sets(shire, bank))
            throw trap_bus_error(addr);
        std::copy_n(reinterpret_cast<const char*>(source), n, &l2scp[shire][pos]);
    }

    void read(uint64_t addr, size_t n, void* result) override {
        uint64_t addr2 = normalize(addr);
        unsigned shire = addr_to_shire(addr2);
        if (shire >= EMU_NUM_SHIRES)
            throw trap_bus_error(addr);
        unsigned bank = addr_to_bank(addr2);
        unsigned pos = addr_to_offset(addr2);
        if (((pos + n) >> 12) > num_scp_sets(shire, bank))
            throw trap_bus_error(addr);
        std::copy_n(&l2scp[shire][pos], n, reinterpret_cast<char*>(result));
    }

    size_t num_scp_sets(unsigned shire, unsigned bank) const {
        uint64_t cfg = shire_cache_esrs[shire].bank[bank].sc_scp_cache_ctl;
        return std::min(((cfg >> 32) & 0x1FFF), uint64_t(L2_SCP_SIZE >> 12));
    }

    static uint64_t normalize(uint64_t addr) {
        if (~addr & 0x40000000)
            return addr;
        uint64_t offset = (((addr >> 11) & 0x1ffff) << 6) | (addr & 0x3f);
        uint64_t shire = (((addr >> 28) & 0x3) << 5) | ((addr >> 6) & 0x1f);
        return (addr & ~0x7fffffffull) | (shire << 23) | offset;
    }

    static unsigned addr_to_offset(uint64_t addr) {
        return addr & 0x7fffff;
    }

    static unsigned addr_to_bank(uint64_t addr) {
        return (addr >> 6) & 0x3;
    }

    static unsigned addr_to_shire(uint64_t addr) {
        unsigned shire = (addr >> 23) & 0x3f;
        if (shire == (IO_SHIRE_ID & 0x3f))
            return EMU_IO_SHIRE_SP;
        if (shire == 0x3f)
            return current_thread / EMU_THREADS_PER_SHIRE;
        return shire;
    }

    // for exposition only
    std::array<std::array<char,L2_SCP_SIZE>,EMU_NUM_SHIRES> l2scp;
};


//} // namespace bemu

#endif // BEMU_MAIN_MEMORY_REGION_SCRATCHPAD_H
