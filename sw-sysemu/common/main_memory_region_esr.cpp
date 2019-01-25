// Global
#include <string.h>
#include <cstdio>

// Local
#include "main_memory_region_esr.h"
#include "emu_gio.h"
#include "emu_memop.h"
#include "emu.h"
#include "txs.h"

extern uint32_t current_pc;
extern uint32_t current_thread;

#ifdef SYS_EMU
extern void fcc_to_threads(unsigned shire_id, unsigned thread_dest, uint64_t thread_mask, unsigned cnt_dest);
extern void ipi_redirect_to_threads(unsigned shire_id, uint64_t thread_mask);
extern void (*pmemwrite64) (uint64_t paddr, uint64_t data);
#endif

extern void write_msg_port_data(uint32_t thread, uint32_t id, uint32_t *data, uint8_t oob);
extern void (*pmemwrite64) (uint64_t paddr, uint64_t data);

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
    static uint8_t brcst0_received = 0;

    esr_info_t esr_info;
    decode_ESR_address(ad, &esr_info);

    LOG(DEBUG, "Writing to ESR Region with address %016" PRIx64, ad);

    if (!esr_info.valid)
        LOG(DEBUG, "Invalid ESR");
    else
    {
        switch(esr_info.region)
        {
            case ESR_Region_HART:

                LOG(DEBUG, "Write to ESR Region HART at ESR address %08" PRIx64, esr_info.address);

                switch(esr_info.address)
                {
                    case ESR_HART_PORT0_OFFSET :
                        write_msg_port_data(esr_info.hart + esr_info.shire * EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION, 0, (uint32_t *) data, 0);
                        break;               
                    case ESR_HART_PORT1_OFFSET :
                        write_msg_port_data(esr_info.hart + esr_info.shire * EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION, 1, (uint32_t *) data, 0);
                        break;               
                    case ESR_HART_PORT2_OFFSET :
                        write_msg_port_data(esr_info.hart + esr_info.shire * EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION, 2, (uint32_t *) data, 0);
                        break;               
                    case ESR_HART_PORT3_OFFSET :
                        write_msg_port_data(esr_info.hart + esr_info.shire * EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION, 3, (uint32_t *) data, 0);
                        break;               
                }

                break;

            case ESR_Region_Neighborhood :
                
                if (esr_info.neighborhood == ESR_NEIGH_BROADCAST)
                {
                    LOG(DEBUG, "Broadcast to ESR Region Neighborhood at ESR address %08" PRIx64, esr_info.address);

                    for (int neigh = 0; neigh < EMU_NEIGH_PER_SHIRE; neigh++)
                    {
                        uint64_t neigh_addr = (ad & ~ESR_NEIGH_MASK) + neigh * ESR_NEIGH_OFFSET;
                        pmemwrite64(neigh_addr, *((uint64_t *) data));
                    }
                }
                else
                {
                    LOG(DEBUG, "Write to ESR Region Neighborhood at ESR address %08" PRIx64, esr_info.address);

                    switch(esr_info.address)
                    {
                        case ESR_NEIGH_TBOX_IMGT_PTR:
                            init_txs(*((uint64_t *) data));
                            break;
                        default : break;
                    }
                }

                break;

            case ESR_Region_Shire:
            {
                LOG(DEBUG, "Write to ESR Region Shire at ESR address %08" PRIx64, esr_info.address);

                switch(esr_info.address)
                {
                    case ESR_SHIRE_IPI_REDIRECT_TRIGGER_OFFSET:
                    {
#ifdef SYS_EMU
                        ipi_redirect_to_threads(esr_info.shire, *((uint64_t *) data));
#endif
                        break;
                    }
                    case ESR_SHIRE_FLB_OFFSET  : break;
                    case ESR_SHIRE_FCC0_OFFSET :
                    {
                        LOG(DEBUG, "Write to FCC0_OFFSET value %016" PRIx64, *((uint64_t *) data));
#ifdef SYS_EMU
                        fcc_to_threads(esr_info.shire, 0, *((uint64_t *) data), 0);
#endif
                        fcc_inc(0, esr_info.shire, *((uint64_t*)data), 0);

                        break;
                    }
                    case ESR_SHIRE_FCC1_OFFSET :
                    {
                        LOG(DEBUG, "Write to FCC1_OFFSET value %016" PRIx64, *((uint64_t *) data));
#ifdef SYS_EMU
                        fcc_to_threads(esr_info.shire, 0, *((uint64_t *) data), 1);
#endif
                        fcc_inc(0, esr_info.shire, *((uint64_t*)data), 1);

                        break;
                    }

                    case ESR_SHIRE_FCC2_OFFSET :
                    {
                        LOG(DEBUG, "Write to FCC2_OFFSET value %016" PRIx64, *((uint64_t *) data));
#ifdef SYS_EMU
                        fcc_to_threads(esr_info.shire, 1, *((uint64_t *) data), 0);
#endif
                        fcc_inc(1, esr_info.shire, *((uint64_t*)data), 0);

                        break;
                    }

                    case ESR_SHIRE_FCC3_OFFSET : ;
                    {
                        LOG(DEBUG, "Write to FCC2_OFFSET value %016" PRIx64, *((uint64_t *) data));
#ifdef SYS_EMU
                        fcc_to_threads(esr_info.shire, 1, *((uint64_t *) data), 1);
#endif
                        fcc_inc(1, esr_info.shire, *((uint64_t*)data), 1);

                        break;
                    }
                    case ESR_SHIRE_BROADCAST0_OFFSET:
                    {
                      if (brcst0_received == 0)
                      {
                        LOG(DEBUG, "Write to BROADCAST0 value %016" PRIx64, *((uint64_t *) data));                      
                        //                        dynamic_cast<main_memory_region*>(this)->write(ad, size, data);
                        brcst0_received= 1;
                      }
                      break;
                        
                    }
                    case ESR_SHIRE_BROADCAST1_OFFSET:
                    {
                      if (brcst0_received == 1)
                      {
                        uint64_t minion_mask;
                        esr_info_data_t esr_info_data;
                        
                        dynamic_cast<main_memory_region*>(this)->read(ad-8, 8, &minion_mask);

                        LOG(DEBUG, "Write to BROADCAST1 value %016" PRIx64, *((uint64_t *) data));

                        decode_ESR_data(*((uint64_t *)data), &esr_info_data);
                        //broadcast(esr_data_info, minion_mask);
                        for (long shire_id = 0; shire_id < ESR_BROADCAST_ESR_MAX_SHIRES; shire_id++)
                        {
                          if ((esr_info_data.shire >> shire_id) & 0x1)
                          {
                            uint64_t new_ad;
                            encode_ESR_address(esr_info_data, shire_id, &new_ad);
                            pmemwrite64(new_ad, minion_mask);
                            //dynamic_cast<main_memory_region*>(this)->write(new_ad, size, &minion_mask);
                          }
                        }
                        brcst0_received = 2;
                      }
                      break;
                    }                    
                    default : break;
                }

                break;   
            }
            default :
                LOG(WARN, "Write to ESR Region UNDEFINED at ESR address %08" PRIx64, esr_info.address);
                break;
        }
    }

    if (brcst0_received != 2) {
        // Fast local barriers implementation is using the ESR space storage!! 
         if(data_ != NULL)
            memcpy(data_ + (ad - base_), data, size);
    }
    else
    {
        brcst0_received = 0;
    }
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

void main_memory_region_esr::encode_ESR_address(esr_info_data_t data, uint64_t shire_id, uint64_t *new_ad)
{
  *new_ad =  data.esr_region;
  *new_ad |= (((uint64_t)data.protection) << ESR_REGION_PROT_SHIFT);
  *new_ad |= (shire_id << ESR_REGION_SHIRE_SHIFT);
  *new_ad |= (((uint64_t)data.esr_sregion) << ESR_SREGION_EXT_SHIFT);
  *new_ad |= (data.esr_address << ESR_SHIRE_ESR_SHIFT);
}

void main_memory_region_esr::decode_ESR_data(uint64_t data, esr_info_data_t *info)
{
  info->esr_region = ESR_REGION; //not in data provided by broadcast
  info->protection = esr_protection_t((data & ESR_BROADCAST_PROT_MASK) >> ESR_BROADCAST_PROT_SHIFT);
  info->esr_sregion = esr_region_t((data & ESR_BROADCAST_ESR_SREGION_MASK) >> ESR_BROADCAST_ESR_SREGION_MASK_SHIFT);
  info->esr_address = ((data & ESR_BROADCAST_ESR_ADDR_MASK) >> ESR_BROADCAST_ESR_ADDR_SHIFT);
  info->shire = (data & ESR_BROADCAST_ESR_SHIRE_MASK);
}
