// Local
#include "emu_defines.h"
#include "main_memory_region_scp.h"

extern uint32_t current_thread;

#define L2_SCP_SHIRE_SHIFT  23
#define L2_SCP_SHIRE_MASK   (0x7FULL << L2_SCP_SHIRE_SHIFT)

// Creator
main_memory_region_scp::main_memory_region_scp(main_memory* parent, uint64_t base, uint64_t size,
                                               testLog & l, func_ptr_get_thread & get_thr,
                                               const main_memory_region_esr* sc_regs,
                                               bool allocate_data)
    : main_memory_region(base, size, l, get_thr, MEM_REGION_RW, allocate_data), mem_(parent), sc_regs_(sc_regs)
{ }

// Destructor: free allocated mem
main_memory_region_scp::~main_memory_region_scp()
{
    if (data_)
        delete[] data_;
}

// Write to L2SCP
void main_memory_region_scp::write(uint64_t ad, int size, const void * data)
{
    if ((ad & L2_SCP_SHIRE_MASK) == L2_SCP_SHIRE_MASK)
    {
        // Local shire, redirect to appropriate scratchpad region
        uint64_t addr = (ad & ~L2_SCP_SHIRE_MASK) | ((current_thread / EMU_THREADS_PER_SHIRE) << L2_SCP_SHIRE_SHIFT);
        main_memory_region_scp* scp = static_cast<main_memory_region_scp*>(mem_->find_region_containing(addr));
        assert(scp);
        scp->write(addr, size, data);
    }
    if (ad + size - base_ > l2_scp_size())
        throw trap_bus_error(ad);
    if (data_)
        std::copy_n(reinterpret_cast<const char*>(data), size, data_ + (ad - base_));
}

// Read from L2SCP
void main_memory_region_scp::read(uint64_t ad, int size, void * data)
{
    if ((ad & L2_SCP_SHIRE_MASK) == L2_SCP_SHIRE_MASK)
    {
        // Local shire, redirect to appropriate scratchpad region
        uint64_t addr = (ad & ~L2_SCP_SHIRE_MASK) | ((current_thread / EMU_THREADS_PER_SHIRE) << L2_SCP_SHIRE_SHIFT);
        main_memory_region_scp* scp = static_cast<main_memory_region_scp*>(mem_->find_region_containing(addr));
        assert(scp);
        scp->read(addr, size, data);
    }
    if (ad + size - base_ > l2_scp_size())
        throw trap_bus_error(ad);
    if (data_)
        std::copy_n(data_ + (ad - base_), size, reinterpret_cast<char*>(data));
}

size_t main_memory_region_scp::l2_scp_size() const
{
    uint64_t esr_sc_cache_ctl = sc_regs_->read(ESR_SC_SCP_CACHE_CTL - ESR_CACHE_M0);
    unsigned set_size = 1 + ((esr_sc_cache_ctl >> 32) & 0x1FFF);
    // total_size = set_size x 64 bytes/line x 4 lines/set x 4 subbanks x 4 banks
    return std::min(set_size * 4096, 1024u * 4096);
}
