/*-------------------------------------------------------------------------
 * Copyright (C) 2023, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#pragma once
#include "utils.h"
#include <cassert>
#include <unordered_map>

namespace device_management {

class DevErrorEvent {
public:
  enum class EventType : uint8_t {
    DramCeEvent = 0,
    MinionCeEvent,
    PcieCeEvent,
    PmicCeEvent,
    SpCeEvent,
    SpExceptCeEvent,
    SramCeEvent,
    ThermOvershootCeEvent,
    ThermThrottleCeEvent,
    SpTraceBufferFullCeEvent,
    DramUceEvent,
    MinionHangUceEvent,
    PcieUceEvent,
    SpHangUceEvent,
    SpWdogUceEvent,
    SramUceEvent,
    TotalEvents
  };

  DevErrorEvent() = delete;
  explicit DevErrorEvent(std::string errStats);
  uint64_t getDevErrorEventCount(EventType event);
  bool hasFailure(std::vector<EventType> skipList);
  DevErrorEvent operator+(const DevErrorEvent& other) const {
    auto result = *this;
    for (auto typeIdx = 0U; typeIdx < static_cast<uint8_t>(EventType::TotalEvents); typeIdx++) {
      result.counters_[typeIdx] = this->counters_[typeIdx] + other.counters_[typeIdx];
    }
    return result;
  }
  DevErrorEvent operator-(const DevErrorEvent& other) const {
    auto result = *this;
    for (auto typeIdx = 0U; typeIdx < static_cast<uint8_t>(EventType::TotalEvents); typeIdx++) {
      result.counters_[typeIdx] = this->counters_[typeIdx] - other.counters_[typeIdx];
    }
    return result;
  }

private:
  static std::string getString(enum EventType type) {
    switch (type) {
    case EventType::DramCeEvent:
      return "DramCeEvent";
    case EventType::MinionCeEvent:
      return "MinionCeEvent";
    case EventType::PcieCeEvent:
      return "PcieCeEvent";
    case EventType::PmicCeEvent:
      return "PmicCeEvent";
    case EventType::SpCeEvent:
      return "SpCeEvent";
    case EventType::SpExceptCeEvent:
      return "SpExceptCeEvent";
    case EventType::SramCeEvent:
      return "SramCeEvent";
    case EventType::ThermOvershootCeEvent:
      return "ThermOvershootCeEvent";
    case EventType::ThermThrottleCeEvent:
      return "ThermThrottleCeEvent";
    case EventType::SpTraceBufferFullCeEvent:
      return "SpTraceBufferFullCeEvent";
    case EventType::DramUceEvent:
      return "DramUceEvent";
    case EventType::MinionHangUceEvent:
      return "MinionHangUceEvent";
    case EventType::PcieUceEvent:
      return "PcieUceEvent";
    case EventType::SpHangUceEvent:
      return "SpHangUceEvent";
    case EventType::SpWdogUceEvent:
      return "SpWdogUceEvent";
    case EventType::SramUceEvent:
      return "SramUceEvent";
    default:
      DV_LOG(INFO) << "Unknown event type: " + std::to_string(static_cast<int>(type));
      return "Unknown";
    }
  }
  std::array<uint64_t, static_cast<uint8_t>(EventType::TotalEvents)> counters_ = {0};
};

} // namespace device_management
