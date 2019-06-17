// Global
#include <cassert>
#include <cstdio>
#include <stdexcept>

// Local
#include "main_memory_region_esr.h"
#include "emu_gio.h"
#include "emu.h"
#include "esrs.h"
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

// Write to memory region
void main_memory_region_esr::write(uint64_t addr, size_t n, const void* source)
{
    assert(n == 8);
    uint64_t value = read64(source);

    unsigned current_shire = current_thread / EMU_THREADS_PER_SHIRE;

    // Broadcast is special...
    switch (addr)
    {
        case ESR_BROADCAST_DATA:
            LOG(DEBUG, "Write to BROADCAST_DATA value 0x%016" PRIx64, value);
            broadcast_esrs[current_shire].data = value;
            return;
        case ESR_UBROADCAST:
        case ESR_SBROADCAST:
        case ESR_MBROADCAST:
            {
                const void* data2 = &broadcast_esrs[current_shire].data;
                LOG(DEBUG, "Write to %cBROADCAST value 0x%016" PRIx64,
                    (addr == ESR_UBROADCAST) ? 'U' : ((addr == ESR_SBROADCAST) ? 'S' : 'M'), value);
                unsigned prot = (value & ESR_BROADCAST_PROT_MASK) >> ESR_BROADCAST_PROT_SHIFT;
                unsigned sregion = (value & ESR_BROADCAST_ESR_SREGION_MASK) >> ESR_BROADCAST_ESR_SREGION_MASK_SHIFT;
                unsigned esraddr = (value & ESR_BROADCAST_ESR_ADDR_MASK) >> ESR_BROADCAST_ESR_ADDR_SHIFT;
                unsigned shiremsk = value & ESR_BROADCAST_ESR_SHIRE_MASK;
                if ((prot == 2) || (prot > ((addr & ESR_REGION_PROT_MASK) >> ESR_REGION_PROT_SHIFT)))
                {
                    LOG(WARN, "Request %cbroadcast to ESR with wrong permissions (%u)",
                        (addr == ESR_UBROADCAST) ? 'u' : ((addr == ESR_SBROADCAST) ? 's' : 'm'), prot);
                    throw trap_bus_error(addr);
                }
                uint64_t new_ad = ESR_REGION
                    | uint64_t(prot) << ESR_REGION_PROT_SHIFT
                    | uint64_t(sregion) << ESR_SREGION_EXT_SHIFT
                    | uint64_t(esraddr) << 3;
                for (uint64_t shire = 0; shire < ESR_BROADCAST_ESR_MAX_SHIRES; shire++)
                {
                    if (shiremsk & (1ul << shire))
                    {
                        this->write(new_ad | (shire << ESR_REGION_SHIRE_SHIFT), 8, data2);
                    }
                }
            }
            return;
        default:
            break;
    }

    // Redirect local shire requests to the corresponding shire
    if ((addr & ESR_REGION_SHIRE_MASK) == ESR_REGION_LOCAL_SHIRE) {
        unsigned current_shireid = (current_shire == EMU_IO_SHIRE_SP) ? IO_SHIRE_ID : current_shire;
        LOG(DEBUG, "Writing to local shire ESR Region with address 0x%016" PRIx64, addr);
        addr = (addr & ~ESR_REGION_SHIRE_MASK) | (current_shireid << ESR_REGION_SHIRE_SHIFT);
    }

    unsigned shire = ((addr & ESR_REGION_SHIRE_MASK) >> ESR_REGION_SHIRE_SHIFT);
    uint64_t addr_sregion = addr & ESR_SREGION_MASK;
    uint64_t addr_sregion_ext = addr & ESR_SREGION_EXT_MASK;

    if (shire == IO_SHIRE_ID)
    {
        uint64_t esr_addr = addr & ESR_IOSHIRE_ESR_MASK;
        LOG(DEBUG, "Write to IOshire ESR Region at ESR address 0x%" PRIx64, esr_addr);
#ifdef SYS_EMU
        switch (esr_addr)
        {
            case ESR_PU_RVTIM_MTIME:
                sys_emu::get_pu_rvtimer().write_mtime(value);
                break;
            case ESR_PU_RVTIM_MTIMECMP:
                sys_emu::get_pu_rvtimer().write_mtimecmp(value);
                break;
            default:
                throw trap_bus_error(addr);
        }
#else
        throw trap_bus_error(addr);
#endif
    }
    else if (addr_sregion == ESR_HART_REGION)
    {
        uint64_t esr_addr = addr & ESR_HART_ESR_MASK;
        unsigned hart = (addr & ESR_REGION_HART_MASK) >> ESR_REGION_HART_SHIFT;
        LOG(DEBUG, "Write to hart ESR region S%u:H%u at ESR address 0x%" PRIx64, shire, hart, esr_addr);
        switch (esr_addr)
        {
            case ESR_PORT0:
                write_msg_port_data(hart + shire * EMU_THREADS_PER_SHIRE, 0, (uint32_t*) source, 0);
                break;
            case ESR_PORT1:
                write_msg_port_data(hart + shire * EMU_THREADS_PER_SHIRE, 1, (uint32_t*) source, 0);
                break;
            case ESR_PORT2:
                write_msg_port_data(hart + shire * EMU_THREADS_PER_SHIRE, 2, (uint32_t*) source, 0);
                break;
            case ESR_PORT3:
                write_msg_port_data(hart + shire * EMU_THREADS_PER_SHIRE, 3, (uint32_t*) source, 0);
                break;
            default:
                throw trap_bus_error(addr);
        }
    }
    else if (addr_sregion == ESR_NEIGH_REGION)
    {
        unsigned neigh = (addr & ESR_REGION_NEIGH_MASK) >> ESR_REGION_NEIGH_SHIFT;
        uint64_t esr_addr = addr & ESR_NEIGH_ESR_MASK;
        if (neigh == ESR_REGION_NEIGH_BROADCAST)
        {
            LOG(DEBUG, "Broadcast to neigh ESR region S%u at ESR address 0x%" PRIx64, shire, esr_addr);
            uint64_t neigh_addr = (addr & ~ESR_REGION_NEIGH_MASK);
            for (int neigh = 0; neigh < EMU_NEIGH_PER_SHIRE; neigh++)
            {
                this->write(neigh_addr, 8, source);
                neigh_addr += (1ull << ESR_REGION_NEIGH_SHIFT);
            }
        }
        else
        {
            LOG(DEBUG, "Write to neigh ESR region S%u:N%u at ESR address 0x%" PRIx64, shire, neigh, esr_addr);
            if (neigh >= EMU_NEIGH_PER_SHIRE) throw trap_bus_error(addr);
            unsigned idx = EMU_NEIGH_PER_SHIRE * ((shire == IO_SHIRE_ID) ? EMU_IO_SHIRE_SP : shire) + neigh;
            uint32_t tbox_id;
            switch(esr_addr)
            {
                case ESR_DUMMY0:
                    value &= 0xffffffff;
                    neigh_esrs[idx].dummy0 = uint32_t(value);
                    break;
                case ESR_DUMMY1:
                    value &= 0xffffffff;
                    neigh_esrs[idx].dummy1 = uint32_t(value);
                    break;
                case ESR_MINION_BOOT:
                    value &= VA_M;
                    neigh_esrs[idx].minion_boot = value;
                    break;
                case ESR_DUMMY2:
                    value &= 1;
                    neigh_esrs[idx].dummy2 = value;
                    break;
                case ESR_DUMMY3:
                    value &= 1;
                    neigh_esrs[idx].dummy3 = value;
                    break;
                case ESR_VMSPAGESIZE:
                    value &= 0x3;
                    neigh_esrs[idx].vmspagesize = value;
                    break;
                case ESR_IPI_REDIRECT_PC:
                    value &= VA_M;
                    neigh_esrs[idx].ipi_redirect_pc = value;
                    break;
                case ESR_PMU_CTRL:
                    value &= 1;
                    neigh_esrs[idx].pmu_ctrl = value;
                    break;
                case ESR_NEIGH_CHICKEN:
                    value &= 0xff;
                    neigh_esrs[idx].neigh_chicken = uint8_t(value);
                    break;
                case ESR_ICACHE_ERR_LOG_CTL:
                    value &= 0xf;
                    neigh_esrs[idx].icache_err_log_ctl = uint8_t(value);
                    break;
                case ESR_ICACHE_ERR_LOG_INFO:
                    value &= 0x0010ff000003ffffull;
                    neigh_esrs[idx].icache_err_log_info = value;
                    break;
                case ESR_ICACHE_ERR_LOG_ADDRESS:
                    value &= 0xffffffffc0ull;
                    neigh_esrs[idx].icache_err_log_address = value;
                    break;
                case ESR_ICACHE_SBE_DBE_COUNTS:
                    value &= 0x7ff;
                    neigh_esrs[idx].icache_sbe_dbe_counts = uint16_t(value);
                    break;
                case ESR_TEXTURE_CONTROL:
                    value &= 0xff80;
                    neigh_esrs[idx].texture_control = uint16_t(value);
                    break;
                case ESR_TEXTURE_STATUS:
                    value &= 0xffe0;
                    neigh_esrs[idx].texture_status = uint16_t(value);
                    break;
                case ESR_MPROT:
                    value &= 0x7;
                    neigh_esrs[idx].mprot = uint8_t(value);
                    break;
                case ESR_TEXTURE_IMAGE_TABLE_PTR:
                    value &= 0xffffffffffffull;
                    neigh_esrs[idx].texture_image_table_ptr = value;
                    tbox_id = tbox_id_from_thread(current_thread);
                    GET_TBOX(shire, tbox_id).set_image_table_address(value);
                    break;
                default:
                    throw trap_bus_error(addr);
            }
        }
    }
    else if (addr_sregion_ext == ESR_CACHE_REGION)
    {
        uint64_t esr_addr = addr & ESR_CACHE_ESR_MASK;
        unsigned idx = (shire == IO_SHIRE_ID) ? EMU_IO_SHIRE_SP : shire;
        // when bank = all 1s then we broadcast to all 4 banks
        unsigned bank       = (addr & ESR_REGION_BANK_MASK) >> ESR_REGION_BANK_SHIFT;
        unsigned begin_bank = (bank < 4) ? bank : 0;
        unsigned end_bank   = (bank < 4) ? (bank + 1) : 4;
        if (bank == (ESR_REGION_BANK_MASK >> ESR_REGION_BANK_SHIFT))
        {
            LOG(DEBUG, "Broadcast to cache_bank ESR region S%u at ESR address 0x%" PRIx64, shire, esr_addr);
        }
        else if (bank >= 4)
        {
            LOG(DEBUG, "Write to cache_bank ESR region S%u:b%u at ESR address 0x%" PRIx64, shire, bank, esr_addr);
            throw trap_bus_error(addr);
        }
        for (unsigned b = begin_bank; b < end_bank; ++b)
        {
            LOG(DEBUG, "Write to cache_bank ESR region S%u:b%u at ESR address 0x%" PRIx64, shire, b, esr_addr);
            switch(esr_addr)
            {
                case ESR_SC_L3_SHIRE_SWIZZLE_CTL:
                    shire_cache_esrs[idx].bank[b].sc_l3_shire_swizzle_ctl = value;
                    break;
                case ESR_SC_REQQ_CTL:
                    shire_cache_esrs[idx].bank[b].sc_reqq_ctl = value;
                    break;
                case ESR_SC_PIPE_CTL:
                    shire_cache_esrs[idx].bank[b].sc_pipe_ctl = value;
                    break;
                case ESR_SC_L2_CACHE_CTL:
                    shire_cache_esrs[idx].bank[b].sc_l2_cache_ctl = value;
                    break;
                case ESR_SC_L3_CACHE_CTL:
                    shire_cache_esrs[idx].bank[b].sc_l3_cache_ctl = value;
                    break;
                case ESR_SC_SCP_CACHE_CTL:
                    shire_cache_esrs[idx].bank[b].sc_scp_cache_ctl = value;
                    break;
                case ESR_SC_IDX_COP_SM_CTL:
                    shire_cache_esrs[idx].bank[b].sc_idx_cop_sm_ctl = value;
                    break;
                case ESR_SC_IDX_COP_SM_PHYSICAL_INDEX:
                    shire_cache_esrs[idx].bank[b].sc_idx_cop_sm_physical_index = value;
                    break;
                case ESR_SC_IDX_COP_SM_DATA0:
                    shire_cache_esrs[idx].bank[b].sc_idx_cop_sm_data0 = value;
                    break;
                case ESR_SC_IDX_COP_SM_DATA1:
                    shire_cache_esrs[idx].bank[b].sc_idx_cop_sm_data1 = value;
                    break;
                case ESR_SC_IDX_COP_SM_ECC:
                    shire_cache_esrs[idx].bank[b].sc_idx_cop_sm_ecc = value;
                    break;
                case ESR_SC_ERR_LOG_CTL:
                    shire_cache_esrs[idx].bank[b].sc_err_log_ctl = value;
                    break;
                case ESR_SC_ERR_LOG_INFO:
                    shire_cache_esrs[idx].bank[b].sc_err_log_info = value;
                    break;
                case ESR_SC_ERR_LOG_ADDRESS:
                    shire_cache_esrs[idx].bank[b].sc_err_log_address = value;
                    break;
                case ESR_SC_SBE_DBE_COUNTS:
                    shire_cache_esrs[idx].bank[b].sc_sbe_dbe_counts = value;
                    break;
                case ESR_SC_REQQ_DEBUG_CTL:
                    shire_cache_esrs[idx].bank[b].sc_reqq_debug_ctl = value;
                    break;
                case ESR_SC_REQQ_DEBUG0:
                    shire_cache_esrs[idx].bank[b].sc_reqq_debug0 = value;
                    break;
                case ESR_SC_REQQ_DEBUG1:
                    shire_cache_esrs[idx].bank[b].sc_reqq_debug1 = value;
                    break;
                case ESR_SC_REQQ_DEBUG2:
                    shire_cache_esrs[idx].bank[b].sc_reqq_debug2 = value;
                    break;
                case ESR_SC_REQQ_DEBUG3:
                    shire_cache_esrs[idx].bank[b].sc_reqq_debug3 = value;
                    break;
                default:
                    throw trap_bus_error(addr);
            }
        }
    }
    else if (addr_sregion_ext == ESR_RBOX_REGION)
    {
        uint64_t esr_addr = addr & ESR_RBOX_ESR_MASK;
        LOG(DEBUG, "Write to rbox ESR region S%u at ESR address 0x%" PRIx64, shire, esr_addr);
        if (shire >= EMU_NUM_COMPUTE_SHIRES) throw trap_bus_error(addr);
        switch(esr_addr)
        {
            case ESR_RBOX_CONFIG:
            case ESR_RBOX_IN_BUF_PG:
            case ESR_RBOX_IN_BUF_CFG:
            case ESR_RBOX_OUT_BUF_PG:
            case ESR_RBOX_OUT_BUF_CFG:
            case ESR_RBOX_START:
            case ESR_RBOX_CONSUME:
                GET_RBOX(shire, 0).write_esr((esr_addr >> 3) & 0x3FFF, value);
                break;
            default:
                throw trap_bus_error(addr);
        }
    }
    else if (addr_sregion_ext == ESR_SHIRE_REGION)
    {
        uint64_t esr_addr = addr & ESR_SHIRE_ESR_MASK;
        LOG(DEBUG, "Write to shire_other ESR region S%u at ESR address 0x%" PRIx64, shire, esr_addr);
        unsigned idx = (shire == IO_SHIRE_ID) ? EMU_IO_SHIRE_SP : shire;
        switch(esr_addr)
        {
            case ESR_MINION_FEATURE:
                value &= 0x3f;
                shire_other_esrs[idx].minion_feature = uint8_t(value);
                break;
            case ESR_SHIRE_CONFIG:
                value &= 0x3ffffff;
                shire_other_esrs[idx].shire_config = uint32_t(value);
                break;
            case ESR_THREAD1_DISABLE:
                value &= 0xffffffff;
                shire_other_esrs[idx].thread1_disable = uint32_t(value);
                break;
            case ESR_IPI_REDIRECT_TRIGGER:
                LOG_ALL_MINIONS(DEBUG, "%s", "Sending IPI_REDIRECT");
                sys_emu::send_ipi_redirect_to_threads(shire, value);
                break;
            case ESR_IPI_REDIRECT_FILTER:
                shire_other_esrs[idx].ipi_redirect_filter = value;
                break;
            case ESR_IPI_TRIGGER:
                LOG_ALL_MINIONS(DEBUG, "%s", "Sending IPI");
                shire_other_esrs[idx].ipi_trigger = value;
                sys_emu::raise_software_interrupt(shire, value);
                break;
            case ESR_IPI_TRIGGER_CLEAR:
                sys_emu::clear_software_interrupt(shire, value);
                break;
            case ESR_FCC_CREDINC_0:
                LOG_ALL_MINIONS(DEBUG, "Write to shire %d FCC0 value %016" PRIx64, shire,value);
                fcc_inc(0, shire, value, 0);
                sys_emu::fcc_to_threads(shire, 0, value, 0);
                break;
            case ESR_FCC_CREDINC_1:
                LOG_ALL_MINIONS(DEBUG, "Write to shire %d FCC1 value %016" PRIx64, shire,value);
                fcc_inc(0, shire, value, 1);
                sys_emu::fcc_to_threads(shire, 0, value, 1);
                break;
            case ESR_FCC_CREDINC_2:
                LOG_ALL_MINIONS(DEBUG, "Write to shire %d FCC2 value %016" PRIx64, shire,value);
                fcc_inc(1, shire, value, 0);
                sys_emu::fcc_to_threads(shire, 1, value, 0);
                break;
            case ESR_FCC_CREDINC_3:
                LOG_ALL_MINIONS(DEBUG, "Write to shire %d FCC3 value %016" PRIx64, shire,value);
                fcc_inc(1, shire, value, 1);
                sys_emu::fcc_to_threads(shire, 1, value, 1);
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
                shire_other_esrs[idx].fast_local_barrier[(esr_addr - ESR_FAST_LOCAL_BARRIER0)>>3] = value;
                break;
            case ESR_MTIME_LOCAL_TARGET:
                shire_other_esrs[idx].mtime_local_target = value;
                break;
            case ESR_THREAD0_DISABLE:
                value &= 0xffffffff;
                shire_other_esrs[idx].thread0_disable = uint32_t(value);
                break;
            case ESR_SHIRE_PLL_AUTO_CONFIG:
                value &= 0x1ffff;
                shire_other_esrs[idx].shire_pll_auto_config = uint32_t(value);
                break;
            case ESR_SHIRE_PLL_CONFIG_DATA_0:
            case ESR_SHIRE_PLL_CONFIG_DATA_1:
            case ESR_SHIRE_PLL_CONFIG_DATA_2:
            case ESR_SHIRE_PLL_CONFIG_DATA_3:
            case ESR_SHIRE_PLL_CONFIG_DATA_4:
            case ESR_SHIRE_PLL_CONFIG_DATA_5:
                shire_other_esrs[idx].shire_pll_config_data[(esr_addr - ESR_SHIRE_PLL_CONFIG_DATA_0)>>3] = value;
                break;
            case ESR_SHIRE_COOP_MODE:
                write_shire_coop_mode(shire, value);
                break;
            case ESR_SHIRE_CTRL_CLOCKMUX:
                value &= 0x4f;
                shire_other_esrs[idx].shire_ctrl_clockmux = value;
                break;
            case ESR_SHIRE_CACHE_RAM_CFG1:
                value &= 0xfffffffffull;
                shire_other_esrs[idx].shire_cache_ram_cfg1 = value;
                break;
            case ESR_SHIRE_CACHE_RAM_CFG2:
                value &= 0x3ffff;
                shire_other_esrs[idx].shire_cache_ram_cfg2 = uint32_t(value);
                break;
            case ESR_SHIRE_CACHE_RAM_CFG3:
                value &= 0xfffffffffull;
                shire_other_esrs[idx].shire_cache_ram_cfg3 = value;
                break;
            case ESR_SHIRE_CACHE_RAM_CFG4:
                value &= 0xfffffffffull;
                shire_other_esrs[idx].shire_cache_ram_cfg4 = value;
                break;
            case ESR_SHIRE_DLL_AUTO_CONFIG:
                value &= 0x3fff;
                shire_other_esrs[idx].shire_dll_auto_config = uint16_t(value);
                break;
            case ESR_SHIRE_DLL_CONFIG_DATA_0:
                shire_other_esrs[idx].shire_dll_config_data_0 = value;
                break;
            case ESR_ICACHE_UPREFETCH:
                write_icache_prefetch(PRV_U, shire, value);
                break;
            case ESR_ICACHE_SPREFETCH:
                write_icache_prefetch(PRV_S, shire, value);
                break;
            case ESR_ICACHE_MPREFETCH:
                write_icache_prefetch(PRV_M, shire, value);
                break;
            default:
                throw trap_bus_error(addr);
        }
    }
    else
    {
        LOG(WARN, "Write to unknown ESR region S%u at address 0x%" PRIx64, shire, addr);
        throw trap_bus_error(addr);
    }
}

// Read from memory region
void main_memory_region_esr::read(uint64_t addr, size_t n, void* result)
{
    assert(n == 8);
    uint64_t* ptr = reinterpret_cast<uint64_t*>(result);

    // Broadcast is special...
    switch (addr)
    {
        case ESR_BROADCAST_DATA:
        case ESR_UBROADCAST:
        case ESR_SBROADCAST:
        case ESR_MBROADCAST:
            LOG(DEBUG, "Read from broadcast register at ESR address 0x%016" PRIx64, addr);
            *ptr = 0;
            return;
        default:
            break;
    }

    // Redirect local shire requests to the corresponding shire
    if ((addr & ESR_REGION_SHIRE_MASK) == ESR_REGION_LOCAL_SHIRE) {
        unsigned current_shire = current_thread / EMU_THREADS_PER_SHIRE;
        unsigned current_shireid = (current_shire == EMU_IO_SHIRE_SP) ? IO_SHIRE_ID : current_shire;
        LOG(DEBUG, "Read from local shire ESR Region at ESR address 0x%" PRIx64, addr);
        addr = (addr & ~ESR_REGION_SHIRE_MASK) | (current_shireid << ESR_REGION_SHIRE_SHIFT);
    }

    unsigned shire = ((addr & ESR_REGION_SHIRE_MASK) >> ESR_REGION_SHIRE_SHIFT);
    uint64_t addr_sregion = addr & ESR_SREGION_MASK;
    uint64_t addr_sregion_ext = addr & ESR_SREGION_EXT_MASK;

    if (shire == IO_SHIRE_ID)
    {
        uint64_t esr_addr = addr & ESR_IOSHIRE_ESR_MASK;
        LOG(DEBUG, "Read from IOshire ESR Region at ESR address 0x%" PRIx64, esr_addr);
#ifdef SYS_EMU
        switch (esr_addr)
        {
            case ESR_PU_RVTIM_MTIME:
                *ptr = sys_emu::get_pu_rvtimer().read_mtime();
                break;
            case ESR_PU_RVTIM_MTIMECMP:
                *ptr = sys_emu::get_pu_rvtimer().read_mtimecmp();
                break;
            default:
                throw trap_bus_error(addr);
        }
#else
        throw trap_bus_error(addr);
#endif
    }
    else if (addr_sregion == ESR_HART_REGION)
    {
        /*unsigned hart = (addr & ESR_REGION_HART_MASK) >> ESR_REGION_HART_SHIFT;*/
        uint64_t esr_addr = addr & ESR_HART_ESR_MASK;
        LOG(DEBUG, "Read from ESR Region HART at ESR address 0x%" PRIx64, esr_addr);
        throw trap_bus_error(addr);
    }
    else if (addr_sregion == ESR_NEIGH_REGION)
    {
        unsigned neigh = (addr & ESR_REGION_NEIGH_MASK) >> ESR_REGION_NEIGH_SHIFT;
        uint64_t esr_addr = addr & ESR_NEIGH_ESR_MASK;
        LOG(DEBUG, "Read from ESR Region Neighborhood at ESR address 0x%" PRIx64, esr_addr);
        if (neigh >= EMU_NEIGH_PER_SHIRE) throw trap_bus_error(addr);
        unsigned idx = EMU_NEIGH_PER_SHIRE * ((shire == IO_SHIRE_ID) ? EMU_IO_SHIRE_SP : shire) + neigh;
        switch(esr_addr)
        {
            case ESR_DUMMY0:
                *ptr = neigh_esrs[idx].dummy0;
                break;
            case ESR_DUMMY1:
                *ptr = neigh_esrs[idx].dummy1;
                break;
            case ESR_MINION_BOOT:
                *ptr = neigh_esrs[idx].minion_boot;
                break;
            case ESR_DUMMY2:
                *ptr = neigh_esrs[idx].dummy2;
                break;
            case ESR_DUMMY3:
                *ptr = neigh_esrs[idx].dummy3;
                break;
            case ESR_VMSPAGESIZE:
                *ptr = neigh_esrs[idx].vmspagesize;
                break;
            case ESR_IPI_REDIRECT_PC:
                *ptr = neigh_esrs[idx].ipi_redirect_pc;
                break;
            case ESR_PMU_CTRL:
                *ptr = neigh_esrs[idx].pmu_ctrl;
                break;
            case ESR_NEIGH_CHICKEN:
                *ptr = neigh_esrs[idx].neigh_chicken;
                break;
            case ESR_ICACHE_ERR_LOG_CTL:
                *ptr = neigh_esrs[idx].icache_err_log_ctl;
                break;
            case ESR_ICACHE_ERR_LOG_INFO:
                *ptr = neigh_esrs[idx].icache_err_log_info;
                break;
            case ESR_ICACHE_ERR_LOG_ADDRESS:
                *ptr = neigh_esrs[idx].icache_err_log_address;
                break;
            case ESR_ICACHE_SBE_DBE_COUNTS:
                *ptr = neigh_esrs[idx].icache_sbe_dbe_counts;
                break;
            case ESR_TEXTURE_CONTROL:
                *ptr = neigh_esrs[idx].texture_control;
                break;
            case ESR_TEXTURE_STATUS:
                *ptr = neigh_esrs[idx].texture_status;
                break;
            case ESR_MPROT:
                *ptr = neigh_esrs[idx].mprot;
                break;
            case ESR_TEXTURE_IMAGE_TABLE_PTR:
                *ptr = neigh_esrs[idx].texture_image_table_ptr;
                break;
            default :
                throw trap_bus_error(addr);
        }
    }
    else if (addr_sregion_ext == ESR_CACHE_REGION)
    {
        uint64_t esr_addr = addr & ESR_CACHE_ESR_MASK;
        LOG(DEBUG, "Read from ESR Region ShireCache at ESR address 0x%" PRIx64, esr_addr);
        unsigned idx = (shire == IO_SHIRE_ID) ? EMU_IO_SHIRE_SP : shire;
        unsigned bank = (addr & ESR_REGION_BANK_MASK) >> ESR_REGION_BANK_SHIFT;
        if (bank >= 4) throw trap_bus_error(addr);
        switch(esr_addr)
        {
            case ESR_SC_L3_SHIRE_SWIZZLE_CTL:
                *ptr = shire_cache_esrs[idx].bank[bank].sc_l3_shire_swizzle_ctl;
                break;
            case ESR_SC_REQQ_CTL:
                *ptr = shire_cache_esrs[idx].bank[bank].sc_reqq_ctl;
                break;
            case ESR_SC_PIPE_CTL:
                *ptr = shire_cache_esrs[idx].bank[bank].sc_pipe_ctl;
                break;
            case ESR_SC_L2_CACHE_CTL:
                *ptr = shire_cache_esrs[idx].bank[bank].sc_l2_cache_ctl;
                break;
            case ESR_SC_L3_CACHE_CTL:
                *ptr = shire_cache_esrs[idx].bank[bank].sc_l3_cache_ctl;
                break;
            case ESR_SC_SCP_CACHE_CTL:
                *ptr = shire_cache_esrs[idx].bank[bank].sc_scp_cache_ctl;
                break;
            case ESR_SC_IDX_COP_SM_PHYSICAL_INDEX:
                *ptr = shire_cache_esrs[idx].bank[bank].sc_idx_cop_sm_physical_index;
                break;
            case ESR_SC_IDX_COP_SM_DATA0:
                *ptr = shire_cache_esrs[idx].bank[bank].sc_idx_cop_sm_data0;
                break;
            case ESR_SC_IDX_COP_SM_DATA1:
                *ptr = shire_cache_esrs[idx].bank[bank].sc_idx_cop_sm_data1;
                break;
            case ESR_SC_IDX_COP_SM_ECC:
                *ptr = shire_cache_esrs[idx].bank[bank].sc_idx_cop_sm_ecc;
                break;
            case ESR_SC_ERR_LOG_CTL:
                *ptr = shire_cache_esrs[idx].bank[bank].sc_err_log_ctl;
                break;
            case ESR_SC_ERR_LOG_INFO:
                *ptr = shire_cache_esrs[idx].bank[bank].sc_err_log_info;
                break;
            case ESR_SC_ERR_LOG_ADDRESS:
                *ptr = shire_cache_esrs[idx].bank[bank].sc_err_log_address;
                break;
            case ESR_SC_SBE_DBE_COUNTS:
                *ptr = shire_cache_esrs[idx].bank[bank].sc_sbe_dbe_counts;
                break;
            case ESR_SC_REQQ_DEBUG_CTL:
                *ptr = shire_cache_esrs[idx].bank[bank].sc_reqq_debug_ctl;
                break;
            case ESR_SC_REQQ_DEBUG0:
                *ptr = shire_cache_esrs[idx].bank[bank].sc_reqq_debug0;
                break;
            case ESR_SC_REQQ_DEBUG1:
                *ptr = shire_cache_esrs[idx].bank[bank].sc_reqq_debug1;
                break;
            case ESR_SC_REQQ_DEBUG2:
                *ptr = shire_cache_esrs[idx].bank[bank].sc_reqq_debug2;
                break;
            case ESR_SC_REQQ_DEBUG3:
                *ptr = shire_cache_esrs[idx].bank[bank].sc_reqq_debug3;
                break;
            case ESR_SC_IDX_COP_SM_CTL:
                *ptr = 4ull << 24; // idx_cop_sm_state = IDLE
                break;
            default:
                throw trap_bus_error(addr);
        }
    }
    else if (addr_sregion_ext == ESR_RBOX_REGION)
    {
        uint64_t esr_addr = addr & ESR_RBOX_ESR_MASK;
        LOG(DEBUG, "Read from ESR Region RBOX at ESR address 0x%" PRIx64, esr_addr);
        if (shire >= EMU_NUM_COMPUTE_SHIRES) throw trap_bus_error(addr);
        switch(esr_addr)
        {
            case ESR_RBOX_CONFIG:
            case ESR_RBOX_IN_BUF_PG:
            case ESR_RBOX_IN_BUF_CFG:
            case ESR_RBOX_OUT_BUF_PG:
            case ESR_RBOX_OUT_BUF_CFG:
            case ESR_RBOX_START:
            case ESR_RBOX_CONSUME:
            case ESR_RBOX_STATUS:
                *ptr = GET_RBOX(shire, 0).read_esr((esr_addr >> 3) & 0x3FFF);
                break;
            default:
                throw trap_bus_error(addr);
        }
    }
    else if (addr_sregion_ext == ESR_SHIRE_REGION)
    {
        uint64_t esr_addr = addr & ESR_SHIRE_ESR_MASK;
        LOG(DEBUG, "Read from ESR Region Shire at ESR address 0x%" PRIx64, esr_addr);
        unsigned idx = (shire == IO_SHIRE_ID) ? EMU_IO_SHIRE_SP : shire;
        switch(esr_addr)
        {
            case ESR_MINION_FEATURE:
                *ptr = shire_other_esrs[idx].minion_feature;
                break;
            case ESR_SHIRE_CONFIG:
                *ptr = shire_other_esrs[idx].shire_config;
                break;
            case ESR_THREAD1_DISABLE:
                *ptr = shire_other_esrs[idx].thread1_disable;
                break;
            case ESR_IPI_REDIRECT_TRIGGER:
                *ptr = 0;
                break;
            case ESR_IPI_REDIRECT_FILTER:
                *ptr = shire_other_esrs[idx].ipi_redirect_filter;
                break;
            case ESR_IPI_TRIGGER:
                *ptr = shire_other_esrs[idx].ipi_trigger;
                break;
            case ESR_IPI_TRIGGER_CLEAR:
            case ESR_FCC_CREDINC_0:
            case ESR_FCC_CREDINC_1:
            case ESR_FCC_CREDINC_2:
            case ESR_FCC_CREDINC_3:
                *ptr = 0;
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
                *ptr = shire_other_esrs[idx].fast_local_barrier[(esr_addr - ESR_FAST_LOCAL_BARRIER0)>>3];
                break;
            case ESR_MTIME_LOCAL_TARGET:
                *ptr = shire_other_esrs[idx].mtime_local_target;
                break;
            case ESR_THREAD0_DISABLE:
                *ptr = shire_other_esrs[idx].thread0_disable;
                break;
            case ESR_SHIRE_PLL_AUTO_CONFIG:
                *ptr = shire_other_esrs[idx].shire_pll_auto_config;
                break;
            case ESR_SHIRE_PLL_CONFIG_DATA_0:
            case ESR_SHIRE_PLL_CONFIG_DATA_1:
            case ESR_SHIRE_PLL_CONFIG_DATA_2:
            case ESR_SHIRE_PLL_CONFIG_DATA_3:
            case ESR_SHIRE_PLL_CONFIG_DATA_4:
            case ESR_SHIRE_PLL_CONFIG_DATA_5:
                *ptr = shire_other_esrs[idx].shire_pll_config_data[(esr_addr - ESR_SHIRE_PLL_CONFIG_DATA_0)>>3];
                break;
            case ESR_SHIRE_COOP_MODE:
                *ptr = read_shire_coop_mode(shire);
                break;
            case ESR_SHIRE_CTRL_CLOCKMUX:
                *ptr = shire_other_esrs[idx].shire_ctrl_clockmux;
                break;
            case ESR_SHIRE_CACHE_RAM_CFG1:
                *ptr = shire_other_esrs[idx].shire_cache_ram_cfg1;
                break;
            case ESR_SHIRE_CACHE_RAM_CFG2:
                *ptr = shire_other_esrs[idx].shire_cache_ram_cfg2;
                break;
            case ESR_SHIRE_CACHE_RAM_CFG3:
                *ptr = shire_other_esrs[idx].shire_cache_ram_cfg3;
                break;
            case ESR_SHIRE_CACHE_RAM_CFG4:
                *ptr = shire_other_esrs[idx].shire_cache_ram_cfg4;
                break;
            case ESR_SHIRE_DLL_AUTO_CONFIG:
                *ptr = shire_other_esrs[idx].shire_dll_auto_config;
                break;
            case ESR_SHIRE_DLL_CONFIG_DATA_0:
                *ptr = shire_other_esrs[idx].shire_dll_config_data_0;
                break;
            case ESR_ICACHE_UPREFETCH:
                *ptr = read_icache_prefetch(PRV_U, shire);
                break;
            case ESR_ICACHE_SPREFETCH:
                *ptr = read_icache_prefetch(PRV_S, shire);
                break;
            case ESR_ICACHE_MPREFETCH:
                *ptr = read_icache_prefetch(PRV_M, shire);
                break;
            default:
                throw trap_bus_error(addr);
        }
    }
    else
    {
        LOG(WARN, "Read from ESR Region UNDEFINED at address 0x%" PRIx64, addr);
        throw trap_bus_error(addr);
    }
}
