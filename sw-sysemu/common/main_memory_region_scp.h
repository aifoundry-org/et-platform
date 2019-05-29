/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#ifndef BEMU_MAIN_MEMORY_REGION_SCP_H
#define BEMU_MAIN_MEMORY_REGION_SCP_H

#include "esrs.h"
#include "main_memory.h"
#include "main_memory_region.h"

//namespace bemu {


struct main_memory_region_scp : public main_memory_region
{
    main_memory_region_scp(main_memory* parent, uint64_t base, uint64_t size,
                           const shire_cache_esrs_t* regs, bool alloc)
    : main_memory_region(base, size, alloc), mem(parent), sc_regs(regs)
    {}

    ~main_memory_region_scp() {}

    // read and write
    void write(uint64_t addr, size_t n, const void* source) override;
    void read(uint64_t addr, size_t n, void* result) override;

    size_t l2_scp_size() const {
        uint64_t esr_sc_cache_ctl = sc_regs->bank[0].sc_scp_cache_ctl;
        unsigned set_size = ((esr_sc_cache_ctl >> 32) & 0x1FFF);
        // total_size = set_size x 64 bytes/line x 4 lines/set x 4 subbanks x 4 banks
        return std::min(set_size * 4096, 1024u * 4096);
    }

    // for exposition only
    main_memory* const mem;
    const shire_cache_esrs_t* const sc_regs;
};


//} // namespace bemu

#endif // BEMU_MAIN_MEMORY_REGION_SCP_H
