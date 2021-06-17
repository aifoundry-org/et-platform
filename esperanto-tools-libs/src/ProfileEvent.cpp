/*-------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include "runtime/IProfileEvent.h"

#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>

#include <cassert>
#include <chrono>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>

namespace std {
std::string to_string(rt::profiling::Class cls) {
  using event = rt::profiling::Class;
  switch (cls) {
  case event::GetDevices:
    return "GetDevices";
  case event::LoadCode:
    return "LoadCode";
  case event::UnloadCode:
    return "UnloadCode";
  case event::MallocDevice:
    return "MallocDevice";
  case event::FreeDevice:
    return "FreeDevice";
  case event::CreateStream:
    return "CreateStream";
  case event::DestroyStream:
    return "DestroyStream";
  case event::KernelLaunch:
    return "KernelLaunch";
  case event::MemcpyHostToDevice:
    return "MemcpyHostToDevice";
  case event::MemcpyDeviceToHost:
    return "MemcpyDeviceToHost";
  case event::WaitForEvent:
    return "WaitForEvent";
  case event::WaitForStream:
    return "WaitForStream";
  case event::DeviceCommand:
    return "DeviceCommand";
  case event::KernelTimestamps:
    return "KernelTimestamps";
  case event::Pmc:
    return "Pmc";
  default:
    assert(false);
    return "Unknown class";
  }
}

std::string to_string(rt::profiling::Type type) {
  using Type = rt::profiling::Type;
  switch (type) {
  case Type::Start:
    return "Start";
  case Type::End:
    return "End";
  case Type::Complete:
    return "Complete";
  case Type::Instant:
    return "Instant";
  default:
    assert(false);
    return "Unknown type";
  }
}
} // end namespace std

namespace rt::profiling {

Class class_from_string(const std::string& str) {
  static std::once_flag s_once_flag;
  static std::unordered_map<std::string, Class> s_map;
  std::call_once(s_once_flag, []() {
    s_map[std::to_string(Class::GetDevices)] = Class::GetDevices;
    s_map[std::to_string(Class::LoadCode)] = Class::LoadCode;
    s_map[std::to_string(Class::UnloadCode)] = Class::UnloadCode;
    s_map[std::to_string(Class::MallocDevice)] = Class::MallocDevice;
    s_map[std::to_string(Class::FreeDevice)] = Class::FreeDevice;
    s_map[std::to_string(Class::CreateStream)] = Class::CreateStream;
    s_map[std::to_string(Class::DestroyStream)] = Class::DestroyStream;
    s_map[std::to_string(Class::KernelLaunch)] = Class::KernelLaunch;
    s_map[std::to_string(Class::MemcpyHostToDevice)] = Class::MemcpyHostToDevice;
    s_map[std::to_string(Class::MemcpyDeviceToHost)] = Class::MemcpyDeviceToHost;
    s_map[std::to_string(Class::WaitForEvent)] = Class::WaitForEvent;
    s_map[std::to_string(Class::WaitForStream)] = Class::WaitForStream;
    s_map[std::to_string(Class::DeviceCommand)] = Class::DeviceCommand;
    s_map[std::to_string(Class::KernelTimestamps)] = Class::KernelTimestamps;
    s_map[std::to_string(Class::Pmc)] = Class::Pmc;
  });

  return s_map[str];
}

Type type_from_string(const std::string& str) {
  static std::once_flag s_once_flag;
  static std::unordered_map<std::string, Type> s_map;
  std::call_once(s_once_flag, []() {
    s_map[std::to_string(Type::Start)] = Type::Start;
    s_map[std::to_string(Type::End)] = Type::End;
    s_map[std::to_string(Type::Complete)] = Type::Complete;
    s_map[std::to_string(Type::Instant)] = Type::Instant;
  });
  return s_map[str];
}

ProfileEvent::ProfileEvent(Type type, Class cls)
  : type_(type)
  , class_(cls) {
  setTimeStamp();
  setThreadId();
}

ProfileEvent::ProfileEvent(Type type, Class cls, StreamId stream, EventId event, ProfileEvent::ExtraMetadata extra)
  : ProfileEvent(type, cls) {
  extra_ = std::move(extra);
  setStream(stream);
  setEvent(event);
}

Type ProfileEvent::getType() const {
  return type_;
}
Class ProfileEvent::getClass() const {
  return class_;
}
ProfileEvent::TimePoint ProfileEvent::getTimeStamp() const {
  return timeStamp_;
}
std::string ProfileEvent::getThreadId() const {
  return threadId_;
}
ProfileEvent::ExtraMetadata ProfileEvent::getExtras() const {
  return extra_;
}

std::optional<ProfileEvent::Id> ProfileEvent::getPairId() const {
  return getExtra<ProfileEvent::Id>("pair_id");
}
std::optional<EventId> ProfileEvent::getEvent() const {
  return getExtra<EventId>("event");
}
std::optional<StreamId> ProfileEvent::getStream() const {
  return getExtra<StreamId>("stream");
}
std::optional<DeviceId> ProfileEvent::getDeviceId() const {
  return getExtra<DeviceId>("device_id");
}
std::optional<KernelId> ProfileEvent::getKernelId() const {
  return getExtra<KernelId>("kernel_id");
}
std::optional<uint64_t> ProfileEvent::getLoadAddress() const {
  return getExtra<uint64_t>("load_address");
}
std::optional<uint64_t> ProfileEvent::getDeviceCmdStartTs() const {
  return getExtra<uint64_t>("device_cmd_start_ts");
}
std::optional<uint64_t> ProfileEvent::getDeviceCmdWaitDur() const {
  return getExtra<uint64_t>("device_cmd_wait_dur");
}
std::optional<uint64_t> ProfileEvent::getDeviceCmdExecDur() const {
  return getExtra<uint64_t>("device_cmd_exec_dur");
}

void ProfileEvent::setType(Type t) {
  type_ = t;
}
void ProfileEvent::setClass(Class c) {
  class_ = c;
}
void ProfileEvent::setTimeStamp(TimePoint t) {
  timeStamp_ = t;
}
void ProfileEvent::setThreadId(std::thread::id id) {
  std::stringstream ss;
  ss << id;
  threadId_ = ss.str();
}
void ProfileEvent::setExtras(ExtraMetadata extras) {
  extra_ = std::move(extras);
}

void ProfileEvent::setPairId(Id id) {
  addExtra("pair_id", id);
}

void ProfileEvent::setEvent(EventId event) {
  addExtra("event", std::move(event));
}

void ProfileEvent::setStream(StreamId stream) {
  addExtra("stream", std::move(stream));
}

void ProfileEvent::setDeviceId(DeviceId deviceId) {
  addExtra("device_id", std::move(deviceId));
}

void ProfileEvent::setKernelId(KernelId kernelId) {
  addExtra("kernel_id", std::move(kernelId));
}

void ProfileEvent::setLoadAddress(uint64_t loadAddress) {
  addExtra("load_address", loadAddress);
}

void ProfileEvent::setDeviceCmdStartTs(uint64_t start_ts) {
  addExtra("device_cmd_start_ts", start_ts);
}

void ProfileEvent::setDeviceCmdWaitDur(uint64_t wait_dur) {
  addExtra("device_cmd_wait_dur", wait_dur);
  addExtra("cmd_wait_time", wait_dur);
}

void ProfileEvent::setDeviceCmdExecDur(uint64_t exec_dur) {
  addExtra("device_cmd_execute_dur", exec_dur);
  addExtra("cmd_execution_time", exec_dur);
}

template <typename... Args> void ProfileEvent::addExtra(std::string name, Args&&... args) {
  extra_.emplace(std::move(name), std::forward<Args>(args)...);
}

template <typename T> std::optional<T> ProfileEvent::getExtra(std::string name) const {
  std::optional<T> optValue;
  if (auto it = extra_.find(name); it != extra_.end()) {
    optValue = std::get<T>(it->second);
  }
  return optValue;
}

} // namespace rt::profiling
