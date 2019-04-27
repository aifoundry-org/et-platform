#ifndef _MAIN_MEMORY_REGION_SCP_
#define _MAIN_MEMORY_REGION_SCP_

#include "main_memory.h"
#include "main_memory_region.h"
#include "main_memory_region_esr.h"

// Memory region used to implement the access to the scratchpad region

class main_memory_region_scp : public main_memory_region
{
    public:
        // Constructors and destructors
        main_memory_region_scp(main_memory* parent, uint64_t base, uint64_t size,
                               testLog& l, func_ptr_get_thread& get_thr,
                               const main_memory_region_esr* sc_regs,
                               bool allocate_data = true);
        ~main_memory_region_scp();

        // read and write
        void write(uint64_t ad, int size, const void* data) override;
        void read(uint64_t ad, int size, void* data) override;

    private:
        main_memory* mem_;
        const main_memory_region_esr* sc_regs_;

        size_t l2_scp_size() const;
};

#endif // _MAIN_MEMORY_REGION_SCP_

