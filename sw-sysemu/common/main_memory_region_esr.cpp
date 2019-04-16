// Global
#include <cassert>
#include <cstdio>
#include <stdexcept>

// Local
#include "main_memory_region_esr.h"
#include "emu_gio.h"
#include "emu_memop.h"
#include "emu.h"
#include "txs.h"
#include "tbox_emu.h"

extern uint32_t current_pc;
extern uint32_t current_thread;
#ifdef SYS_EMU
// FIXME the following needs to be cleaned up l
#include "../sys_emu/sys_emu.h"
void fcc_to_threads(unsigned shire_id, unsigned thread_dest, uint64_t thread_mask, unsigned cnt_dest)
{
    sys_emu::fcc_to_threads(shire_id, thread_dest, thread_mask, cnt_dest);
}

void send_ipi_redirect_to_threads(unsigned shire_id, uint64_t thread_mask) {
    sys_emu::send_ipi_redirect_to_threads(shire_id, thread_mask);
}

void raise_software_interrupt(unsigned shire_id, uint64_t thread_mask)
{
    sys_emu::raise_software_interrupt(shire_id, thread_mask);
}

void clear_software_interrupt(unsigned shire_id, uint64_t thread_mask)
{
    sys_emu::clear_software_interrupt(shire_id, thread_mask);
}
#else
static inline void fcc_to_threads(unsigned, unsigned, uint64_t, unsigned) {}
static inline void send_ipi_redirect_to_threads(unsigned, uint64_t) {}
static inline void raise_software_interrupt(unsigned, uint64_t) {}
static inline void clear_software_interrupt(unsigned, uint64_t) {}
#endif
extern void write_msg_port_data(uint32_t thread, uint32_t id, uint32_t *data, uint8_t oob);

inline uint64_t read64(const void* data)
{
    return *reinterpret_cast<const uint64_t*>(data);
}

inline void write64(void* data, uint64_t value)
{
    *reinterpret_cast<uint64_t*>(data) = value;
}

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
    bool write_data = true; // set to false for the ESRs that return 0 when reading ( storage is bzeroed in ctor)
    esr_info_t esr_info;
    decode_ESR_address(ad, &esr_info);

    assert(size == 8);
    uint64_t value = read64(data);

    LOG(DEBUG, "Writing to ESR Region with address 0x%016" PRIx64, ad);
    if(esr_info.shire == (ESR_REGION_LOCAL_SHIRE >> ESR_REGION_SHIRE_SHIFT))
    {
        uint64_t new_ad = ad & ~ESR_REGION_SHIRE_MASK;
        new_ad = new_ad | ((current_thread / EMU_THREADS_PER_SHIRE) << ESR_REGION_SHIRE_SHIFT);
        pmemwrite64(new_ad, * ((uint64_t *) data));
        return;
    }

    switch(esr_info.region)
    {
        case ESR_Region_HART:
            LOG(DEBUG, "Write to ESR Region HART at ESR address 0x%" PRIx64, esr_info.address);
            switch(esr_info.address)
            {
                case ESR_HART_PORT0:
                    write_msg_port_data(esr_info.hart + esr_info.shire * EMU_THREADS_PER_SHIRE, 0, (uint32_t *) data, 0);
                    break;
                case ESR_HART_PORT1:
                    write_msg_port_data(esr_info.hart + esr_info.shire * EMU_THREADS_PER_SHIRE, 1, (uint32_t *) data, 0);
                    break;
                case ESR_HART_PORT2:
                    write_msg_port_data(esr_info.hart + esr_info.shire * EMU_THREADS_PER_SHIRE, 2, (uint32_t *) data, 0);
                    break;
                case ESR_HART_PORT3:
                    write_msg_port_data(esr_info.hart + esr_info.shire * EMU_THREADS_PER_SHIRE, 3, (uint32_t *) data, 0);
                    break;
                default:
                    throw trap_bus_error(ad);
            }
            break;

        case ESR_Region_Neighborhood:
            if (esr_info.neighborhood == ESR_NEIGH_BROADCAST)
            {
                LOG(DEBUG, "Broadcast to ESR Region Neighborhood at ESR address 0x%" PRIx64, esr_info.address);
                uint64_t neigh_addr = (ad & ~ESR_NEIGH_MASK);
                for (int neigh = 0; neigh < EMU_NEIGH_PER_SHIRE; neigh++)
                {
                    pmemwrite64(neigh_addr, value);
                    neigh_addr += ESR_NEIGH_OFFSET;
                }
            }
            else
            {
                uint32_t tbox_id;
                LOG(DEBUG, "Write to ESR Region Neighborhood at ESR address 0x%" PRIx64, esr_info.address);
                switch(esr_info.address)
                {
                    case ESR_NEIGH_IPI_REDIRECT_PC:
                    case ESR_NEIGH_TBOX_CONTROL:
                    case ESR_NEIGH_TBOX_STATUS:
                        /* do nothing */
                        break;
                    case ESR_NEIGH_TBOX_IMGT_PTR:
                        tbox_id = tbox_id_from_thread(current_thread);
                        GET_TBOX(esr_info.shire, tbox_id).set_image_table_address(value);
                        break;
                    default:
                        throw trap_bus_error(ad);
                }
            }
            break;

        case ESR_Region_Shire_Cache:
            LOG(WARN, "Write to ESR Region ShireCache at ESR address 0x%" PRIx64, esr_info.address);
            switch(esr_info.address)
            {
                case ESR_CACHE_IDX_COP_SM_CTL:
                    /* do nothing */
                    break;
                default:
                    throw trap_bus_error(ad);
            }
            break;

        case ESR_Region_Shire_RBOX:
            LOG(DEBUG, "Write to ESR Region RBOX at ESR address 0x%" PRIx64, esr_info.address);
            switch(esr_info.address)
            {
                case ESR_RBOX_CONFIG:
                case ESR_RBOX_IN_BUF_PG:
                case ESR_RBOX_IN_BUF_CFG:
                case ESR_RBOX_OUT_BUF_PG:
                case ESR_RBOX_OUT_BUF_CFG:
                case ESR_RBOX_START:
                case ESR_RBOX_CONSUME:
                    /* do nothing */
                    break;
                default:
                    throw trap_bus_error(ad);
            }
            break;

        case ESR_Region_Shire:
            LOG(DEBUG, "Write to ESR Region Shire at ESR address 0x%" PRIx64, esr_info.address);
            switch(esr_info.address)
            {
                case ESR_SHIRE_MINION_FEATURE:
                    /* do nothing */
                    break;
                case ESR_SHIRE_IPI_REDIRECT_TRIGGER:
                    LOG_ALL_MINIONS(DEBUG, "%s", "Sending IPI_REDIRECT");
                    send_ipi_redirect_to_threads(esr_info.shire, value);
                    write_data = false;
                    break;
                case ESR_SHIRE_IPI_REDIRECT_FILTER:
                    /* do nothing */
                    break;
                case ESR_SHIRE_IPI_TRIGGER:
                    LOG_ALL_MINIONS(DEBUG, "%s", "Sending IPI");
                    raise_software_interrupt(esr_info.shire, value);
                    break;
                case ESR_SHIRE_IPI_TRIGGER_CLEAR:
                    clear_software_interrupt(esr_info.shire, value);
                    write_data = false;
                    break;
                case ESR_SHIRE_FCC0:
                    LOG_ALL_MINIONS(DEBUG, "Write to FCC0 value %016" PRIx64, value);
                    fcc_inc(0, esr_info.shire, value, 0);
                    fcc_to_threads(esr_info.shire, 0, value, 0);
                    write_data = false;
                    break;
                case ESR_SHIRE_FCC1:
                    LOG_ALL_MINIONS(DEBUG, "Write to FCC1 value %016" PRIx64, value);
                    fcc_inc(0, esr_info.shire, value, 1);
                    fcc_to_threads(esr_info.shire, 0, value, 1);
                    write_data = false;
                    break;
                case ESR_SHIRE_FCC2:
                    LOG_ALL_MINIONS(DEBUG, "Write to FCC2 value %016" PRIx64, value);
                    fcc_inc(1, esr_info.shire, value, 0);
                    fcc_to_threads(esr_info.shire, 1, value, 0);
                    write_data = false;
                    break;
                case ESR_SHIRE_FCC3:
                    LOG_ALL_MINIONS(DEBUG, "Write to FCC2 value %016" PRIx64, value);
                    fcc_inc(1, esr_info.shire, value, 1);
                    fcc_to_threads(esr_info.shire, 1, value, 1);
                    write_data = false;
                    break;
                case ESR_SHIRE_FLB0:
                case ESR_SHIRE_FLB1:
                case ESR_SHIRE_FLB2:
                case ESR_SHIRE_FLB3:
                case ESR_SHIRE_FLB4:
                case ESR_SHIRE_FLB5:
                case ESR_SHIRE_FLB6:
                case ESR_SHIRE_FLB7:
                case ESR_SHIRE_FLB8:
                case ESR_SHIRE_FLB9:
                case ESR_SHIRE_FLB10:
                case ESR_SHIRE_FLB11:
                case ESR_SHIRE_FLB12:
                case ESR_SHIRE_FLB13:
                case ESR_SHIRE_FLB14:
                case ESR_SHIRE_FLB15:
                case ESR_SHIRE_FLB16:
                case ESR_SHIRE_FLB17:
                case ESR_SHIRE_FLB18:
                case ESR_SHIRE_FLB19:
                case ESR_SHIRE_FLB20:
                case ESR_SHIRE_FLB21:
                case ESR_SHIRE_FLB22:
                case ESR_SHIRE_FLB23:
                case ESR_SHIRE_FLB24:
                case ESR_SHIRE_FLB25:
                case ESR_SHIRE_FLB26:
                case ESR_SHIRE_FLB27:
                case ESR_SHIRE_FLB28:
                case ESR_SHIRE_FLB29:
                case ESR_SHIRE_FLB30:
                case ESR_SHIRE_FLB31:
                    /* do nothing */
                    break;
                case ESR_SHIRE_COOP_MODE:
                    write_shire_coop_mode(esr_info.shire, value);
                    write_data = false;
                    break;
                case ESR_SHIRE_ICACHE_UPREFETCH:
                    write_icache_prefetch(CSR_PRV_U, esr_info.shire, value);
                    write_data = false;
                    break;
                case ESR_SHIRE_ICACHE_SPREFETCH:
                    write_icache_prefetch(CSR_PRV_S, esr_info.shire, value);
                    write_data = false;
                    break;
                case ESR_SHIRE_ICACHE_MPREFETCH:
                    write_icache_prefetch(CSR_PRV_M, esr_info.shire, value);
                    write_data = false;
                    break;
                case ESR_SHIRE_BROADCAST0:
                    if (brcst0_received == 0)
                    {
                        LOG(DEBUG, "Write to BROADCAST0 value %016" PRIx64, value);
                        brcst0_received= 1;
                    }
                    break;
                case ESR_SHIRE_BROADCAST1:
                    if (brcst0_received == 1)
                    {
                        uint64_t minion_mask = pmemread64((ad & ~ESR_REGION_PROT_MASK) - 8);
                        LOG(DEBUG, "Write to BROADCAST1 value %016" PRIx64, value);
                        esr_info_data_t esr_info_data;
                        decode_ESR_data(value, &esr_info_data);
                        //broadcast(esr_data_info, minion_mask);
                        for (long shire_id = 0; shire_id < ESR_BROADCAST_ESR_MAX_SHIRES; shire_id++)
                        {
                            if ((esr_info_data.shire >> shire_id) & 0x1)
                            {
                                uint64_t new_ad;
                                encode_ESR_address(esr_info_data, shire_id, &new_ad);
                                pmemwrite64(new_ad, minion_mask);
                            }
                        }
                        brcst0_received = 2;
                    }
                    break;
                default:
                    throw trap_bus_error(ad);
            }
            break;

        default:
            LOG(WARN, "Write to ESR Region UNDEFINED at ESR address 0x%" PRIx64, esr_info.address);
            throw trap_bus_error(ad);
    }

    if (brcst0_received != 2)
    {
        if (data_ && write_data)
            write64(data_ + (ad - base_), value);
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
    bool read_data = true;
    decode_ESR_address(ad, &esr_info);

    assert(size == 8);
    uint64_t* ptr = reinterpret_cast<uint64_t*>(data);

    LOG(DEBUG, "Read from Shire ESR Region @=0x%" PRIx64, ad);
    if(esr_info.shire == (ESR_REGION_LOCAL_SHIRE >> ESR_REGION_SHIRE_SHIFT))
    {
        uint64_t new_ad = ad & ~ESR_REGION_SHIRE_MASK;
        new_ad = new_ad | ((current_thread / EMU_THREADS_PER_SHIRE) << ESR_REGION_SHIRE_SHIFT);
        *ptr = pmemread64(new_ad);
        return;
    }

    switch(esr_info.region)
    {
        case ESR_Region_HART:
            LOG(DEBUG, "Read from ESR Region HART at ESR address 0x%" PRIx64, esr_info.address);
            switch(esr_info.address)
            {
                case ESR_HART_PORT0:
                case ESR_HART_PORT1:
                case ESR_HART_PORT2:
                case ESR_HART_PORT3:
                    /* do nothing */
                    break;
                default:
                    throw trap_bus_error(ad);
            }
            break;

        case ESR_Region_Neighborhood:
            if (esr_info.neighborhood != ESR_NEIGH_BROADCAST)
            {
                LOG(DEBUG, "Read from ESR Region Neighborhood at ESR address 0x%" PRIx64, esr_info.address);
                switch(esr_info.address)
                {
                    case ESR_NEIGH_IPI_REDIRECT_PC:
                    case ESR_NEIGH_TBOX_CONTROL:
                    case ESR_NEIGH_TBOX_STATUS:
                    case ESR_NEIGH_TBOX_IMGT_PTR:
                        /* do nothing */
                        break;
                    default :
                        throw trap_bus_error(ad);
                }
            }
            break;

        case ESR_Region_Shire_Cache:
            LOG(WARN, "Read from ESR Region ShireCache at ESR address 0x%" PRIx64, esr_info.address);
            switch(esr_info.address)
            {
                case ESR_CACHE_IDX_COP_SM_CTL:
                    *ptr = 4 << 24; // idx_cop_sm_state = IDLE
                    read_data = false;
                    break;
                default:
                    throw trap_bus_error(ad);
            }
            break;

        case ESR_Region_Shire_RBOX:
            LOG(DEBUG, "Read from ESR Region RBOX at ESR address 0x%" PRIx64, esr_info.address);
            switch(esr_info.address)
            {
                case ESR_RBOX_CONFIG:
                case ESR_RBOX_IN_BUF_PG:
                case ESR_RBOX_IN_BUF_CFG:
                case ESR_RBOX_OUT_BUF_PG:
                case ESR_RBOX_OUT_BUF_CFG:
                case ESR_RBOX_START:
                case ESR_RBOX_CONSUME:
                    /* do nothing */
                    break;
                default:
                    throw trap_bus_error(ad);
            }
            break;

        case ESR_Region_Shire:
            LOG(DEBUG, "Read from ESR Region Shire at ESR address 0x%" PRIx64, esr_info.address);
            switch(esr_info.address)
            {
                case ESR_SHIRE_MINION_FEATURE:
                case ESR_SHIRE_IPI_REDIRECT_TRIGGER:
                case ESR_SHIRE_IPI_REDIRECT_FILTER:
                case ESR_SHIRE_IPI_TRIGGER:
                case ESR_SHIRE_IPI_TRIGGER_CLEAR:
                case ESR_SHIRE_FCC0:
                case ESR_SHIRE_FCC1:
                case ESR_SHIRE_FCC2:
                case ESR_SHIRE_FCC3:
                case ESR_SHIRE_FLB0:
                case ESR_SHIRE_FLB1:
                case ESR_SHIRE_FLB2:
                case ESR_SHIRE_FLB3:
                case ESR_SHIRE_FLB4:
                case ESR_SHIRE_FLB5:
                case ESR_SHIRE_FLB6:
                case ESR_SHIRE_FLB7:
                case ESR_SHIRE_FLB8:
                case ESR_SHIRE_FLB9:
                case ESR_SHIRE_FLB10:
                case ESR_SHIRE_FLB11:
                case ESR_SHIRE_FLB12:
                case ESR_SHIRE_FLB13:
                case ESR_SHIRE_FLB14:
                case ESR_SHIRE_FLB15:
                case ESR_SHIRE_FLB16:
                case ESR_SHIRE_FLB17:
                case ESR_SHIRE_FLB18:
                case ESR_SHIRE_FLB19:
                case ESR_SHIRE_FLB20:
                case ESR_SHIRE_FLB21:
                case ESR_SHIRE_FLB22:
                case ESR_SHIRE_FLB23:
                case ESR_SHIRE_FLB24:
                case ESR_SHIRE_FLB25:
                case ESR_SHIRE_FLB26:
                case ESR_SHIRE_FLB27:
                case ESR_SHIRE_FLB28:
                case ESR_SHIRE_FLB29:
                case ESR_SHIRE_FLB30:
                case ESR_SHIRE_FLB31:
                case ESR_SHIRE_BROADCAST0:
                case ESR_SHIRE_BROADCAST1:
                    /* do nothing */
                    break;
                case ESR_SHIRE_COOP_MODE:
                    *ptr = read_shire_coop_mode(esr_info.shire);
                    read_data = false;
                    break;
                case ESR_SHIRE_ICACHE_UPREFETCH:
                    *ptr = read_icache_prefetch(CSR_PRV_U, esr_info.shire);
                    read_data = false;
                    break;
                case ESR_SHIRE_ICACHE_SPREFETCH:
                    *ptr = read_icache_prefetch(CSR_PRV_S, esr_info.shire);
                    read_data = false;
                    break;
                case ESR_SHIRE_ICACHE_MPREFETCH:
                    *ptr = read_icache_prefetch(CSR_PRV_M, esr_info.shire);
                    read_data = false;
                    break;
                default:
                    throw trap_bus_error(ad);
            }
            break;

        default:
            LOG(WARN, "Read from ESR Region UNDEFINED at ESR address 0x%" PRIx64, esr_info.address);
            throw trap_bus_error(ad);
    }

    if (data_ && read_data)
        *ptr = read64(data_ + (ad - base_));
}

void main_memory_region_esr::decode_ESR_address(uint64_t address, esr_info_t *info)
{
    if ((address & ESR_REGION_MASK) != ESR_REGION)
        throw std::runtime_error("main_memory_region_esr::decode_ESR_address for address not in ESR region");

    info->protection = esr_protection_t((address & ESR_REGION_PROT_MASK) >> ESR_REGION_PROT_SHIFT);

    info->shire = ((address & ESR_REGION_SHIRE_MASK) >> ESR_REGION_SHIRE_SHIFT);

    switch((address & ESR_SREGION_MASK) >> ESR_SREGION_SHIFT)
    {
        case ESR_Region_HART:
            info->region  = ESR_Region_HART;
            info->hart    = (address & ESR_HART_MASK) >> ESR_HART_SHIFT;
            info->address = (address & ESR_HART_ESR_MASK);
            break;
        case ESR_Region_Neighborhood:
            info->region       = ESR_Region_Neighborhood;
            info->neighborhood = (address & ESR_NEIGH_MASK) >> ESR_NEIGH_SHIFT;
            info->address      = (address & ESR_NEIGH_ESR_MASK);
            break;
        case ESR_Region_Extended:
            switch ((address & ESR_SREGION_EXT_MASK) >> ESR_SREGION_EXT_SHIFT)
            {
                case ESR_Region_Shire_Cache:
                {
                    info->region  = ESR_Region_Shire_Cache;
                    info->bank    = (address & ESR_BANK_MASK) >> ESR_BANK_SHIFT;
                    info->address = (address & ESR_SC_ESR_MASK);
                    break;
                };
                case ESR_Region_Shire_RBOX:
                {
                    info->region  = ESR_Region_Shire_RBOX;
                    info->address = (address & ESR_SHIRE_ESR_MASK);
                    break;
                }
                case ESR_Region_Shire:
                {
                    info->region  = ESR_Region_Shire;
                    info->address = (address & ESR_SHIRE_ESR_MASK);
                }
            }
            break;
        default:
            info->region = ESR_Region_Reserved;
            break;
    }
}

void main_memory_region_esr::encode_ESR_address(esr_info_data_t data, uint64_t shire_id, uint64_t *new_ad)
{
    if (new_ad)
    {
        uint64_t ad =  data.esr_region;
        ad |= (((uint64_t)data.protection) << ESR_REGION_PROT_SHIFT);
        ad |= (shire_id << ESR_REGION_SHIRE_SHIFT);
        ad |= (((uint64_t)data.esr_sregion) << ESR_SREGION_EXT_SHIFT);
        ad |= (data.esr_address << ESR_ESR_ID_SHIFT);
        *new_ad = ad;
    }
}

void main_memory_region_esr::decode_ESR_data(uint64_t data, esr_info_data_t *info)
{
    info->esr_region = ESR_REGION; //not in data provided by broadcast
    info->protection = esr_protection_t((data & ESR_BROADCAST_PROT_MASK) >> ESR_BROADCAST_PROT_SHIFT);
    info->esr_sregion = esr_region_t((data & ESR_BROADCAST_ESR_SREGION_MASK) >> ESR_BROADCAST_ESR_SREGION_MASK_SHIFT);
    info->esr_address = ((data & ESR_BROADCAST_ESR_ADDR_MASK) >> ESR_BROADCAST_ESR_ADDR_SHIFT);
    info->shire = (data & ESR_BROADCAST_ESR_SHIRE_MASK);
}
