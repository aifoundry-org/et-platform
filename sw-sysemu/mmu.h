#ifndef BEMU_MMU_H
#define BEMU_MMU_H

#include <cstdint>
#include <functional>

#include "state.h"
#include "emu_memop.h"

#include "emu_defines.h"

//namespace bemu {


// MMU virtual to physical address translation
uint64_t vmemtranslate(uint64_t addr, size_t size, mem_access_type macc);

// MMU virtual memory read accesses
uint8_t  mmu_load8    (uint64_t eaddr);
uint16_t mmu_load16   (uint64_t eaddr);
uint32_t mmu_load32   (uint64_t eaddr);
uint64_t mmu_load64   (uint64_t eaddr);
void     mmu_loadVLEN (uint64_t eaddr, freg_t& data, mreg_t mask);

// MMU virtual memory write accesses
void mmu_store8    (uint64_t eaddr, uint8_t data);
void mmu_store16   (uint64_t eaddr, uint16_t data);
void mmu_store32   (uint64_t eaddr, uint32_t data);
void mmu_store64   (uint64_t eaddr, uint64_t data);
void mmu_storeVLEN (uint64_t eaddr, freg_t data, mreg_t mask);

// MMU naturally aligned virtual memory read accesses
uint16_t mmu_aligned_load16   (uint64_t eaddr);
uint32_t mmu_aligned_load32   (uint64_t eaddr);
void     mmu_aligned_loadVLEN (uint64_t eaddr, freg_t& data, mreg_t mask);

// MMU naturally aligned virtual memory write accesses
void mmu_aligned_store16   (uint64_t eaddr, uint16_t data);
void mmu_aligned_store32   (uint64_t eaddr, uint32_t data);
void mmu_aligned_storeVLEN (uint64_t eaddr, freg_t data, mreg_t mask);

// MMU global atomic memory accesses
uint32_t mmu_global_atomic32(uint64_t eaddr, uint32_t data,
                             std::function<uint32_t(uint32_t, uint32_t)> fn);
uint64_t mmu_global_atomic64(uint64_t eaddr, uint64_t data,
                             std::function<uint64_t(uint64_t, uint64_t)> fn);
uint32_t mmu_global_compare_exchange32(uint64_t eaddr, uint32_t expected,
                                       uint32_t desired);
uint64_t mmu_global_compare_exchange64(uint64_t eaddr, uint64_t expected,
                                       uint64_t desired);

// MMU local atomic memory accesses
uint32_t mmu_local_atomic32(uint64_t eaddr, uint32_t data,
                            std::function<uint32_t(uint32_t, uint32_t)> fn);
uint64_t mmu_local_atomic64(uint64_t eaddr, uint64_t data,
                            std::function<uint64_t(uint64_t, uint64_t)> fn);
uint32_t mmu_local_compare_exchange32(uint64_t eaddr, uint32_t expected,
                                      uint32_t desired);
uint64_t mmu_local_compare_exchange64(uint64_t eaddr, uint64_t expected,
                                      uint64_t desired);

// Breakpoints
__attribute__((noreturn)) void throw_trap_breakpoint(uint64_t addr);
bool matches_fetch_breakpoint(uint64_t addr);

// Cache-management
bool mmu_check_cacheop_access(uint64_t paddr);


//} // namespace bemu

#endif // BEMU_MMU_H
