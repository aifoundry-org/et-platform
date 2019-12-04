/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#ifndef BEMU_MMU_H
#define BEMU_MMU_H

#include <cstdint>
#include <functional>

#include "state.h"
#include "emu_defines.h"

//namespace bemu {


// MMU virtual to physical address translation
uint64_t vmemtranslate(uint64_t addr, size_t size, mem_access_type macc, mreg_t mask, cacheop_type cop = CacheOp_None);


// MMU virtual memory read accesses
uint32_t mmu_fetch  (uint64_t vaddr);

template <typename T>
T mmu_load          (uint64_t eaddr, mem_access_type macc);

void mmu_loadVLEN   (uint64_t eaddr, freg_t& data, mreg_t mask, mem_access_type macc);


// MMU virtual memory write accesses
template <typename T>
void mmu_store      (uint64_t eaddr, T data, mem_access_type macc);

void mmu_storeVLEN  (uint64_t eaddr, freg_t data, mreg_t mask, mem_access_type macc);


// MMU naturally aligned virtual memory read accesses
uint16_t mmu_aligned_load16   (uint64_t eaddr, mem_access_type macc);
uint32_t mmu_aligned_load32   (uint64_t eaddr, mem_access_type macc);
void     mmu_aligned_loadVLEN (uint64_t eaddr, freg_t& data, mreg_t mask, mem_access_type macc);


// MMU naturally aligned virtual memory write accesses
void mmu_aligned_store16   (uint64_t eaddr, uint16_t data, mem_access_type macc);
void mmu_aligned_store32   (uint64_t eaddr, uint32_t data, mem_access_type macc);
void mmu_aligned_storeVLEN (uint64_t eaddr, freg_t data, mreg_t mask, mem_access_type macc);


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


// Cache-management
bool mmu_check_cacheop_access(uint64_t paddr, cacheop_type cop);


//} // namespace bemu

#endif // BEMU_MMU_H
