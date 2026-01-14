/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#include <array>
#include <cassert>
#include <stdexcept>
#include <type_traits>
#include <climits>

#include "cache.h"
#include "emu_gio.h"
#include "esrs.h"
#include "insn_util.h"
#include "literals.h"
#include "log.h"
#include "pma.h"
#include "mmu.h"
#include "system.h"
#include "traps.h"
#include "utility.h"
#ifdef SYS_EMU
#include "sys_emu.h"
#include "checkers/mem_checker.h"
#endif

namespace bemu {

//------------------------------------------------------------------------------
// Exceptions


Privilege effective_execution_mode(const Hart& cpu, mem_access_type macc)
{
    // Read mstatus
    const uint64_t  mstatus = cpu.mstatus;
    const int       mprv    = (mstatus >> MSTATUS_MPRV) & 0x1;
    const Privilege mpp     = Privilege((mstatus >> MSTATUS_MPP ) & 0x3);
    const Privilege prv     = cpu.prv;
    return (macc == Mem_Access_Fetch) ? prv : (mprv ? mpp : prv);
}


[[noreturn]] void throw_page_fault(uint64_t addr, mem_access_type macc)
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


[[noreturn]] void throw_access_fault(uint64_t addr, mem_access_type macc)
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
    if (halt_on_breakpoint(cpu)) {
        // FIXME(cabul): tdata1.timing
        throw Debug_entry(Debug_entry::Cause::trigger);
    }
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


uint64_t vmemtranslate(const Hart& cpu, uint64_t vaddr, size_t size,
                              mem_access_type macc)
{
    // TODO: We should be using @size (and we probably need a vector mask
    // operand too) to detect page split faults
    (void) size;

    // Read mstatus
    const uint64_t mstatus = cpu.mstatus;
    const int      mxr     = (mstatus >> MSTATUS_MXR ) & 0x1;
    const int      sum     = (mstatus >> MSTATUS_SUM ) & 0x1;

    // Calculate effective privilege level
    const Privilege curprv = effective_execution_mode(cpu, macc);

    // Read matp/satp
    // NB: Sv39/Mv39, Sv48/Mv48, etc. have the same behavior and encoding
    const uint64_t atp = (curprv == Privilege::M)
            ? cpu.core->matp
            : cpu.core->satp;
    const uint64_t atp_mode = (atp >> 60) & 0xF;
    const uint64_t atp_ppn  = atp & PPN_M;

    // V2P mappings are enabled when all of the following are true:
    // - the effective execution mode is 'M' and matp.mode is not "Bare"
    // - the effective execution mode is not 'M' and satp.mode is not "Bare"
    bool vm_enabled = (atp_mode != SATP_MODE_BARE);

    if (!vm_enabled) {
        return vaddr & PA_M;
    }

#if !EMU_HAS_PTW
    throw std::runtime_error("PTW not supported, only BARE mode available");
#endif

    int64_t sign = 0;
    int Num_Levels = 0;
    int PTE_top_Idx_Size = 0;
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
            cpu.chip->memory.read(cpu, pma_check_ptw_access(cpu, vaddr, pte_addr, macc), 8, &pte);
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
            || ((curprv == Privilege::U) && !pte_u)
            || ((curprv == Privilege::S) && pte_u && !sum))
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
            || ((curprv == Privilege::U) && !pte_u)
            || ((curprv == Privilege::S) && pte_u && !sum))
            throw_page_fault(vaddr, macc);
        break;
    case Mem_Access_Fetch:
        if (!pte_x
            || ((curprv == Privilege::U) && !pte_u)
            || ((curprv == Privilege::S) && pte_u))
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
    return paddr;
}


static void ensure_fetch_cache(Hart& cpu, uint64_t vaddr)
{
    if (cpu.fetch_pc == (vaddr & ~31))
        return;

    cpu.fetch_pc = vaddr & ~31;
    try {
        uint64_t paddr = vmemtranslate(cpu, cpu.fetch_pc, 32, Mem_Access_Fetch);
        uint64_t addr = pma_check_fetch_access(cpu, cpu.fetch_pc, paddr, 32);
        cpu.chip->memory.read(cpu, addr, 32, &cpu.fetch_cache);
    }
    catch (const trap_instruction_access_fault&) {
        throw trap_instruction_access_fault(vaddr);
    }
    catch (const trap_instruction_page_fault&) {
        throw trap_instruction_page_fault(vaddr);
    }
    catch (const memory_error&) {
        throw trap_instruction_bus_error();
    }
}


//------------------------------------------------------------------------------
//
// External methods: loads/stores
//
//------------------------------------------------------------------------------

uint64_t mmu_translate(const Hart& cpu, uint64_t vaddr, size_t bytes,
                       mem_access_type macc, cacheop_type cop)
{
    uint64_t paddr = vmemtranslate(cpu, vaddr, bytes, macc);
    return pma_check_data_access(cpu, vaddr, paddr, bytes, macc, mreg_t(-1), cop);
}


uint32_t mmu_fetch(Hart& cpu, uint64_t vaddr)
{
    check_fetch_breakpoint(cpu, vaddr);
    ensure_fetch_cache(cpu, vaddr);
    if (vaddr & 3) {
        // 2B-aligned fetch
        uint16_t low = *reinterpret_cast<const uint16_t*>(&cpu.fetch_cache[vaddr & 31]);
        if ((low & 3) != 3) {
            //LOG_HART(DEBUG, cpu, "Fetched compressed instruction from PC 0x%" PRIx64 ": 0x%04x", vaddr, low);
            return low;
        }
        vaddr += 2;
        ensure_fetch_cache(cpu, vaddr);
        uint16_t high = *reinterpret_cast<const uint16_t*>(&cpu.fetch_cache[vaddr & 31]);
        uint32_t bits = uint32_t(low) + (uint32_t(high) << 16);
        //LOG_HART(DEBUG, cpu, "Fetched instruction from PC 0x%" PRIx64 ": 0x%08x", vaddr, bits);
        return bits;
    }
    // 4B-aligned fetch
    uint32_t bits = *reinterpret_cast<const uint32_t*>(&cpu.fetch_cache[vaddr & 31]);
    if ((bits & 3) != 3) {
        uint16_t low = uint16_t(bits);
        //LOG_HART(DEBUG, cpu, "Fetched compressed instruction from PC 0x%" PRIx64 ": 0x%04x", vaddr, low);
        return low;
    }
    //LOG_HART(DEBUG, cpu, "Fetched instruction from PC 0x%" PRIx64 ": 0x%08x", vaddr, bits);
    return bits;
}


template<typename T>
static T mmu_load_impl(const Hart& cpu, uint64_t eaddr, mem_access_type macc)
{
    uint64_t vaddr = sextVA(eaddr);
    check_load_breakpoint(cpu, vaddr);
    uint64_t paddr = vmemtranslate(cpu, vaddr, sizeof(T), macc);
    size_t len = L1D_LINE_SIZE - (vaddr % L1D_LINE_SIZE);

    T value {};
    if (len >= sizeof(T)) {
        // Access does not cross cache line boundary
        uint64_t addr = pma_check_data_access(cpu, vaddr, paddr, sizeof(T), macc);
        cpu.chip->memory.read(cpu, addr, sizeof(T), &value);
    } else {
        // Access crosses cache line boundary
        uint64_t addr1 = pma_check_data_access(cpu, vaddr, paddr, len, macc);
        uint64_t addr2 = pma_check_data_access(cpu, vaddr + len, paddr + len, sizeof(T) - len, macc);
        cpu.chip->memory.read(cpu, addr1, len, &value);
        cpu.chip->memory.read(cpu, addr2, sizeof(T) - len, reinterpret_cast<char*>(&value) + len);
    }
    LOG_MEMREAD(CHAR_BIT*sizeof(T), paddr, value);
    notify_mem_read(cpu, true, sizeof(T), vaddr, paddr);
    return value;
}


template<typename T>
static T mmu_aligned_load_impl(const Hart& cpu, uint64_t eaddr, mem_access_type macc)
{
    uint64_t vaddr = sextVA(eaddr);
    check_load_breakpoint(cpu, vaddr);
    if (!addr_is_size_aligned(vaddr, sizeof(T))) {
        throw trap_load_access_fault(vaddr);
    }
    uint64_t paddr = vmemtranslate(cpu, vaddr, sizeof(T), macc);
    uint64_t addr = pma_check_data_access(cpu, vaddr, paddr, sizeof(T), macc);
    T value {};
    cpu.chip->memory.read(cpu, addr, sizeof(T), &value);
    LOG_MEMREAD(CHAR_BIT*sizeof(T), paddr, value);
    notify_mem_read(cpu, true, sizeof(T), vaddr, paddr);
    return value;
}


template <size_t Nbytes>
static uint64_t mmu_tensor_load_impl(const Hart& cpu, uint64_t eaddr, uint32_t* data, mem_access_type macc)
{
    uint64_t vaddr = sextVA(eaddr);
    assert(addr_is_size_aligned(vaddr, Nbytes));
    uint64_t paddr = vmemtranslate(cpu, vaddr, Nbytes, macc);
    uint64_t addr = pma_check_data_access(cpu, vaddr, paddr, Nbytes, macc);
    cpu.chip->memory.read(cpu, addr, Nbytes, data);
    return paddr;
}


uint8_t mmu_load8(const Hart& cpu, uint64_t eaddr, mem_access_type macc)
{
    // NB: alignment is irrelevant for byte accesses, but the aligned method
    // is a bit faster to execute
    return mmu_aligned_load_impl<uint8_t>(cpu, eaddr, macc);
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
    return mmu_aligned_load_impl<uint16_t>(cpu, eaddr, macc);
}


uint32_t mmu_aligned_load32(const Hart& cpu, uint64_t eaddr, mem_access_type macc)
{
    return mmu_aligned_load_impl<uint32_t>(cpu, eaddr, macc);
}


void mmu_tensor_load128(const Hart& cpu, uint64_t eaddr, uint32_t* data, mem_access_type macc)
{
    uint64_t addr = mmu_tensor_load_impl<16>(cpu, eaddr, data, macc);
    LOG_MEMREAD128(addr, data);
}


void mmu_tensor_load256(const Hart& cpu, uint64_t eaddr, uint32_t* data, mem_access_type macc)
{
    uint64_t addr = mmu_tensor_load_impl<32>(cpu, eaddr, data, macc);
    LOG_MEMREAD256(addr, data);
}


void mmu_tensor_load512(const Hart& cpu, uint64_t eaddr, uint32_t* data, mem_access_type macc)
{
    uint64_t addr = mmu_tensor_load_impl<64>(cpu, eaddr, data, macc);
    LOG_MEMREAD512(addr, data);
}


void mmu_loadVLEN(const Hart& cpu, uint64_t eaddr, freg_t& data, mreg_t mask, mem_access_type macc)
{
    if (!mask.any())
        return;

    uint64_t vaddr = sextVA(eaddr);
    check_load_breakpoint(cpu, vaddr);
    uint64_t paddr = vmemtranslate(cpu, vaddr, VLENB, macc);
    // We have three cases here:
    // 1. The whole vector fits in one cache line
    // 2. The vector crosses a cache line at an element boundary
    // 3. The vector crosses a cache line in the middle of an element
    size_t len = L1D_LINE_SIZE - (vaddr % L1D_LINE_SIZE);
    if (len >= VLENB) {
        uint64_t addr = pma_check_data_access(cpu, vaddr, paddr, VLENB, macc, mask);
        for (size_t e = 0; e < MLEN; ++e) {
            if (mask[e]) {
                cpu.chip->memory.read(cpu, addr + 4*e, 4, &data.u32[e]);
                LOG_MEMREAD(32, paddr + 4*e, data.u32[e]);
            }
            notify_mem_read(cpu, mask[e], 4, vaddr + 4*e, paddr + 4*e);
        }
    }
    else if (len % 4 == 0) {
        for (size_t e = 0; e < MLEN; ++e) {
            if (mask[e]) {
                uint64_t addr = pma_check_data_access(cpu, vaddr + 4*e, paddr + 4*e, 4, macc);
                cpu.chip->memory.read(cpu, addr, 4, &data.u32[e]);
                LOG_MEMREAD(32, paddr + 4*e, data.u32[e]);
            }
            notify_mem_read(cpu, mask[e], 4, vaddr + 4*e, paddr + 4*e);
        }
    }
    else {
        len = len % 4;
        for (size_t e = 0; e < MLEN; ++e) {
            if (mask[e]) {
                uint64_t addr1 = pma_check_data_access(cpu, vaddr + 4*e, paddr + 4*e, len, macc);
                uint64_t addr2 = pma_check_data_access(cpu, vaddr + 4*e + len, paddr + 4*e + len, 4 - len, macc);
                cpu.chip->memory.read(cpu, addr1, len, &data.u8[4*e]);
                cpu.chip->memory.read(cpu, addr2, 4 - len, &data.u8[4*e + len]);
                LOG_MEMREAD(32, paddr + 4*e, data.u32[e]);
            }
            notify_mem_read(cpu, mask[e], 4, vaddr + 4*e, paddr + 4*e);
        }
    }
}


void mmu_aligned_loadVLEN(const Hart& cpu, uint64_t eaddr, freg_t& data, mreg_t mask, mem_access_type macc)
{
    if (!mask.any())
        return;

    uint64_t vaddr = sextVA(eaddr);
    check_load_breakpoint(cpu, vaddr);
    if (!addr_is_size_aligned(vaddr, VLENB)) {
        throw trap_load_access_fault(vaddr);
    }
    uint64_t paddr = vmemtranslate(cpu, vaddr, VLENB, macc);
    uint64_t addr = pma_check_data_access(cpu, vaddr, paddr, VLENB, macc, mask);
    for (size_t e = 0; e < MLEN; ++e) {
        if (mask[e]) {
            cpu.chip->memory.read(cpu, addr + 4*e, 4, &data.u32[e]);
            LOG_MEMREAD(32, paddr + 4*e, data.u32[e]);
        }
        notify_mem_read(cpu, mask[e], 4, vaddr + 4*e, paddr + 4*e);
    }
}


template <typename T>
static void mmu_store_impl(const Hart& cpu, uint64_t eaddr, T data, mem_access_type macc)
{
    uint64_t vaddr = sextVA(eaddr);
    check_store_breakpoint(cpu, vaddr);
    uint64_t paddr = vmemtranslate(cpu, vaddr, sizeof(T), macc);
    size_t len = L1D_LINE_SIZE - (vaddr % L1D_LINE_SIZE);
    if (len >= sizeof(T)) {
        // Access does not cross cache line boundary
        uint64_t addr = pma_check_data_access(cpu, vaddr, paddr, sizeof(T), macc);
        cpu.chip->memory.write(cpu, addr, sizeof(T), &data);
    } else {
        // Access crosses cache line boundary
        uint64_t addr1 = pma_check_data_access(cpu, vaddr, paddr, len, macc);
        uint64_t addr2 = pma_check_data_access(cpu, vaddr + len, paddr + len, sizeof(T) - len, macc);
        cpu.chip->memory.write(cpu, addr1, len, &data);
        cpu.chip->memory.write(cpu, addr2, sizeof(T) - len, reinterpret_cast<char*>(&data) + len);
    }
    LOG_MEMWRITE(CHAR_BIT*sizeof(T), paddr, data);
    notify_mem_write(cpu, true, sizeof(T), vaddr, paddr, data);
}


template <typename T>
static void mmu_aligned_store_impl(const Hart& cpu, uint64_t eaddr, T data, mem_access_type macc)
{
    uint64_t vaddr = sextVA(eaddr);
    check_store_breakpoint(cpu, vaddr);
    if (!addr_is_size_aligned(vaddr, sizeof(T))) {
        throw trap_store_access_fault(vaddr);
    }
    uint64_t paddr = vmemtranslate(cpu, vaddr, sizeof(T), macc);
    uint64_t addr = pma_check_data_access(cpu, vaddr, paddr, sizeof(T), macc);
    cpu.chip->memory.write(cpu, addr, sizeof(T), &data);
    LOG_MEMWRITE(CHAR_BIT*sizeof(T), paddr, data);
    notify_mem_write(cpu, true, sizeof(T), vaddr, paddr, data);
}


template <size_t Nbytes>
static uint64_t mmu_tensor_store_impl(const Hart& cpu, uint64_t eaddr, const uint32_t* data, mem_access_type macc)
{
    uint64_t vaddr = sextVA(eaddr);
    assert(addr_is_size_aligned(vaddr, Nbytes));
    uint64_t paddr = vmemtranslate(cpu, vaddr, Nbytes, macc);
    uint64_t addr = pma_check_data_access(cpu, vaddr, paddr, Nbytes, macc);
    cpu.chip->memory.write(cpu, addr, Nbytes, data);
    if (macc == Mem_Access_TxStore) {
        static constexpr unsigned n_words = Nbytes / 4;
        for (unsigned i = 0; i < n_words; ++i) {
            notify_tensor_store_write(cpu, paddr + i * 4, data[i]);
        }
    }
    return paddr;
}


void mmu_store8(const Hart& cpu, uint64_t eaddr, uint8_t  data, mem_access_type macc)
{
    // NB: alignment is irrelevant for byte accesses, but the aligned method
    // is a bit faster to execute
    mmu_aligned_store_impl<uint8_t>(cpu, eaddr, data, macc);
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
    mmu_aligned_store_impl<uint16_t>(cpu, eaddr, data, macc);
}


void mmu_aligned_store32(const Hart& cpu, uint64_t eaddr, uint32_t data, mem_access_type macc)
{
    mmu_aligned_store_impl<uint32_t>(cpu, eaddr, data, macc);
}


void mmu_tensor_store128(const Hart& cpu, uint64_t eaddr, const uint32_t* data, mem_access_type macc)
{
    uint64_t addr = mmu_tensor_store_impl<16>(cpu, eaddr, data, macc);
    LOG_MEMWRITE128(addr, data);
}


void mmu_tensor_store256(const Hart& cpu, uint64_t eaddr, const uint32_t* data, mem_access_type macc)
{
    uint64_t addr = mmu_tensor_store_impl<32>(cpu, eaddr, data, macc);
    LOG_MEMWRITE256(addr, data);
}


void mmu_tensor_store512(const Hart& cpu, uint64_t eaddr, const uint32_t* data, mem_access_type macc)
{
    uint64_t addr = mmu_tensor_store_impl<64>(cpu, eaddr, data, macc);
    LOG_MEMWRITE512(addr, data);
}


void mmu_storeVLEN(const Hart& cpu, uint64_t eaddr, const freg_t& data, mreg_t mask, mem_access_type macc)
{
    if (!mask.any())
        return;

    uint64_t vaddr = sextVA(eaddr);
    check_store_breakpoint(cpu, vaddr);
    uint64_t paddr = vmemtranslate(cpu, vaddr, VLENB, macc);
    // We have three cases here:
    // 1. The whole vector fits in one cache line
    // 2. The vector crosses a cache line at an element boundary
    // 3. The vector crosses a cache line in the middle of an element
    size_t len = L1D_LINE_SIZE - (vaddr % L1D_LINE_SIZE);
    if (len >= VLENB) {
        uint64_t addr = pma_check_data_access(cpu, vaddr, paddr, VLENB, macc, mask);
        for (size_t e = 0; e < MLEN; ++e) {
            if (mask[e]) {
                cpu.chip->memory.write(cpu, addr + 4*e, 4, &data.u32[e]);
                LOG_MEMWRITE(32, paddr + 4*e, data.u32[e]);
            }
            notify_mem_write(cpu, mask[e], 4, vaddr + 4*e, paddr + 4*e, data.u32[e]);
        }
    }
    else if (len % 4 == 0) {
        for (size_t e = 0; e < MLEN; ++e) {
            if (mask[e]) {
                uint64_t addr = pma_check_data_access(cpu, vaddr + 4*e, paddr + 4*e, 4, macc);
                cpu.chip->memory.write(cpu, addr, 4, &data.u32[e]);
                LOG_MEMWRITE(32, paddr + 4*e, data.u32[e]);
            }
            notify_mem_write(cpu, mask[e], 4, vaddr + 4*e, paddr + 4*e, data.u32[e]);
        }
    }
    else {
        len = len % 4;
        for (size_t e = 0; e < MLEN; ++e) {
            if (mask[e]) {
                uint64_t addr1 = pma_check_data_access(cpu, vaddr + 4*e, paddr + 4*e, len, macc);
                uint64_t addr2 = pma_check_data_access(cpu, vaddr + 4*e + len, paddr + 4*e + len, 4 - len, macc);
                cpu.chip->memory.write(cpu, addr1, len, &data.u8[4*e]);
                cpu.chip->memory.write(cpu, addr2, 4 - len, &data.u8[4*e + len]);
                LOG_MEMWRITE(32, paddr + 4*e, data.u32[e]);
            }
            notify_mem_write(cpu, mask[e], 4, vaddr + 4*e, paddr + 4*e, data.u32[e]);
        }
    }
}


void mmu_aligned_storeVLEN(const Hart& cpu, uint64_t eaddr, const freg_t& data, mreg_t mask, mem_access_type macc)
{
    if (!mask.any())
        return;

    uint64_t vaddr = sextVA(eaddr);
    check_store_breakpoint(cpu, vaddr);
    if (!addr_is_size_aligned(vaddr, VLENB)) {
        throw trap_store_access_fault(vaddr);
    }
    uint64_t paddr = vmemtranslate(cpu, vaddr, VLENB, macc);
    uint64_t addr = pma_check_data_access(cpu, vaddr, paddr, VLENB, macc, mask);
    for (size_t e = 0; e < MLEN; ++e) {
        if (mask[e]) {
            cpu.chip->memory.write(cpu, addr + 4*e, 4, &data.u32[e]);
            LOG_MEMWRITE(32, paddr + 4*e, data.u32[e]);
        }
        notify_mem_write(cpu, mask[e], 4, vaddr + 4*e, paddr + 4*e, data.u32[e]);
    }
}


template<typename T, mem_access_type M>
T mmu_atomic_impl(const Hart& cpu, uint64_t eaddr, T data, std::function<T(T, T)> fn)
{
    uint64_t vaddr = sextVA(eaddr);
    check_store_breakpoint(cpu, vaddr);
    if (!addr_is_size_aligned(vaddr, sizeof(T))) {
        throw trap_store_access_fault(vaddr);
    }
    uint64_t paddr = vmemtranslate(cpu, vaddr, sizeof(T), M);
    uint64_t addr = pma_check_data_access(cpu, vaddr, paddr, sizeof(T), M);
    T oldval {};
    cpu.chip->memory.read(cpu, addr, sizeof(T), &oldval);
    LOG_MEMREAD(CHAR_BIT*sizeof(T), paddr, oldval);
    T newval = fn(oldval, data);
    cpu.chip->memory.write(cpu, addr, sizeof(T), &newval);
    LOG_MEMWRITE(CHAR_BIT*sizeof(T), paddr, newval);
    notify_mem_read_write(cpu, true, sizeof(T), vaddr, paddr, data);
    return oldval;
}


uint32_t mmu_global_atomic32(const Hart& cpu, uint64_t eaddr, uint32_t data,
                             std::function<uint32_t(uint32_t, uint32_t)> fn)
{
    return mmu_atomic_impl<uint32_t, Mem_Access_AtomicG>(cpu, eaddr, data, fn);
}


uint64_t mmu_global_atomic64(const Hart& cpu, uint64_t eaddr, uint64_t data,
                             std::function<uint64_t(uint64_t, uint64_t)> fn)
{
    return mmu_atomic_impl<uint64_t, Mem_Access_AtomicG>(cpu, eaddr, data, fn);
}


uint32_t mmu_local_atomic32(const Hart& cpu, uint64_t eaddr, uint32_t data,
                            std::function<uint32_t(uint32_t, uint32_t)> fn)
{
    return mmu_atomic_impl<uint32_t, Mem_Access_AtomicL>(cpu, eaddr, data, fn);
}


uint64_t mmu_local_atomic64(const Hart& cpu, uint64_t eaddr, uint64_t data,
                            std::function<uint64_t(uint64_t, uint64_t)> fn)
{
    return mmu_atomic_impl<uint64_t, Mem_Access_AtomicL>(cpu, eaddr, data, fn);
}


template<typename T, mem_access_type M>
T mmu_compare_exchange_impl(const Hart& cpu, uint64_t eaddr, T expected, T desired)
{
    T oldval {};
    uint64_t vaddr = sextVA(eaddr);
    check_store_breakpoint(cpu, vaddr);
    if (!addr_is_size_aligned(vaddr, sizeof(T))) {
        throw trap_store_access_fault(vaddr);
    }
    uint64_t paddr = vmemtranslate(cpu, vaddr, sizeof(T), M);
    uint64_t addr = pma_check_data_access(cpu, vaddr, paddr, sizeof(T), M);
    cpu.chip->memory.read(cpu, addr, sizeof(T), &oldval);
    LOG_MEMREAD(CHAR_BIT*sizeof(T), paddr, oldval);
    if (oldval == expected) {
        cpu.chip->memory.write(cpu, addr, sizeof(T), &desired);
        LOG_MEMWRITE(CHAR_BIT*sizeof(T), paddr, desired);
    }
    notify_mem_read_write(cpu, true, sizeof(T), vaddr, paddr, desired);
    return oldval;
}


uint32_t mmu_global_compare_exchange32(const Hart& cpu, uint64_t eaddr,
                                       uint32_t expected, uint32_t desired)
{
    return mmu_compare_exchange_impl<uint32_t, Mem_Access_AtomicG>(cpu, eaddr, expected, desired);
}


uint64_t mmu_global_compare_exchange64(const Hart& cpu, uint64_t eaddr,
                                       uint64_t expected, uint64_t desired)
{
    return mmu_compare_exchange_impl<uint64_t, Mem_Access_AtomicG>(cpu, eaddr, expected, desired);
}


uint32_t mmu_local_compare_exchange32(const Hart& cpu, uint64_t eaddr,
                                      uint32_t expected, uint32_t desired)
{
    return mmu_compare_exchange_impl<uint32_t, Mem_Access_AtomicL>(cpu, eaddr, expected, desired);
}


uint64_t mmu_local_compare_exchange64(const Hart& cpu, uint64_t eaddr,
                                      uint64_t expected, uint64_t desired)
{
    return mmu_compare_exchange_impl<uint64_t, Mem_Access_AtomicL>(cpu, eaddr, expected, desired);
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
