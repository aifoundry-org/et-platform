//******************************************************************************
// Copyright (C) 2020 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "test_idevice_mgmt_api.h"
#include <array>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <functional>

using namespace dev;
using namespace std::chrono_literals;

namespace {
  constexpr int kIDevice = 0;
  constexpr int kCommandsToProcess = 1;
} //namespace


namespace fs = std::experimental::filesystem;

void TestIDeviceMngtAPI::fListener(TimeDuration waitTime) {
  bool sqAvailable = true, cqAvailable = true;
  int responseReceived = 0;
  auto start = Clock::now();
  auto end = start + waitTime;

  bool isPendingEvent = true;
  while (responseReceived < kCommandsToProcess) {
    if (end < Clock::now()) {
      EXPECT_EQ(responseReceived, kCommandsToProcess);
      return;
    }
    if (isPendingEvent) {
      isPendingEvent = false;
    } else {
      devLayer_->waitForEpollEventsServiceProcessor(kIDevice, sqAvailable, cqAvailable);
    }
    if (cqAvailable) {
      while (popRsp()) {
        responseReceived++;
      }
    }
  }
}

device_mgmt_api::tag_id_t TestIDeviceMngtAPI::getNextTagId() {
  return tag_id_++;
}

bool TestIDeviceMngtAPI::pushCmd(int device, device_mgmt_api::msg_id_t msg_id) {
  switch (msg_id) {
  case device_mgmt_api::DM_CMD_GET_MODULE_MANUFACTURE_NAME: {
    device_mgmt_api::device_mgmt_asset_tracking_cmd_t cmd = {0};
    cmd.command_info.cmd_hdr.tag_id = getNextTagId();
    cmd.command_info.cmd_hdr.msg_id = msg_id;
    cmd.command_info.cmd_hdr.size = sizeof(device_mgmt_api::device_mgmt_asset_tracking_cmd_t);
    return devLayer_->sendCommandServiceProcessor(device, (std::byte*)&cmd, sizeof(cmd));
  }
 }
}

bool TestIDeviceMngtAPI::popRsp(void) {
  std::vector<std::byte> message;
  auto res = devLayer_->receiveResponseServiceProcessor(kIDevice, message);
  if (!res) {
    return res;
  }

  auto response_header = reinterpret_cast<device_mgmt_api::rsp_header_t*>(message.data());
  auto rsp_msg_id = response_header->rsp_hdr.msg_id;
  auto rsp_tag_id = response_header->rsp_hdr.tag_id;

  if (rsp_msg_id == device_mgmt_api::DM_CMD_GET_MODULE_MANUFACTURE_NAME) {
    auto response = reinterpret_cast<device_mgmt_api::device_mgmt_asset_tracking_rsp_t*>(message.data());

    EXPECT_TRUE(response->rsp_hdr.rsp_hdr.msg_id == rsp_msg_id);

    std::cout << "\n =====> Device response verified <====\n" << std::endl;
  } else {
    std::cout << "Unknown response!" << std::endl;
    return false;
  }

  return res;
}

void TestIDeviceMngtAPI::getModuleManufactureName(void) {
  // push cmd to SP SQ
  TestIDeviceMngtAPI::pushCmd(kIDevice, device_mgmt_api::DM_CMD_GET_MODULE_MANUFACTURE_NAME);

  // Create one listener thread
  threadVector_.push_back(
    std::thread(std::bind(&TestIDeviceMngtAPI::fListener, this, fListenerTimeout_)));

  for (auto& t : threadVector_) {
    if (t.joinable()) {
      t.join();
    }
  }
}

