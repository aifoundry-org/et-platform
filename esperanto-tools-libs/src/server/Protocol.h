/*-------------------------------------------------------------------------
 * Copyright (C) 2022, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#pragma once
#include "runtime/Types.h"
#include <cereal/cereal.hpp>
#include <cstddef>
#include <limits>
#include <stdint.h>
#include <type_traits>
namespace rt {
using KernelIdT = std::underlying_type<KernelId>;
using EventIdT = std::underlying_type<EventId>;
using StreamIdT = std::underlying_type<StreamId>;
using DeviceIdT = std::underlying_type<DeviceId>;
using DeviceErrorCodeT = std::underlying_type<DeviceErrorCode>;
using AddressT = uint64_t;

template <class Archive> void serialize(Archive& archive, ErrorContext& ec) {
  archive(ec.type_, ec.cycle_, ec.hartId_, ec.mepc_, ec.mstatus_, ec.mtval_, ec.mcause_, ec.userDefinedError_, ec.gpr_);
}

namespace Request {

// these requests corresponds to first version of the protocol, is expected to add more versions here in the future
enum class Type : uint32_t {
  VERSION,
  MALLOC,
  FREE,
  MEMCPY_H2D,
  MEMCPY_D2H,
  MEMCPY_LIST_H2D,
  MEMCPY_LIST_D2H,
  CREATE_STREAM,
  DESTROY_STREAM,
  LOAD_CODE,
  UNLOAD_CODE,
  KERNEL_LAUNCH,
  GET_DEVICES,
  ABORT_STREAM
};

struct UnloadCode {
  KernelIdT kernel_;
  template <typename T> void archive(T& archive) {
    archive(kernel_);
  }
};
struct KernelLaunch {
  StreamIdT stream_;
  KernelIdT kernel_;
  AddressT kernelArgs_;
  size_t kernelArgsSize_;
};
struct Memcpy {
  StreamIdT stream_;
  AddressT src_;
  AddressT dst_;
  size_t size_;
  bool barrier_;
  template <typename T> void archive(T& archive) {
    archive(stream_, src_, dst_, size_, barrier_);
  }
};
struct MemcpyList {
  struct Op {
    AddressT src_;
    AddressT dst_;
    size_t size_;
  };
  StreamIdT stream_;
  std::vector<Op> ops_;
  bool barrier_;
  template <typename T> void archive(T& archive) {
    archive(stream_, ops_, barrier_);
  }
};

struct CreateStream {
  DeviceIdT device_;
  template <typename T> void archive(T& archive) {
    archive(device_);
  }
};

struct DestroyStream {
  StreamIdT stream_;
  template <typename T> void archive(T& archive) {
    archive(stream_);
  }
};

struct LoadCode {
  StreamIdT stream_;
  uint64_t elf_;
  size_t elfSize_;
};

struct Header {
  uint32_t size_;
  Type type_;
};

struct Version {
  uint64_t value_;
  template <typename T> void archive(T& archive) {
    archive(value_);
  }
};

struct Malloc {
  size_t size_;
  DeviceIdT device_;
  uint32_t alignment_;
  template <typename T> void archive(T& archive) {
    archive(size_, device_, alignment_);
  }
};

struct Free {
  DeviceIdT device_;
  std::byte* address_;
  template <typename T> void archive(T& archive) {
    archive(device_, address_);
  }
};

struct AbortStream {
  StreamIdT streamId_;
  template <typename T> void archive(T& archive) {
    archive(streamId_);
  }
};
} // namespace Request

namespace Response {
enum class Type : uint32_t {
  VERSION = 0x7FFFFFFF,
  MALLOC,
  FREE,
  MEMCPY_H2D,
  MEMCPY_D2H,
  MEMCPY_LIST_H2D,
  MEMCPY_LIST_D2H,
  CREATE_STREAM,
  DESTROY_STREAM,
  LOAD_CODE,
  UNLOAD_CODE,
  KERNEL_LAUNCH,
  GET_DEVICES,
  ABORT_STREAM
};

struct GetDevices {
  std::vector<DeviceIdT> devices_;
  template <typename T> void archive(T& archive) {
    archive(devices_);
  }
};

struct Event {
  EventIdT event_;
  template <typename T> void archive(T& archive) {
    archive(event_);
  }
};
struct CreateStream {
  StreamIdT stream_;
  template <typename T> void archive(T& archive) {
    archive(stream_);
  }
};
struct LoadCode {
  EventIdT event_;
  KernelIdT kernel_;
  uint64_t loadAddress_;
};

struct Header {
  uint32_t size_;
  Type type_;
};

} // namespace Response
} // namespace rt