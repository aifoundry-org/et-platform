/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#include <bitset>
#include <stdio.h>

#include "CommonCode.h"
#include "entryPoint.h"
#include "sync.h"
#include "flbLock.h"
#include "gpsdk_launch_runtime.h"
#include <etsoc/common/utils.h>
#include <etsoc/isa/barriers.h>
#include <etsoc/isa/cacheops-umode.h>
#include <etsoc/isa/hart.h>
#include <system/abi.h>

/* Linker labels to global .bss and .data sections */
extern uint32_t _bss_end;
extern uint32_t _bss_start;
extern uint32_t _data_end;
extern uint32_t _data_start;
extern uint32_t _data_ro_copy_end;
extern uint32_t _data_ro_copy_start;

/* TLS storage linker script labels */
extern uint8_t* __tdata_start;
extern uint8_t* __tbss_end;
extern uint8_t* __tls_alloc_start;

// Generic C function pointer.
using function_t = void (*)();
/* Linker labels for initialization and fini arrays */
extern const function_t __preinit_array_start;
extern const function_t __preinit_array_end;
extern const function_t __init_array_start;
extern const function_t __init_array_end;
extern const function_t __fini_array_start;
extern const function_t __fini_array_end;

// Structs and classes to manage locks in barriers
minionlock_t __barrierLock[1024] = {0U, 0U};
flbLock __shireLock;

// Kernel configuration object. needs to be declared by the user throuch DECLARE_DEVICE_CONFIG
namespace device_config {
extern DeviceConfig config;
/* Global pointer to environment struct allocated by the host */
const __thread kernel_environment_t* env_ = nullptr;
__thread kernel_environment_t visible_env_ = {{0, 0, 0, 0}, 0xFFFFFFFF, 600};
GpSdkLaunchTopology topology_ = {
  0U,
  0xFFFFFFFFULL,
  0xFFFFFFFFULL,
  gpsdk::launch::kAllMinionsMask,
  gpsdk::launch::kMinionsPerShire,
  0U,
  0U,
};
const kernel_environment_t fallback_env = {{0, 0, 0, 0}, 0xFFFFFFFF, 600};
} // namespace device_config

/* Number of times the kernel has been launched */
alignas (CACHE_LINE_SIZE) uint64_t numberOfBoots __attribute__((section("persistentData"))) = {1};
devicebarrier_t __deviceBarrierState[2048] = {};

void resetBSS();
void resetData();
bool hasGlobalData();
extern "C" int deviceGpSdkEntry(void* args, kernel_environment_t* env);

namespace {

const gpsdk::launch::RuntimeArgsHeader* getRuntimeArgsHeader(const void* args) {
  if (args == nullptr) {
    return nullptr;
  }

  const auto* header = reinterpret_cast<const gpsdk::launch::RuntimeArgsHeader*>(args);
  if ((header->magic != gpsdk::launch::kRuntimeArgsMagic) || (header->version != gpsdk::launch::kRuntimeArgsVersion) ||
      (header->headerSize != sizeof(gpsdk::launch::RuntimeArgsHeader))) {
    return nullptr;
  }
  return header;
}

device_config::GpSdkLaunchTopology getLaunchTopology(const gpsdk::launch::RuntimeArgsHeader* header,
                                                     const kernel_environment_t* env) {
  const uint64_t launchedShireMask = (env != nullptr) ? env->shire_mask : 0xFFFFFFFFULL;
  device_config::GpSdkLaunchTopology topology = {
    0U,
    launchedShireMask,
    launchedShireMask,
    gpsdk::launch::kAllMinionsMask,
    gpsdk::launch::kMinionsPerShire,
    0U,
    0U,
  };

  if (header == nullptr) {
    topology.activeThreadsPerShire =
      topology.activeMinionsPerShire * static_cast<uint32_t>(device_config::config.threadsPerCore);
    return topology;
  }

  topology.flags = header->flags;
  topology.activeNeighborhood = header->activeNeighborhood;

  if ((header->computeShireMask == 0ULL) || ((header->computeShireMask & ~launchedShireMask) != 0ULL)) {
    topology.flags = 0U;
    topology.activeNeighborhood = 0U;
  } else {
    topology.computeShireMask = header->computeShireMask;
  }

  if ((header->flags & gpsdk::launch::kLaunchFlagSingleNeighborhoodPerShire) &&
      gpsdk::launch::isValidNeighborhood(header->activeNeighborhood)) {
    topology.activeMinionMaskPerShire = gpsdk::launch::minionMaskForNeighborhood(header->activeNeighborhood);
    topology.activeMinionsPerShire = gpsdk::launch::kMinionsPerNeighborhood;
  } else if ((header->flags & gpsdk::launch::kLaunchFlagSingleNeighborhoodPerShire) != 0U) {
    topology.flags &= ~gpsdk::launch::kLaunchFlagSingleNeighborhoodPerShire;
    topology.activeNeighborhood = 0U;
  }

  topology.activeThreadsPerShire =
    topology.activeMinionsPerShire * static_cast<uint32_t>(device_config::config.threadsPerCore);
  return topology;
}

void* getUserArgsPointer(void* args, const gpsdk::launch::RuntimeArgsHeader* header) {
  if (header == nullptr) {
    return args;
  }
  if (header->payloadSize == 0U) {
    return nullptr;
  }
  return reinterpret_cast<std::byte*>(args) + sizeof(gpsdk::launch::RuntimeArgsHeader);
}

bool isActiveKernelHart(uint32_t shireId, uint32_t localMinionId, uint64_t shireMask) {
  if (((shireMask >> shireId) & 0x1ULL) == 0ULL) {
    return false;
  }

  return isActiveMinionInShire(localMinionId);
}

} // namespace

/* wake up all threads on the shire-Mask group system */
static inline void wakeUpThreads(uint64_t shire_mask) {

  /* uses ESR-broadcast extension to send credits to all shires simultaneously */
  volatile uint64_t* broadcast_data =
    (volatile uint64_t*)ESR_SHIRE_PROT_ADDR(PRV_U, THIS_SHIRE, ESR_SHIRE_BROADCAST0); // 0x013ff5fff0
  volatile uint64_t* broadcast_address =
    (volatile uint64_t*)ESR_SHIRE_PROT_ADDR(PRV_U, THIS_SHIRE, ESR_SHIRE_BROADCAST1); // 0x013ff5fff8

  const uint64_t thread_mask = getActiveMinionMaskPerShire();
  *broadcast_data = thread_mask;
  // broadcast to threads 0
  *broadcast_address =
    (shire_mask & 0xFFFFFFFF) | (((ESR_SHIRE(0, FCC_CREDINC_0) >> 3) & 0x7FFFF) << ESR_BROADCAST_ESR_ADDR_SHIFT);
  // braodcast to threads 1.
  if (device_config::config.threadsPerCore == 2) {
    *broadcast_address =
      (shire_mask & 0xFFFFFFFF) | (((ESR_SHIRE(0, FCC_CREDINC_2) >> 3) & 0x7FFFF) << ESR_BROADCAST_ESR_ADDR_SHIFT);
  }

  // [SW-17765: fixed bug affecting kernels using the Shire 32]
  if (shire_mask & 0x100000000) {
    // wake shire 32 if it is used
    fcc_send(32, THREAD_0, FCC_0, 0xFFFFFFFF);
    if (device_config::config.threadsPerCore == 2) {
      fcc_send(32, THREAD_1, FCC_0, 0xFFFFFFFF);
    }
  }
}

/// @brief Resets global memory region .bss to zero
void resetBSS() {
  uint8_t* bss_end = (uint8_t*)&_bss_end;
  uint8_t* bss_start = (uint8_t*)&_bss_start;
  global_memset(bss_start, 0, bss_end - bss_start);
}

/// @brief Initializes global memory region .data to its original value
void resetData() {
  uint8_t* data_end = (uint8_t*)&_data_end;
  uint8_t* data_start = (uint8_t*)&_data_start;
  uint8_t* data_ro_copy_start = (uint8_t*)&_data_ro_copy_start;

  if (numberOfBoots == 1) {
    // backup .data section
    global_memcpy(data_ro_copy_start, data_start, data_end - data_start);
  } else {
    // restore .data section
    global_memcpy(data_start, data_ro_copy_start, data_end - data_start);
  }
}

/// @brief returns true if Global Data sections (.bss and .data) are not empty.
bool hasGlobalData() {
  uint8_t* bss_end = (uint8_t*)&_bss_end;
  uint8_t* bss_start = (uint8_t*)&_bss_start;
  uint8_t* data_end = (uint8_t*)&_data_end;
  uint8_t* data_start = (uint8_t*)&_data_start;
  return !((bss_end == bss_start) && (data_end == data_start));
}

/// returns true if  there are functions to call on init_arrays
static bool hasInitArrays() {
  return !(__preinit_array_start == __preinit_array_end) && !(__init_array_start == __init_array_end);
}

/// Initialize per hart  Thread Local Storage.
/// note: each hart in the system should call this function.
static void initializeTLS(kernel_environment_t * env) {
  auto tlsSize = (&__tbss_end - &__tdata_start) * sizeof(__tbss_end);
  if (tlsSize == 0) {
    return;
  }
  auto tlsStart = &__tdata_start;
  auto hartId = get_hart_id();
  auto tlsHartBase = (void *) ((uint64_t)&__tls_alloc_start + (hartId * tlsSize));

  // populate tls section for this hart.
  local_memcpy(tlsHartBase, tlsStart, tlsSize);

  // initialize tp with the tls Base address for this hart
  asm volatile("mv tp, %[tlsHartBase] \n" : : [ tlsHartBase ] "r"(tlsHartBase));

  // Expose compute-visible topology through the GP-SDK environment while retaining the full launched mask
  // separately in device_config::topology_. TLS-backed state must only be touched after tp is valid.
  device_config::visible_env_ = env ? *env : device_config::fallback_env;
  device_config::visible_env_.shire_mask = getComputeShireMask();
  device_config::env_ = &device_config::visible_env_;

}

/// calls init_array functions
static void callInitArrayFunctions() {

  for (const function_t* entry = &__preinit_array_start; entry < &__preinit_array_end; ++entry) {
    (*entry)();
  }

  for (const function_t* entry = &__init_array_start; entry < &__init_array_end; ++entry) {
    (*entry)();
  }
}


/// @brief Main entryPoint for all threads when the ETSoC-1 starts executing
/// @param args user kernel arguments
/// @param env struct containing shire_mask and frequency
/// @return Returns 0 if the kernel finished execution correctly
extern "C" int deviceGpSdkEntry(void* args, kernel_environment_t* env) {
  const auto* runtimeArgsHeader = getRuntimeArgsHeader(args);
  device_config::topology_ = getLaunchTopology(runtimeArgsHeader, env);
  args = getUserArgsPointer(args, runtimeArgsHeader);

  uint32_t hart = get_hart_id();
  uint32_t threadId = hart & 1;
  uint32_t minionId = hart >> 1;
  uint32_t shireId = minionId >> 5;
  minionId = minionId & 0x1F;
  const uint64_t computeShireMask = getComputeShireMask();

   
  if (shireId >= 33)
    return 0;

  if (device_config::config.threadsPerCore == 1 && threadId == 1) {
    // exit if thread 1 is unused
    return 0;
  }

  if (!isActiveKernelHart(shireId, minionId, computeShireMask)) {
    return 0;
  }

  auto needSync = hasGlobalData() || hasInitArrays();
  // fast-path: no global data: fast-forward to user code.
  if (!needSync) {
    if (device_config::config.threadsPerCore == 1) {
      initializeTLS(env);
      return device_config::config.entryPoint_0(args);
    } else {
      if (threadId == 0) {
        initializeTLS(env);
        return device_config::config.entryPoint_0(args);
      } else {
        initializeTLS(env);
        return device_config::config.entryPoint_1(args);
      }
    }
  }

  // global data: initialize and forwared to user-code.
  if (get_relative_thread_id(computeShireMask) == 0) {
    // Reset .bss and .data sections on each kernel launch
    resetBSS();
    resetData();
    // increase number of boots atomically
    const uint32_t increment = 1;
    uint32_t result;
    __asm__ __volatile__("amoaddg.w %[result], %[increase], (%[dst])\n"
                         : [ result ] "=r"(result)
                         : [ increase ] "r"(increment), [ dst ] "r"(&numberOfBoots)
                         :);

    // call init array (dynamic initializatoin) functions from thrad 0
    callInitArrayFunctions();
    wakeUpThreads(computeShireMask);
  }

  // Wait initialization to complete and forward to user-code.
  if (device_config::config.threadsPerCore == 1) {
    if (threadId == 0) {
      fcc_consume(FCC_0);
      initializeTLS(env);
      return device_config::config.entryPoint_0(args);
    }
  } else {
    if (threadId == 0) {
      fcc_consume(FCC_0);
      initializeTLS(env);
      return device_config::config.entryPoint_0(args);
    } else {
      fcc_consume(FCC_0);
      initializeTLS(env);
      return device_config::config.entryPoint_1(args);
    }
  }
  return 0;
}

/// @brief Obtains the number of threads assigned to a kernel
/// @return Returns an integer, possible values range from 0 to N (where N = get_num_threads()-1).
int get_num_threads() {
  return __builtin_popcountll(getComputeShireMask()) * static_cast<int>(getActiveThreadsPerShire());
}

/// @brief Obtains the mask of shires assigned to the kernel
/// @return Returns a bitmask, the i-th bit represent the physical i minion shire of the device.
uint64_t get_shire_mask() {
  return getComputeShireMask();
}

/// @brief Obtains the relative thread id assigned to the hart
/// @return Returns an integer ranging from 32 to 1024 if threadsPerCore == 1, from 64 to 2048 if threadsPerCore == 2
int get_relative_thread_id() {
  return get_relative_thread_id(getComputeShireMask());
}

/// @brief Obtains the relative thread id assigned to the hart based on the provided shireMask
/// @param shireMask bit-mask of the active shires, must be consecutive
/// @return Returns an integer ranging from 0 to 1023 if threadsPerCore == 1, from 0 to 2047 if threadsPerCore == 2
inline int get_relative_thread_id(uint64_t shireMask) {
  const auto hartId = static_cast<uint32_t>(get_hart_id());
  const auto threadId = hartId & 0x1U;
  const auto minionId = (hartId >> 1) & 0x1FU;
  const auto shireId = hartId >> 6;

  if (((shireMask >> shireId) & 0x1ULL) == 0ULL) {
    return -1;
  }

  if (!isActiveMinionInShire(minionId)) {
    return -1;
  }

  const uint64_t priorShiresMask = (shireId == 0U) ? 0ULL : (shireMask & ((1ULL << shireId) - 1ULL));
  const int relativeShireId = static_cast<int>(__builtin_popcountll(priorShiresMask));
  const int relativeMinionInShire = static_cast<int>(minionId) - static_cast<int>(getActiveNeighborhoodBaseMinion());
  const int relativeMinionId = (relativeShireId * static_cast<int>(getActiveMinionsPerShire())) + relativeMinionInShire;

  return (relativeMinionId * device_config::config.threadsPerCore) + static_cast<int>(threadId);
}
