/*-------------------------------------------------------------------------
 * Copyright (C) 2023, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
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

uint64_t DevErrorEvent::getDevErrorEventCount(std::string event) {
  return counters_[static_cast<uint8_t>(getEventType(event))];
}

bool DevErrorEvent::hasFailure(std::vector<std::string> list, bool isCheckList) {
  bool failure = false;
  std::vector<std::string> checkList;
  if (isCheckList) {
    checkList = std::move(list);
  } else {
    static std::once_flag listOnceFlag;
    static std::vector<std::string> fullList;
    std::call_once(listOnceFlag, []() {
      for (auto typeIdx = 0U; typeIdx < static_cast<uint8_t>(EventType::TotalEvents); typeIdx++) {
        fullList.push_back(getString(static_cast<EventType>(typeIdx)));
      }
      assert(fullList.size() == static_cast<int>(EventType::TotalEvents));
    });
    std::set_difference(fullList.begin(), fullList.end(), list.begin(), list.end(),
                        std::inserter(checkList, checkList.begin()));
  }
  for (const auto& event : checkList) {
    if (getDevErrorEventCount(event) > 0) {
      DV_LOG(INFO) << fmt::format("Found error event {} (count: {})", event, getDevErrorEventCount(event));
      failure = true;
    }
  }
  return failure;
}
