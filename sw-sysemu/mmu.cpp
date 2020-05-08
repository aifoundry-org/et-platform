/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include <array>
#include <cassert>
#include <stdexcept>
#include <type_traits>
#include <climits>

#include "cache.h"
#include "decode.h"
#include "emu_gio.h"
#include "esrs.h"
#include "literals.h"
#include "log.h"
#include "memmap.h"
#include "memop.h"
#include "mmu.h"
#include "processor.h"
#include "traps.h"
#include "utility.h"
#ifdef SYS_EMU
#include "mem_checker.h"
#endif

namespace bemu {


//------------------------------------------------------------------------------
// Exceptions

static inline int effective_execution_mode(const Hart& cpu, mem_access_type macc)
{
    // Read mstatus
    const uint64_t mstatus = cpu.mstatus;
    const int      mprv    = (mstatus >> MSTATUS_MPRV) & 0x1;
    const int      mpp     = (mstatus >> MSTATUS_MPP ) & 0x3;
    const int      prv     = cpu.prv;
    return (macc == Mem_Access_Fetch) ? prv : (mprv ? mpp : prv);
}


[[noreturn]] static void throw_page_fault(uint64_t addr, mem_access_type macc)
{
    switch (macc)
    {
    case Mem_Access_Load:
    case Mem_Access_LoadL:
    case Mem_Access_LoadG:
    case Mem_Access_TxLoad:
    case Mem_Access_TxLoadL2Scp:
    case Mem_Access_Prefetch:
        throw trap_load_page_fault(addr);
    case Mem_Access_Store:
    case Mem_Access_StoreL:
    case Mem_Access_StoreG:
    case Mem_Access_TxStore:
    case Mem_Access_AtomicL:
    case Mem_Access_AtomicG:
    case Mem_Access_CacheOp:
        throw trap_store_page_fault(addr);
    case Mem_Access_Fetch:
        throw trap_instruction_page_fault(addr);
    case Mem_Access_PTW:
        break;
    }
    throw std::invalid_argument("throw_page_fault()");
}


[[noreturn]] static void throw_access_fault(uint64_t addr, mem_access_type macc)
{
    switch (macc)
    {
    case Mem_Access_Load:
    case Mem_Access_LoadL:
    case Mem_Access_LoadG:
    case Mem_Access_TxLoad:
    case Mem_Access_TxLoadL2Scp:
    case Mem_Access_Prefetch:
        throw trap_load_access_fault(addr);
    case Mem_Access_Store:
    case Mem_Access_StoreL:
    case Mem_Access_StoreG:
    case Mem_Access_TxStore:
    case Mem_Access_AtomicL:
    case Mem_Access_AtomicG:
    case Mem_Access_CacheOp:
        throw trap_store_access_fault(addr);
    case Mem_Access_Fetch:
        throw trap_instruction_access_fault(addr);
    case Mem_Access_PTW:
        break;
    }
    throw std::invalid_argument("throw_access_fault()");
}


//------------------------------------------------------------------------------
// Breakpoints and watchpoints

static inline bool halt_on_breakpoint(const Hart& cpu)
{
    return (~cpu.tdata1 & 0x0800000000001000ull) == 0;
}


[[noreturn]] void throw_trap_breakpoint(const Hart& cpu, uint64_t addr)
{
    if (halt_on_breakpoint(cpu))
        throw std::runtime_error("Debug mode not supported yet!");
    throw trap_breakpoint(addr);
}


static bool matches_breakpoint_address(const Hart& cpu, uint64_t addr)
{
  bool exact = ~cpu.tdata1 & 0x80;
  uint64_t val = cpu.tdata2;
  uint64_t msk = exact ? 0 : (((((~val & (val + 1)) - 1) & 0x3f) << 1) | 1);
  return ((val | msk) == ((addr & VA_M) | msk));
}


static inline void check_fetch_breakpoint(const Hart& cpu, uint64_t addr)
{
    if (cpu.break_on_fetch && matches_breakpoint_address(cpu, addr))
        throw_trap_breakpoint(cpu, addr);
}


static inline void check_load_breakpoint(const Hart& cpu, uint64_t addr)
{
    if (cpu.break_on_load && matches_breakpoint_address(cpu, addr))
        throw_trap_breakpoint(cpu, addr);
}


static inline void check_store_breakpoint(const Hart& cpu, uint64_t addr)
{
    if (cpu.break_on_store && matches_breakpoint_address(cpu, addr))
        throw_trap_breakpoint(cpu, addr);
}


//------------------------------------------------------------------------------
// PMA checks

#define MPROT_IO_ACCESS_MODE(x)    ((x) & 0x3)
#define MPROT_DISABLE_PCIE_ACCESS  0x04
#define MPROT_DISABLE_OSBOX_ACCESS 0x08
#define MPROT_DRAM_SIZE_8G         0x00
#define MPROT_DRAM_SIZE_16G        0x10
#define MPROT_DRAM_SIZE_24G        0x20
#define MPROT_DRAM_SIZE_32G        0x30
#define MPROT_DRAM_SIZE(x)         ((x) & 0x30)
#define MPROT_ENABLE_SECURE_MEMORY 0x40

#define PP(x)   (int(((x) & ESR_REGION_PROT_MASK) >> ESR_REGION_PROT_SHIFT))


static bool data_access_is_write(mem_access_type macc)
{
    switch (macc)
    {
    case Mem_Access_Load:
    case Mem_Access_LoadL:
    case Mem_Access_LoadG:
    case Mem_Access_Fetch:
    case Mem_Access_TxLoad:
    case Mem_Access_TxLoadL2Scp:
    case Mem_Access_Prefetch:
        return false;
    case Mem_Access_Store:
    case Mem_Access_StoreL:
    case Mem_Access_StoreG:
    case Mem_Access_PTW:
    case Mem_Access_AtomicL:
    case Mem_Access_AtomicG:
    case Mem_Access_TxStore:
    case Mem_Access_CacheOp:
        return true;
    }
    throw std::invalid_argument("data_access_is_write()");
}


static inline bool paddr_is_sp_cacheable(uint64_t addr)
{ return paddr_is_sp_rom(addr) || paddr_is_sp_sram(addr); }


// NB: We have only 16GiB of DRAM installed
static inline uint64_t truncated_dram_addr(uint64_t addr)
{
    return 0x8000000000ULL + ((addr - 0x8000000000ULL) % EMU_DRAM_SIZE);
}


static inline uint64_t pma_dram_limit(bool spio, uint8_t mprot)
{
    if (spio) {
        return 0x8000000000ULL + 32_GiB;
    }
    switch (MPROT_DRAM_SIZE(mprot)) {
    case MPROT_DRAM_SIZE_8G : return 0x8000000000ULL +  8_GiB;
    case MPROT_DRAM_SIZE_16G: return 0x8000000000ULL + 16_GiB;
    case MPROT_DRAM_SIZE_24G: return 0x8000000000ULL + 24_GiB;
    case MPROT_DRAM_SIZE_32G: return 0x8000000000ULL + 32_GiB;
    }
    throw std::runtime_error("Illegal mprot.dram_size value");
}


static uint64_t pma_check_data_access(const Hart& cpu, uint64_t vaddr,
                                      uint64_t addr, size_t size,
                                      mem_access_type macc, mreg_t mask,
                                      cacheop_type cop)
{
#ifndef SYS_EMU
    (void) cop;
    (void) mask;
#endif
    bool spio     = (cpu.mhartid == IO_SHIRE_SP_HARTID);
    bool amo      = (macc == Mem_Access_AtomicL) || (macc == Mem_Access_AtomicG);
    bool amo_l    = (macc == Mem_Access_AtomicL);
    bool ts_tl_co = (macc >= Mem_Access_TxLoad) && (macc <= Mem_Access_CacheOp);

    if (paddr_is_dram(addr)) {

        if (spio) {
            uint64_t addr2 = addr & ~0x4000000000ULL;
            if (addr != addr2) {
                if (!addr_is_size_aligned(addr, size)) {
                    throw_access_fault(vaddr, macc);
                }
                if (amo) {
                    throw memory_error(addr);
                }
                addr = addr2;
            }
        }

        if (!spio && !addr_is_size_aligned(addr, size)) {
            // when data cache is in bypass mode all accesses should be aligned
            uint8_t ctrl = neigh_esrs[neigh_index(cpu)].neigh_chicken;
            if (ctrl & 0x2)
                throw_access_fault(vaddr, macc);
        }

        uint8_t mprot = neigh_esrs[neigh_index(cpu)].mprot;

        if (mprot & MPROT_ENABLE_SECURE_MEMORY) {
            if (paddr_is_dram_mcode(addr)) {
                if (!spio && (data_access_is_write(macc)
                              || (effective_execution_mode(cpu, macc) != PRV_M)))
                    throw_access_fault(vaddr, macc);
            }
            else if (paddr_is_dram_mdata(addr)) {
                if (!spio && (effective_execution_mode(cpu, macc) != PRV_M))
                    throw_access_fault(vaddr, macc);
            }
            else if (paddr_is_dram_scode(addr)) {
                if (!spio && (data_access_is_write(macc)
                              ? (effective_execution_mode(cpu, macc) != PRV_M)
                              : (effective_execution_mode(cpu, macc) == PRV_U)))
                    throw_access_fault(vaddr, macc);
            }
            else if (paddr_is_dram_sdata(addr)) {
                if (!spio && (effective_execution_mode(cpu, macc) == PRV_U))
                    throw_access_fault(vaddr, macc);
            }
            else if (paddr_is_dram_osbox(addr)) {
                if (!spio && (mprot & MPROT_DISABLE_OSBOX_ACCESS))
                    throw_access_fault(vaddr, macc);
            }
            else if (addr >= pma_dram_limit(spio, mprot)) {
                throw_access_fault(vaddr, macc);
            }
        } else {
            if (paddr_is_dram_mbox(addr)) {
                if (!spio && (effective_execution_mode(cpu, macc) != PRV_M))
                    throw_access_fault(vaddr, macc);
            }
            else if (paddr_is_dram_sbox(addr)) {
                if (!spio && (mprot & MPROT_DISABLE_OSBOX_ACCESS))
                    throw_access_fault(vaddr, macc);
            }
            else if (paddr_is_dram_osbox(addr)) {
                if (!spio && (mprot & MPROT_DISABLE_OSBOX_ACCESS))
                    throw_access_fault(vaddr, macc);
            }
            else if (addr >= pma_dram_limit(spio, mprot)) {
                throw_access_fault(vaddr, macc);
            }
        }
#ifdef SYS_EMU
        if (sys_emu::get_mem_check()) {
            sys_emu::get_mem_checker().access(addr, macc, cop, hart_index(cpu), size, mask);
        }
#endif
        return truncated_dram_addr(addr);
    }

    if (paddr_is_scratchpad(addr)) {
        if (amo_l)
            throw_access_fault(vaddr, macc);
#ifdef SYS_EMU
        if (sys_emu::get_mem_check()) {
            sys_emu::get_mem_checker().access(addr, macc, cop, hart_index(cpu), size, mask);
        }
        if (sys_emu::get_l2_scp_check()) {
            sys_emu::get_l2_scp_checker().l2_scp_read(hart_index(cpu), addr);
        }
#endif
        return addr;
    }

    if (paddr_is_esr_space(addr)) {
        if (amo
            || ts_tl_co
            || (size != 8)
            || !addr_is_size_aligned(addr, size)
            || (PP(addr) > effective_execution_mode(cpu, macc))
            || (PP(addr) == 2 && !spio))
            throw_access_fault(vaddr, macc);
        return addr;
    }

    if (paddr_is_sp_space(addr)) {
        int mode = effective_execution_mode(cpu, macc);
        if (!spio
            || amo
            || (ts_tl_co && !paddr_is_sp_cacheable(addr))
            || (paddr_is_sp_sram_code(addr) && data_access_is_write(macc) && (mode != PRV_M))
            || (paddr_is_sp_sram_data(addr) && (mode == PRV_U))
            || (!paddr_is_sp_cacheable(addr) && !addr_is_size_aligned(addr, size)))
            throw_access_fault(vaddr, macc);
        return addr;
    }

    if (paddr_is_io_space(addr)) {
        uint8_t mprot = neigh_esrs[neigh_index(cpu)].mprot;
        if (amo
            || ts_tl_co
            || !addr_is_size_aligned(addr, size)
            || (!spio && ((MPROT_IO_ACCESS_MODE(mprot) == 0x2)
                          || (effective_execution_mode(cpu, macc) < MPROT_IO_ACCESS_MODE(mprot)))))
            throw_access_fault(vaddr, macc);
        return addr;
    }

    if (paddr_is_pcie_space(addr)) {
        uint8_t mprot = neigh_esrs[neigh_index(cpu)].mprot;
        if (amo
            || ts_tl_co
            || !addr_is_size_aligned(addr, size)
            || (!spio && (mprot & MPROT_DISABLE_PCIE_ACCESS)))
            throw_access_fault(vaddr, macc);
        return addr;
    }

    throw_access_fault(vaddr, macc);
}


static uint64_t pma_check_fetch_access(const Hart& cpu, uint64_t vaddr,
                                       uint64_t addr, mem_access_type macc)
{
    bool spio = (cpu.mhartid == IO_SHIRE_SP_HARTID);

    if (paddr_is_dram(addr)) {
        if (spio)
            throw_access_fault(vaddr, macc);

        uint8_t mprot = neigh_esrs[neigh_index(cpu)].mprot;

        if (mprot & MPROT_ENABLE_SECURE_MEMORY) {
            if (paddr_is_dram_mcode(addr)) {
                if (effective_execution_mode(cpu, macc) != PRV_M)
                    throw_access_fault(vaddr, macc);
            }
            else if (paddr_is_dram_mdata(addr)) {
                throw_access_fault(vaddr, macc);
            }
            else if (paddr_is_dram_scode(addr)) {
                if (effective_execution_mode(cpu, macc) != PRV_S)
                    throw_access_fault(vaddr, macc);
            }
            else if (paddr_is_dram_sdata(addr)) {
                throw_access_fault(vaddr, macc);
            }
            else if (paddr_is_dram_osbox(addr)) {
                if ((mprot & MPROT_DISABLE_OSBOX_ACCESS) ||
                    (effective_execution_mode(cpu, macc) != PRV_U))
                    throw_access_fault(vaddr, macc);
            }
            else if ((addr >= pma_dram_limit(spio, mprot)) ||
                     (effective_execution_mode(cpu, macc) != PRV_U)) {
                throw_access_fault(vaddr, macc);
            }
        } else {
            if (paddr_is_dram_mbox(addr)) {
                if (effective_execution_mode(cpu, macc) != PRV_M)
                    throw_access_fault(vaddr, macc);
            }
            else if (paddr_is_dram_sbox(addr)) {
                if (mprot & MPROT_DISABLE_OSBOX_ACCESS)
                    throw_access_fault(vaddr, macc);
            }
            else if (paddr_is_dram_osbox(addr)) {
                if (mprot & MPROT_DISABLE_OSBOX_ACCESS)
                    throw_access_fault(vaddr, macc);
            }
            else if (addr >= pma_dram_limit(spio, mprot)) {
                throw_access_fault(vaddr, macc);
            }
        }
#ifdef SYS_EMU
        if (sys_emu::get_mem_check()) {
            sys_emu::get_mem_checker().access(addr, macc, CacheOp_None, hart_index(cpu), 64, mreg_t(-1));
        }
#endif
        return truncated_dram_addr(addr);
    }

    if (paddr_is_sp_rom(addr)) {
        if (!spio)
            throw_access_fault(vaddr, macc);
        return addr;
    }

    if (paddr_is_sp_sram_code(addr)) {
        if (!spio || (effective_execution_mode(cpu, macc) == PRV_U))
            throw_access_fault(vaddr, macc);
        return addr;
    }

    if (paddr_is_sp_sram_data(addr)) {
        if (!spio || (effective_execution_mode(cpu, macc) != PRV_M))
            throw_access_fault(vaddr, macc);
        return addr;
    }

    throw_access_fault(vaddr, macc);
}


static inline uint64_t pma_check_mem_access(const Hart& cpu, uint64_t vaddr,
                                            uint64_t addr, size_t size,
                                            mem_access_type macc, mreg_t mask,
                                            cacheop_type cop)
{
    return (macc == Mem_Access_Fetch)
            ? pma_check_fetch_access(cpu, vaddr, addr, macc)
            : pma_check_data_access(cpu, vaddr, addr, size, macc, mask, cop);
}


static uint64_t pma_check_ptw_access(const Hart& cpu, uint64_t vaddr,
                                     uint64_t addr, mem_access_type macc)
{
    bool spio = (cpu.mhartid == IO_SHIRE_SP_HARTID);

    if (paddr_is_dram(addr)) {
        uint8_t mprot = neigh_esrs[neigh_index(cpu)].mprot;

        if (spio)
            addr &= ~0x4000000000ULL;

        if (mprot & MPROT_ENABLE_SECURE_MEMORY) {
            if (paddr_is_dram_mcode(addr)) {
                if (!spio)
                    throw_access_fault(vaddr, macc);
            }
            else if (paddr_is_dram_mdata(addr)) {
                if (!spio && (effective_execution_mode(cpu, macc) != PRV_M))
                    throw_access_fault(vaddr, macc);
            }
            else if (paddr_is_dram_scode(addr)) {
                if (!spio && (effective_execution_mode(cpu, macc) != PRV_M))
                    throw_access_fault(vaddr, macc);
            }
            else if (paddr_is_dram_osbox(addr)) {
                if (!spio && (mprot & MPROT_DISABLE_OSBOX_ACCESS))
                    throw_access_fault(vaddr, macc);
            }
            else if (addr >= pma_dram_limit(spio, mprot)) {
                throw_access_fault(vaddr, macc);
            }
        } else {
            if (paddr_is_dram_mbox(addr)) {
                if (!spio && (effective_execution_mode(cpu, macc) != PRV_M))
                    throw_access_fault(vaddr, macc);
            }
            else if (paddr_is_dram_sbox(addr)) {
                if (!spio && (mprot & MPROT_DISABLE_OSBOX_ACCESS))
                    throw_access_fault(vaddr, macc);
            }
            else if (paddr_is_dram_osbox(addr)) {
                if (!spio && (mprot & MPROT_DISABLE_OSBOX_ACCESS))
                    throw_access_fault(vaddr, macc);
            }
            else if (addr >= pma_dram_limit(spio, mprot)) {
                throw_access_fault(vaddr, macc);
            }
        }
        return truncated_dram_addr(addr);
    }

    if (paddr_is_sp_rom(addr) || paddr_is_sp_sram(addr)) {
        if (!spio || paddr_is_sp_sram_code(addr))
            throw_access_fault(vaddr, macc);
        return addr;
    }

    throw_access_fault(vaddr, macc);
}


//------------------------------------------------------------------------------
//
// External methods: loads/stores
//
//------------------------------------------------------------------------------

uint64_t vmemtranslate(const Hart& cpu, uint64_t vaddr, size_t size,
                       mem_access_type macc, mreg_t mask, cacheop_type cop)
{
    // Read mstatus
    const uint64_t mstatus = cpu.mstatus;
    const int      mxr     = (mstatus >> MSTATUS_MXR ) & 0x1;
    const int      sum     = (mstatus >> MSTATUS_SUM ) & 0x1;

    // Calculate effective privilege level
    const int curprv = effective_execution_mode(cpu, macc);

    // Read matp/satp
    // NB: Sv39/Mv39, Sv48/Mv48, etc. have the same behavior and encoding
    const uint64_t atp = (curprv == PRV_M)
            ? cpu.core->matp
            : cpu.core->satp;
    const uint64_t atp_mode = (atp >> 60) & 0xF;
    const uint64_t atp_ppn  = atp & PPN_M;

    // V2P mappings are enabled when all of the following are true:
    // - the effective execution mode is 'M' and matp.mode is not "Bare"
    // - the effective execution mode is not 'M' and satp.mode is not "Bare"
    bool vm_enabled = (atp_mode != SATP_MODE_BARE);

    if (!vm_enabled) {
        return pma_check_mem_access(cpu, vaddr, vaddr & PA_M, size, macc, mask, cop);
    }

    int64_t sign;
    int Num_Levels;
    int PTE_top_Idx_Size;
    const int PTE_Size     = 8;
    const int PTE_Idx_Size = 9;
    switch (atp_mode)
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

    LOG_HART(DEBUG, cpu, "Performing page walk on addr 0x%016" PRIx64 "...", vaddr);

    // Perform page walk. Anything that goes wrong raises a page fault error
    // for the access type of the original access, setting tval to the
    // original virtual address.
    uint64_t pte_addr, pte;
    bool pte_v, pte_r, pte_w, pte_x, pte_u, pte_a, pte_d;
    int level    = Num_Levels;
    uint64_t ppn = atp_ppn;
    do {
        if (--level < 0)
            throw_page_fault(vaddr, macc);

        // Take VPN[level]
        uint64_t vpn = (vaddr >> (PG_OFFSET_SIZE + PTE_Idx_Size*level)) & pte_idx_mask;
        // Read PTE
        pte_addr = (ppn << PG_OFFSET_SIZE) + vpn*PTE_Size;
        try {
            pte = pmemread<uint64_t>(cpu, pma_check_ptw_access(cpu, vaddr, pte_addr, macc));
            LOG_MEMREAD(64, pte_addr, pte);
        }
        catch (const memory_error&) {
            throw_access_fault(vaddr, macc);
        }

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
    // - the page has read permissions or the page has execute permissions and
    //   mstatus.mxr is set
    // - if the effective execution mode is user, then the page permits
    //   user-mode access (U=1)
    // - if the effective execution mode is system, then the page permits
    //   system-mode access (U=0 or SUM=1)
    // Store accesses are permitted iff all the following are true:
    // - the page has write permissions
    // - if the effective execution mode is user, then the page permits
    //   user-mode access (U=1)
    // - if the effective execution mode is system, then the page permits
    //   system-mode access (U=0 or SUM=1)
    // Instruction fetches are permitted iff all the following are true:
    // - the page has execute permissions
    // - if the execution mode is user, then the page permits user-mode access
    //   (U=1)
    // - if the execution mode is system, then the page does not permit
    //   user-mode access (U=0)
    switch (macc)
    {
    case Mem_Access_Load:
    case Mem_Access_LoadL:
    case Mem_Access_LoadG:
    case Mem_Access_TxLoad:
    case Mem_Access_TxLoadL2Scp:
    case Mem_Access_Prefetch:
        if (!(pte_r || (mxr && pte_x))
            || ((curprv == PRV_U) && !pte_u)
            || ((curprv == PRV_S) && pte_u && !sum))
            throw_page_fault(vaddr, macc);
        break;
    case Mem_Access_Store:
    case Mem_Access_StoreL:
    case Mem_Access_StoreG:
    case Mem_Access_TxStore:
    case Mem_Access_AtomicL:
    case Mem_Access_AtomicG:
    case Mem_Access_CacheOp:
        if (!pte_w
            || ((curprv == PRV_U) && !pte_u)
            || ((curprv == PRV_S) && pte_u && !sum))
            throw_page_fault(vaddr, macc);
        break;
    case Mem_Access_Fetch:
        if (!pte_x
            || ((curprv == PRV_U) && !pte_u)
            || ((curprv == PRV_S) && pte_u))
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
        // If level > 0, this is a superpage translation so VPN[level-1:0] are
        // part of the page offset
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
    LOG_HART(DEBUG, cpu, "\tPTW: Paddr = 0x%016" PRIx64, paddr);
    return pma_check_mem_access(cpu, vaddr, paddr, size, macc, mask, cop);
}


uint32_t mmu_fetch(const Hart& cpu, uint64_t vaddr)
{
    check_fetch_breakpoint(cpu, vaddr);
    try {
        if (vaddr & 3) {
            // 2B-aligned fetch
            uint64_t paddr = vmemtranslate(cpu, vaddr, 2, Mem_Access_Fetch, mreg_t(-1));
            uint16_t low = pmemread<uint16_t>(cpu, paddr);
            if ((low & 3) != 3) {
                LOG_HART(DEBUG, cpu, "Fetched compressed instruction from PC 0x%" PRIx64 ": 0x%04x", vaddr, low);
                return low;
            }
            paddr = ((paddr & 4095) <= 4092)
                    ? (paddr + 2)
                    : vmemtranslate(cpu, vaddr + 2, 2, Mem_Access_Fetch, mreg_t(-1));
            uint16_t high = pmemread<uint16_t>(cpu, paddr);
            uint32_t bits = uint32_t(low) + (uint32_t(high) << 16);
            LOG_HART(DEBUG, cpu, "Fetched instruction from PC 0x%" PRIx64 ": 0x%08x", vaddr, bits);
            return bits;
        }
        // 4B-aligned fetch
        uint64_t paddr = vmemtranslate(cpu, vaddr, 4, Mem_Access_Fetch, mreg_t(-1));
        uint32_t bits = pmemread<uint32_t>(cpu, paddr);
        if ((bits & 3) != 3) {
            uint16_t low = uint16_t(bits);
            LOG_HART(DEBUG, cpu, "Fetched compressed instruction from PC 0x%" PRIx64 ": 0x%04x", vaddr, low);
            return low;
        }
        LOG_HART(DEBUG, cpu, "Fetched instruction from PC 0x%" PRIx64 ": 0x%08x", vaddr, bits);
        return bits;
    } catch (const memory_error&) {
        throw trap_instruction_bus_error();
    }
}

template<typename T> T mmu_load_impl(const Hart& cpu, uint64_t eaddr, mem_access_type macc)
{
    uint64_t vaddr = sextVA(eaddr);
    check_load_breakpoint(cpu, vaddr);
    uint64_t paddr = vmemtranslate(cpu, vaddr, sizeof(T), macc, mreg_t(-1));
    T value = pmemread<T>(cpu, paddr);
    LOG_MEMREAD(CHAR_BIT*sizeof(T), paddr, value);
    notify_mem_read(cpu, true, sizeof(T), vaddr, paddr);
    return value;
}

uint8_t mmu_load8(const Hart& cpu, uint64_t eaddr, mem_access_type macc)
{
    return mmu_load_impl<uint8_t>(cpu, eaddr, macc);
}

uint16_t mmu_load16(const Hart& cpu, uint64_t eaddr, mem_access_type macc)
{
    return mmu_load_impl<uint16_t>(cpu, eaddr, macc);
}

uint32_t mmu_load32(const Hart& cpu, uint64_t eaddr, mem_access_type macc)
{
    return mmu_load_impl<uint32_t>(cpu, eaddr, macc);
}

uint64_t mmu_load64(const Hart& cpu, uint64_t eaddr, mem_access_type macc)
{
    return mmu_load_impl<uint64_t>(cpu, eaddr, macc);
}

uint16_t mmu_aligned_load16(const Hart& cpu, uint64_t eaddr, mem_access_type macc)
{
    uint64_t vaddr = sextVA(eaddr);
    check_load_breakpoint(cpu, vaddr);
    if (!addr_is_size_aligned(vaddr, 2)) {
        throw trap_load_access_fault(vaddr);
    }
    uint64_t paddr = vmemtranslate(cpu, vaddr, 2, macc, mreg_t(-1));
    uint16_t value = pmemread<uint16_t>(cpu, paddr);
    LOG_MEMREAD(16, paddr, value);
    notify_mem_read(cpu, true, 2, vaddr, paddr);
    return value;
}


uint32_t mmu_aligned_load32(const Hart& cpu, uint64_t eaddr, mem_access_type macc)
{
    uint64_t vaddr = sextVA(eaddr);
    check_load_breakpoint(cpu, vaddr);
    if (!addr_is_size_aligned(vaddr, 4)) {
        throw trap_load_access_fault(vaddr);
    }
    uint64_t paddr = vmemtranslate(cpu, vaddr, 4, macc, mreg_t(-1));
    uint32_t value = pmemread<uint32_t>(cpu, paddr);
    LOG_MEMREAD(32, paddr, value);
    notify_mem_read(cpu, true, 4, vaddr, paddr);
    return value;
}


void mmu_loadVLEN(const Hart& cpu, uint64_t eaddr, freg_t& data, mreg_t mask, mem_access_type macc)
{
    if (mask.any()) {
        uint64_t vaddr = sextVA(eaddr);
        check_load_breakpoint(cpu, vaddr);
        uint64_t paddr = vmemtranslate(cpu, vaddr, VLEN/8, macc, mask);
        for (size_t e = 0; e < MLEN; ++e) {
            if (mask[e]) {
                data.u32[e] = pmemread<uint32_t>(cpu, paddr + 4*e);
                LOG_MEMREAD(32, paddr + 4*e, data.u32[e]);
            }
            notify_mem_read(cpu, mask[e], 4, vaddr + 4*e, paddr + 4*e);
        }
    }
}


void mmu_aligned_loadVLEN(const Hart& cpu, uint64_t eaddr, freg_t& data, mreg_t mask, mem_access_type macc)
{
    if (mask.any()) {
        uint64_t vaddr = sextVA(eaddr);
        check_load_breakpoint(cpu, vaddr);
        if (!addr_is_size_aligned(vaddr, VLEN/8)) {
            throw trap_load_access_fault(vaddr);
        }
        uint64_t paddr = vmemtranslate(cpu, vaddr, VLEN/8, macc, mask);
        for (size_t e = 0; e < MLEN; ++e) {
            if (mask[e]) {
                data.u32[e] = pmemread<uint32_t>(cpu, paddr + 4*e);
                LOG_MEMREAD(32, paddr + 4*e, data.u32[e]);
            }
            notify_mem_read(cpu, mask[e], 4, vaddr + 4*e, paddr + 4*e);
        }
    }
}


template <typename T>
void mmu_store_impl(const Hart& cpu, uint64_t eaddr, T data, mem_access_type macc)
{
    uint64_t vaddr = sextVA(eaddr);
    check_store_breakpoint(cpu, vaddr);
    uint64_t paddr = vmemtranslate(cpu, vaddr, sizeof(T), macc, mreg_t(-1));
    pmemwrite<T>(cpu, paddr, data);
    LOG_MEMWRITE(CHAR_BIT*sizeof(T), paddr, data);
    notify_mem_write(cpu, true, sizeof(T), vaddr, paddr, data);
}

// Template declaration to avoid linker errors
void mmu_store8(const Hart& cpu, uint64_t eaddr, uint8_t  data, mem_access_type macc)
{
    mmu_store_impl<uint8_t>(cpu, eaddr, data, macc);
}

void mmu_store16(const Hart& cpu, uint64_t eaddr, uint16_t data, mem_access_type macc)
{
    mmu_store_impl<uint16_t>(cpu, eaddr, data, macc);
}

void mmu_store32(const Hart& cpu, uint64_t eaddr, uint32_t data, mem_access_type macc)
{
    mmu_store_impl<uint32_t>(cpu, eaddr, data, macc);
}

void mmu_store64(const Hart& cpu, uint64_t eaddr, uint64_t data, mem_access_type macc)
{
    mmu_store_impl<uint64_t>(cpu, eaddr, data, macc);
}

void mmu_aligned_store16(const Hart& cpu, uint64_t eaddr, uint16_t data, mem_access_type macc)
{
    uint64_t vaddr = sextVA(eaddr);
    check_store_breakpoint(cpu, vaddr);
    if (!addr_is_size_aligned(vaddr, 2)) {
        throw trap_store_access_fault(vaddr);
    }
    uint64_t paddr = vmemtranslate(cpu, vaddr, 2, macc, mreg_t(-1));
    pmemwrite<uint16_t>(cpu, paddr, data);
    LOG_MEMWRITE(16, paddr, data);
    notify_mem_write(cpu, true, 2, vaddr, paddr, data);
}


void mmu_aligned_store32(const Hart& cpu, uint64_t eaddr, uint32_t data, mem_access_type macc)
{
    uint64_t vaddr = sextVA(eaddr);
    check_store_breakpoint(cpu, vaddr);
    if (!addr_is_size_aligned(vaddr, 4)) {
        throw trap_store_access_fault(vaddr);
    }
    uint64_t paddr = vmemtranslate(cpu, vaddr, 4, macc, mreg_t(-1));
    pmemwrite<uint32_t>(cpu, paddr, data);
    LOG_MEMWRITE(32, paddr, data);
    notify_mem_write(cpu, true, 4, vaddr, paddr, data);
}


void mmu_storeVLEN(const Hart& cpu, uint64_t eaddr, freg_t data, mreg_t mask, mem_access_type macc)
{
    if (mask.any()) {
        uint64_t vaddr = sextVA(eaddr);
        check_store_breakpoint(cpu, vaddr);
        uint64_t paddr = vmemtranslate(cpu, vaddr, VLEN/8, macc, mask);
        for (size_t e = 0; e < MLEN; ++e) {
            if (mask[e]) {
                pmemwrite<uint32_t>(cpu, paddr + 4*e, data.u32[e]);
                LOG_MEMWRITE(32, paddr + 4*e, data.u32[e]);
            }
            notify_mem_write(cpu, mask[e], 4, vaddr + 4*e, paddr + 4*e, data.u32[e]);
        }
    }
}


void mmu_aligned_storeVLEN(const Hart& cpu, uint64_t eaddr, freg_t data, mreg_t mask, mem_access_type macc)
{
    if (mask.any()) {
        uint64_t vaddr = sextVA(eaddr);
        check_store_breakpoint(cpu, vaddr);
        if (!addr_is_size_aligned(vaddr, VLEN/8)) {
            throw trap_store_access_fault(vaddr);
        }
        uint64_t paddr = vmemtranslate(cpu, vaddr, VLEN/8, macc, mask);
        for (size_t e = 0; e < MLEN; ++e) {
            if (mask[e]) {
                pmemwrite<uint32_t>(cpu, paddr + 4*e, data.u32[e]);
                LOG_MEMWRITE(32, paddr + 4*e, data.u32[e]);
            }
            notify_mem_write(cpu, mask[e], 4, vaddr + 4*e, paddr + 4*e, data.u32[e]);
        }
    }
}


uint32_t mmu_global_atomic32(const Hart& cpu, uint64_t eaddr, uint32_t data,
                             std::function<uint32_t(uint32_t, uint32_t)> fn)
{
    uint64_t vaddr = sextVA(eaddr);
    check_store_breakpoint(cpu, vaddr);
    if (!addr_is_size_aligned(vaddr, 4)) {
        throw trap_store_access_fault(vaddr);
    }
    uint64_t paddr = vmemtranslate(cpu, vaddr, 4, Mem_Access_AtomicG, mreg_t(-1));
    uint32_t oldval = pmemread<uint32_t>(cpu, paddr);
    LOG_MEMREAD(32, paddr, oldval);
    uint32_t newval = fn(oldval, data);
    pmemwrite<uint32_t>(cpu, paddr, newval);
    LOG_MEMWRITE(32, paddr, newval);
    notify_mem_read_write(cpu, true, 4, vaddr, paddr, data);
    return oldval;
}


uint64_t mmu_global_atomic64(const Hart& cpu, uint64_t eaddr, uint64_t data,
                             std::function<uint64_t(uint64_t, uint64_t)> fn)
{
    uint64_t vaddr = sextVA(eaddr);
    check_store_breakpoint(cpu, vaddr);
    if (!addr_is_size_aligned(vaddr, 8)) {
        throw trap_store_access_fault(vaddr);
    }
    uint64_t paddr = vmemtranslate(cpu, vaddr, 8, Mem_Access_AtomicG, mreg_t(-1));
    uint64_t oldval = pmemread<uint64_t>(cpu, paddr);
    LOG_MEMREAD(64, paddr, oldval);
    uint64_t newval = fn(oldval, data);
    pmemwrite<uint64_t>(cpu, paddr, newval);
    LOG_MEMWRITE(64, paddr, newval);
    notify_mem_read_write(cpu, true, 8, vaddr, paddr, data);
    return oldval;
}


uint32_t mmu_local_atomic32(const Hart& cpu, uint64_t eaddr, uint32_t data,
                            std::function<uint32_t(uint32_t, uint32_t)> fn)
{
    uint64_t vaddr = sextVA(eaddr);
    check_store_breakpoint(cpu, vaddr);
    if (!addr_is_size_aligned(vaddr, 4)) {
        throw trap_store_access_fault(vaddr);
    }
    uint64_t paddr = vmemtranslate(cpu, vaddr, 4, Mem_Access_AtomicL, mreg_t(-1));
    uint32_t oldval = pmemread<uint32_t>(cpu, paddr);
    LOG_MEMREAD(32, paddr, oldval);
    uint32_t newval = fn(oldval, data);
    pmemwrite<uint32_t>(cpu, paddr, newval);
    LOG_MEMWRITE(32, paddr, newval);
    notify_mem_read_write(cpu, true, 4, vaddr, paddr, data);
    return oldval;
}


uint64_t mmu_local_atomic64(const Hart& cpu, uint64_t eaddr, uint64_t data,
                            std::function<uint64_t(uint64_t, uint64_t)> fn)
{
    uint64_t vaddr = sextVA(eaddr);
    check_store_breakpoint(cpu, vaddr);
    if (!addr_is_size_aligned(vaddr, 8)) {
        throw trap_store_access_fault(vaddr);
    }
    uint64_t paddr = vmemtranslate(cpu, vaddr, 8, Mem_Access_AtomicL, mreg_t(-1));
    uint64_t oldval = pmemread<uint64_t>(cpu, paddr);
    LOG_MEMREAD(64, paddr, oldval);
    uint64_t newval = fn(oldval, data);
    pmemwrite<uint64_t>(cpu, paddr, newval);
    LOG_MEMWRITE(64, paddr, newval);
    notify_mem_read_write(cpu, true, 8, vaddr, paddr, data);
    return oldval;
}


uint32_t mmu_global_compare_exchange32(const Hart& cpu, uint64_t eaddr,
                                       uint32_t expected, uint32_t desired)
{
    uint64_t vaddr = sextVA(eaddr);
    check_store_breakpoint(cpu, vaddr);
    if (!addr_is_size_aligned(vaddr, 4)) {
        throw trap_store_access_fault(vaddr);
    }
    uint64_t paddr = vmemtranslate(cpu, vaddr, 4, Mem_Access_AtomicG, mreg_t(-1));
    uint32_t oldval = pmemread<uint32_t>(cpu, paddr);
    LOG_MEMREAD(32, paddr, oldval);
    if (oldval == expected) {
        pmemwrite<uint32_t>(cpu, paddr, desired);
        LOG_MEMWRITE(32, paddr, desired);

    }
    notify_mem_read_write(cpu, true, 4, vaddr, paddr, desired);
    return oldval;
}


uint64_t mmu_global_compare_exchange64(const Hart& cpu, uint64_t eaddr,
                                       uint64_t expected, uint64_t desired)
{
    uint64_t vaddr = sextVA(eaddr);
    check_store_breakpoint(cpu, vaddr);
    if (!addr_is_size_aligned(vaddr, 8)) {
        throw trap_store_access_fault(vaddr);
    }
    uint64_t paddr = vmemtranslate(cpu, vaddr, 8, Mem_Access_AtomicG, mreg_t(-1));
    uint64_t oldval = pmemread<uint64_t>(cpu, paddr);
    LOG_MEMREAD(64, paddr, oldval);
    if (oldval == expected) {
        pmemwrite<uint64_t>(cpu, paddr, desired);
        LOG_MEMWRITE(64, paddr, desired);
    }
    notify_mem_read_write(cpu, true, 8, vaddr, paddr, desired);
    return oldval;
}


uint32_t mmu_local_compare_exchange32(const Hart& cpu, uint64_t eaddr,
                                      uint32_t expected, uint32_t desired)
{
    uint64_t vaddr = sextVA(eaddr);
    check_store_breakpoint(cpu, vaddr);
    if (!addr_is_size_aligned(vaddr, 4)) {
        throw trap_store_access_fault(vaddr);
    }
    uint64_t paddr = vmemtranslate(cpu, vaddr, 4, Mem_Access_AtomicL, mreg_t(-1));
    uint32_t oldval = pmemread<uint32_t>(cpu, paddr);
    LOG_MEMREAD(32, paddr, oldval);
    if (oldval == expected) {
        pmemwrite<uint32_t>(cpu, paddr, desired);
        LOG_MEMWRITE(32, paddr, desired);
    }
    notify_mem_read_write(cpu, true, 4, vaddr, paddr, desired);
    return oldval;
}


uint64_t mmu_local_compare_exchange64(const Hart& cpu, uint64_t eaddr,
                                      uint64_t expected, uint64_t desired)
{
    uint64_t vaddr = sextVA(eaddr);
    check_store_breakpoint(cpu, vaddr);
    if (!addr_is_size_aligned(vaddr, 8)) {
        throw trap_store_access_fault(vaddr);
    }
    uint64_t paddr = vmemtranslate(cpu, vaddr, 8, Mem_Access_AtomicL, mreg_t(-1));
    uint64_t oldval = pmemread<uint64_t>(cpu, paddr);
    LOG_MEMREAD(64, paddr, oldval);
    if (oldval == expected) {
        pmemwrite<uint64_t>(cpu, paddr, desired);
        LOG_MEMWRITE(64, paddr, desired);
    }
    notify_mem_read_write(cpu, true, 8, vaddr, paddr, desired);
    return oldval;
}


//------------------------------------------------------------------------------
//
// External methods: cache ops
//
//------------------------------------------------------------------------------

bool mmu_check_cacheop_access(const Hart& cpu, uint64_t paddr, cacheop_type cop)
{
    try {
        uint64_t addr = paddr & ~(L1D_LINE_SIZE-1ull);
        (void) pma_check_data_access(cpu, addr, addr,  L1D_LINE_SIZE,
                                     Mem_Access_CacheOp, mreg_t(-1), cop);
    }
    catch (const trap_store_access_fault&) {
        return false;
    }
    return true;
}


} // namespace bemu
