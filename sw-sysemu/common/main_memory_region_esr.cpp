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

// ESR region 'base address' field in bits [39:32], and mask to determine if
// address is in the ESR region (bits [39:32])
#define ESR_REGION             0x0100000000ULL

// Broadcast ESR fields
#define ESR_BROADCAST_PROT_MASK              0x1800000000000000ULL // Region protection is defined in bits [60:59] in esr broadcast data write.
#define ESR_BROADCAST_PROT_SHIFT             59
#define ESR_BROADCAST_ESR_SREGION_MASK       0x07C0000000000000ULL // bits [21:17] in Memory Shire Esr Map. Esr region.
#define ESR_BROADCAST_ESR_SREGION_MASK_SHIFT 54
#define ESR_BROADCAST_ESR_ADDR_MASK          0x003FFF0000000000ULL // bits[17:3] in Memory Shire Esr Map. Esr address
#define ESR_BROADCAST_ESR_ADDR_SHIFT         40
#define ESR_BROADCAST_ESR_SHIRE_MASK         0x000000FFFFFFFFFFULL // bit mask Shire to spread the broadcast bits
#define ESR_BROADCAST_ESR_MAX_SHIRES         ESR_BROADCAST_ESR_ADDR_SHIFT

#define ESR_NEIGH_MINION_BOOT_RESET_VAL   0x8000001000
#define ESR_ICACHE_ERR_LOG_CTL_RESET_VAL  0x6

extern uint32_t current_pc;
extern uint32_t current_thread;

// FIXME: the following needs to be cleaned up
#ifdef SYS_EMU
#include "../sys_emu/sys_emu.h"
#else
namespace sys_emu {
static inline void fcc_to_threads(unsigned, unsigned, uint64_t, unsigned) {}
static inline void send_ipi_redirect_to_threads(unsigned, uint64_t) {}
static inline void raise_software_interrupt(unsigned, uint64_t) {}
static inline void clear_software_interrupt(unsigned, uint64_t) {}
}
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
main_memory_region_esr::main_memory_region_esr(main_memory* parent, uint64_t base, uint64_t size, testLog & l, func_ptr_get_thread& get_thr)
    : main_memory_region(base, size, l, get_thr), mem_(parent)
{
    uint64_t addr_sregion = base_ & ESR_SREGION_MASK;

    if (data_ && (addr_sregion == ESR_NEIGH_REGION))
    {
        // Initialize Neigh ESRs
        if (((base_ & ESR_REGION_PROT_MASK) >> ESR_REGION_PROT_SHIFT) == 3)
        {
            write64(data_ + (ESR_MINION_BOOT - ESR_NEIGH_M0), ESR_NEIGH_MINION_BOOT_RESET_VAL);
            write64(data_ + (ESR_ICACHE_ERR_LOG_CTL - ESR_NEIGH_M0), ESR_ICACHE_ERR_LOG_CTL_RESET_VAL);
        }
    }
}

// Destructor: free allocated mem
main_memory_region_esr::~main_memory_region_esr()
{
}

// Write to memory region
void main_memory_region_esr::write(uint64_t ad, int size, const void * data)
{
    bool write_data = true;

    assert(size == 8);
    uint64_t value = read64(data);

    // Broadcast is special...
    switch (ad)
    {
        case ESR_BROADCAST_DATA:
            LOG(DEBUG, "Write to BROADCAST_DATA value 0x%016" PRIx64, value);
            // FIXME: This is a hack, and it assumes that the region has at
            // least 8*EMU_NUM_SHIRES bytes allocated
            if (data_)
                write64(data_ + 8*(current_thread / EMU_THREADS_PER_SHIRE), value);
            return;
        case ESR_UBROADCAST:
        case ESR_SBROADCAST:
        case ESR_MBROADCAST:
            {
                main_memory_region_esr* region = static_cast<main_memory_region_esr*>(mem_->find_region_containing(ESR_BROADCAST_DATA));
                assert(region);
                uint64_t minion_mask = region->data_ ? read64(region->data_ + 8*(current_thread / EMU_THREADS_PER_SHIRE)) : 0;
                LOG(DEBUG, "Write to %cBROADCAST value 0x%016" PRIx64,
                    (ad == ESR_UBROADCAST) ? 'U' : ((ad == ESR_SBROADCAST) ? 'S' : 'M'), value);
                unsigned prot = (value & ESR_BROADCAST_PROT_MASK) >> ESR_BROADCAST_PROT_SHIFT;
                unsigned sregion = (value & ESR_BROADCAST_ESR_SREGION_MASK) >> ESR_BROADCAST_ESR_SREGION_MASK_SHIFT;
                unsigned esraddr = (value & ESR_BROADCAST_ESR_ADDR_MASK) >> ESR_BROADCAST_ESR_ADDR_SHIFT;
                unsigned shiremsk = value & ESR_BROADCAST_ESR_SHIRE_MASK;
                if ((prot == 2) || (prot > ((ad & ESR_REGION_PROT_MASK) >> ESR_REGION_PROT_SHIFT)))
                {
                    LOG(WARN, "%s", "Request broadcast to ESR with wrong permissions");
                    throw trap_bus_error(ad);
                }
                uint64_t new_ad = ESR_REGION
                    | uint64_t(prot) << ESR_REGION_PROT_SHIFT
                    | uint64_t(sregion) << ESR_SREGION_EXT_SHIFT
                    | uint64_t(esraddr) << 3;
                for (uint64_t shire = 0; shire < ESR_BROADCAST_ESR_MAX_SHIRES; shire++)
                {
                    if (shiremsk & (1ul << shire))
                    {
                        pmemwrite64(new_ad | (shire << ESR_REGION_SHIRE_SHIFT), minion_mask);
                    }
                }
            }
            return;
        default:
            break;
    }

    // Redirect local shire requests to the corresponding shire
    if ((ad & ESR_REGION_SHIRE_MASK) == ESR_REGION_LOCAL_SHIRE) {
        LOG(DEBUG, "Writing to local shire ESR Region with address 0x%016" PRIx64, ad);
        uint64_t shire_addr =
            (ad & ~ESR_REGION_SHIRE_MASK) |
            ((current_thread / EMU_THREADS_PER_SHIRE) << ESR_REGION_SHIRE_SHIFT);
        pmemwrite64(shire_addr, value);
        return;
    }

    unsigned shire = ((ad & ESR_REGION_SHIRE_MASK) >> ESR_REGION_SHIRE_SHIFT);
    uint64_t addr_sregion = ad & ESR_SREGION_MASK;
    uint64_t addr_sregion_ext = ad & ESR_SREGION_EXT_MASK;

    if (addr_sregion == ESR_HART_REGION)
    {
        unsigned hart = (ad & ESR_REGION_HART_MASK) >> ESR_REGION_HART_SHIFT;
        uint64_t esr_addr = ad & ESR_HART_ESR_MASK;
        LOG(DEBUG, "Write to ESR Region HART at ESR address 0x%" PRIx64, esr_addr);
        switch (esr_addr)
        {
            case ESR_PORT0:
                write_msg_port_data(hart + shire * EMU_THREADS_PER_SHIRE, 0, (uint32_t *) data, 0);
                break;
            case ESR_PORT1:
                write_msg_port_data(hart + shire * EMU_THREADS_PER_SHIRE, 1, (uint32_t *) data, 0);
                break;
            case ESR_PORT2:
                write_msg_port_data(hart + shire * EMU_THREADS_PER_SHIRE, 2, (uint32_t *) data, 0);
                break;
            case ESR_PORT3:
                write_msg_port_data(hart + shire * EMU_THREADS_PER_SHIRE, 3, (uint32_t *) data, 0);
                break;
            default:
                throw trap_bus_error(ad);
        }
    }
    else if (addr_sregion == ESR_NEIGH_REGION)
    {
        //unsigned neigh = (ad & ESR_REGION_NEIGH_MASK) >> ESR_REGION_NEIGH_SHIFT;
        uint64_t esr_addr = ad & ESR_NEIGH_ESR_MASK;
        if ((ad & ESR_REGION_NEIGH_MASK) == ESR_REGION_NEIGH_BROADCAST)
        {
            LOG(DEBUG, "Broadcast to ESR Region Neighborhood at ESR address 0x%" PRIx64, esr_addr);
            uint64_t neigh_addr = (ad & ~ESR_REGION_NEIGH_MASK);
            for (int neigh = 0; neigh < EMU_NEIGH_PER_SHIRE; neigh++)
            {
                pmemwrite64(neigh_addr, value);
                neigh_addr += (1ull << ESR_REGION_NEIGH_SHIFT);
            }
            write_data = false;
        }
        else
        {
            uint32_t tbox_id;
            LOG(DEBUG, "Write to ESR Region Neighborhood at ESR address 0x%" PRIx64, esr_addr);
            switch(esr_addr)
            {
                case ESR_DUMMY0:
                case ESR_DUMMY1:
                    value &= 0xffffffff;
                    break;
                case ESR_MINION_BOOT:
                    value &= VA_M;
                    break;
                case ESR_DUMMY2:
                case ESR_DUMMY3:
                    value &= 1;
                    break;
                case ESR_VMSPAGESIZE:
                    /* TODO: to be cached into the minions of the neighborhood (for performance) */
                    value &= 0x3;
                    break;
                case ESR_IPI_REDIRECT_PC:
                    value &= VA_M;
                    break;
                case ESR_PMU_CTRL:
                    value &= 1;
                    break;
                case ESR_NEIGH_CHICKEN:
                    value &= 0xff;
                    break;
                case ESR_ICACHE_ERR_LOG_CTL:
                    value &= 0xf;
                    break;
                case ESR_ICACHE_ERR_LOG_INFO:
                    value &= 0x0010ff000003ffffull;
                    break;
                case ESR_ICACHE_ERR_LOG_ADDRESS:
                    value &= 0xffffffffc0ull;
                    break;
                case ESR_ICACHE_SBE_DBE_COUNTS:
                    value &= 0x7ff;
                    break;
                case ESR_TEXTURE_CONTROL:
                    value &= 0xff80;
                    break;
                case ESR_TEXTURE_STATUS:
                    value &= 0xffe0;
                    break;
                case ESR_MPROT:
                    /* TODO: to be cached into the minions of the neighborhood (for performance) */
                    value &= 0x7;
                    break;
                case ESR_TEXTURE_IMAGE_TABLE_PTR:
                    value &= 0xffffffffffffull;
                    tbox_id = tbox_id_from_thread(current_thread);
                    GET_TBOX(shire, tbox_id).set_image_table_address(value);
                    break;
                default:
                    throw trap_bus_error(ad);
            }
        }
    }
    else if (addr_sregion_ext == ESR_CACHE_REGION)
    {
        /*unsigned bank = (ad & ESR_REGION_BANK_MASK) >> ESR_REGION_BANK_SHIFT;*/
        uint64_t esr_addr = ad & ESR_CACHE_ESR_MASK;
        LOG(DEBUG, "Write to ESR Region ShireCache at ESR address 0x%" PRIx64, esr_addr);
        switch(esr_addr)
        {
        case ESR_SC_IDX_COP_SM_CTL:
            /* do nothing */
            break;
        default:
            throw trap_bus_error(ad);
        }
    }
    else if (addr_sregion_ext == ESR_RBOX_REGION)
    {
        uint64_t esr_addr = ad & ESR_RBOX_ESR_MASK;
        LOG(DEBUG, "Write to ESR Region RBOX at ESR address 0x%" PRIx64, esr_addr);
        switch(esr_addr)
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
    }
    else if (addr_sregion_ext == ESR_SHIRE_REGION)
    {
        uint64_t esr_addr = ad & ESR_SHIRE_ESR_MASK;
        LOG(DEBUG, "Write to ESR Region Shire at ESR address 0x%" PRIx64, esr_addr);
        switch(esr_addr)
        {
            case ESR_MINION_FEATURE:
                /* do nothing */
                break;
            case ESR_IPI_REDIRECT_TRIGGER:
                LOG_ALL_MINIONS(DEBUG, "%s", "Sending IPI_REDIRECT");
                sys_emu::send_ipi_redirect_to_threads(shire, value);
                write_data = false;
                break;
            case ESR_IPI_REDIRECT_FILTER:
                /* do nothing */
                break;
            case ESR_IPI_TRIGGER:
                LOG_ALL_MINIONS(DEBUG, "%s", "Sending IPI");
                sys_emu::raise_software_interrupt(shire, value);
                break;
            case ESR_IPI_TRIGGER_CLEAR:
                sys_emu::clear_software_interrupt(shire, value);
                write_data = false;
                break;
            case ESR_FCC_CREDINC_0:
                LOG_ALL_MINIONS(DEBUG, "Write to FCC0 value %016" PRIx64, value);
                fcc_inc(0, shire, value, 0);
                sys_emu::fcc_to_threads(shire, 0, value, 0);
                write_data = false;
                break;
            case ESR_FCC_CREDINC_1:
                LOG_ALL_MINIONS(DEBUG, "Write to FCC1 value %016" PRIx64, value);
                fcc_inc(0, shire, value, 1);
                sys_emu::fcc_to_threads(shire, 0, value, 1);
                write_data = false;
                break;
            case ESR_FCC_CREDINC_2:
                LOG_ALL_MINIONS(DEBUG, "Write to FCC2 value %016" PRIx64, value);
                fcc_inc(1, shire, value, 0);
                sys_emu::fcc_to_threads(shire, 1, value, 0);
                write_data = false;
                break;
            case ESR_FCC_CREDINC_3:
                LOG_ALL_MINIONS(DEBUG, "Write to FCC2 value %016" PRIx64, value);
                fcc_inc(1, shire, value, 1);
                sys_emu::fcc_to_threads(shire, 1, value, 1);
                write_data = false;
                break;
            case ESR_FAST_LOCAL_BARRIER0:
            case ESR_FAST_LOCAL_BARRIER1:
            case ESR_FAST_LOCAL_BARRIER2:
            case ESR_FAST_LOCAL_BARRIER3:
            case ESR_FAST_LOCAL_BARRIER4:
            case ESR_FAST_LOCAL_BARRIER5:
            case ESR_FAST_LOCAL_BARRIER6:
            case ESR_FAST_LOCAL_BARRIER7:
            case ESR_FAST_LOCAL_BARRIER8:
            case ESR_FAST_LOCAL_BARRIER9:
            case ESR_FAST_LOCAL_BARRIER10:
            case ESR_FAST_LOCAL_BARRIER11:
            case ESR_FAST_LOCAL_BARRIER12:
            case ESR_FAST_LOCAL_BARRIER13:
            case ESR_FAST_LOCAL_BARRIER14:
            case ESR_FAST_LOCAL_BARRIER15:
            case ESR_FAST_LOCAL_BARRIER16:
            case ESR_FAST_LOCAL_BARRIER17:
            case ESR_FAST_LOCAL_BARRIER18:
            case ESR_FAST_LOCAL_BARRIER19:
            case ESR_FAST_LOCAL_BARRIER20:
            case ESR_FAST_LOCAL_BARRIER21:
            case ESR_FAST_LOCAL_BARRIER22:
            case ESR_FAST_LOCAL_BARRIER23:
            case ESR_FAST_LOCAL_BARRIER24:
            case ESR_FAST_LOCAL_BARRIER25:
            case ESR_FAST_LOCAL_BARRIER26:
            case ESR_FAST_LOCAL_BARRIER27:
            case ESR_FAST_LOCAL_BARRIER28:
            case ESR_FAST_LOCAL_BARRIER29:
            case ESR_FAST_LOCAL_BARRIER30:
            case ESR_FAST_LOCAL_BARRIER31:
                /* do nothing */
                break;
            case ESR_SHIRE_COOP_MODE:
                write_shire_coop_mode(shire, value);
                write_data = false;
                break;
            case ESR_ICACHE_UPREFETCH:
                write_icache_prefetch(CSR_PRV_U, shire, value);
                write_data = false;
                break;
            case ESR_ICACHE_SPREFETCH:
                write_icache_prefetch(CSR_PRV_S, shire, value);
                write_data = false;
                break;
            case ESR_ICACHE_MPREFETCH:
                write_icache_prefetch(CSR_PRV_M, shire, value);
                write_data = false;
                break;
            default:
                throw trap_bus_error(ad);
        }
    }
    else
    {
        LOG(WARN, "Write to ESR Region UNDEFINED at address 0x%" PRIx64, ad);
        throw trap_bus_error(ad);
    }
    if (data_ && write_data)
        write64(data_ + (ad - base_), value);
}

// Read from memory region
void main_memory_region_esr::read(uint64_t ad, int size, void * data)
{
    bool read_data = true;

    assert(size == 8);
    uint64_t* ptr = reinterpret_cast<uint64_t*>(data);

    // Broadcast is special...
    switch (ad)
    {
        case ESR_BROADCAST_DATA:
        case ESR_UBROADCAST:
        case ESR_SBROADCAST:
        case ESR_MBROADCAST:
            LOG(DEBUG, "Read from broadcast register at ESR address 0x%016" PRIx64, ad);
            *ptr = 0;
            return;
        default:
            break;
    }

    // Redirect local shire requests to the corresponding shire
    if ((ad & ESR_REGION_SHIRE_MASK) == ESR_REGION_LOCAL_SHIRE) {
        LOG(DEBUG, "Read from local shire ESR Region at ESR address 0x%" PRIx64, ad);

        uint64_t shire_addr =
            (ad & ~ESR_REGION_SHIRE_MASK) |
            ((current_thread / EMU_THREADS_PER_SHIRE) << ESR_REGION_SHIRE_SHIFT);
        *ptr = pmemread64(shire_addr);
        return;
    }

    unsigned shire = ((ad & ESR_REGION_SHIRE_MASK) >> ESR_REGION_SHIRE_SHIFT);
    uint64_t addr_sregion = ad & ESR_SREGION_MASK;
    uint64_t addr_sregion_ext = ad & ESR_SREGION_EXT_MASK;

    if (addr_sregion == ESR_HART_REGION)
    {
        /*unsigned hart = (ad & ESR_REGION_HART_MASK) >> ESR_REGION_HART_SHIFT;*/
        uint64_t esr_addr = ad & ESR_HART_ESR_MASK;
        LOG(DEBUG, "Read from ESR Region HART at ESR address 0x%" PRIx64, esr_addr);
        switch(esr_addr)
        {
            case ESR_PORT0:
            case ESR_PORT1:
            case ESR_PORT2:
            case ESR_PORT3:
                /* do nothing */
                break;
            default:
                throw trap_bus_error(ad);
        }
    }
    else if (addr_sregion == ESR_NEIGH_REGION)
    {
        //unsigned neigh = (ad & ESR_REGION_NEIGH_MASK) >> ESR_REGION_NEIGH_SHIFT;
        uint64_t esr_addr = ad & ESR_NEIGH_ESR_MASK;
        LOG(DEBUG, "Read from ESR Region Neighborhood at ESR address 0x%" PRIx64, esr_addr);
        if ((ad & ESR_REGION_NEIGH_MASK) != ESR_REGION_NEIGH_BROADCAST)
        {
            switch(esr_addr)
            {
                case ESR_DUMMY0:
                case ESR_DUMMY1:
                case ESR_MINION_BOOT:
                case ESR_DUMMY2:
                case ESR_DUMMY3:
                case ESR_VMSPAGESIZE:
                case ESR_IPI_REDIRECT_PC:
                case ESR_PMU_CTRL:
                case ESR_NEIGH_CHICKEN:
                case ESR_ICACHE_ERR_LOG_CTL:
                case ESR_ICACHE_ERR_LOG_INFO:
                case ESR_ICACHE_ERR_LOG_ADDRESS:
                case ESR_ICACHE_SBE_DBE_COUNTS:
                case ESR_TEXTURE_CONTROL:
                case ESR_TEXTURE_STATUS:
                case ESR_TEXTURE_IMAGE_TABLE_PTR:
                    /* do nothing */
                    break;
                default :
                    throw trap_bus_error(ad);
            }
        }
    }
    else if (addr_sregion_ext == ESR_CACHE_REGION)
    {
        /*unsigned bank = (ad & ESR_REGION_BANK_MASK) >> ESR_REGION_BANK_SHIFT;*/
        uint64_t esr_addr = ad & ESR_CACHE_ESR_MASK;
        LOG(DEBUG, "Read from ESR Region ShireCache at ESR address 0x%" PRIx64, esr_addr);
        switch(esr_addr)
        {
            case ESR_SC_IDX_COP_SM_CTL:
                *ptr = 4ull << 24; // idx_cop_sm_state = IDLE
                read_data = false;
                break;
            default:
                throw trap_bus_error(ad);
        }
    }
    else if (addr_sregion_ext == ESR_RBOX_REGION)
    {
        uint64_t esr_addr = ad & ESR_RBOX_ESR_MASK;
        LOG(DEBUG, "Read from ESR Region RBOX at ESR address 0x%" PRIx64, esr_addr);
        switch(esr_addr)
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
    }
    else if (addr_sregion_ext == ESR_SHIRE_REGION)
    {
        uint64_t esr_addr = ad & ESR_SHIRE_ESR_MASK;
        LOG(DEBUG, "Read from ESR Region Shire at ESR address 0x%" PRIx64, esr_addr);
        switch(esr_addr)
        {
            case ESR_MINION_FEATURE:
            case ESR_IPI_REDIRECT_TRIGGER:
            case ESR_IPI_REDIRECT_FILTER:
            case ESR_IPI_TRIGGER:
            case ESR_IPI_TRIGGER_CLEAR:
            case ESR_FCC_CREDINC_0:
            case ESR_FCC_CREDINC_1:
            case ESR_FCC_CREDINC_2:
            case ESR_FCC_CREDINC_3:
            case ESR_FAST_LOCAL_BARRIER0:
            case ESR_FAST_LOCAL_BARRIER1:
            case ESR_FAST_LOCAL_BARRIER2:
            case ESR_FAST_LOCAL_BARRIER3:
            case ESR_FAST_LOCAL_BARRIER4:
            case ESR_FAST_LOCAL_BARRIER5:
            case ESR_FAST_LOCAL_BARRIER6:
            case ESR_FAST_LOCAL_BARRIER7:
            case ESR_FAST_LOCAL_BARRIER8:
            case ESR_FAST_LOCAL_BARRIER9:
            case ESR_FAST_LOCAL_BARRIER10:
            case ESR_FAST_LOCAL_BARRIER11:
            case ESR_FAST_LOCAL_BARRIER12:
            case ESR_FAST_LOCAL_BARRIER13:
            case ESR_FAST_LOCAL_BARRIER14:
            case ESR_FAST_LOCAL_BARRIER15:
            case ESR_FAST_LOCAL_BARRIER16:
            case ESR_FAST_LOCAL_BARRIER17:
            case ESR_FAST_LOCAL_BARRIER18:
            case ESR_FAST_LOCAL_BARRIER19:
            case ESR_FAST_LOCAL_BARRIER20:
            case ESR_FAST_LOCAL_BARRIER21:
            case ESR_FAST_LOCAL_BARRIER22:
            case ESR_FAST_LOCAL_BARRIER23:
            case ESR_FAST_LOCAL_BARRIER24:
            case ESR_FAST_LOCAL_BARRIER25:
            case ESR_FAST_LOCAL_BARRIER26:
            case ESR_FAST_LOCAL_BARRIER27:
            case ESR_FAST_LOCAL_BARRIER28:
            case ESR_FAST_LOCAL_BARRIER29:
            case ESR_FAST_LOCAL_BARRIER30:
            case ESR_FAST_LOCAL_BARRIER31:
                /* do nothing */
                break;
            case ESR_SHIRE_COOP_MODE:
                *ptr = read_shire_coop_mode(shire);
                read_data = false;
                break;
            case ESR_ICACHE_UPREFETCH:
                *ptr = read_icache_prefetch(CSR_PRV_U, shire);
                read_data = false;
                break;
            case ESR_ICACHE_SPREFETCH:
                *ptr = read_icache_prefetch(CSR_PRV_S, shire);
                read_data = false;
                break;
            case ESR_ICACHE_MPREFETCH:
                *ptr = read_icache_prefetch(CSR_PRV_M, shire);
                read_data = false;
                break;
            default:
                throw trap_bus_error(ad);
        }
    }
    else
    {
        LOG(WARN, "Read from ESR Region UNDEFINED at address 0x%" PRIx64, ad);
        throw trap_bus_error(ad);
    }
    if (data_ && read_data)
        *ptr = read64(data_ + (ad - base_));
}
