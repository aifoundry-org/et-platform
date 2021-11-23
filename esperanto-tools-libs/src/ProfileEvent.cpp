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

namespace rt::profiling {
std::string getString(Class cls) {
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
  case event::Pmc:
    return "Pmc";
  case event::CommandSent:
    return "CommandSent";
  case event::ResponseReceived:
    return "ResponseReceived";
  default:
    assert(false);
    return "Unknown class";
  }
}

std::string getString(Type type) {
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

std::string getString(ResponseType rspType) {
  switch (rspType) {
  case ResponseType::DMARead:
    return "DMA Read";
  case ResponseType::DMAWrite:
    return "DMA Write";
  case ResponseType::Kernel:
    return "Kernel";
  default:
    assert(false);
    return "Unknown type";
  }
}

Class class_from_string(const std::string& str) {
  static std::once_flag s_once_flag;
  static std::unordered_map<std::string, Class> s_map;
  std::call_once(s_once_flag, []() {
    s_map[getString(Class::GetDevices)] = Class::GetDevices;
    s_map[getString(Class::LoadCode)] = Class::LoadCode;
    s_map[getString(Class::UnloadCode)] = Class::UnloadCode;
    s_map[getString(Class::MallocDevice)] = Class::MallocDevice;
    s_map[getString(Class::FreeDevice)] = Class::FreeDevice;
    s_map[getString(Class::CreateStream)] = Class::CreateStream;
    s_map[getString(Class::DestroyStream)] = Class::DestroyStream;
    s_map[getString(Class::KernelLaunch)] = Class::KernelLaunch;
    s_map[getString(Class::MemcpyHostToDevice)] = Class::MemcpyHostToDevice;
    s_map[getString(Class::MemcpyDeviceToHost)] = Class::MemcpyDeviceToHost;
    s_map[getString(Class::WaitForEvent)] = Class::WaitForEvent;
    s_map[getString(Class::WaitForStream)] = Class::WaitForStream;
    s_map[getString(Class::DeviceCommand)] = Class::DeviceCommand;
    s_map[getString(Class::Pmc)] = Class::Pmc;
    s_map[getString(Class::CommandSent)] = Class::CommandSent;
    s_map[getString(Class::ResponseReceived)] = Class::ResponseReceived;

    assert(s_map.size() == static_cast<int>(Class::COUNT));
  });

  return s_map[str];
}

Type type_from_string(const std::string& str) {
  static std::once_flag s_once_flag;
  static std::unordered_map<std::string, Type> s_map;
  std::call_once(s_once_flag, []() {
    s_map[getString(Type::Start)] = Type::Start;
    s_map[getString(Type::End)] = Type::End;
    s_map[getString(Type::Complete)] = Type::Complete;
    s_map[getString(Type::Instant)] = Type::Instant;
  });
  return s_map[str];
}

ResponseType response_type_from_string(const std::string& str) {
  static std::once_flag s_once_flag;
  static std::unordered_map<std::string, ResponseType> s_map;
  std::call_once(s_once_flag, []() {
    s_map[getString(ResponseType::DMARead)] = ResponseType::DMARead;
    s_map[getString(ResponseType::DMAWrite)] = ResponseType::DMAWrite;
    s_map[getString(ResponseType::Kernel)] = ResponseType::Kernel;

    assert(s_map.size() == static_cast<int>(ResponseType::COUNT));
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
std::optional<ResponseType> ProfileEvent::getResponseType() const {
  return getExtra<ResponseType>("rsp_type");
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

void ProfileEvent::setResponseType(ResponseType rspType) {
  addExtra("rsp_type", rspType);
}

void ProfileEvent::setLoadAddress(uint64_t loadAddress) {
  addExtra("load_address", loadAddress);
}

void ProfileEvent::setDeviceCmdStartTs(uint64_t start_ts) {
  addExtra("device_cmd_start_ts", start_ts);
}

void ProfileEvent::setDeviceCmdWaitDur(uint64_t wait_dur) {
  addExtra("device_cmd_wait_dur", wait_dur);
}

void ProfileEvent::setDeviceCmdExecDur(uint64_t exec_dur) {
  addExtra("device_cmd_exec_dur", exec_dur);
}

template <typename... Args> void ProfileEvent::addExtra(std::string name, Args&&... args) {
  extra_.emplace(std::move(name), std::forward<Args>(args)...);
}

template <typename T> std::optional<T> ProfileEvent::getExtra(const std::string& name) const {
  std::optional<T> optValue;
  if (auto it = extra_.find(name); it != extra_.end()) {
    optValue = std::get<T>(it->second);
  }
  return optValue;
}

} // namespace rt::profiling
