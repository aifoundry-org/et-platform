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

struct StressMem : Fixture {};

TEST_F(StressMem, 1KB_1_memcpys_1stream_20thread) {
  run_stress_mem(runtime_.get(), 1<<10, 1, 1, 20);
}

TEST_F(StressMem, 1KB_1_memcpys_1stream_30thread) {
  run_stress_mem(runtime_.get(), 1<<10, 1, 1, 30);
}

TEST_F(StressMem, 1KB_1_memcpys_1stream_50thread) {
  run_stress_mem(runtime_.get(), 1<<10, 1, 1, 50);
}

TEST_F(StressMem, 1KB_1_memcpys_1stream_75thread) {
  run_stress_mem(runtime_.get(), 1<<10, 1, 1, 75);
}

TEST_F(StressMem, 1KB_100_memcpys_1stream_1thread) {
  run_stress_mem(runtime_.get(), 1 << 10, 1e2, 1, 1);
}

TEST_F(StressMem, 1KB_100_memcpys_100stream_1thread) {
  run_stress_mem(runtime_.get(), 1<<10, 100, 100, 1);
}

TEST_F(StressMem, 1M_20_memcpys_2stream_1thread) {
  run_stress_mem(runtime_.get(), 1<<20, 20, 2, 1);
}

TEST_F(StressMem, 1M_20_memcpys_2stream_2thread) {
  run_stress_mem(runtime_.get(), 1<<20, 20, 2, 2);
}

TEST_F(StressMem, 1KB_100_memcpys_2stream_2thread) {
  run_stress_mem(runtime_.get(), 1 << 10, 1e2, 2, 2);
}

TEST_F(StressMem, 1KB_1_memcpys_1stream_1thread) {
  run_stress_mem(runtime_.get(), 1<<10, 1, 1, 1);
}

TEST_F(StressMem, 1KB_1_memcpys_1stream_2thread) {
  run_stress_mem(runtime_.get(), 1<<10, 1, 1, 2);
}

TEST_F(StressMem, 1KB_6_memcpys_1stream_1thread) {
  run_stress_mem(runtime_.get(), 1<<10, 6, 1, 1);
}

TEST_F(StressMem, 1KB_6_memcpys_1stream_2thread) {
  run_stress_mem(runtime_.get(), 1<<10, 6, 1, 2);
}

TEST_F(StressMem, 1KB_6_memcpys_2stream_1thread) {
  run_stress_mem(runtime_.get(), 1<<10, 6, 2, 1);
}

TEST_F(StressMem, 1KB_6_memcpys_2stream_2thread) {
  run_stress_mem(runtime_.get(), 1<<10, 6, 2, 2);
}

TEST_F(StressMem, 1KB_50_memcpys_10stream_2thread) {
  run_stress_mem(runtime_.get(), 1<<10, 50, 10, 2);
}

TEST_F(StressMem, 1KB_2_memcpys_2stream_1thread) {
  run_stress_mem(runtime_.get(), 1<<10, 2, 2, 1);
}

TEST_F(StressMem, 1KB_50_memcpys_10stream_2thread_8dev) {
  if (sMode == Mode::PCIE) {
    RT_LOG(INFO) << "This multi device test is not design to be run on PCIE, skipping it.";
    return;
  }
  decltype(sNumDevices) oldNumDevices = sNumDevices;
  try {
    TearDown();
    sNumDevices = 8;
    SetUp();
    std::vector<std::future<void>> futs;
    for (auto i = 0U; i < 8; ++i) {
      futs.emplace_back(
        std::async(std::launch::async, [this, i] { run_stress_mem(runtime_.get(), 1 << 10, 50, 10, 2, true, i); }));
    }
    for (auto& f : futs) {
      f.get();
    }
    sNumDevices = oldNumDevices;
  } catch (const std::exception& e) {
    sNumDevices = oldNumDevices;
    FAIL() << e.what();
  }
}

TEST_F(StressMem, 1KB_1M_memcpies_10th_NOSYSEMU) {
  if (sMode == Mode::SYSEMU) {
    RT_LOG(INFO) << "This test is too slow to be run on sysemu.";
    return;
  }
  auto memcpySize = 1U << 10;
  auto numCopies = static_cast<uint32_t>(1e6);
  auto numThreads = 10U;
  auto numBatches = 100U;
  for (auto i = 1U; i <= numBatches; ++i) {
    RT_LOG(INFO) << "Running batch " << i << "/" << numBatches;
    run_stress_mem(runtime_.get(), memcpySize, numCopies / numThreads / numBatches, 1, numThreads, false);
  }
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  ParseArguments(argc, argv);
  return RUN_ALL_TESTS();
}
