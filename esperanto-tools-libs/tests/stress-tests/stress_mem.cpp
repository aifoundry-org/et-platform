//******************************************************************************
// Copyright (C) 2021, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------
#include "TestUtils.h"
#include <algorithm>
#include <device-layer/IDeviceLayerMock.h>
#include <thread>

using namespace testing;

class SysEmu : public Fixture {};

TEST_F(SysEmu, 1KB_1_memcpys_1stream_20thread) {
  run_stress_mem(runtime_.get(), 1<<10, 1, 1, 20);
}

TEST_F(SysEmu, 1KB_1_memcpys_1stream_30thread) {
  run_stress_mem(runtime_.get(), 1<<10, 1, 1, 30);
}

TEST_F(SysEmu, 1KB_1_memcpys_1stream_50thread) {
  run_stress_mem(runtime_.get(), 1<<10, 1, 1, 50);
}

TEST_F(SysEmu, 1KB_1_memcpys_1stream_75thread) {
  run_stress_mem(runtime_.get(), 1<<10, 1, 1, 75);
}

TEST_F(SysEmu, 1KB_100_memcpys_1stream_1thread) {
  run_stress_mem(runtime_.get(), 1 << 10, 1e2, 1, 1);
}

TEST_F(SysEmu, 1KB_100_memcpys_100stream_1thread) {
  run_stress_mem(runtime_.get(), 1<<10, 100, 100, 1);
}

TEST_F(SysEmu, 1M_20_memcpys_2stream_1thread) {
  run_stress_mem(runtime_.get(), 1<<20, 20, 2, 1);
}

TEST_F(SysEmu, 1M_20_memcpys_2stream_2thread) {
  run_stress_mem(runtime_.get(), 1<<20, 20, 2, 2);
}

TEST_F(SysEmu, 1KB_100_memcpys_2stream_2thread) {
  run_stress_mem(runtime_.get(), 1 << 10, 1e2, 2, 2);
}

TEST_F(SysEmu, 1KB_1_memcpys_1stream_1thread) {
  run_stress_mem(runtime_.get(), 1<<10, 1, 1, 1);
}

TEST_F(SysEmu, 1KB_1_memcpys_1stream_2thread) {
  run_stress_mem(runtime_.get(), 1<<10, 1, 1, 2);
}

TEST_F(SysEmu, 1KB_6_memcpys_1stream_1thread) {
  run_stress_mem(runtime_.get(), 1<<10, 6, 1, 1);
}

TEST_F(SysEmu, 1KB_6_memcpys_1stream_2thread) {
  run_stress_mem(runtime_.get(), 1<<10, 6, 1, 2);
}

TEST_F(SysEmu, 1KB_6_memcpys_2stream_1thread) {
  run_stress_mem(runtime_.get(), 1<<10, 6, 2, 1);
}

TEST_F(SysEmu, 1KB_6_memcpys_2stream_2thread) {
  run_stress_mem(runtime_.get(), 1<<10, 6, 2, 2);
}

TEST_F(SysEmu, 1KB_50_memcpys_10stream_2thread) {
  run_stress_mem(runtime_.get(), 1<<10, 50, 10, 2);
}

TEST_F(SysEmu, 1KB_2_memcpys_2stream_1thread) {
  run_stress_mem(runtime_.get(), 1<<10, 2, 2, 1);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  Fixture::sMode = Fixture::Mode::SYSEMU;
  return RUN_ALL_TESTS();
}
