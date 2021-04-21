//******************************************************************************
// Copyright (C) 2021 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef TEST_DEVICE_MANAGEMENT_H
#define TEST_DEVICE_MANAGEMENT_H

#include <atomic>
#include <common/logging/Logger.h>
#include <deviceLayer/IDeviceLayer.h>
#include <esperanto/device-apis/management-api/device_mgmt_api_cxx.h>
#include <experimental/filesystem>
#include <gtest/gtest.h>
#include <thread>

using Clock = std::chrono::system_clock;
using Timepoint = Clock::time_point;
using TimeDuration = Clock::duration;

class TestIDeviceMngtAPI : public ::testing::Test {
public:

  void fListener(TimeDuration waitTime);

protected:

  device_mgmt_api::tag_id_t getNextTagId();

  bool pushCmd(int device, device_mgmt_api::msg_id_t msg_id);
  bool popRsp(void);
  void getModuleManufactureName(void);

  logging::LoggerDefault loggerDefault_;
  std::unique_ptr<dev::IDeviceLayer> devLayer_;
  std::atomic<device_mgmt_api::tag_id_t> tag_id_;
  std::vector<std::thread> threadVector_;
  TimeDuration fListenerTimeout_;
};

#endif //TEST_DEVICE_MANAGEMENT_H

