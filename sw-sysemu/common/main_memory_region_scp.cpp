#include <cassert>
#include "emu_defines.h"
#include "main_memory_region_scp.h"

extern uint32_t current_thread;

#define L2_SCP_SHIRE_SHIFT  23
#define L2_SCP_SHIRE_MASK   (0x7FULL << L2_SCP_SHIRE_SHIFT)

typedef std::shared_ptr<main_memory_region_scp> scp_region_pointer;

// Write to L2SCP
void main_memory_region_scp::write(uint64_t ad, int size, const void * data)
{
    if ((ad & L2_SCP_SHIRE_MASK) == L2_SCP_SHIRE_MASK)
    {
        // Local shire, redirect to appropriate scratchpad region
        uint64_t addr = (ad & ~L2_SCP_SHIRE_MASK) | ((current_thread / EMU_THREADS_PER_SHIRE) << L2_SCP_SHIRE_SHIFT);
        scp_region_pointer scp = std::static_pointer_cast<main_memory_region_scp>(mem_->find_region_containing(addr));
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
        scp_region_pointer scp = std::static_pointer_cast<main_memory_region_scp>(mem_->find_region_containing(addr));
        assert(scp);
        scp->read(addr, size, data);
    }
    if (ad + size - base_ > l2_scp_size())
        throw trap_bus_error(ad);
    if (data_)
        std::copy_n(data_ + (ad - base_), size, reinterpret_cast<char*>(data));
}
