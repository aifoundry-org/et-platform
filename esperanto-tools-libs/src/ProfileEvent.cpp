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


namespace rt {
namespace profiling {

std::string to_string(Class cls) {
  using event = Class;
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

std::string to_string(Type type) {
  switch (type) {
  case Type::Start:
    return "Start";
  case Type::End:
    return "End";
  case Type::Single:
    return "Single";
  default:
    assert(false);
    return "Unknown type";
  }
}

Class GetClass(const std::string& str) {
  static std::once_flag s_once_flag;
  static std::unordered_map<std::string, Class> s_map;
  std::call_once(s_once_flag, []() {
    for (auto cls : {Class::First, Class::Last}) {
      s_map[to_string(cls)] = cls;
    }
  });

  return s_map[str];
}

Type GetType(const std::string& str) {
  static std::once_flag s_once_flag;
  static std::unordered_map<std::string, Type> s_map;
  std::call_once(s_once_flag, []() {
    for (auto type : {Type::First, Type::Last}) {
      s_map[to_string(type)] = type;
    }
  });
  return s_map[str];
}


ProfileEvent::ProfileEvent(Type type, Class cls)
  : type_(type)
  , class_(cls) {
    setTimeStamp();
}


ProfileEvent::ProfileEvent(Type type, Class cls, StreamId stream, EventId event,
                        std::unordered_map<std::string, uint64_t> extra)
  : ProfileEvent(type, cls) {
  extra_ = std::move(extra);
  setStream(stream);
  setEvent(event);
}

void ProfileEvent::setTimeStamp() {
  timeStamp_ = std::chrono::steady_clock::now();
}

void ProfileEvent::setPairId(Id id) {
  addExtra("pair_id", id);
}

void ProfileEvent::setEvent(EventId event) {
  addExtra("event", static_cast<uint64_t>(event));
}

void ProfileEvent::setStream(StreamId stream) {
  addExtra("stream", static_cast<uint64_t>(stream));
}

void ProfileEvent::setDeviceId(DeviceId deviceId) {
  addExtra("device_id", static_cast<uint64_t>(deviceId));
}

void ProfileEvent::setKernelId(KernelId kernelId) {
  addExtra("kernel_id", static_cast<uint64_t>(kernelId));
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

Type ProfileEvent::getType() const { return type_; }
Class ProfileEvent::getClass() const { return class_; }
ProfileEvent::TimePoint ProfileEvent::getTimeStamp() const { return timeStamp_; }
std::string ProfileEvent::getThreadId() const { return threadId_; }

std::optional<ProfileEvent::Id> ProfileEvent::getPairId() const {
  return getExtra("pair_id");
}
std::optional<EventId> ProfileEvent::getEvent() const {
  std::optional<EventId> optEventId;
  auto it = extra_.find("event");
  if (it != extra_.end()) {
    optEventId = static_cast<EventId>(it->second);
  }
  return optEventId;
}
std::optional<StreamId> ProfileEvent::getStream() const {
  std::optional<StreamId> optStreamId;
  auto it = extra_.find("stream");
  if (it != extra_.end()) {
    optStreamId = static_cast<StreamId>(it->second);
  }
  return optStreamId;
}
std::optional<DeviceId> ProfileEvent::getDeviceId() const {
  std::optional<DeviceId> optDeviceId;
  auto it = extra_.find("device_id");
  if (it != extra_.end()) {
    optDeviceId = static_cast<DeviceId>(it->second);
  }
  return optDeviceId;
}
std::optional<KernelId> ProfileEvent::getKernelId() const {
  std::optional<KernelId> optKernelId;
  auto it = extra_.find("kernel_id");
  if (it != extra_.end()) {
    optKernelId = static_cast<KernelId>(it->second);
  }
  return optKernelId;
}
std::optional<uint64_t> ProfileEvent::getLoadAddress() const {
  return getExtra("load_address");
}
std::optional<uint64_t> ProfileEvent::getDeviceCmdStartTs() const{
  return getExtra("device_cmd_start_ts");
}
std::optional<uint64_t> ProfileEvent::getDeviceCmdWaitDur() const{
  return getExtra("device_cmd_wait_dur");
}
std::optional<uint64_t> ProfileEvent::getDeviceCmdExecDur() const{
  return getExtra("device_cmd_exec_dur");
}

void ProfileEvent::addExtra(std::string name, uint64_t value) {
  extra_.insert({name, value});
}

template <> void ProfileEvent::load(cereal::JSONInputArchive& ar) {
  std::string type;
  ar(cereal::make_nvp("type", type));
  type_ = GetType(type);
  std::string cls;
  ar(cereal::make_nvp("class", cls));
  class_ = GetClass(cls.c_str());
  
  ar(cereal::make_nvp("timeStamp", timeStamp_));
  ar(cereal::make_nvp("thread_id", threadId_));
  ar(cereal::make_nvp("extra", extra_));
}
template <> void ProfileEvent::load(cereal::PortableBinaryOutputArchive& ar) {
  std::string type;
  ar(cereal::make_nvp("type", type));
  type_ = GetType(type);
  std::string cls;
  ar(cereal::make_nvp("class", cls));
  class_ = GetClass(cls.c_str());
  
  ar(cereal::make_nvp("timeStamp", timeStamp_));
  ar(cereal::make_nvp("thread_id", threadId_));
  ar(cereal::make_nvp("extra", extra_));
}
template <> void ProfileEvent::save(cereal::JSONOutputArchive& ar) const {
  ar(cereal::make_nvp("type", to_string(type_)));
  ar(cereal::make_nvp("class", to_string(class_)));
  ar(cereal::make_nvp("timeStamp", timeStamp_));
  std::stringstream ss;
  ss << std::this_thread::get_id();
  ar(cereal::make_nvp("thread_id", ss.str()));
  ar(cereal::make_nvp("extra", extra_));
}
template <> void ProfileEvent::save(cereal::PortableBinaryOutputArchive& ar) const {
  ar(cereal::make_nvp("type", to_string(type_)));
  ar(cereal::make_nvp("class", to_string(class_)));
  ar(cereal::make_nvp("timeStamp", timeStamp_));
  std::stringstream ss;
  ss << std::this_thread::get_id();
  ar(cereal::make_nvp("thread_id", ss.str()));
  ar(cereal::make_nvp("extra", extra_));
}

std::optional<uint64_t> ProfileEvent::getExtra(std::string name) const {
  std::optional<uint64_t> optValue;
  auto it = extra_.find(name);
  if (it != extra_.end()) {
    optValue = it->second;
  }
  return optValue;
}

} // namespace profiling
} // namespace rt
