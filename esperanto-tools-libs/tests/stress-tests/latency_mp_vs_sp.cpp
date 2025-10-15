//******************************************************************************
// Copyright (c) 2025 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------

#include "RuntimeFixture.h"
#include <easy/profiler.h>
#include <gtest/gtest.h>

TEST_F(RuntimeFixture, LatencyTest_1M_1024_memcpy_1stream_1thread) {
  // force to be fake
  RuntimeFixture::sDlType = DeviceLayerImp::FAKE;
  // run first SP
  RuntimeFixture::sRtType = RtType::SP;
  TearDown();
  SetUp();
  EASY_BLOCK("SP")
  run_stress_mem(runtime_.get(), 1 << 20, 1024, 1, 1, false);
  EASY_END_BLOCK
  // now run MP
  RuntimeFixture::sRtType = RtType::MP;
  TearDown();
  SetUp();
  EASY_BLOCK("MP")
  run_stress_mem(runtime_.get(), 1 << 20, 1024, 1, 1, false);
  EASY_END_BLOCK
  profiler::dumpBlocksToFile("LatencyTest_1M_1024_memcpy_1stream_1thread.prof");
}

TEST_F(RuntimeFixture, LatencyTest_1KB_10_memcpy_10stream_10thread) {
  // force to be fake
  RuntimeFixture::sDlType = DeviceLayerImp::FAKE;
  // run first SP
  RuntimeFixture::sRtType = RtType::SP;
  TearDown();
  SetUp();
  EASY_BLOCK("SP")
  run_stress_mem(runtime_.get(), 1 << 10, 1024, 10, 10, false);
  EASY_END_BLOCK
  // now run MP
  RuntimeFixture::sRtType = RtType::MP;
  TearDown();
  SetUp();
  EASY_BLOCK("MP")
  run_stress_mem(runtime_.get(), 1 << 10, 1024, 10, 10, false);
  EASY_END_BLOCK
  profiler::dumpBlocksToFile("LatencyTest_1KB_10_memcpy_10stream_10thread.prof");
}

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
