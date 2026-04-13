/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#ifndef GPSDK_SYNC_H
#define GPSDK_SYNC_H

#include <bitset>
#include <type_traits>

// Device common
#include <etsoc/common/utils.h>
#include <etsoc/isa/fcc.h>
#include <etsoc/isa/hart.h>
#include <etsoc/isa/tensors.h>
#include <etsoc/isa/utils.h>
#include <etsoc/isa/atomic.h>


// FW syscall IDs
#include <etsoc/isa/syscall.h>
#include "system/abi.h"

#include <profiling.h>

#include "entryPoint.h"
#include "flbLock.h"

namespace device_config {
extern DeviceConfig config;
extern const __thread kernel_environment_t * env_;
}

#ifndef CACHE_LINE_SIZE
#define CACHE_LINE_SIZE 64
#endif

// lock structs for cross entry-point sync
typedef struct {
        uint32_t flag_0;
        uint32_t flag_1;
} __attribute__((aligned(CACHE_LINE_SIZE))) minionlock_t;
enum LOCK_STATUS : uint32_t { LOCK_WAIT, LOCK_CONTINUE };
extern minionlock_t __barrierLock[1024];

typedef struct {
  uint32_t arrived_0;
  uint32_t arrived_1;
  uint32_t generation_0;
  uint32_t generation_1;
} devicebarrier_t;

extern devicebarrier_t __deviceBarrierState[2048];

// flb register locks
extern flbLock __shireLock;

/// @brief Obtains the relative minion id where the relative thread id is located
/// @param relative_thread_id
/// @return integer containing the relative minion id, values range from 0 to 2047
static inline int get_minion_from_thread(int relative_thread_id) {
  et_assert(relative_thread_id >= 0 && "Invalid thread Id (relative_thread_id < 0)");
  return relative_thread_id >> (device_config::config.threadsPerCore - 1);
}

/// @brief Obtains the shire id of a global or relative minion id.
/// @param relative_thread_id
/// @return int containing the shire id, values range from 0 to 31
static inline int get_shire_from_minion(int minion_id) {
  et_assert(minion_id >= 0 && "Invalid minion Id (minion_id < 0)");
  // 32 minions per shire
  return minion_id >> 5;
}

static inline int get_num_entrypoints() {
  return (device_config::config.sameEntryPoint || device_config::config.threadsPerCore == 1) ? 1 : 2;
}

/**
 * \brief Namespace including low-level routines for the minion.
 **/
namespace hart {

/*! \cond PRIVATE */


/**
 * \brief Obtains the physical minion where the \p relative_thread_id is assigned.
 *
 * \param relative_thread_id Thread Id between 0 and get_num_threads()-1.
 * \return returns an integer with value between 0 and 1023
 */
static inline int get_physical_minion_id(int relative_thread_id) {
  // get the virtual minionId of the thread based on the threads per core configuration
  const auto relativeMinionId = get_minion_from_thread(relative_thread_id);
  const auto minionsPerShire = static_cast<int>(getActiveMinionsPerShire());
  const auto relativeShireId = relativeMinionId / minionsPerShire;
  const auto relativeMinionInShire = relativeMinionId % minionsPerShire;
  const auto localMinionBase = static_cast<int>(getActiveNeighborhoodBaseMinion());
  auto shireMask = device_config::env_->shire_mask;
  int shireId = -1;

  for (int idx = 0; idx <= relativeShireId; ++idx) {
    et_assert(shireMask != 0ULL && "Invalid shire mask");
    shireId = static_cast<int>(__builtin_ctzll(shireMask));
    shireMask &= (shireMask - 1ULL);
  }

  return (shireId * SOC_MINIONS_PER_SHIRE) + localMinionBase + relativeMinionInShire;
}

/**
 * \brief Computes integer log2(value).
 *
 * \param value The number to apply the logarithm
 * \return Return unsigned int floored to the closest integer.
 */
static constexpr size_t log2(size_t value) {
  size_t result = 0;
  while (value >>= 1) {
    result++;
  }
  return result;
}

static inline int getMinionThreadsToSync() {
  if (device_config::config.sameEntryPoint) {
    return 2;
  }
  else {
    return 1;
  } 
}
/*! \endcond */

/** @enum Scope
 *  @brief list of possible Scope types in a barrier().
 */
enum class Scope {
  // future: kernel scope for multi-device support
  device, /*!< Synchronizes threads in the device (group by thread_id) */
  shire,  /*!< Synchronizes threads in a shire (group by thread_id) */
  minion  /*!< Synchronizes both thread_id=0 and thread_id=1 in a minion */
};

template <Scope S>
inline typename std::enable_if_t<(S == Scope::shire), void>
barrier(std::bitset<64> hartMask);

/**
 * \brief Synchronizes both threads in a minion
 *
 * Blocks thread execution until both threads in the minion reach the barrier.
 */
template <Scope S> inline typename std::enable_if_t<(S == Scope::minion), void> barrier() {
  // Note: barrier<shire> assumes full availability of FLB's for its synchronizations
  // Therefore we cannot make use of FLB's to implement barrier<minion> as they might be in use by the first.
  // Additionally, an FCC-based solution is not possible because barrier<shire> and barrier<device> use both FCC0 and FCC1
  // so sends using either FCC would conflict.
  const uint32_t thread = get_hart_id() & 0x1; // Thread 0 or 1
  const auto minionId = get_hart_id() >> 1;

  uint32_t * selfFlag;
  uint32_t * otherFlag;

  if (thread == 1) {
    selfFlag = &__barrierLock[minionId].flag_1;
    otherFlag = &__barrierLock[minionId].flag_0;
  } else {
    selfFlag = &__barrierLock[minionId].flag_0;
    otherFlag = &__barrierLock[minionId].flag_1;
  }
   
  // Loop until the 'other' thread lock is available (LOCK_WAIT)
  while (atomic_load_local_32(otherFlag) != LOCK_STATUS::LOCK_WAIT) {
      asm volatile("fence\n" ::: "memory");
  }
  asm volatile("fence\n" ::: "memory");

  // Set other thread flag to LOCK_CONTINUE idicating self thread arrived to the barrier.
  atomic_store_local_32(otherFlag, LOCK_STATUS::LOCK_CONTINUE);
  asm volatile("fence\n" ::: "memory");

  // Loop until selfFlag is set to LOCK_CONTINUE
  while (atomic_load_local_32(selfFlag) != LOCK_STATUS::LOCK_CONTINUE) {
      asm volatile("fence\n" ::: "memory");
  }
  asm volatile("fence\n" ::: "memory");

  // Before exit, set selfFlag as available (LOCK_WAIT).
  atomic_store_local_32(selfFlag, LOCK_STATUS::LOCK_WAIT);
  asm volatile("fence\n" ::: "memory");
}


/**
 * \brief Synchronizes threads per shire.
 * 
 * Blocks thread execution until all harts in the shire executing the same entry point reach the barrier.
 */
template <Scope S>
inline typename std::enable_if_t<(S == Scope::shire), void>
barrier() {
  if (isRestrictedTopologyEnabled()) {
    barrier<Scope::shire>(std::bitset<64>(getActiveMinionMaskPerShire()));
    return;
  }

  constexpr uint32_t fcc = 0;
  if (device_config::config.sameEntryPoint) {
    const uint64_t flb = 0UL;
    const auto flbCount = 63U; // All 64 threads - 1

    if (flbarrier(flb, flbCount)) {
      fcc_send(SHIRE_OWN, 0, fcc, 0xFFFFFFFF);
      fcc_send(SHIRE_OWN, 1, fcc, 0xFFFFFFFF);
    }
  } else {
    const auto hartId = get_hart_id();
    const uint32_t localThread = hartId & 0x1;
    const uint64_t flb = localThread;
    const auto flbCount = 31U; // 32 threads - 1

    if (flbarrier(flb, flbCount)) {
      fcc_send(SHIRE_OWN, localThread, fcc, 0xFFFFFFFF);
    }
  }
  fcc_consume(fcc);
}


/**
 * \brief Synchronizes threads per shire.
 *
 * Blocks thread execution until all selected harts in the shire reach the barrier. 
 * Threads in a shire that are synced can progress even if others shires still have not.
 * Barrier is only applied to threads executing the same entry point code.
 *
 * Every bit in \p hartMask maps to a hart in the shire.
 *
 * \param hartMask The n-th bit of the mask enables the sync of the n-th thread in the shire.
 * hartMask.count() must be a power of 2.
 * hartMask.count() max value is 32.
 */
template <Scope S>
inline typename std::enable_if_t<(S == Scope::shire), void>
barrier(std::bitset<64> hartMask) {
  auto threadsToSync = getMinionThreadsToSync();
  if ((hartMask.count() * threadsToSync) < 2)
    return; // no threads to sync

  // Use credit counter 0
  constexpr uint32_t fcc = 0;
  // Get the local thread id (values: 0 or 1)
  const auto hartId = get_hart_id();
  const uint32_t localThread = hartId & 0x1;
  // Get the local minion id [0,31]
  const auto localMinion = (hartId >> 1) & 0x1F;
  // Get the global shire Id
  const auto shireId = (hartId >> 6) & 0x1F;
  // Compute number of threads to sync in the FLB
  const auto flbCount = static_cast<uint32_t>(threadsToSync * hartMask.count()) - 1U;

  // Wait (spinlock) until you can join the FLB barrier
  // range of threads that can acquire/join the FLB lock
  uint32_t begin = __builtin_ctzll(hartMask.to_ullong()) * threadsToSync;
  uint32_t end = begin + flbCount;
  // Id of this thread in the range
  uint32_t tId = (localMinion * threadsToSync) + localThread;

  // Last minion to reach the fast local barrier wakes up the others
  uint64_t flb = __shireLock.acquireAny(shireId, tId, begin, end, threadsToSync);
  if (flbarrier(flb, flbCount)) {
    if (device_config::config.sameEntryPoint) {
      fcc_send(SHIRE_OWN, 0, fcc, hartMask.to_ullong());
      fcc_send(SHIRE_OWN, 1, fcc, hartMask.to_ullong());
    }
    else {
      fcc_send(SHIRE_OWN, localThread, fcc, hartMask.to_ullong());
    }
    // only last thread releases the lock
    __shireLock.release(shireId, flb, tId, begin, end, threadsToSync);
  }
  fcc_consume(fcc);
}

/*! \cond PRIVATE */
/**
 * \brief Synchronizes a range of threads in a device.
 *
 * Blocks thread execution until all the threads in the specified range reach this barrier. This barrier is shared only
 * between threads with the same thread_id (get_thread_id()).
 *
 * Synchronizes \p count contiguous threads starting at \p startingMinion.
 *
 * \param startingMinion First relative thread id in the synchronization range. Must be a multiple of 32 or 0.
 * \param count Number of threads in the synchronization range. Must be a multiple of 32.
 */
template <Scope S>
inline typename std::enable_if_t<(S == Scope::device), void> barrier(size_t startingThread, size_t count) {
  if (isRestrictedTopologyEnabled()) {
    const auto threadId = get_relative_thread_id();
    const auto numEntrypoints = static_cast<uint32_t>(get_num_entrypoints());
    const auto lane = (numEntrypoints == 1U) ? 0U : static_cast<uint32_t>(get_hart_id() & 0x1U);
    auto& barrierState = __deviceBarrierState[startingThread];
    volatile uint32_t* arrivals = (lane == 0U) ? &barrierState.arrived_0 : &barrierState.arrived_1;
    volatile uint32_t* generation = (lane == 0U) ? &barrierState.generation_0 : &barrierState.generation_1;
    const auto expectedArrivals = static_cast<uint32_t>(count / numEntrypoints);
    const auto currentGeneration = atomic_load_global_32(generation);

    et_assert(threadId >= 0 && "barrier: inactive thread reached a device barrier");
    et_assert(static_cast<size_t>(threadId) >= startingThread &&
                static_cast<size_t>(threadId) < (startingThread + count) &&
                "barrier: thread not part of synchronization group");

    const auto arrivalsBefore = atomic_add_global_32(arrivals, 1U);
    asm volatile("fence\n" ::: "memory");

    if ((arrivalsBefore + 1U) == expectedArrivals) {
      atomic_store_global_32(arrivals, 0U);
      atomic_store_global_32(generation, currentGeneration + 1U);
      asm volatile("fence\n" ::: "memory");
    } else {
      while (atomic_load_global_32(generation) == currentGeneration) {
        asm volatile("fence\n" ::: "memory");
      }
    }
    return;
  }

  constexpr uint32_t fcc = 1;
  constexpr std::bitset<64> allOnesMask = std::bitset<64>(0xFFFFFFFF);

  const auto thread = static_cast<uint32_t>(get_hart_id() & 0x1);
  const auto startingMinion = get_physical_minion_id(static_cast<int>(startingThread));
  const auto masterShireId = get_shire_from_minion(startingMinion);
  const auto numShires = static_cast<int>(count / (SOC_MINIONS_PER_SHIRE * device_config::config.threadsPerCore));

  et_assert((startingMinion % SOC_MINIONS_PER_SHIRE == 0) && "barrier: startingMinion must be multiple of 32");
  et_assert((count % SOC_MINIONS_PER_SHIRE == 0) && "barrier: count must be multiple of 32");
  et_assert((numShires < 33 && masterShireId < 32) && "barrier: number of shires must be <= 32");

  const auto localId = static_cast<int>(get_minion_id() & 0x1F);
  const auto shireId = get_shire_from_minion(get_minion_id());

  hart::barrier<Scope::shire>();

  if (masterShireId == shireId) {
    if (localId > masterShireId && localId < (masterShireId + numShires)) {
      const uint64_t flbCount = ((numShires - 1) * getMinionThreadsToSync()) - 1;
      const uint64_t flb = device_config::config.sameEntryPoint ? 0 : thread;

      // wait for wake-up credit from each active shire
      fcc_consume(fcc);
      if (flbarrier(flb, flbCount)) {
        // wake-up all minion in master shire
        if (device_config::config.sameEntryPoint) {
          fcc_send(masterShireId, 0, fcc, allOnesMask.to_ullong());
          fcc_send(masterShireId, 1, fcc, allOnesMask.to_ullong());
        } else {
          fcc_send(masterShireId, thread, fcc, allOnesMask.to_ullong());
        }
      }
    }
    fcc_consume(fcc);
    // Invariant: at this point all shires in the device are successfully synchronized

    // masterShire wakes up the other shires in the barrier
    if (localId > masterShireId && localId < (masterShireId + numShires)) {
      // Minion[i] wakes up shire[i], where i is the id of each assigned shire
      fcc_send(localId, thread, fcc, allOnesMask.to_ullong());
    }
  } else if (shireId < (masterShireId + numShires)) {
    // Global sync
    // Each active shire sends a credit to notify masterShire it has reached the barrier.
    if (localId == 0) {
      const uint64_t masterMinion = 1UL << shireId;
      fcc_send(masterShireId, thread, fcc, masterMinion);
    }
    // wait for wake-up from master shire
    fcc_consume(fcc);
  }
}
/*! \endcond */

/**
 * \brief Synchronizes a range of threads in a device.
 *
 * Blocks thread execution until all the threads in the specified range reach this barrier. 
 * Synchronizes \p count contiguous threads starting at \p startingThread.
 * \param startingThread First thread in the synchronization group. Must be a multiple of \p count or 0.
 * \param count Number of threads to synchronize. Must be a multiple of 32 or a power of two <= 32.
 */
inline void barrier(const size_t startingThread, const size_t count) {
  if (count < 2)
    return;
  
  const std::bitset<64> cmask = std::bitset<64>(count);
  const auto numMinions = count / device_config::config.threadsPerCore;
  // Assert if count is a power of two
  et_assert(cmask.count() == 1 && "Count must be a power of two");
  // Assert if startingThread is multiple of count (or 0)
  et_assert((startingThread % count == 0 || startingThread == 0) && "startingThread must be a multiple of count");
  
  const auto minionsPerShire = static_cast<size_t>(getActiveMinionsPerShire());

  if (numMinions > minionsPerShire) {
    barrier<Scope::device>(startingThread, count);
  } else if (numMinions == minionsPerShire) {
    barrier<Scope::shire>(); // fast-path
  } else {
    const auto physicalStartingMinionId = get_physical_minion_id(static_cast<int>(startingThread));
    const size_t localId = static_cast<size_t>(physicalStartingMinionId & 0x1F);
    // set mask bits corresponding to the range of minions to sync
    std::bitset<64> minionMask;
    for (size_t i = localId; i < localId + numMinions; i++) {
      minionMask.set(i);
    }
    barrier<Scope::shire>(minionMask);
  }
}

/*! \cond PRIVATE */
/**
 * \brief Synchronizes all threads in the kernel.
 *
 * Blocks thread execution until all the threads (with the same thread_id) reach this barrier.
 */
static inline void barrier() {
  const auto numShires = __builtin_popcountll(device_config::env_->shire_mask);
  const auto numThreads = get_num_threads();
  et_assert(numShires < 33 && "barrier: number of shires must be <= 32");

  if (numShires > 1) {
    barrier<Scope::device>(0, numThreads);
  } else {
    barrier<Scope::shire>();
  }
} 

/*! \endcond */
} // namespace hart
#endif // GPSDK_SYNC_H
