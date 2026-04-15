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
#include <etsoc/isa/cacheops-umode.h>
#include <etsoc/isa/tensors.h>
#include <system/abi.h>

#include "gpsdk_launch_runtime.h"

static inline uint8_t readByte(uint8_t * addr);
static inline void writeByte(uint8_t * addr, uint8_t val);
static inline void evictCacheLine(uint64_t dst, uint8_t * addr);
static inline bool isFormat0ScratchpadAddress(uint64_t addr);
static inline bool isAccessibleErbiumSimScratchpadAddress(uint64_t addr, size_t sizeBytes);
static inline void assertErbiumSimScratchpadAddress(const void* ptr, size_t sizeBytes);

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

static inline uint32_t getFormat0ScratchpadShireId(uint64_t addr) {
  return static_cast<uint32_t>((addr - 0x80000000ULL) >> 23);
}

static inline uint64_t getFormat0ScratchpadOffset(uint64_t addr) {
  return (addr - 0x80000000ULL) & ((1ULL << 23) - 1ULL);
}

static inline bool isFormat0ScratchpadAddress(uint64_t addr) {
  if (addr < 0x80000000ULL) {
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

  return isScratchpadClusterShire(shireId);
}

static inline void assertErbiumSimScratchpadAddress(const void* ptr, size_t sizeBytes) {
  if (!isErbiumSimEnabled() || (ptr == nullptr) || (sizeBytes == 0U)) {
    return;
  }

  et_assert(isAccessibleErbiumSimScratchpadAddress(reinterpret_cast<uint64_t>(ptr), sizeBytes));
}

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
