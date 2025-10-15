//******************************************************************************
// Copyright (c) 2025 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------
#include "Utils.h"
#include "runtime/DeviceLayerFake.h"
#include "runtime/IRuntime.h"
#include "runtime/Types.h"
#include <device-layer/DeviceLayerMock.h>
#include <device-layer/IDeviceLayer.h>
#include <gmock/gmock-actions.h>
#include <gtest/gtest.h>

using namespace rt;
using namespace testing;
using namespace std::chrono;

TEST(devices, get_properties) {
  dev::DeviceLayerFake fake;
  auto deviceLayer = std::shared_ptr<NiceMock<dev::DeviceLayerMock>>(new NiceMock<dev::DeviceLayerMock>{&fake});
  deviceLayer->Delegate();

  auto fakeMemorySize = 40U * 1024U * 1024U;
  auto fakeAvailableShires = 16;

  uint32_t L3SizeInMB = 30;
  uint32_t L2SizeInMB = 15;
  uint32_t ScratchPadSizeInMB = 10;

  dev::DeviceConfig dc;
  dc.formFactor_ = dev::DeviceConfig::FormFactor::PCIE;
  dc.tdp_ = 250;
  dc.totalL3Size_ = L3SizeInMB * 1024;
  dc.totalL2Size_ = L2SizeInMB * 1024;
  dc.totalScratchPadSize_ = ScratchPadSizeInMB * 1024;
  dc.cacheLineSize_ = 64;
  dc.numL2CacheBanks_ = 8;
  dc.ddrBandwidth_ = 300;
  dc.minionBootFrequency_ = 1000;
  dc.computeMinionShireMask_ = 0xFFFFFFFF;
  dc.spareComputeMinionShireId_ = 33;
  dc.archRevision_ = dev::DeviceConfig::ArchRevision::ETSOC1;

  auto runtime = IRuntime::create(deviceLayer, rt::Options{false, false});

  EXPECT_CALL(*deviceLayer, getDeviceConfig(_)).Times(1).WillRepeatedly(Return(dc));
  EXPECT_CALL(*deviceLayer, getDramSize(_)).Times(1).WillRepeatedly(Return(fakeMemorySize));
  EXPECT_CALL(*deviceLayer, getActiveShiresNum(_)).Times(1).WillRepeatedly(Return(fakeAvailableShires));

  auto properties = runtime->getDeviceProperties(rt::DeviceId(0));

  EXPECT_EQ(properties.frequency_, dc.minionBootFrequency_);
  EXPECT_EQ(properties.availableShires_, fakeAvailableShires);
  EXPECT_EQ(properties.memoryBandwidth_, dc.ddrBandwidth_);
  EXPECT_EQ(properties.memorySize_, fakeMemorySize);
  EXPECT_EQ(properties.l3Size_, L3SizeInMB);
  EXPECT_EQ(properties.l2shireSize_, L2SizeInMB);
  EXPECT_EQ(properties.l2scratchpadSize_, ScratchPadSizeInMB);
  EXPECT_EQ(properties.cacheLineSize_, dc.cacheLineSize_);
  EXPECT_EQ(properties.l2CacheBanks_, dc.numL2CacheBanks_);
  EXPECT_EQ(properties.spareComputeMinionShireId_, dc.spareComputeMinionShireId_);

  EXPECT_EQ(properties.localScpFormat0BaseAddress_, dc.localScpFormat0BaseAddress_);
  EXPECT_EQ(properties.localScpFormat1BaseAddress_, dc.localScpFormat1BaseAddress_);
  EXPECT_EQ(properties.localDRAMBaseAddress_, dc.localDRAMBaseAddress_);
  EXPECT_EQ(properties.onPkgScpFormat2BaseAddress_, dc.onPkgScpFormat2BaseAddress_);
  EXPECT_EQ(properties.onPkgDRAMBaseAddress_, dc.onPkgDRAMBaseAddress_);
  EXPECT_EQ(properties.onPkgDRAMInterleavedBaseAddress_, dc.onPkgDRAMInterleavedBaseAddress_);

  EXPECT_EQ(properties.localDRAMSize_, dc.localDRAMSize_);
  EXPECT_EQ(properties.minimumAddressAlignmentBits_, dc.minimumAddressAlignmentBits_);
  EXPECT_EQ(properties.numChiplets_, dc.numChiplets_);

  EXPECT_EQ(properties.localScpFormat0ShireLSb_, dc.localScpFormat0ShireLSb_);
  EXPECT_EQ(properties.localScpFormat0ShireBits_, dc.localScpFormat0ShireBits_);
  EXPECT_EQ(properties.localScpFormat0LocalShire_, dc.localScpFormat0LocalShire_);

  EXPECT_EQ(properties.localScpFormat1ShireLSb_, dc.localScpFormat1ShireLSb_);
  EXPECT_EQ(properties.localScpFormat1ShireBits_, dc.localScpFormat1ShireBits_);

  EXPECT_EQ(properties.onPkgScpFormat2ShireLSb_, dc.onPkgScpFormat2ShireLSb_);
  EXPECT_EQ(properties.onPkgScpFormat2ShireBits_, dc.onPkgScpFormat2ShireBits_);
  EXPECT_EQ(properties.onPkgScpFormat2ChipletLSb_, dc.onPkgScpFormat2ChipletLSb_);
  EXPECT_EQ(properties.onPkgScpFormat2ChipletBits_, dc.onPkgScpFormat2ChipletBits_);

  EXPECT_EQ(properties.onPkgDRAMChipletLSb_, dc.onPkgDRAMChipletLSb_);
  EXPECT_EQ(properties.onPkgDRAMChipletBits_, dc.onPkgDRAMChipletBits_);
  EXPECT_EQ(properties.onPkgDRAMInterleavedChipletLSb_, dc.onPkgDRAMInterleavedChipletLSb_);
  EXPECT_EQ(properties.onPkgDRAMInterleavedChipletBits_, dc.onPkgDRAMInterleavedChipletBits_);
}

int main(int argc, char** argv) {
  logging::LoggerDefault logger_;
  g3::log_levels::disable(DEBUG);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
