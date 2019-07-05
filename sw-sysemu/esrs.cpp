/* vim: set sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#include "emu.h"
#include "emu_gio.h"
#include "esrs.h"
#include "txs.h"

//namespace bemu {


#define ESR_NEIGH_MINION_BOOT_RESET_VAL   0x8000001000
#define ESR_ICACHE_ERR_LOG_CTL_RESET_VAL  0x6
#define ESR_TEXTURE_CONTROL_RESET_VAL     0x5

#define ESR_SC_L3_SHIRE_SWIZZLE_CTL_RESET_VAL   0x0000987765543210ULL
#define ESR_SC_REQQ_CTL_RESET_VAL               0x00038A80
#define ESR_SC_PIPE_CTL_RESET_VAL               0x0000005CFFFFFFFFULL
#define ESR_SC_L2_CACHE_CTL_RESET_VAL           0x02800080007F007FULL
#define ESR_SC_L3_CACHE_CTL_RESET_VAL           0x0300010000FF00FFULL
#define ESR_SC_SCP_CACHE_CTL_RESET_VAL          0x0000028003FF01FFULL
#define ESR_SC_ERR_LOG_CTL_RESET_VAL            0x1FE

#define ESR_FILTER_IPI_RESET_VAL                0xFFFFFFFFFFFFFFFFULL
#define ESR_SHIRE_CONFIG_CONST_RESET_VAL        0x392A000
#define ESR_SHIRE_CACHE_RAM_CFG1_RESET_VAL      0xE800340
#define ESR_SHIRE_CACHE_RAM_CFG2_RESET_VAL      0x03A0
#define ESR_SHIRE_CACHE_RAM_CFG3_RESET_VAL      0x0C8C0323
#define ESR_SHIRE_CACHE_RAM_CFG4_RESET_VAL      0x34000C8C03A0


neigh_esrs_t        neigh_esrs[EMU_NUM_NEIGHS];
shire_cache_esrs_t  shire_cache_esrs[EMU_NUM_SHIRES];
shire_other_esrs_t  shire_other_esrs[EMU_NUM_SHIRES];
broadcast_esrs_t    broadcast_esrs[EMU_NUM_SHIRES];


// ESR region 'base address' field in bits [39:32], and mask to determine if
// address is in the ESR region (bits [39:32])
#define ESR_REGION             0x0100000000ULL

// Broadcast ESR fields
// - ESR privilege (bits [31:30]) is in bits [60:59] in broadcast_data.
// - ESR region (bits [21:17]) is in bits [58:45] in {usm}broadcast
// - ESR address (bits [17:3]) is in bits [54:40] in {usm}broadcast
// - Broadcast destination shires' bitmask is in bits [39:0]
#define ESR_BROADCAST_PROT_MASK              0x1800000000000000ULL
#define ESR_BROADCAST_PROT_SHIFT             59
#define ESR_BROADCAST_ESR_SREGION_MASK       0x07C0000000000000ULL
#define ESR_BROADCAST_ESR_SREGION_MASK_SHIFT 54
#define ESR_BROADCAST_ESR_ADDR_MASK          0x003FFF0000000000ULL
#define ESR_BROADCAST_ESR_ADDR_SHIFT         40
#define ESR_BROADCAST_ESR_SHIRE_MASK         0x000000FFFFFFFFFFULL
#define ESR_BROADCAST_ESR_MAX_SHIRES         ESR_BROADCAST_ESR_ADDR_SHIFT


#define PP(val) \
    (((val) & ESR_REGION_PROT_MASK) >> ESR_REGION_PROT_SHIFT)

#define SHIREID(shire)  (((shire) == EMU_IO_SHIRE_SP) ? IO_SHIRE_ID : (shire))
#define MINION(hart)    ((hart) / EMU_THREADS_PER_MINION)
#define THREAD(hart)    ((hart) % EMU_THREADS_PER_MINION)


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
extern uint32_t current_thread;


static uint64_t legalize_esr_address(uint64_t addr)
{
    uint64_t shire = addr & ESR_REGION_SHIRE_MASK;
    if (shire == ESR_REGION_SHIRE_MASK) {
        shire = (current_thread / EMU_THREADS_PER_SHIRE) << ESR_REGION_SHIRE_SHIFT;
    }
    if (shire == (IO_SHIRE_ID << ESR_REGION_SHIRE_SHIFT)) {
        shire = EMU_IO_SHIRE_SP << ESR_REGION_SHIRE_SHIFT;
    }
    return (addr & ~ESR_REGION_SHIRE_MASK) | shire;
}


static uint64_t decode_broadcast_esr_value(uint64_t value)
{
    uint64_t p = (value & ESR_BROADCAST_PROT_MASK) >> ESR_BROADCAST_PROT_SHIFT;
    uint64_t x = (value & ESR_BROADCAST_ESR_SREGION_MASK) >> ESR_BROADCAST_ESR_SREGION_MASK_SHIFT;
    uint64_t r = (value & ESR_BROADCAST_ESR_ADDR_MASK) >> ESR_BROADCAST_ESR_ADDR_SHIFT;
    return ESR_REGION
            | (p << ESR_REGION_PROT_SHIFT)
            | (x << ESR_SREGION_EXT_SHIFT)
            | (r << 3);
}


void neigh_esrs_t::reset()
{
    ipi_redirect_pc = 0;
    minion_boot = ESR_NEIGH_MINION_BOOT_RESET_VAL;
    texture_image_table_ptr = 0;
    texture_control = ESR_TEXTURE_CONTROL_RESET_VAL;
    texture_status = 0;
    icache_err_log_ctl = ESR_ICACHE_ERR_LOG_CTL_RESET_VAL;
    mprot = 0;
    neigh_chicken = 0;
    vmspagesize = 0;
    pmu_ctrl = 0;
}


void shire_cache_esrs_t::reset()
{
    for (int i = 0; i < 4; ++i) {
        bank[i].sc_l2_cache_ctl = ESR_SC_L2_CACHE_CTL_RESET_VAL;
        bank[i].sc_l3_cache_ctl = ESR_SC_L3_CACHE_CTL_RESET_VAL;
        bank[i].sc_l3_shire_swizzle_ctl = ESR_SC_L3_SHIRE_SWIZZLE_CTL_RESET_VAL;
        bank[i].sc_pipe_ctl = ESR_SC_PIPE_CTL_RESET_VAL;
        bank[i].sc_scp_cache_ctl = ESR_SC_SCP_CACHE_CTL_RESET_VAL;
        bank[i].sc_reqq_ctl = ESR_SC_REQQ_CTL_RESET_VAL;
        bank[i].sc_err_log_ctl = ESR_SC_ERR_LOG_CTL_RESET_VAL;
    }
}


void shire_other_esrs_t::reset(unsigned shire)
{
    for (int i = 0; i < 32; ++i) {
        fast_local_barrier[i] = 0;
    }
    ipi_redirect_filter = ESR_FILTER_IPI_RESET_VAL;
    ipi_trigger = 0;
    shire_cache_ram_cfg1 = ESR_SHIRE_CACHE_RAM_CFG1_RESET_VAL;
    shire_cache_ram_cfg3 = ESR_SHIRE_CACHE_RAM_CFG3_RESET_VAL;
    shire_cache_ram_cfg4 = ESR_SHIRE_CACHE_RAM_CFG4_RESET_VAL;
    shire_cache_ram_cfg2 = ESR_SHIRE_CACHE_RAM_CFG2_RESET_VAL;
    shire_config = ESR_SHIRE_CONFIG_CONST_RESET_VAL | shire;
    thread0_disable = 0xffffffff;
    thread1_disable = 0xffffffff;
    mtime_local_target = 0;
    shire_power_ctrl = 0;
    power_ctrl_neigh_nsleepin = 0;
    power_ctrl_neigh_isolation = 0;
    shire_pll_auto_config = 0;
    shire_dll_auto_config = 0;
    if (shire == IO_SHIRE_ID || shire == EMU_IO_SHIRE_SP) {
        minion_feature = 0x3b;
    } else {
        minion_feature = 0;
    }
    shire_ctrl_clockmux = 0;
}


uint64_t esr_read(uint64_t addr)
{
    // Broadcast is special...
    switch (addr) {
    case ESR_BROADCAST_DATA:
    case ESR_UBROADCAST:
    case ESR_SBROADCAST:
    case ESR_MBROADCAST:
        return 0;
    default:
        break;
    }

    // Redirect local shire requests to the corresponding shire
    uint64_t addr2 = legalize_esr_address(addr);

    // parse address
    unsigned shire = ((addr2 & ESR_REGION_SHIRE_MASK) >> ESR_REGION_SHIRE_SHIFT);

    if (shire == EMU_IO_SHIRE_SP) {
        uint64_t esr = addr2 & ESR_IOSHIRE_ESR_MASK;
#ifdef SYS_EMU
        switch (esr) {
        case ESR_PU_RVTIM_MTIME:
            return sys_emu::get_pu_rvtimer().read_mtime();
        case ESR_PU_RVTIM_MTIMECMP:
            return sys_emu::get_pu_rvtimer().read_mtimecmp();
        }
#endif
        LOG(WARN, "Read unknown IOshire ESR 0x%" PRIx64, esr);
        throw bemu::memory_error(addr);
    }

    if (shire >= EMU_NUM_MINION_SHIRES) {
        LOG(WARN, "Read illegal ESR S%u:0x%llx", shire, addr2 & ~ESR_REGION_SHIRE_MASK);
        throw bemu::memory_error(addr);
    }

    uint64_t sregion = addr2 & ESR_SREGION_MASK;

    if (sregion == ESR_HART_REGION) {
        uint64_t esr = addr2 & ESR_HART_ESR_MASK;
        unsigned hart = (addr2 & ESR_REGION_HART_MASK) >> ESR_REGION_HART_SHIFT;
        LOG(WARN, "Read unknown hart ESR S%u:M%u:T%u:0x%" PRIx64,
            SHIREID(shire), MINION(hart), THREAD(hart), esr);
        throw bemu::memory_error(addr);
    }

    if (sregion == ESR_NEIGH_REGION) {
        unsigned neigh = (addr2 & ESR_REGION_NEIGH_MASK) >> ESR_REGION_NEIGH_SHIFT;
        uint64_t esr = addr2 & ESR_NEIGH_ESR_MASK;
        if (neigh >= EMU_NEIGH_PER_SHIRE) {
            LOG(WARN, "Read illegal neigh ESR S%u:N%u:0x%" PRIx64, shire, neigh, esr);
            throw bemu::memory_error(addr);
        }
        unsigned pos = neigh + EMU_NEIGH_PER_SHIRE * shire;
        switch (esr) {
        case ESR_DUMMY0:
            return neigh_esrs[pos].dummy0;
        case ESR_DUMMY1:
            return 0;
        case ESR_MINION_BOOT:
            return neigh_esrs[pos].minion_boot;
        case ESR_DUMMY2:
            return neigh_esrs[pos].dummy2;
        case ESR_DUMMY3:
            return 0;
        case ESR_VMSPAGESIZE:
            return neigh_esrs[pos].vmspagesize;
        case ESR_IPI_REDIRECT_PC:
            return neigh_esrs[pos].ipi_redirect_pc;
        case ESR_PMU_CTRL:
            return neigh_esrs[pos].pmu_ctrl;
        case ESR_NEIGH_CHICKEN:
            return neigh_esrs[pos].neigh_chicken;
        case ESR_ICACHE_ERR_LOG_CTL:
            return neigh_esrs[pos].icache_err_log_ctl;
        case ESR_ICACHE_ERR_LOG_INFO:
            return neigh_esrs[pos].icache_err_log_info;
        case ESR_ICACHE_ERR_LOG_ADDRESS:
            return 0;
        case ESR_ICACHE_SBE_DBE_COUNTS:
            return neigh_esrs[pos].icache_sbe_dbe_counts;
        case ESR_TEXTURE_CONTROL:
            return neigh_esrs[pos].texture_control;
        case ESR_TEXTURE_STATUS:
            return neigh_esrs[pos].texture_status;
        case ESR_MPROT:
            return neigh_esrs[pos].mprot;
        case ESR_TEXTURE_IMAGE_TABLE_PTR:
            return neigh_esrs[pos].texture_image_table_ptr;
        }
        LOG(WARN, "Read unknown neigh ESR S%u:N%u:0x%" PRIx64, shire, neigh, esr);
        throw bemu::memory_error(addr);
    }

    uint64_t sregion_extra = addr2 & ESR_SREGION_EXT_MASK;

    if (sregion_extra == ESR_CACHE_REGION) {
        uint64_t esr = addr2 & ESR_CACHE_ESR_MASK;
        unsigned bnk = (addr2 & ESR_REGION_BANK_MASK) >> ESR_REGION_BANK_SHIFT;
        if (bnk >= 4) {
            LOG(WARN, "Read illegal shire_cache ESR S%u:B%u:0x%" PRIx64, shire, bnk, esr);
            throw bemu::memory_error(addr);
        }
        switch (esr) {
        case ESR_SC_L3_SHIRE_SWIZZLE_CTL:
            return shire_cache_esrs[shire].bank[bnk].sc_l3_shire_swizzle_ctl;
        case ESR_SC_REQQ_CTL:
            return shire_cache_esrs[shire].bank[bnk].sc_reqq_ctl;
        case ESR_SC_PIPE_CTL:
            return shire_cache_esrs[shire].bank[bnk].sc_pipe_ctl;
        case ESR_SC_L2_CACHE_CTL:
            return shire_cache_esrs[shire].bank[bnk].sc_l2_cache_ctl;
        case ESR_SC_L3_CACHE_CTL:
            return shire_cache_esrs[shire].bank[bnk].sc_l3_cache_ctl;
        case ESR_SC_SCP_CACHE_CTL:
            return shire_cache_esrs[shire].bank[bnk].sc_scp_cache_ctl;
        case ESR_SC_IDX_COP_SM_PHYSICAL_INDEX:
            return shire_cache_esrs[shire].bank[bnk].sc_idx_cop_sm_physical_index;
        case ESR_SC_IDX_COP_SM_DATA0:
            return shire_cache_esrs[shire].bank[bnk].sc_idx_cop_sm_data0;
        case ESR_SC_IDX_COP_SM_DATA1:
            return shire_cache_esrs[shire].bank[bnk].sc_idx_cop_sm_data1;
        case ESR_SC_IDX_COP_SM_ECC:
            return shire_cache_esrs[shire].bank[bnk].sc_idx_cop_sm_ecc;
        case ESR_SC_ERR_LOG_CTL:
            return shire_cache_esrs[shire].bank[bnk].sc_err_log_ctl;
        case ESR_SC_ERR_LOG_INFO:
            return shire_cache_esrs[shire].bank[bnk].sc_err_log_info;
        case ESR_SC_ERR_LOG_ADDRESS:
            return 0;
        case ESR_SC_SBE_DBE_COUNTS:
            return shire_cache_esrs[shire].bank[bnk].sc_sbe_dbe_counts;
        case ESR_SC_REQQ_DEBUG_CTL:
            return shire_cache_esrs[shire].bank[bnk].sc_reqq_debug_ctl;
        case ESR_SC_REQQ_DEBUG0:
        case ESR_SC_REQQ_DEBUG1:
        case ESR_SC_REQQ_DEBUG2:
        case ESR_SC_REQQ_DEBUG3:
            return 0;
        case ESR_SC_IDX_COP_SM_CTL:
            return 4ull << 24; // idx_cop_sm_state = IDLE
        }
        LOG(WARN, "Read unknown shire_cache ESR S%u:B%u:0x%" PRIx64, shire, bnk, esr);
        throw bemu::memory_error(addr);
    }

    if (sregion_extra == ESR_RBOX_REGION) {
        uint64_t esr = addr2 & ESR_RBOX_ESR_MASK;
        if (shire >= EMU_NUM_COMPUTE_SHIRES) {
            LOG(WARN, "Read illegal rbox ESR S%u:0x%" PRIx64, shire, esr);
            throw bemu::memory_error(addr);
        }
        switch (esr) {
        case ESR_RBOX_CONFIG:
        case ESR_RBOX_IN_BUF_PG:
        case ESR_RBOX_IN_BUF_CFG:
        case ESR_RBOX_OUT_BUF_PG:
        case ESR_RBOX_OUT_BUF_CFG:
        case ESR_RBOX_START:
        case ESR_RBOX_CONSUME:
        case ESR_RBOX_STATUS:
            return GET_RBOX(shire, 0).read_esr((esr >> 3) & 0x3FFF);
        }
        LOG(WARN, "Read unknown rbox ESR S%u:0x%" PRIx64, shire, esr);
        throw bemu::memory_error(addr);
    }

    if (sregion_extra == ESR_SHIRE_REGION) {
        uint64_t esr = addr2 & ESR_SHIRE_ESR_MASK;
        switch (esr) {
        case ESR_MINION_FEATURE:
            return shire_other_esrs[shire].minion_feature;
        case ESR_SHIRE_CONFIG:
            return shire_other_esrs[shire].shire_config;
        case ESR_THREAD1_DISABLE:
            return shire_other_esrs[shire].thread1_disable;
        case ESR_SHIRE_CACHE_BUILD_CONFIG:
            return 0x2040040404040400ull;
        case ESR_SHIRE_CACHE_REVISION_ID:
            return 0x0000000900000001ull;
        case ESR_IPI_REDIRECT_TRIGGER:
            return 0;
        case ESR_IPI_REDIRECT_FILTER:
            return shire_other_esrs[shire].ipi_redirect_filter;
        case ESR_IPI_TRIGGER:
            return shire_other_esrs[shire].ipi_trigger;
        case ESR_IPI_TRIGGER_CLEAR:
        case ESR_FCC_CREDINC_0:
        case ESR_FCC_CREDINC_1:
        case ESR_FCC_CREDINC_2:
        case ESR_FCC_CREDINC_3:
            return 0;
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
            return shire_other_esrs[shire].fast_local_barrier[(esr - ESR_FAST_LOCAL_BARRIER0)>>3];
        case ESR_MTIME_LOCAL_TARGET:
            return shire_other_esrs[shire].mtime_local_target;
        case ESR_SHIRE_POWER_CTRL:
            return shire_other_esrs[shire].shire_power_ctrl;
        case ESR_POWER_CTRL_NEIGH_NSLEEPIN:
            return shire_other_esrs[shire].power_ctrl_neigh_nsleepin;
        case ESR_POWER_CTRL_NEIGH_ISOLATION:
            return shire_other_esrs[shire].power_ctrl_neigh_isolation;
        case ESR_POWER_CTRL_NEIGH_NSLEEPOUT:
            return 0;
        case ESR_THREAD0_DISABLE:
            return shire_other_esrs[shire].thread0_disable;
        case ESR_SHIRE_ERROR_LOG:
            return 0;
        case ESR_SHIRE_PLL_AUTO_CONFIG:
            return shire_other_esrs[shire].shire_pll_auto_config;
        case ESR_SHIRE_PLL_CONFIG_DATA_0:
        case ESR_SHIRE_PLL_CONFIG_DATA_1:
        case ESR_SHIRE_PLL_CONFIG_DATA_2:
        case ESR_SHIRE_PLL_CONFIG_DATA_3:
        case ESR_SHIRE_PLL_CONFIG_DATA_4:
        case ESR_SHIRE_PLL_CONFIG_DATA_5:
            return shire_other_esrs[shire].shire_pll_config_data[(esr - ESR_SHIRE_PLL_CONFIG_DATA_0)>>3];
        case ESR_SHIRE_COOP_MODE:
            return read_shire_coop_mode(shire);
        case ESR_SHIRE_CTRL_CLOCKMUX:
            return shire_other_esrs[shire].shire_ctrl_clockmux;
        case ESR_SHIRE_CACHE_RAM_CFG1:
            return shire_other_esrs[shire].shire_cache_ram_cfg1;
        case ESR_SHIRE_CACHE_RAM_CFG2:
            return shire_other_esrs[shire].shire_cache_ram_cfg2;
        case ESR_SHIRE_CACHE_RAM_CFG3:
            return shire_other_esrs[shire].shire_cache_ram_cfg3;
        case ESR_SHIRE_CACHE_RAM_CFG4:
            return shire_other_esrs[shire].shire_cache_ram_cfg4;
        case ESR_SHIRE_DLL_AUTO_CONFIG:
            return shire_other_esrs[shire].shire_dll_auto_config;
        case ESR_SHIRE_DLL_CONFIG_DATA_0:
            return shire_other_esrs[shire].shire_dll_config_data_0;
        case ESR_ICACHE_UPREFETCH:
            return read_icache_prefetch(PRV_U, shire);
        case ESR_ICACHE_SPREFETCH:
            return read_icache_prefetch(PRV_S, shire);
        case ESR_ICACHE_MPREFETCH:
            return read_icache_prefetch(PRV_M, shire);
        }
        LOG(WARN, "Read unknown shire_other ESR S%u:0x%" PRIx64, shire, esr);
        throw bemu::memory_error(addr);
    }

    LOG(WARN, "Read illegal ESR 0x%" PRIx64, addr);
    throw bemu::memory_error(addr);
}


void esr_write(uint64_t addr, uint64_t value)
{
    uint64_t addr2;
    uint64_t mask;
    unsigned shire = current_thread / EMU_THREADS_PER_SHIRE;

    // Broadcast is special...
    switch (addr) {
    case ESR_BROADCAST_DATA:
        broadcast_esrs[shire].data = value;
        return;
    case ESR_UBROADCAST:
    case ESR_SBROADCAST:
    case ESR_MBROADCAST:
        addr2 = decode_broadcast_esr_value(value);
        if ((PP(addr2) == 2) || (PP(addr2) > PP(addr))) {
            LOG(WARN, "Request %cbroadcast to %c-mode ESR",
                "uhsm"[PP(addr)], "UHSM"[PP(addr2)]);
            throw bemu::memory_error(addr);
        }
        mask = value & ESR_BROADCAST_ESR_SHIRE_MASK;
        while (mask) {
            if (mask & 1)
                esr_write(addr2, broadcast_esrs[shire].data);
            addr2 += 1ull << ESR_REGION_SHIRE_SHIFT;
            mask >>= 1;
        }
        return;
    }

    // Redirect local shire requests to the corresponding shire
    addr2 = legalize_esr_address(addr);

    // parse address
    shire = ((addr2 & ESR_REGION_SHIRE_MASK) >> ESR_REGION_SHIRE_SHIFT);

    if (shire == EMU_IO_SHIRE_SP) {
        uint64_t esr = addr2 & ESR_IOSHIRE_ESR_MASK;
#ifdef SYS_EMU
        switch (esr) {
        case ESR_PU_RVTIM_MTIME:
            sys_emu::get_pu_rvtimer().write_mtime(value);
            return;
        case ESR_PU_RVTIM_MTIMECMP:
            sys_emu::get_pu_rvtimer().write_mtimecmp(value);
            return;
        }
#endif
        LOG(WARN, "Write unknown IOshire ESR 0x%" PRIx64, esr);
        throw bemu::memory_error(addr);
    }

    if (shire >= EMU_NUM_MINION_SHIRES) {
        LOG(WARN, "Write illegal ESR S%u:0x%llx", shire, addr2 & ~ESR_REGION_SHIRE_MASK);
        throw bemu::memory_error(addr);
    }

    uint64_t sregion = addr2 & ESR_SREGION_MASK;

    if (sregion == ESR_HART_REGION) {
        uint64_t esr = addr2 & ESR_HART_ESR_MASK;
        unsigned hart = (addr2 & ESR_REGION_HART_MASK) >> ESR_REGION_HART_SHIFT;
        if ((esr >= ESR_PORT0) && (esr <= ESR_PORT3)) {
            unsigned hartid = hart + shire * EMU_THREADS_PER_SHIRE;
            unsigned port = (esr - ESR_PORT0) >> 3;
            if (get_msg_port_write_width(hartid, port) > 8)
                throw std::runtime_error("Write to port with incompatible size");
            uint64_t tmp = value;
            write_msg_port_data(hartid, port, (uint32_t*)&tmp, 0);
            return;
        }
        LOG(WARN, "Write unknown hart ESR S%u:M%u:T%u:0x%" PRIx64,
            SHIREID(shire), MINION(hart), THREAD(hart), esr);
        throw bemu::memory_error(addr);
    }

    if (sregion == ESR_NEIGH_REGION) {
        unsigned neigh = (addr2 & ESR_REGION_NEIGH_MASK) >> ESR_REGION_NEIGH_SHIFT;
        uint64_t esr = addr2 & ESR_NEIGH_ESR_MASK;
        unsigned frst = neigh + EMU_NEIGH_PER_SHIRE * shire;
        unsigned last = frst + 1;
        if (neigh == ESR_REGION_NEIGH_BROADCAST) {
            frst = EMU_NEIGH_PER_SHIRE * shire;
            last = frst + EMU_NEIGH_PER_SHIRE;
        } else if (neigh >= EMU_NEIGH_PER_SHIRE) {
            LOG(WARN, "Write illegal neigh ESR S%u:N%u:0x%" PRIx64, shire, neigh, esr);
            throw bemu::memory_error(addr);
        }
        for (unsigned pos = frst; pos < last; ++pos) {
            unsigned tbox_id;
            switch (esr) {
            case ESR_DUMMY0:
                neigh_esrs[pos].dummy0 = uint32_t(value & 0xffffffff);
                break;
            case ESR_MINION_BOOT:
                neigh_esrs[pos].minion_boot = value & VA_M;
                break;
            case ESR_DUMMY2:
                neigh_esrs[pos].dummy2 = bool(value & 1);
                break;
            case ESR_VMSPAGESIZE:
                neigh_esrs[pos].vmspagesize = value & 0x3;
                break;
            case ESR_IPI_REDIRECT_PC:
                neigh_esrs[pos].ipi_redirect_pc = value & VA_M;
                break;
            case ESR_PMU_CTRL:
                neigh_esrs[pos].pmu_ctrl = bool(value & 1);
                break;
            case ESR_NEIGH_CHICKEN:
                neigh_esrs[pos].neigh_chicken = uint8_t(value & 0xff);
                break;
            case ESR_ICACHE_ERR_LOG_CTL:
                neigh_esrs[pos].icache_err_log_ctl = uint8_t(value & 0xf);
                break;
            case ESR_ICACHE_ERR_LOG_INFO:
                neigh_esrs[pos].icache_err_log_info = value & 0x0010ff000003ffffull;
                break;
            case ESR_ICACHE_SBE_DBE_COUNTS:
                neigh_esrs[pos].icache_sbe_dbe_counts = uint16_t(value & 0x7ff);
                break;
            case ESR_TEXTURE_CONTROL:
                neigh_esrs[pos].texture_control = uint16_t(value & 0xff80);
                break;
            case ESR_MPROT:
                neigh_esrs[pos].mprot = uint8_t(value & 0x7);
                break;
            case ESR_TEXTURE_IMAGE_TABLE_PTR:
                value &= VA_M;
                neigh_esrs[pos].texture_image_table_ptr = value;
                tbox_id = tbox_id_from_thread(current_thread);
                GET_TBOX(shire, tbox_id).set_image_table_address(value);
                break;
            default:
                LOG(WARN, "Write unknown neigh ESR S%u:N%u:0x%" PRIx64, shire, neigh, esr);
                throw bemu::memory_error(addr);
            }
        }
        return;
    }

    uint64_t sregion_extra = addr2 & ESR_SREGION_EXT_MASK;

    if (sregion_extra == ESR_CACHE_REGION) {
        uint64_t esr = addr2 & ESR_CACHE_ESR_MASK;
        unsigned bnk = (addr2 & ESR_REGION_BANK_MASK) >> ESR_REGION_BANK_SHIFT;
        unsigned frst = bnk;
        unsigned last = frst + 1;
        if (bnk == (ESR_REGION_BANK_MASK >> ESR_REGION_BANK_SHIFT)) {
            frst = 0;
            last = frst + 4;
        } else if (bnk >= 4) {
            LOG(WARN, "Write illegal shire_cache ESR S%u:B%u:0x%" PRIx64, shire, bnk, esr);
            throw bemu::memory_error(addr);
        }
        for (unsigned b = frst; b < last; ++b) {
            switch (esr) {
            case ESR_SC_L3_SHIRE_SWIZZLE_CTL:
                shire_cache_esrs[shire].bank[b].sc_l3_shire_swizzle_ctl = value;
                break;
            case ESR_SC_REQQ_CTL:
                shire_cache_esrs[shire].bank[b].sc_reqq_ctl = value;
                break;
            case ESR_SC_PIPE_CTL:
                shire_cache_esrs[shire].bank[b].sc_pipe_ctl = value;
                break;
            case ESR_SC_L2_CACHE_CTL:
                shire_cache_esrs[shire].bank[b].sc_l2_cache_ctl = value;
                break;
            case ESR_SC_L3_CACHE_CTL:
                shire_cache_esrs[shire].bank[b].sc_l3_cache_ctl = value;
                break;
            case ESR_SC_SCP_CACHE_CTL:
                shire_cache_esrs[shire].bank[b].sc_scp_cache_ctl = value;
                break;
            case ESR_SC_IDX_COP_SM_CTL:
                // shire_cache_esrs[shire].bank[b].sc_idx_cop_sm_ctl = value;
                break;
            case ESR_SC_IDX_COP_SM_PHYSICAL_INDEX:
                shire_cache_esrs[shire].bank[b].sc_idx_cop_sm_physical_index = value;
                break;
            case ESR_SC_IDX_COP_SM_DATA0:
                shire_cache_esrs[shire].bank[b].sc_idx_cop_sm_data0 = value;
                break;
            case ESR_SC_IDX_COP_SM_DATA1:
                shire_cache_esrs[shire].bank[b].sc_idx_cop_sm_data1 = value;
                break;
            case ESR_SC_IDX_COP_SM_ECC:
                shire_cache_esrs[shire].bank[b].sc_idx_cop_sm_ecc = value;
                break;
            case ESR_SC_ERR_LOG_CTL:
                shire_cache_esrs[shire].bank[b].sc_err_log_ctl = value;
                break;
            case ESR_SC_ERR_LOG_INFO:
                shire_cache_esrs[shire].bank[b].sc_err_log_info = value;
                break;
            case ESR_SC_SBE_DBE_COUNTS:
                shire_cache_esrs[shire].bank[b].sc_sbe_dbe_counts = value;
                break;
            case ESR_SC_REQQ_DEBUG_CTL:
                shire_cache_esrs[shire].bank[b].sc_reqq_debug_ctl = value;
                break;
            default:
                LOG(WARN, "Write unknown shire_cache ESR S%u:B%u:0x%" PRIx64, shire, bnk, esr);
                throw bemu::memory_error(addr);
            }
        }
        return;
    }

    if (sregion_extra == ESR_RBOX_REGION) {
        uint64_t esr = addr2 & ESR_RBOX_ESR_MASK;
        if (shire >= EMU_NUM_COMPUTE_SHIRES) {
            LOG(WARN, "Write illegal rbox ESR S%u:0x%" PRIx64, shire, esr);
            throw bemu::memory_error(addr);
        }
        switch (esr) {
        case ESR_RBOX_CONFIG:
        case ESR_RBOX_IN_BUF_PG:
        case ESR_RBOX_IN_BUF_CFG:
        case ESR_RBOX_OUT_BUF_PG:
        case ESR_RBOX_OUT_BUF_CFG:
        case ESR_RBOX_START:
        case ESR_RBOX_CONSUME:
            GET_RBOX(shire, 0).write_esr((esr >> 3) & 0x3FFF, value);
            return;
        }
        LOG(WARN, "Write unknown rbox ESR S%u:0x%" PRIx64, shire, esr);
        throw bemu::memory_error(addr);
    }

    if (sregion_extra == ESR_SHIRE_REGION) {
        uint64_t esr = addr2 & ESR_SHIRE_ESR_MASK;
        switch (esr) {
        case ESR_MINION_FEATURE:
            shire_other_esrs[shire].minion_feature = uint8_t(value & 0x3f);
            return;
        case ESR_SHIRE_CONFIG:
            shire_other_esrs[shire].shire_config = uint32_t(value & 0x3ffffff);
            return;
        case ESR_THREAD1_DISABLE:
            shire_other_esrs[shire].thread1_disable = uint32_t(value);
            return;
        case ESR_IPI_REDIRECT_TRIGGER:
            LOG_ALL_MINIONS(DEBUG, "%s", "Sending IPI_REDIRECT");
            sys_emu::send_ipi_redirect_to_threads(shire, value);
            return;
        case ESR_IPI_REDIRECT_FILTER:
            shire_other_esrs[shire].ipi_redirect_filter = value;
            return;
        case ESR_IPI_TRIGGER:
            LOG_ALL_MINIONS(DEBUG, "%s", "Sending IPI");
            shire_other_esrs[shire].ipi_trigger = value;
            sys_emu::raise_software_interrupt(shire, value);
            return;
        case ESR_IPI_TRIGGER_CLEAR:
            sys_emu::clear_software_interrupt(shire, value);
            return;
        case ESR_FCC_CREDINC_0:
            LOG_ALL_MINIONS(DEBUG, "Write to FCC0 shire %d value %016" PRIx64, shire, value);
            fcc_inc(0, shire, value, 0);
            sys_emu::fcc_to_threads(shire, 0, value, 0);
            return;
        case ESR_FCC_CREDINC_1:
            LOG_ALL_MINIONS(DEBUG, "Write to FCC1 shire %d value %016" PRIx64, shire, value);
            fcc_inc(0, shire, value, 1);
            sys_emu::fcc_to_threads(shire, 0, value, 1);
            return;
        case ESR_FCC_CREDINC_2:
            LOG_ALL_MINIONS(DEBUG, "Write to FCC2 shire %d value %016" PRIx64, shire, value);
            fcc_inc(1, shire, value, 0);
            sys_emu::fcc_to_threads(shire, 1, value, 0);
            return;
        case ESR_FCC_CREDINC_3:
            LOG_ALL_MINIONS(DEBUG, "Write to FCC3 shire %d value %016" PRIx64, shire, value);
            fcc_inc(1, shire, value, 1);
            sys_emu::fcc_to_threads(shire, 1, value, 1);
            return;
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
            shire_other_esrs[shire].fast_local_barrier[(esr - ESR_FAST_LOCAL_BARRIER0)>>3] = uint8_t(value);
            return;
        case ESR_MTIME_LOCAL_TARGET:
            shire_other_esrs[shire].mtime_local_target = value;
            return;
        case ESR_SHIRE_POWER_CTRL:
            shire_other_esrs[shire].shire_power_ctrl = uint16_t(value & 0xfff);
            return;
        case ESR_POWER_CTRL_NEIGH_NSLEEPIN:
            shire_other_esrs[shire].power_ctrl_neigh_nsleepin = uint32_t(value);
            return;
        case ESR_POWER_CTRL_NEIGH_ISOLATION:
            shire_other_esrs[shire].power_ctrl_neigh_isolation = uint32_t(value);
            return;
        case ESR_THREAD0_DISABLE:
            shire_other_esrs[shire].thread0_disable = uint32_t(value);
            return;
        case ESR_SHIRE_PLL_AUTO_CONFIG:
            shire_other_esrs[shire].shire_pll_auto_config = uint32_t(value & 0x1ffff);
            return;
        case ESR_SHIRE_PLL_CONFIG_DATA_0:
        case ESR_SHIRE_PLL_CONFIG_DATA_1:
        case ESR_SHIRE_PLL_CONFIG_DATA_2:
        case ESR_SHIRE_PLL_CONFIG_DATA_3:
        case ESR_SHIRE_PLL_CONFIG_DATA_4:
        case ESR_SHIRE_PLL_CONFIG_DATA_5:
            shire_other_esrs[shire].shire_pll_config_data[(esr - ESR_SHIRE_PLL_CONFIG_DATA_0)>>3] = value;
            return;
        case ESR_SHIRE_COOP_MODE:
            write_shire_coop_mode(shire, value);
            return;
        case ESR_SHIRE_CTRL_CLOCKMUX:
            shire_other_esrs[shire].shire_ctrl_clockmux = uint8_t(value & 0x4f);
            return;
        case ESR_SHIRE_CACHE_RAM_CFG1:
            shire_other_esrs[shire].shire_cache_ram_cfg1 = value & 0xfffffffffull;
            return;
        case ESR_SHIRE_CACHE_RAM_CFG2:
            shire_other_esrs[shire].shire_cache_ram_cfg2 = uint32_t(value & 0x3ffff);
            return;
        case ESR_SHIRE_CACHE_RAM_CFG3:
            shire_other_esrs[shire].shire_cache_ram_cfg3 = value & 0xfffffffffull;
            return;
        case ESR_SHIRE_CACHE_RAM_CFG4:
            shire_other_esrs[shire].shire_cache_ram_cfg4 = value & 0xfffffffffull;
            return;
        case ESR_SHIRE_DLL_AUTO_CONFIG:
            shire_other_esrs[shire].shire_dll_auto_config = uint16_t(value & 0x3fff);
            return;
        case ESR_SHIRE_DLL_CONFIG_DATA_0:
            shire_other_esrs[shire].shire_dll_config_data_0 = value;
            return;
        case ESR_ICACHE_UPREFETCH:
            write_icache_prefetch(PRV_U, shire, value);
            return;
        case ESR_ICACHE_SPREFETCH:
            write_icache_prefetch(PRV_S, shire, value);
            return;
        case ESR_ICACHE_MPREFETCH:
            write_icache_prefetch(PRV_M, shire, value);
            return;
        }
        LOG(WARN, "Write unknown shire_other ESR S%u:0x%" PRIx64, shire, esr);
        throw bemu::memory_error(addr);
    }

    LOG(WARN, "Write illegal ESR 0x%" PRIx64, addr);
    throw bemu::memory_error(addr);
}


//} // namespace bemu
