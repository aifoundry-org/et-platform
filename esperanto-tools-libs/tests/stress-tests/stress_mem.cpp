//******************************************************************************
// Copyright (c) 2025 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------
#include "RuntimeFixture.h"
#include <algorithm>
#include <thread>

using namespace testing;

struct StressMem : RuntimeFixture {};

TEST_F(StressMem, 1KB_1_memcpys_1stream_20thread) {
  run_stress_mem(runtime_.get(), 1 << 10, 1, 1, 20);
}

TEST_F(StressMem, 1KB_1_memcpys_1stream_30thread) {
  run_stress_mem(runtime_.get(), 1 << 10, 1, 1, 30);
}

TEST_F(StressMem, 1KB_1_memcpys_1stream_50thread) {
  run_stress_mem(runtime_.get(), 1 << 10, 1, 1, 50);
}

TEST_F(StressMem, 1KB_1_memcpys_1stream_75thread) {
  if (sDlType == DeviceLayerImp::SYSEMU) {
    RT_LOG(INFO) << "This test is too slow to be run on sysemu.";
    return;
  }
  run_stress_mem(runtime_.get(), 1 << 10, 1, 1, 75);
}

TEST_F(StressMem, 1KB_100_memcpys_1stream_1thread) {
  run_stress_mem(runtime_.get(), 1 << 10, 1e2, 1, 1);
}

TEST_F(StressMem, 1KB_100_memcpys_100stream_1thread) {
  run_stress_mem(runtime_.get(), 1 << 10, 100, 100, 1);
}

TEST_F(StressMem, 1M_20_memcpys_2stream_1thread) {
  run_stress_mem(runtime_.get(), 1 << 20, 20, 2, 1);
}

TEST_F(StressMem, 1M_20_memcpys_2stream_2thread) {
  run_stress_mem(runtime_.get(), 1 << 20, 20, 2, 2);
}

TEST_F(StressMem, 1KB_100_memcpys_2stream_2thread) {
  run_stress_mem(runtime_.get(), 1 << 10, 1e2, 2, 2);
}

TEST_F(StressMem, 1KB_1_memcpys_1stream_1thread) {
  run_stress_mem(runtime_.get(), 1 << 10, 1, 1, 1);
}

TEST_F(StressMem, 1KB_1_memcpys_1stream_2thread) {
  run_stress_mem(runtime_.get(), 1 << 10, 1, 1, 2);
}

TEST_F(StressMem, 1KB_6_memcpys_1stream_1thread) {
  run_stress_mem(runtime_.get(), 1 << 10, 6, 1, 1);
}

TEST_F(StressMem, 1KB_6_memcpys_1stream_2thread) {
  run_stress_mem(runtime_.get(), 1 << 10, 6, 1, 2);
}

TEST_F(StressMem, 1KB_6_memcpys_2stream_1thread) {
  run_stress_mem(runtime_.get(), 1 << 10, 6, 2, 1);
}

TEST_F(StressMem, 1KB_6_memcpys_2stream_2thread) {
  run_stress_mem(runtime_.get(), 1 << 10, 6, 2, 2);
}

TEST_F(StressMem, 1KB_50_memcpys_10stream_2thread) {
  run_stress_mem(runtime_.get(), 1 << 10, 50, 10, 2);
}

TEST_F(StressMem, 1KB_2_memcpys_2stream_1thread) {
  run_stress_mem(runtime_.get(), 1 << 10, 2, 2, 1);
}

TEST_F(StressMem, 1KB_10_memcpys_1stream_10thread_1ratio) {
  run_stress_mem(runtime_.get(), 1 << 10, 10, 1, 10, true, 0, 1.0f);
}

TEST_F(StressMem, 16B_10_memcpys_1stream_10thread_1ratio) {
  run_stress_mem(runtime_.get(), 1 << 4, 10, 1, 10, true, 0, 1.0f);
}

TEST_F(StressMem, 1MB_1000_memcpys_1stream_10thread_05ratio) {
  if (sDlType == DeviceLayerImp::SYSEMU) {
    RT_LOG(INFO) << "This test is too slow to be run on sysemu.";
    return;
  }
  run_stress_mem(runtime_.get(), 1 << 20, 1000, 1, 10, false, 0, 0.5f);
}

TEST_F(StressMem, 256MB_3_memcpys_1stream_10thread_05ratio) {
  if (sDlType == DeviceLayerImp::SYSEMU) {
    RT_LOG(INFO) << "This test is too slow to be run on sysemu.";
    return;
  }
  run_stress_mem(runtime_.get(), 256 << 20, 3, 1, 10, false, 0, 0.5f);
}

TEST_F(StressMem, 128MB_6_memcpys_4stream_10thread_075ratio_10loops) {
  if (sDlType == DeviceLayerImp::SYSEMU) {
    RT_LOG(INFO) << "This test is too slow to be run on sysemu.";
    return;
  }
  for (int i = 0; i < 10; ++i) {
    run_stress_mem(runtime_.get(), 256 << 20, 3, 1, 10, false, 0, 0.75f);
  }
}

TEST_F(StressMem, 1KB_50_memcpys_10stream_2thread_8dev) {
  if (sDlType != DeviceLayerImp::SYSEMU) {
    RT_LOG(INFO) << "This multi device test is designed to be run on SYSEMU, skipping it.";
    return;
  }
  try {
    TearDown();
    numDevices_ = 8;
    SetUp();
    std::vector<std::future<void>> futs;
    for (auto i = 0U; i < 8; ++i) {
      futs.emplace_back(
        std::async(std::launch::async, [this, i] { run_stress_mem(runtime_.get(), 1 << 10, 50, 10, 2, true, i); }));
    }
    for (auto& f : futs) {
      f.get();
    }
  } catch (const std::exception& e) {
    FAIL() << e.what();
  }
}

TEST_F(StressMem, 1KB_10K_memcpies_10th_NOSYSEMU) {
  if (sDlType == DeviceLayerImp::SYSEMU) {
    RT_LOG(INFO) << "This test is too slow to be run on sysemu.";
    return;
  }
  auto memcpySize = 1U << 10;
  auto numCopies = static_cast<uint32_t>(1e4);
  auto numThreads = 10U;
  auto numBatches = 100U;
  for (auto i = 1U; i <= numBatches; ++i) {
    RT_LOG(INFO) << "Running batch " << i << "/" << numBatches;
    run_stress_mem(runtime_.get(), memcpySize, numCopies / numThreads / numBatches, 1, numThreads, false);
  }
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  RuntimeFixture::ParseArguments(argc, argv);
  return RUN_ALL_TESTS();
}
