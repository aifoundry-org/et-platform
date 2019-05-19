// Global
#include <cassert>

// Local
#include "emu_defines.h"
#include "main_memory_region_scp.h"
#include "main_memory_region_scp_linear.h"

typedef std::shared_ptr<main_memory_region_scp> scp_region_pointer;

// Write to memory region
void main_memory_region_scp_linear::write(uint64_t ad, int size, const void * data)
{
    // Forwards write to correct memory
    uint64_t addr = ad_to_l2_scp_ad(ad);
    scp_region_pointer scp = std::static_pointer_cast<main_memory_region_scp>(mem_->find_region_containing(addr));
    if (!scp)
        throw trap_bus_error(ad);
    scp->write(addr, size, data);
}

// Read from memory region
void main_memory_region_scp_linear::read(uint64_t ad, int size, void * data)
{
    // Forwards write to correct memory
    uint64_t addr = ad_to_l2_scp_ad(ad);
    scp_region_pointer scp = std::static_pointer_cast<main_memory_region_scp>(mem_->find_region_containing(addr));
    if (!scp)
        throw trap_bus_error(ad);
    scp->read(addr, size, data);
}

// Converts from linear addressing to offset addressing
uint64_t main_memory_region_scp_linear::ad_to_l2_scp_ad(uint64_t ad)
{
    // Gets the L2 Scp offset and shire id
    uint64_t l2_scp_offset = (((ad >> 11) & 0x1FFFF) << 6) | (ad & 0x3F);
    uint64_t shire_id = (((ad >> 28) & 0x3) << 5) | ((ad >> 6) & 0x1F);

    // Generates new address on regular l2 scp region
    uint64_t new_ad = L2_SCP_BASE | (shire_id << 23) | l2_scp_offset;

    //printf("Original offset is %llx, shire_id %llx\n", (long long unsigned int) l2_scp_offset, (long long unsigned int) shire_id);
    //printf("L2 SCP from %010llx to %010llx\n", (long long unsigned int) ad, (long long unsigned int) new_ad);
    return new_ad;
}

