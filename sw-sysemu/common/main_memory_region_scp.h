#ifndef _MAIN_MEMORY_REGION_SCP_
#define _MAIN_MEMORY_REGION_SCP_

#include "esrs.h"
#include "main_memory.h"
#include "main_memory_region.h"

// Memory region used to implement the access to the scratchpad region

class main_memory_region_scp : public main_memory_region
{
    public:
        // Constructors and destructors
        main_memory_region_scp(main_memory* parent, uint64_t base, uint64_t size,
                               testLog& l, func_ptr_get_thread& get_thr,
                               const shire_cache_esrs_t* sc_regs,
                               bool allocate_data = true);
        ~main_memory_region_scp();

        // read and write
        void write(uint64_t ad, int size, const void* data) override;
        void read(uint64_t ad, int size, void* data) override;

    private:
        main_memory* mem_;
        const shire_cache_esrs_t* sc_regs_;

        size_t l2_scp_size() const {
            uint64_t esr_sc_cache_ctl = sc_regs_->bank[0].sc_scp_cache_ctl;
            unsigned set_size = ((esr_sc_cache_ctl >> 32) & 0x1FFF);
            // total_size = set_size x 64 bytes/line x 4 lines/set x 4 subbanks x 4 banks
            return std::min(set_size * 4096, 1024u * 4096);
        }
};

#endif // _MAIN_MEMORY_REGION_SCP_

