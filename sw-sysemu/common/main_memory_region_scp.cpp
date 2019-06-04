#include <cassert>
#include "emu_defines.h"
#include "main_memory_region_scp.h"

extern uint32_t current_thread;

#define L2_SCP_SHIRE_SHIFT  23
#define L2_SCP_SHIRE_MASK   (0x7FULL << L2_SCP_SHIRE_SHIFT)

// Write to L2SCP
void main_memory_region_scp::write(uint64_t addr, size_t n, const void* source)
{
    if ((addr & L2_SCP_SHIRE_MASK) == L2_SCP_SHIRE_MASK)
    {
        // Local shire, redirect to appropriate scratchpad region
        uint64_t current_shire = current_thread / EMU_THREADS_PER_SHIRE;
        uint64_t current_shireid = (current_shire == EMU_IO_SHIRE_SP) ? IO_SHIRE_ID : current_shire;
        uint64_t addr2 = (addr & ~L2_SCP_SHIRE_MASK) | ((current_shireid << L2_SCP_SHIRE_SHIFT) & L2_SCP_SHIRE_MASK);
        auto scp = mem->find_region_containing(addr2);
        if (!scp)
            throw trap_bus_error(addr);
        scp->write(addr2, n, source);
        return;
    }
    if (addr + n - base > l2_scp_size())
        throw trap_bus_error(addr);
    if (buf)
        std::copy_n(reinterpret_cast<const char*>(source), n, buf + (addr-base));
}

// Read from L2SCP
void main_memory_region_scp::read(uint64_t addr, size_t n, void* result)
{
    if ((addr & L2_SCP_SHIRE_MASK) == L2_SCP_SHIRE_MASK)
    {
        // Local shire, redirect to appropriate scratchpad region
        uint64_t current_shire = current_thread / EMU_THREADS_PER_SHIRE;
        uint64_t current_shireid = (current_shire == EMU_IO_SHIRE_SP) ? IO_SHIRE_ID : current_shire;
        uint64_t addr2 = (addr & ~L2_SCP_SHIRE_MASK) | ((current_shireid << L2_SCP_SHIRE_SHIFT) & L2_SCP_SHIRE_MASK);
        auto scp = mem->find_region_containing(addr2);
        if (!scp)
            throw trap_bus_error(addr);
        scp->read(addr2, n, result);
    }
    if (addr + n - base > l2_scp_size())
        throw trap_bus_error(addr);
    if (buf)
        std::copy_n(buf + (addr - base), n, reinterpret_cast<char*>(result));
}
