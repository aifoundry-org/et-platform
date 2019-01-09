// Global
#include <string.h>
#include <cstdio>

// Local
#include "main_memory_region_esr.h"
#include "emu_gio.h"
#include "emu.h"

extern uint32_t current_pc;
extern uint32_t current_thread;

#ifdef SYS_EMU
extern int32_t minion_only_log;
extern void fcc_to_threads(unsigned shire_id, unsigned thread_dest, uint64_t thread_mask, bool log_en, int log_min);
#endif

extern void write_msg_port_data(uint32_t thread, uint32_t id, uint32_t *data, uint8_t oob);

using namespace std;

// Creator
main_memory_region_esr::main_memory_region_esr(uint64_t base, uint64_t size, testLog & l, func_ptr_get_thread& get_thr)
    : main_memory_region(base, size, l, get_thr)
{
}
 
// Destructor: free allocated mem
main_memory_region_esr::~main_memory_region_esr()
{
}

// Write to memory region
void main_memory_region_esr::write(uint64_t ad, int size, const void * data)
{
    esr_info_t esr_info;
    decode_ESR_address(ad, &esr_info);

    //log << LOG_DEBUG << "Writing to ESR Region @=" <<hex<<ad<<dec<< endm;

    LOG(DEBUG, "Writing to ESR Region with address %016" PRIx64, ad);

    if (!esr_info.valid)
        LOG(DEBUG, "Invalid ESR");
    else
    {
        switch(esr_info.region)
        {
            case ESR_Region_HART:

                switch(esr_info.address)
                {
                    case ESR_HART_PORT0_OFFSET :
                        write_msg_port_data(esr_info.hart, 0, (uint32_t *) data, 0);
                        break;               
                    case ESR_HART_PORT1_OFFSET :
                        write_msg_port_data(esr_info.hart, 1, (uint32_t *) data, 0);
                        break;               
                    case ESR_HART_PORT2_OFFSET :
                        write_msg_port_data(esr_info.hart, 2, (uint32_t *) data, 0);
                        break;               
                    case ESR_HART_PORT3_OFFSET :
                        write_msg_port_data(esr_info.hart, 3, (uint32_t *) data, 0);
                        break;               
                }

                break;

            case ESR_Region_Neighborhood :
                
                if (esr_info.neighborhood == ESR_NEIGH_BROADCAST)
                {
                    for (int neigh = 0; neigh < EMU_NEIGH_PER_SHIRE; neigh++)
                    {
                        uint64_t neigh_addr = (ad & ~ESR_NEIGH_MASK) + neigh * ESR_NEIGH_OFFSET;
                        write(neigh_addr, size, data);
                    }
                }

                break;

            case ESR_Region_Shire:
            {
                LOG(DEBUG, "Write to ESR Region Shire at ESR address %08" PRIx64, esr_info.address);
                switch(esr_info.address)
                {
                    case ESR_SHIRE_FLB_OFFSET  : break;
                    case ESR_SHIRE_FCC0_OFFSET :
                    case ESR_SHIRE_FCC1_OFFSET : 		      
                    {
                        LOG(DEBUG, "Write to FCC0_OFFSET value %016" PRIx64, *((uint64_t *) data));
#ifdef SYS_EMU
                        bool log_en = (emu_log().getLogLevel() == LOG_DEBUG);
                        fcc_to_threads(esr_info.shire, 0, *((uint64_t *) data), log_en, minion_only_log);
#else
			fcc_inc(0, esr_info.shire, *((uint64_t*)data), ESR_SHIRE_FCC1_OFFSET==esr_info.address);
#endif
                        break;
                    }

                    case ESR_SHIRE_FCC2_OFFSET :
                    case ESR_SHIRE_FCC3_OFFSET :		      
                    {
                        LOG(DEBUG, "Write to FCC2_OFFSET value %016" PRIx64, *((uint64_t *) data));
#ifdef SYS_EMU
                        bool log_en = (emu_log().getLogLevel() == LOG_DEBUG);
                        fcc_to_threads(esr_info.shire, 1, *((uint64_t *) data), log_en, minion_only_log);
#else

			fcc_inc(1, esr_info.shire, *((uint64_t*)data), ESR_SHIRE_FCC3_OFFSET==esr_info.address);
#endif
                        break;
                    }

                    default : break;
                }

                break;   
            }
	  break;
            default : break;
        }
    }

    // Fast local barriers implementation is using the ESR space storage!! 
    if(data_ != NULL)
        memcpy(data_ + (ad - base_), data, size);
}


// Read from memory region
void main_memory_region_esr::read(uint64_t ad, int size, void * data)
{
    esr_info_t esr_info;
    decode_ESR_address(ad, &esr_info);

    log << LOG_DEBUG << "Reading from Shire ESR Region @=" <<hex<<ad<<dec<< endm;

    // Fast local barriers implementation is using the ESR space storage!! 
    if(data_ != NULL)
        memcpy(data, data_ + (ad - base_), size);
}

void main_memory_region_esr::decode_ESR_address(uint64_t address, esr_info_t *info)
{
    info->valid      = ((address & ESR_REGION_MASK) == ESR_REGION);
    info->protection = esr_protection_t((address & ESR_REGION_PROT_MASK) >> ESR_REGION_PROT_SHIFT);

    if ((address & ESR_REGION_SHIRE_MASK) == ESR_REGION_LOCAL_SHIRE)
        info->shire = current_thread / (EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION);
    else
        info->shire = ((address & ESR_REGION_SHIRE_MASK) >> ESR_REGION_SHIRE_SHIFT);

    switch((address & ESR_SREGION_MASK) >> ESR_SREGION_SHIFT)
    {
        case ESR_Region_HART :
        {
            info->region  = ESR_Region_HART;
            info->hart    = (address & ESR_HART_MASK) >> ESR_HART_SHIFT;
            info->address = (address & ESR_HART_ESR_MASK);
            break;
        }
        case ESR_Region_Neighborhood :
        {
            info->region       = ESR_Region_Neighborhood;
            info->neighborhood = (address & ESR_NEIGH_MASK) >> ESR_NEIGH_SHIFT;
            info->address      = (address & ESR_NEIGH_ESR_MASK);
            break;
        }
        case ESR_Region_Extended :
        {
            switch ((address & ESR_SREGION_EXT_MASK) >> ESR_SREGION_EXT_SHIFT)
            {
                case ESR_Region_Shire_Cache :
                {
                    info->region  = ESR_Region_Shire_Cache;
                    info->bank    = (address & ESR_BANK_MASK) >> ESR_BANK_SHIFT;
                    info->address = (address & ESR_SC_ESR_MASK);
                    break;
                };
                case ESR_Region_Shire_RBOX :
                {
                    info->region  = ESR_Region_Shire_RBOX;
                    info->address = (address & ESR_SHIRE_ESR_MASK);
                    break;
                }
                case ESR_Region_Shire :
                {
                    info->region  = ESR_Region_Shire;
                    info->address = (address & ESR_SHIRE_ESR_MASK);
                }
            }
            break;
        }
    }
}


