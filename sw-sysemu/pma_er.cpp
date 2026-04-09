/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#include <cinttypes>

#include "pma.h"
#include "mmu.h"
#include "esrs.h"
#include "system.h"
#include "utility.h"

namespace bemu {

//------------------------------------------------------------------------------
// Erbium memory region address helpers
//
//   0x0200_0000 - 0x0200_0FFF: System registers (4K)
//   0x0200_1000 - 0x0200_1FFF: MRAM registers (4K)
//   0x0200_2000 - 0x0200_2FFF: Periph registers (4K)
//   0x0200_F000 - 0x0200_FFFF: xSPI registers (4K)
//   0x0200_4000 - 0x0200_4FFF: UART registers (4K)
//   0x0200_8000 - 0x0200_9FFF: Bootrom (8K)
//   0x0200_C000 - 0x0200_CFFF: Scratch SRAM (4K)
//   0x4000_0000 - 0x7FFF_FFFF: MRAM (1G, but only 16MB installed)
//   0x7FFF_D000 - 0x7FFF_FFFF: OTP (12K, read-only)
//   0x8000_0000 - 0x9FFF_FFFF: ESR/CPU registers (512M)
//   0xA000_0000 - 0xA3FF_FFFF: PLIC (64M)

static inline bool paddr_is_sysreg(uint64_t addr)
{ return (addr >= 0x02000000ull) && (addr < 0x02001000ull); }

static inline bool paddr_is_mramreg(uint64_t addr)
{ return (addr >= 0x02001000ull) && (addr < 0x02002000ull); }

static inline bool paddr_is_periph(uint64_t addr)
{ return (addr >= 0x02002000ull) && (addr < 0x02003000ull); }

static inline bool paddr_is_xspi(uint64_t addr)
{ return (addr >= 0x0200F000ull) && (addr < 0x02010000ull); }

static inline bool paddr_is_uart(uint64_t addr)
{ return (addr >= 0x02004000ull) && (addr < 0x02005000ull); }

static inline bool paddr_is_bootrom(uint64_t addr)
{ return (addr >= 0x02008000ull) && (addr < 0x0200A000ull); }

static inline bool paddr_is_sram(uint64_t addr)
{ return (addr >= 0x0200C000ull) && (addr < 0x0200D000ull); }

static inline bool paddr_is_otp(uint64_t addr)
{ return (addr >= 0x7FFFD000ull) && (addr < 0x80000000ull); }

static inline bool paddr_is_mram(uint64_t addr)
{ return (addr >= 0x40000000ull) && (addr < 0x80000000ull); }

static inline bool paddr_is_esr(uint64_t addr)
{ return (addr >= 0x80000000ull) && (addr < 0xA0000000ull); }

static inline bool paddr_is_plic(uint64_t addr)
{ return (addr >= 0xA0000000ull) && (addr < 0xA4000000ull); }



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


// PP (privilege protection) bits extraction from ESR address
#define PP(x) (int(((x) & ESR_REGION_PROT_MASK) >> ESR_REGION_PROT_SHIFT))


//------------------------------------------------------------------------------
// MRAM PMP

#define MPROT_EN              0x100
#define MPROT_MMODE_SIZE(x)   (((x) >> 4) & 0xF)
#define MPROT_SMODE_SIZE(x)   ((x) & 0xF)
#define MRAM_BASE             0x40000000ull

// mmode_end = MRAM_BASE + 4KB * (2^mmode_size), capped at 16MB
static inline uint64_t mmode_region_end(uint16_t mprot)
{
    unsigned mmode_size = MPROT_MMODE_SIZE(mprot);
    // Cap at 16MB
    uint64_t size = 0x1000ull << (mmode_size > 12 ? 12 : mmode_size);
    return MRAM_BASE + size;
}

// smode_end = MRAM_BASE + 4KB * (2^smode_size), capped at 16MB
static inline uint64_t smode_region_end(uint16_t mprot)
{
    unsigned smode_size = MPROT_SMODE_SIZE(mprot);
    // Cap at 16MB
    uint64_t size = 0x1000ull << (smode_size > 12 ? 12 : smode_size);
    return MRAM_BASE + size;
}

static bool check_mram_pmp_access(uint64_t addr, uint16_t mprot, Privilege mode) {
    // When MPROT_EN is set:
    //   - [MRAM_BASE, mmode_end): M-mode only
    //   - [mmode_end, smode_end): S-mode and above
    //   - [smode_end, ...): All modes (U/S/M)

    if (mprot & MPROT_EN) {
        uint64_t mmode_end = mmode_region_end(mprot);
        uint64_t smode_end = smode_region_end(mprot);

        if (addr < mmode_end) {
            if (mode != Privilege::M) {
                return false;
            }
        }
        else if (smode_end > mmode_end && addr < smode_end) {
            if (mode == Privilege::U) {
                return false;
            }
        }
    }

    return true;
}


//------------------------------------------------------------------------------
// PMA check functions

uint64_t pma_check_data_access(const Hart& cpu, uint64_t vaddr,
                               uint64_t addr, size_t size,
                               mem_access_type macc,
                               mreg_t mask,
                               cacheop_type cop)
{
    (void)mask; (void)cop;

    bool amo = (macc == Mem_Access_AtomicL) || (macc == Mem_Access_AtomicG);
    bool tensor = (macc == Mem_Access_TxLoad) || (macc == Mem_Access_TxStore)
                  || (macc == Mem_Access_TxLoadL2Scp);
    bool cacheop = (macc == Mem_Access_CacheOp);
    bool ts_tl_co = tensor || cacheop;

    if (paddr_is_otp(addr)) {
        // OTP: read-only, no AMO/TensorOp/CacheOp
        if (amo
            || ts_tl_co
            || data_access_is_write(macc))
        {
            throw_access_fault(vaddr, macc);
        }
        return addr;
    }

    if (paddr_is_mram(addr)) {
        uint16_t mprot = cpu.chip->neigh_esrs[neigh_index(cpu)].mprot;

        Privilege mode = effective_execution_mode(cpu, macc);

        if (!check_mram_pmp_access(addr, mprot, mode)) {
            throw_access_fault(vaddr, macc);
        }

        return addr;
    }

    if (paddr_is_esr(addr)) {
        int pp = PP(addr);
        Privilege mode = effective_execution_mode(cpu, macc);

        // ESRs: 64-bit aligned, 64-bit access, no AMO/TensorOp/CacheOp,
        // address encoded (PP) privilege requirements
        if (amo
            || ts_tl_co
            || (size != 8)
            || !addr_is_size_aligned(addr, size)
            || (pp > static_cast<int>(mode)))
        {
            throw_access_fault(vaddr, macc);
        }
        return addr;
    }

    if (paddr_is_plic(addr)) {
        // PLIC: 32-bit aligned, 32-bit access, M/S privilege, no AMO/TensorOp/CacheOp
        Privilege mode = effective_execution_mode(cpu, macc);
        if (amo
            || ts_tl_co
            || (size != 4)
            || !addr_is_size_aligned(addr, 4)
            || (mode == Privilege::U))
        {
            throw_access_fault(vaddr, macc);
        }
        return addr;
    }

    if (paddr_is_bootrom(addr)) {
        // Bootrom: Read-only, no AMO/TensorOp/CacheOp/Prefetch
        if (amo
            || tensor
            || macc == Mem_Access_Prefetch
            || data_access_is_write(macc))
        {
            throw_access_fault(vaddr, macc);
        }
        return addr;
    }

    if (paddr_is_sram(addr)) {
        // SRAM: No AMO (TensorOp and CacheOp allowed)
        if (amo) {
            throw_access_fault(vaddr, macc);
        }
        return addr;
    }

    if (paddr_is_sysreg(addr)) {
        // System registers: 64-bit aligned, 32-bit access, M/S privilege,
        // no AMO/TensorOp/CacheOp
        Privilege mode = effective_execution_mode(cpu, macc);
        if (amo
            || ts_tl_co
            || (size != 4)
            || !addr_is_size_aligned(addr, 8)
            || (mode == Privilege::U))
        {
            throw_access_fault(vaddr, macc);
        }
        return addr;
    }

    if (paddr_is_mramreg(addr)) {
        // MRAM registers: 64-bit aligned, 64-bit access, M-mode only,
        // no AMO/TensorOp/CacheOp
        Privilege mode = effective_execution_mode(cpu, macc);
        if (amo
            || ts_tl_co
            || (size != 8)
            || !addr_is_size_aligned(addr, 8)
            || (mode != Privilege::M))
        {
            throw_access_fault(vaddr, macc);
        }
        return addr;
    }

    if (paddr_is_periph(addr)) {
        // Periph registers: 32-bit aligned, 32-bit access, M/S privilege,
        // no AMO/TensorOp/CacheOp
        Privilege mode = effective_execution_mode(cpu, macc);
        if (amo
            || ts_tl_co
            || (size != 4)
            || !addr_is_size_aligned(addr, 4)
            || (mode == Privilege::U))
        {
            throw_access_fault(vaddr, macc);
        }
        return addr;
    }

    if (paddr_is_xspi(addr)) {
        // xSPI registers: 32-bit aligned, 32-bit access, M-mode only,
        // no AMO/TensorOp/CacheOp
        Privilege mode = effective_execution_mode(cpu, macc);
        if (amo
            || ts_tl_co
            || (size != 4)
            || !addr_is_size_aligned(addr, 4)
            || (mode != Privilege::M))
        {
            throw_access_fault(vaddr, macc);
        }
        return addr;
    }

    if (paddr_is_uart(addr)) {
        // UART registers: 64-bit aligned, 32-bit access, M/S privilege,
        // no AMO/TensorOp/CacheOp
        Privilege mode = effective_execution_mode(cpu, macc);
        if (amo
            || ts_tl_co
            || (size != 4)
            || !addr_is_size_aligned(addr, 8)
            || (mode == Privilege::U))
        {
            throw_access_fault(vaddr, macc);
        }
        return addr;
    }

    // Unknown/reserved region - access fault
    throw_access_fault(vaddr, macc);
}


uint64_t pma_check_fetch_access(const Hart& cpu, uint64_t vaddr,
                                uint64_t addr, size_t size)
{
    (void)size;

    if (paddr_is_bootrom(addr)) {
        return addr;
    }

    if (paddr_is_sram(addr)) {
        return addr;
    }

    if (paddr_is_mram(addr)) {
        // MRAM subject to MPROT protection
        uint16_t mprot = cpu.chip->neigh_esrs[neigh_index(cpu)].mprot;

        Privilege mode = effective_execution_mode(cpu, Mem_Access_Fetch);

        if (!check_mram_pmp_access(addr, mprot, mode)) {
            throw_access_fault(vaddr, Mem_Access_Fetch);
        }

        return addr;
    }

    throw_access_fault(vaddr, Mem_Access_Fetch);
}


uint64_t pma_check_ptw_access(const Hart& cpu, uint64_t vaddr,
                              uint64_t addr, mem_access_type macc)
{
    (void)cpu; (void)vaddr; (void)addr; (void)macc;
    // Erbium does not support PTW - always operates in bare mode
    throw std::runtime_error("pma_check_ptw_access: PTW not supported on Erbium");
}

} // namespace bemu
