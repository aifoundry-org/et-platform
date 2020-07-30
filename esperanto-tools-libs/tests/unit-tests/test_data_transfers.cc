//******************************************************************************
// Copyright (C) 2020,, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "esperanto/runtime/Core/CommandLineOptions.h"
#include "esperanto/runtime/Core/Device.h"
#include "esperanto/runtime/Core/DeviceManager.h"
#include "esperanto/runtime/Core/Memory.h"
#include "esperanto/runtime/Core/MemoryManager.h"

#include <absl/flags/flag.h>
#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>

using namespace et_runtime;
using namespace et_runtime::device;

class TestDevice : public ::testing::Test {
public:
  void SetUp() override {
    absl::SetFlag(&FLAGS_dev_target, DeviceTargetOption("sysemu_grpc"));
    auto device_manager = et_runtime::getDeviceManager();
    auto ret_value = device_manager->registerDevice(0);
    dev_ = ret_value.get();

    // Start the simulator
    ASSERT_EQ(dev_->init(), etrtSuccess);
  }

  void TearDown() override {

    // Stop the simulator
    EXPECT_EQ(etrtSuccess, dev_->resetDevice());
  }

  std::shared_ptr<Device> dev_;
};

TEST_F(TestDevice, memcpy_host_to_dev) {
  BufferDebugInfo info = {1, 1, 1};
  int size = 100;
  auto res = dev_->mem_manager().mallocConstant(size, info);
  auto buffer = res.get();
  ASSERT_TRUE((bool)res);
  int data[size];
  HostBuffer host_ptr(data);
  dev_->memcpy(buffer, host_ptr, size);
}

TEST_F(TestDevice, memcpy_dev_to_host) {
  BufferDebugInfo info = {1, 1, 1};
  int size = 100;
  auto res = dev_->mem_manager().mallocConstant(size, info);
  auto buffer = res.get();
  ASSERT_TRUE((bool)res);
  int data[size];
  HostBuffer host_ptr(data);
  dev_->memcpy(host_ptr, buffer, size);
}
