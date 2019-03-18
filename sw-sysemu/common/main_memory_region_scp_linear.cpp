// Global
#include <cassert>

// Local
#include "main_memory_region_scp_linear.h"
#include "emu_defines.h"
#include "emu_memop.h"

// Creator
main_memory_region_scp_linear::main_memory_region_scp_linear(uint64_t base, uint64_t size, testLog & l, func_ptr_get_thread& get_thr)
    : main_memory_region(base, size, l, get_thr, MEM_REGION_RW, false)
{
}

// Destructor: free allocated mem
main_memory_region_scp_linear::~main_memory_region_scp_linear()
{
}

// Write to memory region
void main_memory_region_scp_linear::write(uint64_t ad, int size, const void * data)
{
    assert((size == 8) || (size == 4));

    // Forwards write to correct memory
    uint64_t new_ad = ad_to_l2_scp_ad(ad);
    if(size == 4) pmemwrite32(new_ad, * ((uint32_t *) data));
    else          pmemwrite64(new_ad, * ((uint64_t *) data));
}

// Read from memory region
void main_memory_region_scp_linear::read(uint64_t ad, int size, void * data)
{
    assert((size == 8) || (size == 4));

    // Forwards write to correct memory
    uint64_t new_ad = ad_to_l2_scp_ad(ad);
    if(size == 4) * ((uint32_t *) data) = pmemread32(new_ad);
    else          * ((uint64_t *) data) = pmemread64(new_ad);
}

// Converts from linear addressing to offset addressing
uint64_t main_memory_region_scp_linear::ad_to_l2_scp_ad(uint64_t ad)
{
    // Gets the L2 Scp offset and shire id
    uint64_t l2_scp_offset = (((ad >> 11) & 0x1FFFF) << 6)
                           | (ad & 0x3F);
    uint64_t shire_id = (((ad >> 28) & 0x3) << 5)
                      | ((ad >> 6) & 0x1F);

    // Generates new address on regular l2 scp region
    uint64_t new_ad = L2_SCP_BASE
                    | (shire_id << 23)
                    | l2_scp_offset;

    //printf("Original offset is %llx, shire_id %llx\n", (long long unsigned int) l2_scp_offset, (long long unsigned int) shire_id);
    //printf("L2 SCP from %010llx to %010llx\n", (long long unsigned int) ad, (long long unsigned int) new_ad);
    return new_ad;
}

