//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_COMMON_TYPES_H
#define ET_RUNTIME_COMMON_TYPES_H

#include <cstdint>

#ifdef __cplusplus
namespace et_runtime {
#endif // __cplusplus

/// Unique identifier for a code module loaded by the runtime it effectively keeps
/// track of the ELF files loaded through the runtime
typedef int64_t CodeModuleID;

/// Unique ID of each library code loaded through the runtime. A library ID will
/// effectively match the CodeModuleID since a library corresponds to an ELF
typedef int64_t LibraryCodeID;

/// Unique ID for each kernel code in the system.
typedef int64_t KernelCodeID;

/// Unique ID for each uber-kernel code in the system.
typedef int64_t UberKernelCodeID;

/// Enumeration with the different types of events we can create in the system
enum etrt_event_flags_e {
  ETRT_EVENT_FLAGS_STREAM_DEFAULT = 0, ///< Default flags
  ETRT_EVENT_FLAGS_STREAM_BLOCKING_SYNC =
      1, ///< Event uses blocking synchronization
  ETRT_EVENT_FLAGS_STREAM_NON_BLOCKING =
      2, ///< Event uses non blocking synchronization
  ETRT_EVENT_FLAGS_INTERPROCESS =
      4, ///<  Event is suitable for interprocess use
  ETRT_EVENT_FLAGS_DISABLE_TIMING =
      8, ///<  Disable timing on this event, @todo deprecate this option
};

/// Enumeration with the different allocations we can have in the system
enum etrt_mem_alloc_e {
  ETRT_MEM_ALLOC_UNKNOWN = 0, ///< Unknown type of memory allocation
  ETRT_MEM_ALLOC_HOST = 1,    ///< Allocate memory on the host
  ETRT_MEM_ALLOC_DEVICE = 2,  ///< Allocate memory on the device
};

///
/// @brief Esperanto Device properties
///
struct etrtDeviceProp {
  char name[256]; ///< ASCII string identifying the device
};

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // ET_RUNTIME_COMMON_TYPES_H
