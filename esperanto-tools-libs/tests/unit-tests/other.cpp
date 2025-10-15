//******************************************************************************
// Copyright (c) 2025 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------

#include "RuntimeFixture.h"
#include "Utils.h"
#include "runtime/IRuntime.h"
using namespace rt;

TEST_F(RuntimeFixture, checkStackTraceException) {
  // run a kernelLaunch with bad parameters to check for the exception
  try {
    runtime_->kernelLaunch(StreamId{7}, KernelId{8}, nullptr, 0, 0);
  } catch (const rt::Exception& e) {
    RT_VLOG(HIGH) << e.what();
  }
}

TEST_F(RuntimeFixture, checkStackConfigBadSizeException) {
  // run a kernelLaunch with size not aligned to 4096B
  constexpr size_t kTraceBytesPerHart = 4096;

  KernelLaunchOptions opts;
  std::byte* baseAddrptr = reinterpret_cast<std::byte*>((1UL << 12));
  EXPECT_THROW(opts.setStackConfig(baseAddrptr, kTraceBytesPerHart + 5), rt::Exception);
}

TEST_F(RuntimeFixture, checkStackConfigBadBaseAddressException) {
  // run a kernelLaunch with not aligned baseAdress pointer
  constexpr size_t kTraceBytesPerHart = 4096;

  KernelLaunchOptions opts;
  std::byte* baseAddrptr = reinterpret_cast<std::byte*>((1UL << 12) + 15);
  EXPECT_THROW(opts.setStackConfig(baseAddrptr, kTraceBytesPerHart), rt::Exception);
}

int main(int argc, char** argv) {
  RuntimeFixture::sDlType = RuntimeFixture::DeviceLayerImp::FAKE;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}