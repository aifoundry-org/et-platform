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
  KernelId kernel_;
  template <class Archive> void serialize(Archive& archive) {
    archive(kernel_);
  }
};
struct KernelLaunch {
  StreamId stream_;
  KernelId kernel_;
  AddressT kernelArgs_;
  size_t kernelArgsSize_;
  template <class Archive> void serialize(Archive& archive) {
    archive(stream_, kernel_, kernelArgs_, kernelArgsSize_);
  }
};
struct Memcpy {
  StreamId stream_;
  AddressT src_;
  AddressT dst_;
  size_t size_;
  bool barrier_;
  template <class Archive> void serialize(Archive& archive) {
    archive(stream_, src_, dst_, size_, barrier_);
  }
};
struct MemcpyList {
  struct Op {
    AddressT src_;
    AddressT dst_;
    size_t size_;
  };
  StreamId stream_;
  std::vector<Op> ops_;
  bool barrier_;
  template <class Archive> void serialize(Archive& archive) {
    archive(stream_, ops_, barrier_);
  }
};

struct CreateStream {
  DeviceId device_;
  template <class Archive> void serialize(Archive& archive) {
    archive(device_);
  }
};

struct DestroyStream {
  StreamId stream_;
  template <class Archive> void serialize(Archive& archive) {
    archive(stream_);
  }
};

struct LoadCode {
  StreamId stream_;
  uint64_t elf_;
  size_t elfSize_;
  template <class Archive> void serialize(Archive& archive) {
    archive(stream_, elf_, elfSize_);
  }
};

struct Header {
  uint32_t size_;
  Type type_;
  template <class Archive> void serialize(Archive& archive) {
    archive(size_, type_);
  }
};

struct Version {
  uint64_t value_;
  template <class Archive> void serialize(Archive& archive) {
    archive(value_);
  }
};

struct Malloc {
  size_t size_;
  DeviceId device_;
  uint32_t alignment_;
  template <class Archive> void serialize(Archive& archive) {
    archive(size_, device_, alignment_);
  }
};

struct Free {
  DeviceId device_;
  AddressT address_;
  template <class Archive> void serialize(Archive& archive) {
    archive(device_, address_);
  }
};

struct AbortStream {
  StreamId streamId_;
  template <class Archive> void serialize(Archive& archive) {
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
  std::vector<DeviceId> devices_;
  template <class Archive> void serialize(Archive& archive) {
    archive(devices_);
  }
};

struct Event {
  EventId event_;
  template <class Archive> void serialize(Archive& archive) {
    archive(event_);
  }
};
struct CreateStream {
  StreamId stream_;
  template <class Archive> void serialize(Archive& archive) {
    archive(stream_);
  }
};
struct LoadCode {
  EventId event_;
  KernelId kernel_;
  uint64_t loadAddress_;
  template <class Archive> void serialize(Archive& archive) {
    archive(event_, kernel_, loadAddress_);
  }
};
struct StreamError {
  DeviceErrorCode errorCode_;
  std::optional<std::vector<ErrorContext>> errorContext_;
  template <class Archive> void serialize(Archive& archive) {
    archive(errorCode_, errorContext_);
  }
};
struct Header {
  uint32_t size_;
  Type type_;
  template <class Archive> void serialize(Archive& archive) {
    archive(size_, type_);
  }
};

} // namespace Response
} // namespace rt