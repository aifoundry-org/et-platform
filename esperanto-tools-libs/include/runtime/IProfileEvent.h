

/*-------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#pragma once

#include "runtime/IRuntime.h"

#include <cereal/cereal.hpp>
#include <cereal/types/chrono.hpp>
#include <cereal/types/optional.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/unordered_map.hpp>

#include <chrono>
#include <string>
#include <thread>
#include <sstream>
#include <unordered_map>

/// \defgroup runtime_profile_event_api Runtime Profile Event API
///
/// The runtime profiler API allows to gather profiling data from runtime execution.
/// This API expects to receive an output stream where all profiling events will be dumped, the user can explicitely
/// indicate when to start pofiling and when to stop profiling.
/// @{
namespace rt {
namespace profiling {


enum class Type { Start, End, Single, First = Start, Last = End };
enum class Class {
  GetDevices,
  LoadCode,
  UnloadCode,
  MallocDevice,
  FreeDevice,
  CreateStream,
  DestroyStream,
  KernelLaunch,
  MemcpyHostToDevice,
  MemcpyDeviceToHost,
  WaitForEvent,
  WaitForStream,
  DeviceCommand,
  KernelTimestamps,
  Pmc,
  First = GetDevices,
  Last = Pmc
};

std::string to_string(Class cls);
std::string to_string(Type type);
Class GetClass(const std::string& str);
Type GetType(const std::string& str);

class ScopedProfileEvent;

/// \brief IProfileEvent exposes serialization internals for runtime users.
///
class ProfileEvent {
public:
  using Id = uint64_t;
  using Cycles = uint64_t;
  using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;

  ProfileEvent() = default;
  explicit ProfileEvent(Type type, Class cls);
  explicit ProfileEvent(Type type, Class cls, StreamId stream, EventId event, std::unordered_map<std::string, uint64_t> extra = {});

  void setTimeStamp();
  void setPairId(Id id);
  void setEvent(EventId event);
  void setStream(StreamId stream);
  void setDeviceId(DeviceId deviceId);
  void setKernelId(KernelId kernelId);
  void setLoadAddress(uint64_t loadAddress);
  void setDeviceCmdStartTs(uint64_t start_ts);
  void setDeviceCmdWaitDur(uint64_t wait_dur);
  void setDeviceCmdExecDur(uint64_t exec_dur);

  Type getType() const;
  Class getClass() const;
  TimePoint getTimeStamp() const;
  std::string getThreadId() const;
  std::optional<Id> getPairId() const;
  std::optional<EventId> getEvent() const;
  std::optional<StreamId> getStream() const;
  std::optional<DeviceId> getDeviceId() const;
  std::optional<KernelId> getKernelId() const;
  std::optional<uint64_t> getLoadAddress() const;
  std::optional<Cycles> getDeviceCmdStartTs() const;
  std::optional<Cycles> getDeviceCmdWaitDur() const;
  std::optional<Cycles> getDeviceCmdExecDur() const;

  template <class Archive> void load(Archive& ar);
  template <class Archive> void save(Archive& ar) const;

  friend ScopedProfileEvent;

private:
  void addExtra(std::string name, uint64_t value);
  std::optional<uint64_t> getExtra(std::string name) const;

private:
  Type type_;
  Class class_;
  TimePoint timeStamp_;
  std::string threadId_;
  std::unordered_map<std::string, uint64_t> extra_;
};

} // end namespace profiling
} // namespace rt

/// @}
// End of runtime_profile_event_api