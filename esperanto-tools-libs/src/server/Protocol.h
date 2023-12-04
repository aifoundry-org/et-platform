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
#include "KernelLaunchOptionsImp.h"
#include "runtime/Types.h"
#include <cereal/cereal.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/optional.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/variant.hpp>
#include <cereal/types/vector.hpp>
#include <chrono>
#include <cstddef>
#include <g3log/loglevels.hpp>
#include <limits>
#include <stdint.h>
#include <type_traits>
#include <variant>

namespace cereal {
template <class Archive> void serialize(Archive& archive, rt::DmaInfo& dmaInfo) {
  archive(dmaInfo.maxElementCount_, dmaInfo.maxElementSize_);
}
} // namespace cereal
namespace rt {
using AddressT = uint64_t;

template <class Archive> void serialize(Archive& archive, ErrorContext& ec) {
  archive(ec.type_, ec.cycle_, ec.hartId_, ec.mepc_, ec.mstatus_, ec.mtval_, ec.mcause_, ec.userDefinedError_, ec.gpr_);
}

template <class Archive> void serialize(Archive& archive, UserTrace& uc) {
  archive(uc.buffer_, uc.buffer_size_, uc.eventMask_, uc.filterMask_, uc.shireMask_, uc.threadMask_, uc.threshold_);
}

template <class Archive> void serialize(Archive& archive, StreamError& se) {
  archive(se.errorCode_, se.cmShireMask_, se.errorContext_);
}

namespace req {

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
  ABORT_STREAM,
  ABORT_COMMAND,
  DMA_INFO,
  DEVICE_PROPERTIES,
  KERNEL_ABORT_RELEASE_RESOURCES,
  GET_CURRENT_CLIENTS,
  GET_FREE_MEMORY,
  GET_WAITING_COMMANDS,
  GET_ALIVE_EVENTS,
  GET_P2P_COMPATIBILITY,
  MEMCPY_P2P_READ,
  MEMCPY_P2P_WRITE,
};

using Id = uint32_t;

// special Id used for asynchronous events from runtime. This Id won't correspond to any request
constexpr Id ASYNC_RUNTIME_EVENT = 0xFFFFFFFF;
constexpr Id INVALID_REQUEST_ID = ASYNC_RUNTIME_EVENT - 1;
constexpr size_t kMaxKernelSize = 10 << 20;

constexpr bool IsRegularId(Id id) {
  return id != ASYNC_RUNTIME_EVENT && id != INVALID_REQUEST_ID;
}

struct UnloadCode {
  KernelId kernel_;
  template <class Archive> void serialize(Archive& archive) {
    archive(kernel_);
  }
};

struct MemcpyP2P {
  StreamId stream_;
  DeviceId device_;
  AddressT src_;
  AddressT dst_;
  size_t size_;
  bool barrier_;
  template <class Archive> void serialize(Archive& archive) {
    archive(stream_, device_, src_, dst_, size_, barrier_);
  }
};

struct KernelLaunch {
  StreamId stream_;
  KernelId kernel_;
  std::vector<std::byte> kernelArgs_;
  KernelLaunchOptionsImp kernelOptionsImp_;
  template <class Archive> void serialize(Archive& archive) {
    archive(stream_, kernel_, kernelArgs_, kernelOptionsImp_);
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
    template <class Archive> void serialize(Archive& archive) {
      archive(src_, dst_, size_);
    }
  };
  explicit operator rt::MemcpyList() const {
    rt::MemcpyList list;
    for (auto& o : ops_) {
      list.addOp(reinterpret_cast<std::byte*>(o.src_), reinterpret_cast<std::byte*>(o.dst_), o.size_);
    }
    return list;
  }
  explicit MemcpyList() = default;
  explicit MemcpyList(const rt::MemcpyList& list, StreamId st, bool barrier)
    : stream_(st)
    , barrier_(barrier) {
    for (auto& o : list.operations_) {
      ops_.emplace_back(Op{reinterpret_cast<AddressT>(o.src_), reinterpret_cast<AddressT>(o.dst_), o.size_});
    }
  }

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
  LoadCode() = default;
  LoadCode(StreamId st, const std::byte* data, size_t size)
    : stream_(st)
    , elfData_(reinterpret_cast<uint64_t>(data))
    , elfSize_(size) {
  }
  StreamId stream_;
  AddressT elfData_;
  size_t elfSize_;
  template <class Archive> void serialize(Archive& archive) {
    archive(stream_, elfData_, elfSize_);
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

struct AbortCommand {
  EventId eventId_;
  std::chrono::milliseconds timeout_;
  template <class Archive> void serialize(Archive& archive) {
    archive(eventId_, timeout_);
  }
};

struct Request {
  Request() = default;
  template <typename T>
  Request(Type type, Id id, T payload)
    : type_(type)
    , id_(id)
    , payload_(payload) {
  }
  Type type_;
  Id id_ = INVALID_REQUEST_ID;
  std::variant<std::monostate, UnloadCode, KernelLaunch, Memcpy, MemcpyList, CreateStream, DestroyStream, LoadCode,
               Malloc, Free, AbortStream, AbortCommand, DeviceId, EventId, MemcpyP2P>
    payload_;
  template <class Archive> void serialize(Archive& archive) {
    archive(type_, id_, payload_);
  }
};
} // namespace req

namespace resp {
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
  ABORT_STREAM,
  ABORT_COMMAND,
  EVENT_DISPATCHED,
  STREAM_ERROR,
  RUNTIME_EXCEPTION,
  DMA_INFO,
  DEVICE_PROPERTIES,
  KERNEL_ABORTED,
  KERNEL_ABORT_RELEASE_RESOURCES,
  GET_CURRENT_CLIENTS,
  GET_FREE_MEMORY,
  GET_WAITING_COMMANDS,
  GET_ALIVE_EVENTS,
  GET_P2P_COMPATIBILITY,
  MEMCPY_P2P_READ,
  MEMCPY_P2P_WRITE,
};

constexpr auto getStr(Type t) {
  switch (t) {
#define STR_TYPE(t)                                                                                                    \
  case Type::t:                                                                                                        \
    return #t;
    STR_TYPE(VERSION)
    STR_TYPE(MALLOC)
    STR_TYPE(FREE)
    STR_TYPE(MEMCPY_H2D)
    STR_TYPE(MEMCPY_D2H)
    STR_TYPE(MEMCPY_LIST_H2D)
    STR_TYPE(MEMCPY_LIST_D2H)
    STR_TYPE(CREATE_STREAM)
    STR_TYPE(DESTROY_STREAM)
    STR_TYPE(LOAD_CODE)
    STR_TYPE(UNLOAD_CODE)
    STR_TYPE(KERNEL_LAUNCH)
    STR_TYPE(GET_DEVICES)
    STR_TYPE(ABORT_STREAM)
    STR_TYPE(ABORT_COMMAND)
    STR_TYPE(EVENT_DISPATCHED)
    STR_TYPE(STREAM_ERROR)
    STR_TYPE(RUNTIME_EXCEPTION)
    STR_TYPE(DMA_INFO)
    STR_TYPE(DEVICE_PROPERTIES)
    STR_TYPE(KERNEL_ABORTED)
    STR_TYPE(KERNEL_ABORT_RELEASE_RESOURCES)
    STR_TYPE(GET_CURRENT_CLIENTS)
    STR_TYPE(GET_FREE_MEMORY)
    STR_TYPE(GET_WAITING_COMMANDS)
    STR_TYPE(GET_ALIVE_EVENTS)
    STR_TYPE(GET_P2P_COMPATIBILITY)
    STR_TYPE(MEMCPY_P2P_READ)
    STR_TYPE(MEMCPY_P2P_WRITE)

  default:
    return "Unknown type";
  }
}

using Id = req::Id;

struct NumClients {
  size_t numClients_;
  template <class Archive> void serialize(Archive& archive) {
    archive(numClients_);
  }
};
struct FreeMemory {
  std::unordered_map<DeviceId, uint64_t> freeMemory_;
  template <class Archive> void serialize(Archive& archive) {
    archive(freeMemory_);
  }
};
struct WaitingCommands {
  std::unordered_map<DeviceId, uint32_t> waitingCommands_;
  template <class Archive> void serialize(Archive& archive) {
    archive(waitingCommands_);
  }
};
struct AliveEvents {
  std::unordered_map<DeviceId, uint32_t> aliveEvents_;
  template <class Archive> void serialize(Archive& archive) {
    archive(aliveEvents_);
  }
};
struct GetDevices {
  std::vector<DeviceId> devices_;
  template <class Archive> void serialize(Archive& archive) {
    archive(devices_);
  }
};

struct Version {
  static constexpr auto INVALID_VERSION = 0xFFFFFFFF;
  uint32_t major_ = INVALID_VERSION;
  uint32_t minor_ = INVALID_VERSION;

  template <class Archive> void serialize(Archive& archive) {
    archive(major_, minor_);
  }
};

struct P2PCompatibility {
  std::vector<uint64_t> compatibilityArray_;
  bool isCompatible(DeviceId one, DeviceId other) const {
    auto d0 = static_cast<uint32_t>(one);
    auto d1 = static_cast<uint32_t>(other);
    if (d0 > d1) {
      std::swap(d0, d1);
    }
    if (d0 == d1 || d1 >= compatibilityArray_.size() || d1 >= 64) {
      return false;
    }
    return compatibilityArray_[d0] & (1ULL << d1);
  }
  template <class Archive> void serialize(Archive& archive) {
    archive(compatibilityArray_);
  }
};

struct Malloc {
  AddressT address_;
  template <class Archive> void serialize(Archive& archive) {
    archive(address_);
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
struct RuntimeException {
  RuntimeException() = default;
  explicit RuntimeException(const Exception& e)
    : exception_(e) {
  }
  Exception exception_ = Exception{""};

  template <class Archive> void save(Archive& archive) const {
    archive(std::string{exception_.what()});
  }

  template <class Archive> void load(Archive& archive) {
    std::string what_message;
    archive(what_message);
    exception_ = Exception{what_message};
  }
};
struct StreamError {
  EventId event_;
  rt::StreamError error_;
  template <class Archive> void serialize(Archive& archive) {
    archive(event_, error_);
  }
};

struct KernelAborted {
  size_t size_;
  AddressT buffer_;
  EventId event_;
  template <class Archive> void serialize(Archive& archive) {
    archive(size_, buffer_, event_);
  }
};

struct Response {
  Response() = default;
  template <typename T>
  Response(Type type, Id id, T payload)
    : type_(type)
    , id_(id)
    , payload_(payload) {
  }

  using Payload_t = std::variant<std::monostate, Version, Malloc, GetDevices, Event, CreateStream, LoadCode,
                                 StreamError, RuntimeException, DmaInfo, DeviceProperties, KernelAborted, NumClients,
                                 FreeMemory, WaitingCommands, AliveEvents, P2PCompatibility>;
  Type type_;
  Id id_ = req::INVALID_REQUEST_ID;
  Payload_t payload_;
  template <class Archive> void serialize(Archive& archive) {
    archive(type_, id_, payload_);
  }
};
} // namespace resp
} // namespace rt