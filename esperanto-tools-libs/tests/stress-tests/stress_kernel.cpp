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
#include <algorithm>
#include <easy/profiler.h>
#include <future>
#include <g3log/loglevels.hpp>
#include <gtest/gtest.h>
#include <limits>
#include <thread>

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wkeyword-macro"
#endif
#define private public
#pragma GCC diagnostic pop
#include "RuntimeImp.h"
#undef private

bool areEqual(int one, int other, int index, int thread, int stream) {
  (void)index;
  (void)thread;
  (void)stream;
  return one == other;
}
class StressKernel : public RuntimeFixture {
public:
  void SetUp() override {
    RuntimeFixture::SetUp();
    kernels_.clear();
    for (auto i = 0U; i < devices_.size(); ++i) {
      kernels_.emplace_back(loadKernel("add_vector.elf", i));
    }
  }
  void stressKernelThreadFunc(rt::DeviceId dev, uint32_t num_streams, uint32_t num_executions, uint32_t elems,
                              bool check_results, int thread_num) const {
    std::vector<rt::StreamId> streams_(num_streams);
    std::vector<std::vector<int>> host_src1(num_executions);
    std::vector<std::vector<int>> host_src2(num_executions);
    std::vector<std::vector<int>> host_dst(num_executions);
    std::vector<std::byte*> dev_mem_src1(num_executions);
    std::vector<std::byte*> dev_mem_src2(num_executions);
    std::vector<std::byte*> dev_mem_dst(num_executions);
    for (auto j = 0U; j < num_streams; ++j) {
      streams_[j] = runtime_->createStream(dev);
      for (auto k = 0U; k < num_executions / num_streams; ++k) {
        auto idx = k + j * num_executions / num_streams;
        host_src1[idx] = std::vector<int>(elems);
        host_src2[idx] = std::vector<int>(elems);
        host_dst[idx] = std::vector<int>(elems);
        // put random junk
        randomize(host_src1[idx], std::numeric_limits<int>::lowest(), std::numeric_limits<int>::max());
        randomize(host_src2[idx], std::numeric_limits<int>::lowest(), std::numeric_limits<int>::max());
        dev_mem_src1[idx] = runtime_->mallocDevice(dev, elems * sizeof(int));
        dev_mem_src2[idx] = runtime_->mallocDevice(dev, elems * sizeof(int));
        dev_mem_dst[idx] = runtime_->mallocDevice(dev, elems * sizeof(int));
        runtime_->memcpyHostToDevice(streams_[j], reinterpret_cast<std::byte*>(host_src1[idx].data()),
                                     dev_mem_src1[idx], elems * sizeof(int));
        runtime_->memcpyHostToDevice(streams_[j], reinterpret_cast<std::byte*>(host_src2[idx].data()),
                                     dev_mem_src2[idx], elems * sizeof(int));
        struct {
          void* src1;
          void* src2;
          void* dst;
          int elements;
        } params{dev_mem_src1[idx], dev_mem_src2[idx], dev_mem_dst[idx], static_cast<int>(elems)};
        runtime_->kernelLaunch(streams_[j], kernels_[static_cast<uint32_t>(dev)], reinterpret_cast<std::byte*>(&params),
                               sizeof(params), 0x1);
        runtime_->memcpyDeviceToHost(streams_[j], dev_mem_dst[idx], reinterpret_cast<std::byte*>(host_dst[idx].data()),
                                     elems * sizeof(int));
      }
    }
    for (auto j = 0U; j < num_streams; ++j) {
      runtime_->waitForStream(streams_[j]);
      for (auto k = 0U; k < num_executions / num_streams; ++k) {
        auto idx = k + j * num_executions / num_streams;
        ASSERT_LT(idx, num_executions);
        runtime_->freeDevice(dev, dev_mem_src1[idx]);
        runtime_->freeDevice(dev, dev_mem_src2[idx]);
        runtime_->freeDevice(dev, dev_mem_dst[idx]);
        for (auto l = 0U; check_results && l < elems; ++l) {
          ASSERT_PRED5(areEqual, host_dst[idx][l], host_src1[idx][l] + host_src2[idx][l], l, thread_num, j);
        }
      }
      runtime_->destroyStream(streams_[j]);
    };
  }
  void run_stress_kernel(size_t elems, uint32_t num_executions, uint32_t num_streams, uint32_t num_threads,
                         bool check_results = true, size_t deviceId = 0) const {
    std::vector<std::thread> threads;
    ASSERT_TRUE(deviceId < devices_.size());
    auto dev = devices_[deviceId];

    for (auto i = 0U; i < num_threads; ++i) {
      threads.emplace_back(std::bind(&StressKernel::stressKernelThreadFunc, this, dev, num_streams, num_executions,
                                     elems, check_results, i));
    }
    for (auto& t : threads) {
      t.join();
    }
  }
  std::vector<rt::KernelId> kernels_;
};

TEST_F(StressKernel, 256_ele_10_exe_10_st_2_th) {
  run_stress_kernel(1 << 8, 10, 10, 2);
}

TEST_F(StressKernel, 1024_ele_1_exe_1_st_1_th) {
  run_stress_kernel(1 << 10, 1, 1, 1);
}

TEST_F(StressKernel, 1024_ele_1_exe_1_st_2_th) {
  run_stress_kernel(1 << 10, 1, 1, 2);
}

TEST_F(StressKernel, 256_ele_10_exe_1_st_1_th) {
  run_stress_kernel(1 << 8, 10, 1, 1);
}

TEST_F(StressKernel, 1024_ele_100_exe_1_st_2_th) {
  if (sDlType == DeviceLayerImp::SYSEMU) {
    RT_LOG(INFO) << "This test is too slow to be run on sysemu.";
    return;
  }
  run_stress_kernel(1 << 10, 100, 1, 2);
}

TEST_F(StressKernel, 128_ele_1_exe_1_st_2_th) {
  run_stress_kernel(1 << 7, 1, 1, 2);
}

TEST_F(StressKernel, 64_ele_1_exe_1_st_20_th) {
  run_stress_kernel(1 << 6, 1, 1, 20);
}

TEST_F(StressKernel, 64_ele_1_exe_1_st_15_th) {
  run_stress_kernel(1 << 6, 1, 1, 15);
}

TEST_F(StressKernel, 64_ele_1_exe_1_st_25_th) {
  run_stress_kernel(1 << 6, 1, 1, 25);
}

TEST_F(StressKernel, 64_ele_1_exe_1_st_50_th) {
  run_stress_kernel(1 << 6, 1, 1, 50);
}

TEST_F(StressKernel, 64_ele_1_exe_1_st_100_th) {
  run_stress_kernel(1 << 6, 1, 1, 100);
}

TEST_F(StressKernel, 256_ele_10_exe_10_st_2_th_8dev) {
  if (sDlType == DeviceLayerImp::PCIE) {
    RT_LOG(INFO) << "This multi device test is not design to be run on PCIE, skipping it.";
    return;
  }
  try {
    TearDown();
    numDevices_ = 8;
    SetUp();
    std::vector<std::future<void>> futs;
    for (auto i = 0U; i < 8; ++i) {
      futs.emplace_back(std::async(std::launch::async, [this, i] { run_stress_kernel(1 << 8, 10, 10, 2, true, i); }));
    }
    for (auto& f : futs) {
      f.get();
    }
  } catch (const std::exception& e) {
    FAIL() << e.what();
  }
}

TEST_F(StressKernel, 256_ele_10_exe_2_st_4_th_n_devices) {
  std::vector<std::future<void>> futs;
  auto ndevs = devices_.size();
  RT_LOG(INFO) << "Running test on " << ndevs << " devices.";
  for (auto dev = 0U; dev < static_cast<uint32_t>(ndevs); ++dev) {
    futs.emplace_back(std::async(std::launch::async, [this, dev] { run_stress_kernel(1 << 8, 10, 2, 4, true, dev); }));
  }
  for (auto& f : futs) {
    f.get();
  }
}

TEST_F(StressKernel, 128_ele_1K_exe_1st_10_th_nocheck_NOSYSEMU) {
  if (sDlType == DeviceLayerImp::SYSEMU) {
    RT_LOG(INFO) << "This test is too slow to be run on SYSEMU, skipping it.";
    return;
  }
  auto numElements = 128U;
  auto numIters = 100U;
  auto numExecs = static_cast<uint32_t>(1e3) / numIters;
  for (auto i = 1U; i <= numIters; ++i) {
    RT_LOG(INFO) << "Running batch " << i << "/" << numExecs;
    run_stress_kernel(numElements, numExecs, 1, 10, false);
  }
}

#if 0
disabled test, this is used only for debugging locally, not for CI
TEST_F(StressKernel, LatencyTestSysEmu64_ele_10_exe_1_st_1_th_8devs) {
  // force to be sysemu
  RuntimeFixture::sDlType = DeviceLayerImp::SYSEMU;
  RuntimeFixture::sNumDevices = 8;
  // run first SP
  RuntimeFixture::sRtType = RtType::SP;
  std::vector<std::thread> threads;
  TearDown();
  SetUp();
  EASY_BLOCK("SP")
  for (auto i = 0U; i < sNumDevices; ++i) {
    threads.emplace_back([this, i] { run_stress_kernel(1 << 6, 10, 1, 10, false, i); });
  }
  for (auto& t : threads) {
    t.join();
  }
  EASY_END_BLOCK
  threads.clear();
  // now run MP
  RuntimeFixture::sRtType = RtType::MP;
  TearDown();
  SetUp();
  EASY_BLOCK("MP")
  for (auto i = 0U; i < sNumDevices; ++i) {
    threads.emplace_back([this, i] { run_stress_kernel(1 << 6, 10, 1, 10, false, i); });
  }
  for (auto& t : threads) {
    t.join();
  }
  EASY_END_BLOCK
  threads.clear();
  profiler::dumpBlocksToFile("LatencyTestSysEmu64_ele_10_exe_1_st_1_th_8devs.prof");
}
#endif
int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  RuntimeFixture::ParseArguments(argc, argv);
  g3::log_levels::disable(DEBUG);
  return RUN_ALL_TESTS();
}
