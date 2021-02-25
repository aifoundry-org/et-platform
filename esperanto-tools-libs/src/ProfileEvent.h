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

#include <cassert>
#include <chrono>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>

namespace rt {
namespace profiling {
using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;

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

enum class Type { Start, End, Single, First = Start, Last = End };

inline std::string GetName(Class cls) {
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

inline std::string GetType(Type type) {
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

inline auto GetClass(const std::string& str) {
  static std::once_flag s_once_flag;
  static std::unordered_map<std::string, Class> s_map;
  std::call_once(s_once_flag, []() {
    for (auto cls : {Class::First, Class::Last}) {
      s_map[GetName(cls)] = cls;
    }
  });

  return s_map[str];
}

inline auto GetType(const std::string& str) {
  static std::once_flag s_once_flag;
  static std::unordered_map<std::string, Type> s_map;
  std::call_once(s_once_flag, []() {
    for (auto type : {Type::First, Type::Last}) {
      s_map[GetType(type)] = type;
    }
  });
  return s_map[str];
}

struct ProfileEvent {

  explicit ProfileEvent(Type type, Class cls, StreamId stream, EventId event,
                        std::unordered_map<std::string, uint64_t> extra = {})
    : ProfileEvent(type, cls) {
    extra_ = std::move(extra);
    setStream(stream);
    setEvent(event);
  }

  void setStream(StreamId stream) {
    addExtra("stream", static_cast<uint64_t>(stream));
  }

  void setEvent(EventId event) {
    addExtra("event", static_cast<uint64_t>(event));
  }

  void addExtra(std::string name, uint64_t value) {
    extra_.insert({name, value});
  }
  void setTimeStamp() {
    timeStamp_ = std::chrono::steady_clock::now();
  }

  explicit ProfileEvent(Type type, Class cls)
    : type_(type)
    , class_(cls) {
      setTimeStamp();
  }

  template <class Archive> void load(Archive& ar) {
    ar(cereal::make_nvp("timeStamp", timeStamp_));
    std::string cls;
    ar(cereal::make_nvp("class", cls));
    class_ = GetClass(cls.c_str());
    std::string type;
    ar(cereal::make_nvp("type", type));
    type_ = GetType(type);
    ar(cereal::make_nvp("thread_id", threadId_));
    ar(cereal::make_nvp("extra", extra_));
  }
  template <class Archive> void save(Archive& ar) const {
    ar(cereal::make_nvp("timeStamp", timeStamp_));
    ar(cereal::make_nvp("class", GetName(class_)));
    ar(cereal::make_nvp("type", GetType(type_)));
    std::stringstream ss;
    ss << std::this_thread::get_id();
    ar(cereal::make_nvp("thread_id", ss.str()));
    ar(cereal::make_nvp("extra", extra_));
  }

  std::string threadId_;
  std::unordered_map<std::string, uint64_t> extra_;
  TimePoint timeStamp_;
  Type type_;
  Class class_;
};
} // namespace profiling
} // namespace rt
