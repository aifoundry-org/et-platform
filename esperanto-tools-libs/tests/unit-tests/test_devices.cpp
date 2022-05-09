//******************************************************************************
// Copyright (C) 2021, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------
#include "Utils.h"
#include "runtime/IRuntime.h"
#include <device-layer/IDeviceLayerMock.h>
#include <gmock/gmock-actions.h>
#include <gtest/gtest.h>

using namespace rt;
using namespace testing;
using namespace std::chrono;

TEST(devices, get_properties) {
  auto deviceLayer = NiceMock<dev::IDeviceLayerMock>{};
  deviceLayer.DelegateToFake();

  auto fakeMemorySize = 40U * 1024U * 1024U;
  auto fakeAvailableShires = 16;

  dev::DeviceConfig dc;
  dc.formFactor_ = dev::DeviceConfig::FormFactor::PCIE;
  dc.tdp_ = 300;
  dc.totalL3Size_ = 30;
  dc.totalL2Size_ = 15;
  dc.totalScratchPadSize_ = 10;
  dc.cacheLineSize_ = 64;
  dc.numL2CacheBanks_ = 8;
  dc.ddrBandwidth_ = 300;
  dc.minionBootFrequency_ = 1000;
  dc.computeMinionShireMask_ = static_cast<uint32_t>(0xFFFFFFFF);
  dc.spareComputeMinionoShireId_ = 33;
  dc.archRevision_ = 0;

  auto runtime = IRuntime::create(&deviceLayer);

  EXPECT_CALL(deviceLayer, getDeviceConfig(_)).Times(1).WillRepeatedly(Return(dc));
  EXPECT_CALL(deviceLayer, getDramSize()).Times(1).WillRepeatedly(Return(fakeMemorySize));
  EXPECT_CALL(deviceLayer, getActiveShiresNum(_)).Times(1).WillRepeatedly(Return(fakeAvailableShires));
  
  auto properties = runtime->getDeviceProperties(rt::DeviceId(0));

  EXPECT_EQ(properties.frequency_, dc.minionBootFrequency_);
  EXPECT_EQ(properties.availableShires_, fakeAvailableShires);
  EXPECT_EQ(properties.memoryBandwidth_, dc.ddrBandwidth_);
  EXPECT_EQ(properties.memorySize_, fakeMemorySize);
  EXPECT_EQ(properties.l3Size_, dc.totalL3Size_);
  EXPECT_EQ(properties.l2shireSize_, dc.totalL2Size_);
  EXPECT_EQ(properties.l2scratchpadSize_, dc.totalScratchPadSize_);
  EXPECT_EQ(properties.cacheLineSize_, dc.cacheLineSize_);
  EXPECT_EQ(properties.l2CacheBanks_, dc.numL2CacheBanks_);
  EXPECT_EQ(properties.spareComputeMinionoShireId_, dc.spareComputeMinionoShireId_);
  EXPECT_EQ(properties.deviceArch_, dc.archRevision_);
}

int main(int argc, char** argv) {
  logging::LoggerDefault logger_;
  g3::log_levels::disable(DEBUG);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
