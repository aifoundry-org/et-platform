/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/

#include "DevErrorEvent.h"
#include <fmt/format.h>
#include <regex>

using namespace device_management;

DevErrorEvent::DevErrorEvent(std::string errStats) {
  for (auto typeIdx = 0U; typeIdx < static_cast<uint8_t>(EventType::TotalEvents); typeIdx++) {
    std::smatch match;
    std::regex rgx(getString(static_cast<EventType>(typeIdx)) + ":\\s+(\\d+)");
    if (std::regex_search(errStats, match, rgx)) {
      counters_[typeIdx] = std::strtoull(match.str(1).c_str(), nullptr, 10);
    }
  }
}

uint64_t DevErrorEvent::getDevErrorEventCount(DevErrorEvent::EventType eventType) {
  return counters_[static_cast<uint8_t>(eventType)];
}

bool DevErrorEvent::hasFailure(std::vector<DevErrorEvent::EventType> skipList) {
  bool failure = false;
  static decltype(skipList) fullList;
  static std::once_flag listOnceFlag;
  std::call_once(listOnceFlag, []() {
    for (auto typeIdx = 0U; typeIdx < static_cast<uint8_t>(EventType::TotalEvents); typeIdx++) {
      fullList.push_back(static_cast<EventType>(typeIdx));
    }
    assert(fullList.size() == static_cast<int>(EventType::TotalEvents));
  });
  decltype(skipList) checkList;
  std::sort(skipList.begin(), skipList.end(), [this](auto a, auto b) { return a < b; });
  std::set_difference(fullList.begin(), fullList.end(), skipList.begin(), skipList.end(), std::back_inserter(checkList),
                      [this](auto a, auto b) { return a < b; });
  for (auto eventType : checkList) {
    if (getDevErrorEventCount(eventType) > 0) {
      DV_LOG(INFO) << fmt::format("Found error event {} (count: {})", getString(eventType),
                                  getDevErrorEventCount(eventType));
      failure = true;
    }
  }
  return failure;
}
