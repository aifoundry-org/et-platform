//******************************************************************************
// Copyright (C) 2021, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "RuntimeFixture.h"
#include <gtest/gtest.h>

TEST_F(RuntimeFixture, Launch_100k_Kernels_withBarrier_NOSYSEMU) {
  if (sDlType == DeviceLayerImp::SYSEMU) {
    RT_LOG(INFO) << "Not running this test on sysemu, its too slow";
    return;
  }
  auto kernel = loadKernel("empty.elf");
  auto args = std::array<std::byte, 32>{};
  auto innerLoopSize = 1e5;
  RT_LOG(INFO) << "Launching " << innerLoopSize << " kernels with a waitForEvent after each launch.";

  for (auto i = 0U; i < innerLoopSize; ++i) {
    auto e = runtime_->kernelLaunch(defaultStreams_[0], kernel, args.data(), args.size(), 0x1);
    runtime_->waitForEvent(e);
  }
}

TEST_F(RuntimeFixture, Launch_100k_Kernels_noBarrier_NOSYSEMU) {
  if (sDlType == DeviceLayerImp::SYSEMU) {
    RT_LOG(INFO) << "Not running this test on sysemu, its too slow";
    return;
  }
  auto kernel = loadKernel("empty.elf");
  auto args = std::array<std::byte, 32>{};
  auto innerLoopSize = 1e5;
  RT_LOG(INFO) << "Launching " << innerLoopSize << " kernels with a waitForStream at the end.";

  for (auto i = 0U; i < innerLoopSize; ++i) {
    try {
      runtime_->kernelLaunch(defaultStreams_[0], kernel, args.data(), args.size(), 0x1);
    } catch (const rt::Exception&) {
      runtime_->waitForStream(defaultStreams_[0]);
      --i;
    }
  }
  runtime_->waitForStream(defaultStreams_[0]);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  RuntimeFixture::ParseArguments(argc, argv);
  g3::log_levels::disable(DEBUG);
  return RUN_ALL_TESTS();
}
