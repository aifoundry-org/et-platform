

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
#include <cereal/types/variant.hpp>

#include <chrono>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <variant>

/// \defgroup runtime_profile_event_api Runtime Profile Event API
///
/// The runtime profiler API allows to gather profiling data from runtime execution.
/// This API expects to receive an output stream where all profiling events will be dumped, the user can explicitely
/// indicate when to start pofiling and when to stop profiling.
/// @{
namespace rt {
class RuntimeImp;
class CommandSender;

namespace tests {
class ProfileEventDeserializationTest;
}

namespace profiling {

enum class Type { Start, End, Complete, Instant };
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
  Pmc,
  CommandSent,
  ResponseReceived,
  DispatchEvent,

  COUNT
};
enum class ResponseType { DMARead, DMAWrite, Kernel, COUNT };
Class class_from_string(const std::string& str);
Type type_from_string(const std::string& str);
ResponseType response_type_from_string(const std::string& str);

class ScopedProfileEvent;

/// \brief IProfileEvent exposes serialization internals for runtime users.
///
class ProfileEvent {
public:
  using Id = uint64_t;
  using Cycles = uint64_t;
  using Clock = std::chrono::steady_clock;
  using TimePoint = std::chrono::time_point<Clock>;
  using Duration = TimePoint::duration;
  using ExtraValues = std::variant<uint64_t, EventId, StreamId, DeviceId, KernelId, ResponseType, Duration>;
  using ExtraMetadata = std::unordered_map<std::string, ExtraValues>;

  ProfileEvent() = default;
  explicit ProfileEvent(Type type, Class cls);
  explicit ProfileEvent(Type type, Class cls, StreamId stream, EventId event, ExtraMetadata extra = {});

  Type getType() const;
  Class getClass() const;
  TimePoint getTimeStamp() const;
  std::string getThreadId() const;
  ExtraMetadata getExtras() const;

  std::optional<Id> getPairId() const;
  std::optional<Duration> getDuration() const;
  std::optional<EventId> getEvent() const;
  std::optional<StreamId> getStream() const;
  std::optional<DeviceId> getDeviceId() const;
  std::optional<KernelId> getKernelId() const;
  std::optional<ResponseType> getResponseType() const;
  std::optional<uint64_t> getLoadAddress() const;
  std::optional<Cycles> getDeviceCmdStartTs() const;
  std::optional<Cycles> getDeviceCmdWaitDur() const;
  std::optional<Cycles> getDeviceCmdExecDur() const;

  void setType(Type t);
  void setClass(Class c);
  void setTimeStamp(TimePoint t = Clock::now());
  void setThreadId(std::thread::id id = std::this_thread::get_id());
  void setExtras(ExtraMetadata extras);

  void setPairId(Id id);
  void setDuration(Duration d);
  void setEvent(EventId event);
  void setStream(StreamId stream);
  void setDeviceId(DeviceId deviceId);
  void setKernelId(KernelId kernelId);
  void setResponseType(ResponseType rspType);
  void setLoadAddress(uint64_t loadAddress);
  void setDeviceCmdStartTs(uint64_t start_ts);
  void setDeviceCmdWaitDur(uint64_t wait_dur);
  void setDeviceCmdExecDur(uint64_t exec_dur);

  template <class Archive> friend void load(Archive& ar, ProfileEvent& evt);

private:
  template <typename... Args> void addExtra(std::string name, Args&&... args);
  template <typename T> std::optional<T> getExtra(const std::string& name) const;

  ExtraMetadata extra_;
  std::string threadId_;
  TimePoint timeStamp_;
  Type type_;
  Class class_;
};
} // end namespace profiling
} // end namespace rt

namespace rt::profiling {
std::string getString(rt::profiling::Class cls);
std::string getString(rt::profiling::Type type);
std::string getString(rt::profiling::ResponseType rspType);

template <typename Archive> void serialize(Archive& ar, rt::EventId& id) {
  ar(cereal::make_nvp("event_id", id));
}

template <typename Archive> void serialize(Archive& ar, rt::StreamId& id) {
  ar(cereal::make_nvp("stream_id", id));
}
template <typename Archive> void serialize(Archive& ar, rt::DeviceId& id) {
  ar(cereal::make_nvp("device_id", id));
}
template <typename Archive> void serialize(Archive& ar, rt::KernelId& id) {
  ar(cereal::make_nvp("kernel_id", id));
}

template <class Archive> void load(Archive& ar, ProfileEvent& evt) {
  std::string type;
  ar(cereal::make_nvp("type", type));
  evt.setType(type_from_string(type));

  std::string cls;
  ar(cereal::make_nvp("class", cls));
  evt.setClass(class_from_string(cls));

  ProfileEvent::TimePoint timeStamp;
  ar(cereal::make_nvp("timeStamp", timeStamp));
  evt.setTimeStamp(timeStamp);

  std::string threadId;
  ar(cereal::make_nvp("thread_id", threadId));
  auto tid = std::thread::id(std::stoull(threadId));
  evt.setThreadId(tid);

  ProfileEvent::ExtraMetadata extra;
  ar(cereal::make_nvp("extra", extra));
  evt.setExtras(std::move(extra));
}

template <class Archive> void save(Archive& ar, const ProfileEvent& evt) {
  ar(cereal::make_nvp("type", getString(evt.getType())));
  ar(cereal::make_nvp("class", getString(evt.getClass())));
  ar(cereal::make_nvp("timeStamp", evt.getTimeStamp()));
  ar(cereal::make_nvp("thread_id", evt.getThreadId()));
  ar(cereal::make_nvp("extra", evt.getExtras()));
}

} // end namespace rt::profiling

/// @}
// End of runtime_profile_event_api