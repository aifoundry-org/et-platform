/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#include <cassert>
#include <stdexcept>

#include "emu_gio.h"
#include "log.h"
#include "mmu.h"
#include "traps.h"
#include "utility.h"
#include "memmap.h"

// FIXME: Replace with "state.h"
extern uint32_t current_thread;
extern uint64_t csr_satp[EMU_NUM_THREADS];
extern uint64_t csr_mstatus[EMU_NUM_THREADS];
extern uint64_t csr_tdata1[EMU_NUM_THREADS];
extern uint64_t csr_tdata2[EMU_NUM_THREADS];
extern uint8_t  csr_prv[EMU_NUM_THREADS];
extern bool break_on_load[EMU_NUM_THREADS];
extern bool break_on_store[EMU_NUM_THREADS];
extern bool break_on_fetch[EMU_NUM_THREADS];


#define LOG_MEMWRITE(size, addr, value) do { \
   LOG(DEBUG, "\tMEM" #size "[0x%" PRIx64 "] = 0x%" PRIx ##size , addr, value); \
} while (0)


#define LOG_MEMREAD(size, addr, value) do { \
   LOG(DEBUG, "\tMEM" #size "[0x%" PRIx64 "] : 0x%" PRIx ##size , addr, value); \
} while (0)


//------------------------------------------------------------------------------
// Exceptions

static inline int effective_execution_mode(mem_access_type macc)
{
    // Read mstatus
    const uint64_t mstatus = csr_mstatus[current_thread];
    const int      mprv    = (mstatus >> MSTATUS_MPRV) & 0x1;
    const int      mpp     = (mstatus >> MSTATUS_MPP ) & 0x3;
    return (macc == Mem_Access_Fetch || macc == Mem_Access_PTW)
            ? csr_prv[current_thread]
            : (mprv ? mpp : csr_prv[current_thread]);
}


static void throw_page_fault(uint64_t addr, mem_access_type macc)
{
    switch (macc)
    {
    case Mem_Access_Load:
    case Mem_Access_TxLoad:
    case Mem_Access_Prefetch:
        throw trap_load_page_fault(addr);
    case Mem_Access_Store:
    case Mem_Access_TxStore:
    case Mem_Access_AtomicL:
    case Mem_Access_AtomicG:
    case Mem_Access_CacheOp:
        throw trap_store_page_fault(addr);
    case Mem_Access_Fetch:
        throw trap_instruction_page_fault(addr);
    case Mem_Access_PTW:
        assert(0);
    }
}


static void throw_access_fault(uint64_t addr, mem_access_type macc)
{
    switch (macc)
    {
    case Mem_Access_Load:
    case Mem_Access_TxLoad:
    case Mem_Access_Prefetch:
        throw trap_load_access_fault(addr);
    case Mem_Access_Store:
    case Mem_Access_TxStore:
    case Mem_Access_AtomicL:
    case Mem_Access_AtomicG:
    case Mem_Access_CacheOp:
        throw trap_store_access_fault(addr);
    case Mem_Access_Fetch:
        throw trap_instruction_access_fault(addr);
    case Mem_Access_PTW:
        assert(0);
    }
}


//------------------------------------------------------------------------------
// Breakpoints and watchpoints

static inline bool halt_on_breakpoint()
{
    return (~csr_tdata1[current_thread] & 0x0800000000001000ull) == 0;
}


static bool matches_breakpoint_address(uint64_t addr)
{
    uint64_t mcontrol = csr_tdata1[current_thread];
    uint64_t mvalue = csr_tdata2[current_thread];
    bool exact = (~mcontrol & 0x80);
    uint64_t mask = exact ? 0 : (((~mvalue & (mvalue + 1)) - 1) & 0x3f);
    return (mvalue == ((addr & VA_M) | mask));
}


static inline void check_load_breakpoint(uint64_t addr)
{
    if (break_on_load[current_thread] && matches_breakpoint_address(addr))
        throw_trap_breakpoint(addr);
}


static inline void check_store_breakpoint(uint64_t addr)
{
    if (break_on_store[current_thread] && matches_breakpoint_address(addr))
        throw_trap_breakpoint(addr);
}


//------------------------------------------------------------------------------
// PMA checks

static inline bool paddr_is_cacheable(uint64_t addr)
{
    return paddr_is_dram(addr)
        || paddr_is_scratchpad(addr)
        || paddr_is_sp_rom(addr)
        || paddr_is_sp_sram(addr);
}


static bool pma_data_access_permitted(uint64_t addr, size_t size, mem_access_type macc)
{
    bool spio = (current_thread / EMU_THREADS_PER_SHIRE) == EMU_IO_SHIRE_SP;

    bool amo      = (macc == Mem_Access_AtomicL || macc == Mem_Access_AtomicG);
    bool amo_g    = (macc == Mem_Access_AtomicG);
    bool ts_tl_co = (macc >= Mem_Access_TxLoad && macc <= Mem_Access_CacheOp);

    if (paddr_is_io_space(addr))
        return !amo
            && !ts_tl_co
            && addr_is_size_aligned(addr, size)
            && (spio || /*!mprot.disable_io_access*/true);

    if (paddr_is_sp_space(addr))
        return spio
            && !amo
            && !ts_tl_co
            && (paddr_is_sp_rom(addr) || paddr_is_sp_sram(addr) ||
                addr_is_size_aligned(addr, size));

    if (paddr_is_scratchpad(addr))
        return !amo || amo_g;

    if (paddr_is_esr_space(addr))
        return !amo
            && !ts_tl_co
            && (size == 8)
            && addr_is_size_aligned(addr, size)
            && ( int((addr >> 30) & 0x3) <= effective_execution_mode(macc) )
            && ( int((addr >> 30) & 0x3) != 2 || spio );

    if (paddr_is_pcie_space(addr))
        return !amo
            && !ts_tl_co
            && addr_is_size_aligned(addr, size)
            && (spio || /*!mprot.disable_pcie_access*/true);

    if (paddr_is_dram_mbox(addr))
        return effective_execution_mode(macc) == CSR_PRV_M;

    if (paddr_is_dram_osbox(addr))
        return spio || /*!mprot.disable_osbox_access*/true;

    return paddr_is_dram(addr);
}


static bool pma_check_ptw_access(uint64_t addr)
{
    bool spio = (current_thread / EMU_THREADS_PER_SHIRE) == EMU_IO_SHIRE_SP;

    if (paddr_is_sp_rom(addr))
        return spio;

    if (paddr_is_sp_sram(addr))
        return spio;

    if (paddr_is_dram_mbox(addr))
        return effective_execution_mode(Mem_Access_PTW) == CSR_PRV_M;

    if (paddr_is_dram_osbox(addr))
        return spio || /*!mprot.disable_osbox_access*/true;

    return paddr_is_dram(addr);
}


static bool pma_fetch_access_permitted(uint64_t addr)
{
    bool spio = (current_thread / EMU_THREADS_PER_SHIRE) == EMU_IO_SHIRE_SP;

    if (paddr_is_sp_rom(addr))
        return spio;

    if (paddr_is_sp_sram(addr))
        return spio;

    if (paddr_is_dram_mbox(addr))
        return effective_execution_mode(Mem_Access_Fetch) == CSR_PRV_M;

    if (paddr_is_dram_osbox(addr))
        return spio || /*!mprot.disable_osbox_access*/true;

    return paddr_is_dram(addr);
}


//------------------------------------------------------------------------------
//
// External methods: loads/stores
//
//------------------------------------------------------------------------------

__attribute__((noreturn)) void throw_trap_breakpoint(uint64_t addr)
{
    if (halt_on_breakpoint())
        throw std::runtime_error("Debug mode not supported yet!");
    throw trap_breakpoint(addr);
}


bool matches_fetch_breakpoint(uint64_t addr)
{
    return break_on_fetch[current_thread] && matches_breakpoint_address(addr);
}


uint64_t vmemtranslate(uint64_t vaddr, size_t size, mem_access_type macc)
{
    // Read mstatus
    const uint64_t mstatus = csr_mstatus[current_thread];
    const int      mxr     = (mstatus >> MSTATUS_MXR ) & 0x1;
    const int      sum     = (mstatus >> MSTATUS_SUM ) & 0x1;
    const int      mprv    = (mstatus >> MSTATUS_MPRV) & 0x1;
    const int      mpp     = (mstatus >> MSTATUS_MPP ) & 0x3;

    // Read satp
    const uint64_t satp      = csr_satp[current_thread];
    const uint64_t satp_mode = (satp >> 60) & 0xF;
    const uint64_t satp_ppn  = satp & PPN_M;

    // Calculate effective privilege level
    const int prv = (macc == Mem_Access_Fetch)
            ? csr_prv[current_thread]
            : (mprv ? mpp : csr_prv[current_thread]);

    // V2P mappings are enabled when all of the following are true:
    // - the effective execution mode is not 'M'
    // - satp.mode is not "Bare"
    bool vm_enabled = (prv < CSR_PRV_M) && (satp_mode != SATP_MODE_BARE);

    if (!vm_enabled) {
        uint64_t paddr = vaddr & PA_M;
        bool okay = (macc == Mem_Access_Fetch)
                ? pma_fetch_access_permitted(paddr)
                : pma_data_access_permitted(paddr, size, macc);
        if (!okay)
            throw_access_fault(vaddr, macc);
        return paddr;
    }

    int64_t sign;
    int Num_Levels;
    int PTE_top_Idx_Size;
    const int PTE_Size     = 8;
    const int PTE_Idx_Size = 9;
    switch (satp_mode)
    {
    case SATP_MODE_SV39:
        Num_Levels = 3;
        PTE_top_Idx_Size = 26;
        // bits 63-39 of address must be equal to bit 38
        sign = int64_t(vaddr) >> 38;
        break;
    case SATP_MODE_SV48:
        Num_Levels = 4;
        PTE_top_Idx_Size = 17;
        // bits 63-48 of address must be equal to bit 47
        sign = int64_t(vaddr) >> 47;
        break;
    default:
        assert(0); // we should never get here!
        break;
    }

    if (sign != int64_t(0) && sign != ~int64_t(0))
        throw_page_fault(vaddr, macc);

    const uint64_t pte_idx_mask     = (uint64_t(1) << PTE_Idx_Size) - 1;
    const uint64_t pte_top_idx_mask = (uint64_t(1) << PTE_top_Idx_Size) - 1;

    LOG(DEBUG, "Virtual memory enabled. Performing page walk on addr 0x%016" PRIx64 "...", vaddr);

    // Perform page walk. Anything that goes wrong raises a page fault error
    // for the access type of the original access, setting tval to the
    // original virtual address.
    uint64_t pte_addr, pte;
    bool pte_v, pte_r, pte_w, pte_x, pte_u, pte_a, pte_d;
    int level    = Num_Levels;
    uint64_t ppn = satp_ppn;
    do {
        if (--level < 0)
            throw_page_fault(vaddr, macc);

        // Take VPN[level]
        uint64_t vpn = (vaddr >> (PG_OFFSET_SIZE + PTE_Idx_Size*level)) & pte_idx_mask;
        // Read PTE
        pte_addr = (ppn << PG_OFFSET_SIZE) + vpn*PTE_Size;
        if (!pma_check_ptw_access(pte_addr))
        {
            throw_access_fault(vaddr, macc);
        }
        pte = pmemread64(pte_addr);
        LOG(DEBUG, "\tPTW: %016" PRIx64 " <-- PMEM64[%016" PRIx64 "]", pte, pte_addr);

        // Read PTE fields
        pte_v = (pte >> PTE_V_OFFSET) & 0x1;
        pte_r = (pte >> PTE_R_OFFSET) & 0x1;
        pte_w = (pte >> PTE_W_OFFSET) & 0x1;
        pte_x = (pte >> PTE_X_OFFSET) & 0x1;
        pte_u = (pte >> PTE_U_OFFSET) & 0x1;
        pte_a = (pte >> PTE_A_OFFSET) & 0x1;
        pte_d = (pte >> PTE_D_OFFSET) & 0x1;
        // Read PPN
        ppn = (pte >> PTE_PPN_OFFSET) & PPN_M;

        // Check invalid entry
        if (!pte_v || (!pte_r && pte_w))
            throw_page_fault(vaddr, macc);

        // Check if PTE is a pointer to next table level
    } while (!pte_r && !pte_x);

    // A leaf PTE has been found

    // Check permissions. This is different for each access type.
    // Load accesses are permitted iff all the following are true:
    // - the page has read permissions or the page has execute permissions and mstatus.mxr is set
    // - if the effective execution mode is user, then the page permits user-mode access (U=1)
    // - if the effective execution mode is system, then the page permits system-mode access (U=0 or SUM=1)
    // Store accesses are permitted iff all the following are true:
    // - the page has write permissions
    // - if the effective execution mode is user, then the page permits user-mode access (U=1)
    // - if the effective execution mode is system, then the page permits system-mode access (U=0 or SUM=1)
    // Instruction fetches are permitted iff all the following are true:
    // - the page has execute permissions
    // - if the execution mode is user, then the page permits user-mode access (U=1)
    // - if the execution mode is system, then the page does not permit user-mode access (U=0)
    switch (macc)
    {
    case Mem_Access_Load:
    case Mem_Access_TxLoad:
    case Mem_Access_Prefetch:
        if (!(pte_r || (mxr && pte_x))
            || ((prv == CSR_PRV_U) && !pte_u)
            || ((prv == CSR_PRV_S) && pte_u && !sum))
            throw_page_fault(vaddr, macc);
        break;
    case Mem_Access_Store:
    case Mem_Access_TxStore:
    case Mem_Access_AtomicL:
    case Mem_Access_AtomicG:
    case Mem_Access_CacheOp:
        if (!pte_w
            || ((prv == CSR_PRV_U) && !pte_u)
            || ((prv == CSR_PRV_S) && pte_u && !sum))
            throw_page_fault(vaddr, macc);
        break;
    case Mem_Access_Fetch:
        if (!pte_x
            || ((prv == CSR_PRV_U) && !pte_u)
            || ((prv == CSR_PRV_S) && pte_u))
            throw_page_fault(vaddr, macc);
        break;
    case Mem_Access_PTW:
        assert(0);
        break;
    }

    // Check if it is a misaligned superpage
    if ((level > 0) && ((ppn & ((1<<(PTE_Idx_Size*level))-1)) != 0))
        throw_page_fault(vaddr, macc);

    // Check if A/D bit should be updated
    if (!pte_a || ((macc == Mem_Access_Store) && !pte_d))
        throw_page_fault(vaddr, macc);

    // Obtain physical address

    // Copy page offset
    uint64_t paddr = vaddr & PG_OFFSET_M;

    for (int i = 0; i < Num_Levels; i++) {
        // If level > 0, this is a superpage translation so VPN[level-1:0] are part of the page offset
        if (i < level) {
            paddr |= vaddr & (pte_idx_mask << (PG_OFFSET_SIZE + PTE_Idx_Size*i));
        }
        else if (i == Num_Levels-1) {
            paddr |= (ppn & (pte_top_idx_mask << (PTE_Idx_Size*i))) << PG_OFFSET_SIZE;
        }
        else {
            paddr |= (ppn & (pte_idx_mask << (PTE_Idx_Size*i))) << PG_OFFSET_SIZE;
        }
    }

    // Final physical address only uses 40 bits
    paddr &= PA_M;
    LOG(DEBUG, "\tPTW: Paddr = 0x%016" PRIx64, paddr);
    bool okay = (macc == Mem_Access_Fetch)
            ? pma_fetch_access_permitted(paddr)
            : pma_data_access_permitted(paddr, size, macc);
    if (!okay)
        throw_access_fault(vaddr, macc);
    return paddr;
}


uint8_t mmu_load8(uint64_t eaddr)
{
    uint64_t vaddr = sextVA(eaddr);
    check_load_breakpoint(vaddr);
    uint64_t paddr = vmemtranslate(vaddr, 1, Mem_Access_Load);
    uint8_t value = pmemread8(paddr);
    LOG_MEMREAD(8, paddr, value);
    return value;
}


uint16_t mmu_load16(uint64_t eaddr)
{
    uint64_t vaddr = sextVA(eaddr);
    check_load_breakpoint(vaddr);
    uint64_t paddr = vmemtranslate(vaddr, 2, Mem_Access_Load);
    uint16_t value = pmemread16(paddr);
    LOG_MEMREAD(16, paddr, value);
    return value;
}


uint16_t mmu_aligned_load16(uint64_t eaddr)
{
    uint64_t vaddr = sextVA(eaddr);
    check_load_breakpoint(vaddr);
    if (!addr_is_size_aligned(vaddr, 2)) {
        throw trap_load_access_fault(vaddr);
    }
    uint64_t paddr = vmemtranslate(vaddr, 2, Mem_Access_Load);
    uint16_t value = pmemread16(paddr);
    LOG_MEMREAD(16, paddr, value);
    return value;
}


uint32_t mmu_load32(uint64_t eaddr)
{
    uint64_t vaddr = sextVA(eaddr);
    check_load_breakpoint(vaddr);
    uint64_t paddr = vmemtranslate(vaddr, 4, Mem_Access_Load);
    uint32_t value = pmemread32(paddr);
    LOG_MEMREAD(32, paddr, value);
    return value;
}


uint32_t mmu_aligned_load32(uint64_t eaddr)
{
    uint64_t vaddr = sextVA(eaddr);
    check_load_breakpoint(vaddr);
    if (!addr_is_size_aligned(vaddr, 4)) {
        throw trap_load_access_fault(vaddr);
    }
    uint64_t paddr = vmemtranslate(vaddr, 4, Mem_Access_Load);
    uint32_t value = pmemread32(paddr);
    LOG_MEMREAD(32, paddr, value);
    return value;
}


uint64_t mmu_load64(uint64_t eaddr)
{
    uint64_t vaddr = sextVA(eaddr);
    check_load_breakpoint(vaddr);
    uint64_t paddr = vmemtranslate(vaddr, 8, Mem_Access_Load);
    uint64_t value = pmemread64(paddr);
    LOG_MEMREAD(64, paddr, value);
    return value;
}


void mmu_loadVLEN(uint64_t eaddr, freg_t& data, mreg_t mask)
{
    if (mask.any()) {
        uint64_t vaddr = sextVA(eaddr);
        check_load_breakpoint(vaddr);
        uint64_t paddr = vmemtranslate(vaddr, VLEN/8, Mem_Access_Load);
        for (size_t e = 0; e < MLEN; ++e) {
            if (mask[e]) {
                data.u32[e] = pmemread32(paddr + 4*e);
                LOG_MEMREAD(32, paddr + 4*e, data.u32[e]);
            }
        }
    }
}


void mmu_aligned_loadVLEN(uint64_t eaddr, freg_t& data, mreg_t mask)
{
    if (mask.any()) {
        uint64_t vaddr = sextVA(eaddr);
        check_load_breakpoint(vaddr);
        if (!addr_is_size_aligned(vaddr, VLEN/8)) {
            throw trap_load_access_fault(vaddr);
        }
        uint64_t paddr = vmemtranslate(vaddr, VLEN/8, Mem_Access_Load);
        for (size_t e = 0; e < MLEN; ++e) {
            if (mask[e]) {
                data.u32[e] = pmemread32(paddr + 4*e);
                LOG_MEMREAD(32, paddr + 4*e, data.u32[e]);
            }
        }
    }
}


void mmu_store8(uint64_t eaddr, uint8_t data)
{
    uint64_t vaddr = sextVA(eaddr);
    check_store_breakpoint(vaddr);
    uint64_t paddr = vmemtranslate(vaddr, 1, Mem_Access_Store);
    pmemwrite8(paddr, data);
    LOG_MEMWRITE(8, paddr, data);
    log_mem_write(true, 1, vaddr, data);
}


void mmu_store16(uint64_t eaddr, uint16_t data)
{
    uint64_t vaddr = sextVA(eaddr);
    check_store_breakpoint(vaddr);
    uint64_t paddr = vmemtranslate(vaddr, 2, Mem_Access_Store);
    pmemwrite16(paddr, data);
    LOG_MEMWRITE(16, paddr, data);
    log_mem_write(true, 2, vaddr, data);
}


void mmu_aligned_store16(uint64_t eaddr, uint16_t data)
{
    uint64_t vaddr = sextVA(eaddr);
    check_store_breakpoint(vaddr);
    if (!addr_is_size_aligned(vaddr, 2)) {
        throw trap_store_access_fault(vaddr);
    }
    uint64_t paddr = vmemtranslate(vaddr, 2, Mem_Access_Store);
    pmemwrite16(paddr, data);
    LOG_MEMWRITE(16, paddr, data);
    log_mem_write(true, 2, vaddr, data);
}


void mmu_store32(uint64_t eaddr, uint32_t data)
{
    uint64_t vaddr = sextVA(eaddr);
    check_store_breakpoint(vaddr);
    uint64_t paddr = vmemtranslate(vaddr, 4, Mem_Access_Store);
    pmemwrite32(paddr, data);
    LOG_MEMWRITE(32, paddr, data);
    log_mem_write(true, 4, vaddr, data);
}


void mmu_aligned_store32(uint64_t eaddr, uint32_t data)
{
    uint64_t vaddr = sextVA(eaddr);
    check_store_breakpoint(vaddr);
    if (!addr_is_size_aligned(vaddr, 4)) {
        throw trap_store_access_fault(vaddr);
    }
    uint64_t paddr = vmemtranslate(vaddr, 4, Mem_Access_Store);
    pmemwrite32(paddr, data);
    LOG_MEMWRITE(32, paddr, data);
    log_mem_write(true, 4, vaddr, data);
}


void mmu_store64(uint64_t eaddr, uint64_t data)
{
    uint64_t vaddr = sextVA(eaddr);
    check_store_breakpoint(vaddr);
    uint64_t paddr = vmemtranslate(vaddr, 8, Mem_Access_Store);
    pmemwrite64(paddr, data);
    LOG_MEMWRITE(64, paddr, data);
    log_mem_write(true, 8, vaddr, data);
}


void mmu_storeVLEN(uint64_t eaddr, freg_t data, mreg_t mask)
{
    if (mask.any()) {
        uint64_t vaddr = sextVA(eaddr);
        check_store_breakpoint(vaddr);
        uint64_t paddr = vmemtranslate(vaddr, VLEN/8, Mem_Access_Store);
        for (size_t e = 0; e < MLEN; ++e) {
            if (mask[e]) {
                pmemwrite32(paddr + 4*e, data.u32[e]);
                LOG_MEMWRITE(32, paddr + 4*e, data.u32[e]);
            }
            log_mem_write(mask[e], 4, vaddr + 4*e, data.u32[e]);
        }
    }
}


void mmu_aligned_storeVLEN(uint64_t eaddr, freg_t data, mreg_t mask)
{
    if (mask.any()) {
        uint64_t vaddr = sextVA(eaddr);
        check_store_breakpoint(vaddr);
        if (!addr_is_size_aligned(vaddr, VLEN/8)) {
            throw trap_store_access_fault(vaddr);
        }
        uint64_t paddr = vmemtranslate(vaddr, VLEN/8, Mem_Access_Store);
        for (size_t e = 0; e < MLEN; ++e) {
            if (mask[e]) {
                pmemwrite32(paddr + 4*e, data.u32[e]);
                LOG_MEMWRITE(32, paddr + 4*e, data.u32[e]);
            }
            log_mem_write(mask[e], 4, vaddr + 4*e, data.u32[e]);
        }
    }
}


uint32_t mmu_global_atomic32(uint64_t eaddr, uint32_t data,
                             std::function<uint32_t(uint32_t, uint32_t)> fn)
{
    uint64_t vaddr = sextVA(eaddr);
    check_store_breakpoint(vaddr);
    if (!addr_is_size_aligned(vaddr, 4)) {
        throw trap_store_access_fault(vaddr);
    }
    uint64_t paddr = vmemtranslate(vaddr, 4, Mem_Access_AtomicG);
    uint32_t oldval = pmemread32(paddr);
    LOG_MEMREAD(32, paddr, oldval);
    uint32_t newval = fn(oldval, data);
    pmemwrite32(paddr, newval);
    LOG_MEMWRITE(32, paddr, newval);
    log_mem_write(true, 4, vaddr, data);
    return oldval;
}


uint64_t mmu_global_atomic64(uint64_t eaddr, uint64_t data,
                             std::function<uint64_t(uint64_t, uint64_t)> fn)
{
    uint64_t vaddr = sextVA(eaddr);
    check_store_breakpoint(vaddr);
    if (!addr_is_size_aligned(vaddr, 4)) {
        throw trap_store_access_fault(vaddr);
    }
    uint64_t paddr = vmemtranslate(vaddr, 4, Mem_Access_AtomicG);
    uint64_t oldval = pmemread64(paddr);
    LOG_MEMREAD(64, paddr, oldval);
    uint64_t newval = fn(oldval, data);
    pmemwrite64(paddr, newval);
    LOG_MEMWRITE(64, paddr, newval);
    log_mem_write(true, 8, vaddr, data);
    return oldval;
}


uint32_t mmu_local_atomic32(uint64_t eaddr, uint32_t data,
                            std::function<uint32_t(uint32_t, uint32_t)> fn)
{
    uint64_t vaddr = sextVA(eaddr);
    check_store_breakpoint(vaddr);
    if (!addr_is_size_aligned(vaddr, 4)) {
        throw trap_store_access_fault(vaddr);
    }
    uint64_t paddr = vmemtranslate(vaddr, 4, Mem_Access_AtomicL);
    uint32_t oldval = pmemread32(paddr);
    LOG_MEMREAD(32, paddr, oldval);
    uint32_t newval = fn(oldval, data);
    pmemwrite32(paddr, newval);
    LOG_MEMWRITE(32, paddr, newval);
    log_mem_write(true, 4, vaddr, data);
    return oldval;
}


uint64_t mmu_local_atomic64(uint64_t eaddr, uint64_t data,
                            std::function<uint64_t(uint64_t, uint64_t)> fn)
{
    uint64_t vaddr = sextVA(eaddr);
    check_store_breakpoint(vaddr);
    if (!addr_is_size_aligned(vaddr, 4)) {
        throw trap_store_access_fault(vaddr);
    }
    uint64_t paddr = vmemtranslate(vaddr, 4, Mem_Access_AtomicL);
    uint64_t oldval = pmemread64(paddr);
    LOG_MEMREAD(64, paddr, oldval);
    uint64_t newval = fn(oldval, data);
    pmemwrite64(paddr, newval);
    LOG_MEMWRITE(64, paddr, newval);
    log_mem_write(true, 8, vaddr, data);
    return oldval;
}


//------------------------------------------------------------------------------
//
// External methods: cache ops
//
//------------------------------------------------------------------------------

bool mmu_check_cacheop_access(uint64_t paddr)
{
    bool spio = (current_thread / EMU_THREADS_PER_SHIRE) == EMU_IO_SHIRE_SP;

    if (paddr_is_scratchpad(paddr))
        return true;

    if (paddr_is_dram_mbox(paddr))
        return effective_execution_mode(Mem_Access_CacheOp) == CSR_PRV_M;

    if (paddr_is_dram_osbox(paddr))
        return spio || /*!mprot.disable_osbox_access*/true;

    return paddr_is_dram(paddr);
}
