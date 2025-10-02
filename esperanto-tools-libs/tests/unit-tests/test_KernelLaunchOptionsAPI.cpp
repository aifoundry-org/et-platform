//******************************************************************************
// Copyright (C) 2023, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "KernelLaunchOptionsImp.h"
#include "RuntimeFixture.h"
#include <optional>

using namespace rt;

TEST_F(RuntimeFixture, checkSetShireMask) {
  size_t shireMask = 0xFFFUL;
  KernelLaunchOptions opts;
  opts.setShireMask(shireMask);
  EXPECT_EQ(opts.imp_->shireMask_, shireMask);
}

TEST_F(RuntimeFixture, checkSetBarrier) {
  bool barrier = true;
  KernelLaunchOptions opts;
  opts.setBarrier(barrier);
  EXPECT_TRUE(opts.imp_->barrier_);
}

TEST_F(RuntimeFixture, checkSetsetFlushL3) {
  bool flushL3 = true;
  KernelLaunchOptions opts;
  opts.setFlushL3(flushL3);
  EXPECT_TRUE(opts.imp_->flushL3_);
}

TEST_F(RuntimeFixture, checkSetUserTracing) {
  uint64_t buffer = 0x12345;
  uint32_t bufferSize = 0x1000;
  uint32_t threshold = 3202;
  uint64_t shireMask = 0x100FFUL;
  uint64_t threadMask = 0x333FFFUL;
  uint32_t eventMask = 0x5555111UL;
  uint32_t filterMask = 0x777333UL;

  KernelLaunchOptions opts;

  opts.setUserTracing(buffer, bufferSize, threshold, shireMask, threadMask, eventMask, filterMask);

  EXPECT_TRUE(opts.imp_->userTraceConfig_.has_value());
  EXPECT_EQ(opts.imp_->userTraceConfig_->buffer_, buffer);
  EXPECT_EQ(opts.imp_->userTraceConfig_->buffer_size_, bufferSize);
  EXPECT_EQ(opts.imp_->userTraceConfig_->threshold_, threshold);
  EXPECT_EQ(opts.imp_->userTraceConfig_->shireMask_, shireMask);
  EXPECT_EQ(opts.imp_->userTraceConfig_->threadMask_, threadMask);
  EXPECT_EQ(opts.imp_->userTraceConfig_->eventMask_, eventMask);
  EXPECT_EQ(opts.imp_->userTraceConfig_->filterMask_, filterMask);
}

TEST_F(RuntimeFixture, checkSetStackConfig) {
  uint64_t baseAddr = 61440;
  uint64_t totalSize = 24576;

  KernelLaunchOptions opts;
  opts.setStackConfig(reinterpret_cast<std::byte*>(baseAddr), totalSize);

  EXPECT_TRUE(opts.imp_->stackConfig_.has_value());
  EXPECT_EQ(reinterpret_cast<uint64_t>(opts.imp_->stackConfig_->baseAddress_), baseAddr);
  EXPECT_EQ(opts.imp_->stackConfig_->totalSize_, totalSize);
}

TEST_F(RuntimeFixture, checkSetCoreDumpFilePath) {
  std::string coreDumpFilePath = "coreDumpFilePath";

  KernelLaunchOptions opts;
  opts.setCoreDumpFilePath(coreDumpFilePath);

  EXPECT_TRUE(opts.imp_->coreDumpFilePath_ == coreDumpFilePath);
}

TEST_F(RuntimeFixture, checkAllAPIAtOnce) {
  bool barrier = true;
  bool flushL3 = true;
  uint64_t buffer = 0x12345;
  uint32_t bufferSize = 0x1000;
  uint32_t threshold = 3202;
  uint64_t shireMask = 0x100FFUL;
  uint64_t threadMask = 0x333FFFUL;
  uint32_t eventMask = 0x5555111UL;
  uint32_t filterMask = 0x777333UL;
  uint64_t baseAddr = 61440;
  uint64_t totalSize = 24576;
  std::string coreDumpFilePath = "coreDumpFilePath";

  KernelLaunchOptions opts;

  opts.setShireMask(shireMask);
  opts.setBarrier(barrier);
  opts.setFlushL3(flushL3);
  opts.setUserTracing(buffer, bufferSize, threshold, shireMask, threadMask, eventMask, filterMask);
  opts.setStackConfig(reinterpret_cast<std::byte*>(baseAddr), totalSize);
  opts.setCoreDumpFilePath(coreDumpFilePath);

  EXPECT_EQ(opts.imp_->shireMask_, shireMask);
  EXPECT_TRUE(opts.imp_->barrier_);
  EXPECT_TRUE(opts.imp_->flushL3_);
  EXPECT_TRUE(opts.imp_->userTraceConfig_.has_value());
  EXPECT_EQ(opts.imp_->userTraceConfig_->buffer_, buffer);
  EXPECT_EQ(opts.imp_->userTraceConfig_->buffer_size_, bufferSize);
  EXPECT_EQ(opts.imp_->userTraceConfig_->threshold_, threshold);
  EXPECT_EQ(opts.imp_->userTraceConfig_->shireMask_, shireMask);
  EXPECT_EQ(opts.imp_->userTraceConfig_->threadMask_, threadMask);
  EXPECT_EQ(opts.imp_->userTraceConfig_->eventMask_, eventMask);
  EXPECT_EQ(opts.imp_->userTraceConfig_->filterMask_, filterMask);
  EXPECT_TRUE(opts.imp_->stackConfig_.has_value());
  EXPECT_EQ(reinterpret_cast<uint64_t>(opts.imp_->stackConfig_->baseAddress_), baseAddr);
  EXPECT_EQ(opts.imp_->stackConfig_->totalSize_, totalSize);
  EXPECT_TRUE(opts.imp_->coreDumpFilePath_ == coreDumpFilePath);
}

int main(int argc, char** argv) {
  RuntimeFixture::sDlType = RuntimeFixture::DeviceLayerImp::FAKE;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}