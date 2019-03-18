#ifndef _MAIN_MEMORY_REGION_SCP_LINEAR_
#define _MAIN_MEMORY_REGION_SCP_LINEAR_

#include "main_memory_region.h"

// Memory region used to implement the access to linear scratchpad region
class main_memory_region_scp_linear : main_memory_region
{
    public:
        // Constructors and destructors
        main_memory_region_scp_linear(uint64_t base, uint64_t size, testLog & l, func_ptr_get_thread& get_th);
        ~main_memory_region_scp_linear();
    
        // read and write
        void write(uint64_t ad, int size, const void* data) override;
        void read(uint64_t ad, int size, void* data) override;
    
    private:
        uint64_t ad_to_l2_scp_ad(uint64_t ad);
};

#endif // _MAIN_MEMORY_REGION_SCP_LINEAR_

