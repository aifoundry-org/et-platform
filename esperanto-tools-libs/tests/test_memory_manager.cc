//******************************************************************************
// Copyright (C) 2018,2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "Core/CommandLineOptions.h"
#include "Core/Device.h"
#include "Core/MemoryManager.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace et_runtime::device;

class FakeDevice : public et_runtime::Device {
public:
  FakeDevice() : Device(0) {}
};

TEST(MemoryManager, alloc_host) {
  absl::SetFlag(&FLAGS_dev_target, DeviceTargetOption("fake_device"));
  FakeDevice dev;
  auto mem_manager = MemoryManager(dev);
  uint8_t *ptr = nullptr;
  auto res = mem_manager.mallocHost((void **)&ptr, 10);
  ASSERT_TRUE(ptr != nullptr);
  ASSERT_EQ(res, etrtSuccess);
  res = mem_manager.freeHost(ptr);
  ASSERT_EQ(res, etrtSuccess);
}
