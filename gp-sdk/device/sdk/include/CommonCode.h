/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#ifndef _COMMON_CODE_H_
#define _COMMON_CODE_H_

/*! \file CommonCode.h
    \brief Efficient routines for common mem copy operations.
*/

// Global
#include <cstddef>
#include <inttypes.h>
#include <etsoc/common/utils.h>
#include <etsoc/isa/atomic.h>
#include <etsoc/isa/cacheops-umode.h>
#include <etsoc/isa/tensors.h>
#include <system/abi.h>

#include "gpsdk_launch_runtime.h"
#include "gpsdk_star_scratchpad.h"

static inline uint8_t readByte(uint8_t * addr);
static inline void writeByte(uint8_t * addr, uint8_t val);
static inline void evictCacheLine(uint64_t dst, uint8_t * addr);
static inline bool isFormat0ScratchpadAddress(uint64_t addr);
static inline bool isAccessibleErbiumSimScratchpadAddress(uint64_t addr, size_t sizeBytes);
static inline void assertErbiumSimScratchpadAddress(const volatile void* ptr, size_t sizeBytes);
static inline void assertErbiumSimTensorLineAccess(uint64_t addr, uint64_t stride, uint64_t lineCount,
                                                   size_t accessBytes);

/**
 * Copies \p num_bytes bytes from the object pointed to by \p src to the object pointed to by \p dst. Both object
 * pointers must be 32-byte aligned. 
 * \brief Copies bytes from a memory position in the device to another. global address scope.
 * \param src pointer to the memory location to copy from
 * \param dst pointer to the memory location to copy to
 * \param num_bytes
 * number of bytes to copy
 */
static inline int global_memcpy(void * dst, const void * src, size_t num_bytes) {
  assertErbiumSimScratchpadAddress(dst, num_bytes);
  assertErbiumSimScratchpadAddress(src, num_bytes);
  
  constexpr size_t stride = 32; // vector width is 32 bytes
  /* cast to 1-byte ptr type, needed for pointer arithmetic */
  uint8_t *d = static_cast<uint8_t *>(dst);
  const uint8_t *s = static_cast<const uint8_t *>(src);

  /* Check 32 byte alignment start and size multiple */
  bool aligned_ptrs = (reinterpret_cast<uintptr_t>(s) % 32UL == 0) &&
                      (reinterpret_cast<uintptr_t>(d) % 32UL == 0);
  bool aligned_size = (num_bytes == 0) || ((num_bytes >> 5) != 0);

  et_assert(aligned_ptrs && "src/dst pointers are not 32-byte aligned");
  et_assert(aligned_size && "num_bytes is not multiple of 32-bytes")

  float tmp;
  constexpr uint32_t mask = 0xff;
#ifndef __clang__
    mask_set(0, mask);
#endif
  for (size_t i = 0; i < num_bytes; i += stride) {
    __asm__ __volatile__("flwg.ps %[tmp], (%[src])\n"
                         "fswg.ps %[tmp], (%[dst])\n"
                         : [ tmp ] "=&f" (tmp)
                         : [ src ] "r" (s + i),
                           [ dst ] "r"(d + i)
                       #ifdef __clang__
                         , [ mask ] "M"(mask) 
                       #endif
                         :);
  }
  return 0;
}

/**
 * Copies \p num_bytes bytes from the object pointed to by \p src to the object pointed to by \p dst. Both object
 * pointers must be 32-byte aligned. 
 * \brief Copies bytes from a memory position in the device to another. local address scope.
 *  \param src  pointer to the memory location to copy from 
 *  \param dst pointer to the memory location to copy to 
 *  \param num_bytes  bytes to copy
 */
static inline int local_memcpy(void * dst, const void * src, size_t num_bytes) {
  
  constexpr size_t stride = 32; // vector width is 32 bytes
  /* cast to 1-byte ptr type, needed for pointer arithmetic */
  uint8_t *d = static_cast<uint8_t *>(dst);
  const uint8_t *s = static_cast<const uint8_t *>(src);

  /* Check 32 byte alignment start and size multiple */
  bool aligned_ptrs = (reinterpret_cast<uintptr_t>(s) % 32UL == 0) &&
                      (reinterpret_cast<uintptr_t>(d) % 32UL == 0);
  bool aligned_size = ((num_bytes >> 5) != 0);

  // slow-path: non aligned.
  if(!aligned_ptrs || !aligned_size) {
    et_memcpy(dst,src,num_bytes);
    return 0;
  }
  // fast-path: aligned.
  float tmp;
  constexpr uint32_t mask = 0xff;
#ifndef __clang__
    mask_set(0, mask);
#endif
  for (size_t i = 0; i < num_bytes; i += stride) {
    __asm__ __volatile__("flw.ps %[tmp], 0(%[src])\n"
                         "fsw.ps %[tmp], 0(%[dst])\n"
                         : [ tmp ] "=&f" (tmp)
                         : [ src ] "r" (s + i),
                           [ dst ] "r"(d + i)
                       #ifdef __clang__
                         , [ mask ] "M"(mask) 
                       #endif
                         :);
  }
  return 0;
}


/*! \cond PRIVATE */

/**
 * Copies the static_cast<int>(\p value) repeatedly in 32-bit strides starting at \p ptr memory addres until \p
 * num_bytes are written. \brief Sets a region of memory to the same value
 *
 * \param ptr pointer to the memory location to copy to
 * \param value 32-bit value to write in memory
 * \param num_bytes number of bytes to write
 */
static inline int global_memset(void * ptr, const int value, size_t num_bytes) {
  (void)value;
  assertErbiumSimScratchpadAddress(ptr, num_bytes);
  /* vector width is 32 bytes (256-bit) */
  constexpr int64_t stride = 32;

  /* cast to 1-byte type, enables pointer arithmetic */
  uint8_t *p = static_cast<uint8_t *>(ptr);

  /* Check 32 byte alignment start */
  bool aligned_start = (reinterpret_cast<uintptr_t>(p) % 32UL == 0);
  // bool aligned_size = (num_bytes >> 5) != 0;

  et_assert(aligned_start && "ptr is not 32-byte aligned");
  // et_assert(aligned_size && "num_bytes is not multiple of 32-bytes");

  // Broadcast value
  float valueVector;
  constexpr uint32_t mask = 0xff;
#ifndef __clang__
    mask_set(0, mask);
#endif
  __asm__ __volatile__("fbcx.ps %[valueVector], %[value]\n"
                      : [ valueVector ] "=&f" (valueVector)
                      : [ value ] "r" (value)
                     #ifdef __clang__
                       , [ mask ] "M"(mask) 
                     #endif
                      :);
  int64_t i;
  for (i = 0; i < (int64_t) num_bytes - (stride - 1); i += stride) {
    __asm__ __volatile__( "fswg.ps %[valueVector], (%[ptr])\n"
                          :  
                          : [ ptr ] "r"(p + i), [ valueVector ] "f" (valueVector)
                       #ifdef __clang__
                         , [ mask ] "M"(mask) 
                       #endif
                          :);
  }
  // this line will be evicted
  uint8_t * evict_addr = p + i;
  for (; i < (int64_t) num_bytes; i++) {
    uint8_t tmp = readByte(p + i);
    writeByte(p + i, tmp);
  }

  evictCacheLine(0x3, evict_addr);

  return 0;
}
/*! \endcond */

static inline uint8_t readByte(uint8_t * addr)
{
  uint8_t val;
  asm volatile(
      "lb %0, %1\n"
      : "=r" (val)
      : "m" (*(const volatile uint8_t *)addr));
  return val;
}

static inline void writeByte(uint8_t * addr, uint8_t val)
{
  asm volatile(
      "sb %1, %0\n"
      : "=m" (*(volatile uint8_t *)addr)
      : "r" (val));
}

void evictCacheLine(uint64_t dst, uint8_t * addr) {
  cache_ops_evict_va(0, dst, (uint64_t)addr, 0, 64, 0);
}


namespace device_config {
extern const __thread kernel_environment_t * env_;

typedef struct {
  uint32_t flags;
  uint64_t launchedShireMask;
  uint64_t computeShireMask;
  uint32_t activeMinionMaskPerShire;
  uint32_t activeMinionsPerShire;
  uint32_t activeThreadsPerShire;
  uint8_t activeNeighborhood;
  uint8_t effectiveCenterShire;
  uint8_t scratchpadRelayCount;
  uint8_t scratchpadAuxiliaryCount;
  uint8_t scratchpadRelayShires[gpsdk::launch::kMaxScratchpadRelayShires];
  uint8_t scratchpadAuxiliaryShires[gpsdk::launch::kMaxScratchpadAuxiliaryShires];
} GpSdkLaunchTopology;

extern GpSdkLaunchTopology topology_;
}


/**
 * provides the Minion base frequency in Mhz
 *  \return Minion Base Freq (Mhz)
 */
static inline uint32_t getMinionBaseFrequency() {
  return device_config::env_->frequency;
}


/**
 * provides the Shire-mask used in the current kernel
 * \return shireMask (ones-hot mask of active Shires)
 */
static inline uint64_t getKernelShireMask() {
  return device_config::env_->shire_mask;
}

static inline uint64_t getLaunchedShireMask() {
  return device_config::topology_.launchedShireMask;
}

static inline uint64_t getComputeShireMask() {
  return device_config::topology_.computeShireMask;
}

static inline bool isRestrictedTopologyEnabled() {
  return device_config::topology_.flags != 0U;
}

static inline bool isScratchpadStarClusterEnabled() {
  return (device_config::topology_.flags & gpsdk::launch::kLaunchFlagScratchpadStarCluster) != 0U;
}

static inline bool isScratchpadBlockClusterEnabled() {
  return (device_config::topology_.flags & gpsdk::launch::kLaunchFlagScratchpadBlockCluster) != 0U;
}

static inline bool isScratchpadNestedStarClusterEnabled() {
  return (device_config::topology_.flags & gpsdk::launch::kLaunchFlagScratchpadNestedStarCluster) != 0U;
}

static inline bool isErbiumSimEnabled() {
  return (device_config::topology_.flags & gpsdk::launch::kLaunchFlagErbiumSim) != 0U;
}

static inline uint32_t getActiveMinionMaskPerShire() {
  return device_config::topology_.activeMinionMaskPerShire;
}

static inline uint32_t getActiveMinionsPerShire() {
  return device_config::topology_.activeMinionsPerShire;
}

static inline uint32_t getActiveThreadsPerShire() {
  return device_config::topology_.activeThreadsPerShire;
}

static inline uint32_t getActiveNeighborhood() {
  return device_config::topology_.activeNeighborhood;
}

static inline uint32_t getEffectiveCenterShire() {
  return device_config::topology_.effectiveCenterShire;
}

static inline uint32_t getScratchpadRelayCount() {
  return device_config::topology_.scratchpadRelayCount;
}

static inline uint32_t getScratchpadAuxiliaryCount() {
  return device_config::topology_.scratchpadAuxiliaryCount;
}

static inline uint32_t getScratchpadRelayShire(uint32_t index) {
  et_assert(index < getScratchpadRelayCount());
  return device_config::topology_.scratchpadRelayShires[index];
}

static inline uint32_t getScratchpadAuxiliaryShire(uint32_t index) {
  et_assert(index < getScratchpadAuxiliaryCount());
  return device_config::topology_.scratchpadAuxiliaryShires[index];
}

static inline bool isScratchpadClusterShire(uint32_t shireId) {
  if (shireId == getEffectiveCenterShire()) {
    return true;
  }

  for (uint32_t idx = 0U; idx < getScratchpadRelayCount(); ++idx) {
    if (getScratchpadRelayShire(idx) == shireId) {
      return true;
    }
  }

  for (uint32_t idx = 0U; idx < getScratchpadAuxiliaryCount(); ++idx) {
    if (getScratchpadAuxiliaryShire(idx) == shireId) {
      return true;
    }
  }

  return false;
}

static inline bool isScratchpadAuxiliaryShire(uint32_t shireId) {
  for (uint32_t idx = 0U; idx < getScratchpadAuxiliaryCount(); ++idx) {
    if (getScratchpadAuxiliaryShire(idx) == shireId) {
      return true;
    }
  }
  return false;
}

static inline uint32_t getFormat0ScratchpadShireId(uint64_t addr) {
  return static_cast<uint32_t>((addr - 0x80000000ULL) >> 23);
}

static inline uint64_t getFormat0ScratchpadOffset(uint64_t addr) {
  return (addr - 0x80000000ULL) & ((1ULL << 23) - 1ULL);
}

static inline bool isFormat0ScratchpadAddress(uint64_t addr) {
  constexpr uint64_t kFormat0BaseAddress = 0x80000000ULL;
  constexpr uint64_t kFormat0AddressSpaceBytes = (1ULL << (23 + 7));

  if ((addr < kFormat0BaseAddress) || (addr >= (kFormat0BaseAddress + kFormat0AddressSpaceBytes))) {
    return false;
  }

  return getFormat0ScratchpadOffset(addr) < 0x280000ULL;
}

static inline bool isAccessibleErbiumSimScratchpadAddress(uint64_t addr, size_t sizeBytes) {
  if (!isFormat0ScratchpadAddress(addr) || (sizeBytes == 0U)) {
    return true;
  }

  const auto shireId = getFormat0ScratchpadShireId(addr);
  const auto offset = getFormat0ScratchpadOffset(addr);
  if (static_cast<uint64_t>(sizeBytes) > (0x280000ULL - offset)) {
    return false;
  }

  if (!isScratchpadClusterShire(shireId)) {
    return false;
  }

  if (isScratchpadAuxiliaryShire(shireId) &&
      (offset >= gpsdk::star_scratchpad::kPoolBaseOffset) &&
      (offset < (gpsdk::star_scratchpad::kPoolBaseOffset + gpsdk::star_scratchpad::kPoolBytesPerAuxShire))) {
    return static_cast<uint64_t>(sizeBytes) <=
           ((gpsdk::star_scratchpad::kPoolBaseOffset + gpsdk::star_scratchpad::kPoolBytesPerAuxShire) - offset);
  }

  return true;
}

static inline void assertErbiumSimScratchpadAddress(const volatile void* ptr, size_t sizeBytes) {
  if (!isErbiumSimEnabled() || (ptr == nullptr) || (sizeBytes == 0U)) {
    return;
  }

  et_assert(isAccessibleErbiumSimScratchpadAddress(reinterpret_cast<uint64_t>(ptr), sizeBytes));
}

static inline void assertErbiumSimTensorLineAccess(uint64_t addr, uint64_t stride, uint64_t lineCount,
                                                   size_t accessBytes) {
  if (!isErbiumSimEnabled() || (lineCount == 0U)) {
    return;
  }

  for (uint64_t line = 0U; line < lineCount; ++line) {
    assertErbiumSimScratchpadAddress(reinterpret_cast<const void*>(addr + (line * stride)), accessBytes);
  }
}

static inline uint8_t gpsdk_checked_atomic_load_global_8(volatile const uint8_t* address) {
  assertErbiumSimScratchpadAddress(address, sizeof(uint8_t));
  return ::atomic_load_global_8(address);
}

static inline uint16_t gpsdk_checked_atomic_load_global_16(volatile const uint16_t* address) {
  assertErbiumSimScratchpadAddress(address, sizeof(uint16_t));
  return ::atomic_load_global_16(address);
}

static inline uint32_t gpsdk_checked_atomic_load_global_32(volatile const uint32_t* address) {
  assertErbiumSimScratchpadAddress(address, sizeof(uint32_t));
  return ::atomic_load_global_32(address);
}

static inline uint64_t gpsdk_checked_atomic_load_global_64(volatile const uint64_t* address) {
  assertErbiumSimScratchpadAddress(address, sizeof(uint64_t));
  return ::atomic_load_global_64(address);
}

static inline void gpsdk_checked_atomic_store_global_8(volatile uint8_t* address, uint8_t value) {
  assertErbiumSimScratchpadAddress(address, sizeof(uint8_t));
  ::atomic_store_global_8(address, value);
}

static inline void gpsdk_checked_atomic_store_global_16(volatile uint16_t* address, uint16_t value) {
  assertErbiumSimScratchpadAddress(address, sizeof(uint16_t));
  ::atomic_store_global_16(address, value);
}

static inline void gpsdk_checked_atomic_store_global_32(volatile uint32_t* address, uint32_t value) {
  assertErbiumSimScratchpadAddress(address, sizeof(uint32_t));
  ::atomic_store_global_32(address, value);
}

static inline void gpsdk_checked_atomic_store_global_64(volatile uint64_t* address, uint64_t value) {
  assertErbiumSimScratchpadAddress(address, sizeof(uint64_t));
  ::atomic_store_global_64(address, value);
}

static inline uint32_t gpsdk_checked_atomic_add_global_32(volatile uint32_t* address, uint32_t value) {
  assertErbiumSimScratchpadAddress(address, sizeof(uint32_t));
  return ::atomic_add_global_32(address, value);
}

static inline uint64_t gpsdk_checked_atomic_add_global_64(volatile uint64_t* address, uint64_t value) {
  assertErbiumSimScratchpadAddress(address, sizeof(uint64_t));
  return ::atomic_add_global_64(address, value);
}

static inline uint32_t gpsdk_checked_atomic_or_global_32(volatile uint32_t* address, uint32_t value) {
  assertErbiumSimScratchpadAddress(address, sizeof(uint32_t));
  return ::atomic_or_global_32(address, value);
}

static inline uint64_t gpsdk_checked_atomic_or_global_64(volatile uint64_t* address, uint64_t value) {
  assertErbiumSimScratchpadAddress(address, sizeof(uint64_t));
  return ::atomic_or_global_64(address, value);
}

static inline uint32_t gpsdk_checked_atomic_and_global_32(volatile uint32_t* address, uint32_t value) {
  assertErbiumSimScratchpadAddress(address, sizeof(uint32_t));
  return ::atomic_and_global_32(address, value);
}

static inline uint64_t gpsdk_checked_atomic_and_global_64(volatile uint64_t* address, uint64_t value) {
  assertErbiumSimScratchpadAddress(address, sizeof(uint64_t));
  return ::atomic_and_global_64(address, value);
}

static inline uint32_t gpsdk_checked_atomic_exchange_global_32(volatile uint32_t* address, uint32_t value) {
  assertErbiumSimScratchpadAddress(address, sizeof(uint32_t));
  return ::atomic_exchange_global_32(address, value);
}

static inline uint64_t gpsdk_checked_atomic_exchange_global_64(volatile uint64_t* address, uint64_t value) {
  assertErbiumSimScratchpadAddress(address, sizeof(uint64_t));
  return ::atomic_exchange_global_64(address, value);
}

static inline uint32_t gpsdk_checked_atomic_compare_and_exchange_global_32(volatile uint32_t* address,
                                                                           uint32_t expected, uint32_t desired) {
  assertErbiumSimScratchpadAddress(address, sizeof(uint32_t));
  return ::atomic_compare_and_exchange_global_32(address, expected, desired);
}

static inline uint64_t gpsdk_checked_atomic_compare_and_exchange_global_64(volatile uint64_t* address,
                                                                           uint64_t expected, uint64_t desired) {
  assertErbiumSimScratchpadAddress(address, sizeof(uint64_t));
  return ::atomic_compare_and_exchange_global_64(address, expected, desired);
}

static inline void gpsdk_checked_tensor_load(bool use_tmask, bool use_coop, uint64_t dst_start,
                                             uint64_t transformation, uint64_t use_tenb, uint64_t addr,
                                             uint64_t offset, uint64_t num_lines, uint64_t stride, uint64_t id) {
  (void)use_tmask;
  (void)use_coop;
  (void)dst_start;
  (void)transformation;
  (void)use_tenb;
  (void)offset;
  (void)id;
  assertErbiumSimTensorLineAccess(addr, stride, num_lines, 64U);
  ::tensor_load(use_tmask, use_coop, dst_start, transformation, use_tenb, addr, offset, num_lines, stride, id);
}

static inline void gpsdk_checked_et_tensor_load(et_tensor_load_conf_t* conf) {
  et_assert(conf != nullptr);
  assertErbiumSimTensorLineAccess(conf->addr, conf->stride, conf->num_lines, 64U);
  ::et_tensor_load(conf);
}

static inline void gpsdk_checked_tensor_load_setup_b(bool use_coop, uint64_t addr, uint64_t num_lines,
                                                     uint64_t stride, uint64_t id) {
  (void)use_coop;
  (void)id;
  assertErbiumSimTensorLineAccess(addr, stride, num_lines, 64U);
  ::tensor_load_setup_b(use_coop, addr, num_lines, stride, id);
}

static inline void gpsdk_checked_et_tensor_load_l2scp(et_tensor_load_l2scp_conf_t* conf) {
  et_assert(conf != nullptr);
  assertErbiumSimTensorLineAccess(conf->addr, conf->stride, conf->num_lines, 64U);
  ::et_tensor_load_l2scp(conf);
}

static inline void gpsdk_checked_tensor_store_scp(uint64_t entry_stride, uint64_t start_scp_entry, uint64_t Arows,
                                                  uint64_t addr, uint64_t stride) {
  (void)entry_stride;
  (void)start_scp_entry;
  assertErbiumSimTensorLineAccess(addr, stride, Arows, 64U);
  ::tensor_store_scp(entry_stride, start_scp_entry, Arows, addr, stride);
}

static inline void gpsdk_checked_tensor_store(uint64_t reg_stride, uint64_t start_reg, uint64_t cols, uint64_t Arows,
                                              uint64_t addr, uint64_t coop_store, uint64_t stride) {
  (void)reg_stride;
  (void)start_reg;
  (void)cols;
  (void)coop_store;
  assertErbiumSimTensorLineAccess(addr, stride, Arows, 64U);
  ::tensor_store(reg_stride, start_reg, cols, Arows, addr, coop_store, stride);
}

#define atomic_load_global_8 gpsdk_checked_atomic_load_global_8
#define atomic_load_global_16 gpsdk_checked_atomic_load_global_16
#define atomic_load_global_32 gpsdk_checked_atomic_load_global_32
#define atomic_load_global_64 gpsdk_checked_atomic_load_global_64
#define atomic_store_global_8 gpsdk_checked_atomic_store_global_8
#define atomic_store_global_16 gpsdk_checked_atomic_store_global_16
#define atomic_store_global_32 gpsdk_checked_atomic_store_global_32
#define atomic_store_global_64 gpsdk_checked_atomic_store_global_64
#define atomic_add_global_32 gpsdk_checked_atomic_add_global_32
#define atomic_add_global_64 gpsdk_checked_atomic_add_global_64
#define atomic_or_global_32 gpsdk_checked_atomic_or_global_32
#define atomic_or_global_64 gpsdk_checked_atomic_or_global_64
#define atomic_and_global_32 gpsdk_checked_atomic_and_global_32
#define atomic_and_global_64 gpsdk_checked_atomic_and_global_64
#define atomic_exchange_global_32 gpsdk_checked_atomic_exchange_global_32
#define atomic_exchange_global_64 gpsdk_checked_atomic_exchange_global_64
#define atomic_compare_and_exchange_global_32 gpsdk_checked_atomic_compare_and_exchange_global_32
#define atomic_compare_and_exchange_global_64 gpsdk_checked_atomic_compare_and_exchange_global_64
#define tensor_load gpsdk_checked_tensor_load
#define et_tensor_load gpsdk_checked_et_tensor_load
#define tensor_load_setup_b gpsdk_checked_tensor_load_setup_b
#define et_tensor_load_l2scp gpsdk_checked_et_tensor_load_l2scp
#define tensor_store_scp gpsdk_checked_tensor_store_scp
#define tensor_store gpsdk_checked_tensor_store

static inline uint32_t getActiveNeighborhoodBaseMinion() {
  return getActiveNeighborhood() * gpsdk::launch::kMinionsPerNeighborhood;
}

static inline bool isActiveMinionInShire(uint32_t localMinionId) {
  return ((getActiveMinionMaskPerShire() >> localMinionId) & 0x1U) != 0U;
}

int get_num_threads();

int get_relative_thread_id();

int get_relative_thread_id(uint64_t shireMask);


/**
 * Converts cycles to us.
 * \param cycles cycles to convert
 * \return converted time in us.
 */

static inline uint64_t cyclesToUs(uint64_t cycles) {
  const uint64_t frequency = getMinionBaseFrequency();
  return cycles / frequency;
}

/**
 * Converts cycles to ns.
 * \param cycles cycles to convert
 * \return converted time in ns.
 */
static inline uint64_t cyclesToNs(uint64_t cycles) {
  const uint64_t frequency = getMinionBaseFrequency();
  return (cycles * 1000) / frequency;
}

/**
 * Provides a running kernel timestamp in microsecs. 
 * \return kernel timestamp in microseconds.
 */
static inline uint64_t getTimestampUs() {
  uint64_t cycles = et_get_timestamp();
  return cyclesToUs(cycles);
}

/**
 * Provides a running kernel timestamp in nanoseconds. 
 * \return kernel timestamp in nanosecs.
 */
static inline uint64_t getTimestampNs() {
  uint64_t cycles = et_get_timestamp();
  return cyclesToNs(cycles);
}


#endif
