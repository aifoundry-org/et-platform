//******************************************************************************
// Copyright (C) 2021, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------
#pragma once
#include "RuntimeImp.h"
#include "runtime/Types.h"
#include <cfloat>
#include <common/Constants.h>
#include <experimental/filesystem>
#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <hostUtils/logging/Logger.h>
#include <hostUtils/threadPool/ThreadPool.h>
#include <optional>
#include <random>
#include <runtime/DeviceLayerFake.h>
#include <runtime/IRuntime.h>
#include <sw-sysemu/SysEmuOptions.h>

namespace fs = std::experimental::filesystem;

inline auto getSysemuDefaultOptions() {
  constexpr uint64_t kSysEmuMaxCycles = std::numeric_limits<uint64_t>::max();
  constexpr uint64_t kSysEmuMinionShiresMask = 0x1FFFFFFFFu;

  emu::SysEmuOptions sysEmuOptions;
  if (fs::exists(BOOTROM_TRAMPOLINE_TO_BL2_ELF)) {
    sysEmuOptions.bootromTrampolineToBL2ElfPath = BOOTROM_TRAMPOLINE_TO_BL2_ELF;
  }
  if (fs::exists(BL2_ELF)) {
    sysEmuOptions.spBL2ElfPath = BL2_ELF;
  }
  if (fs::exists(MACHINE_MINION_ELF)) {
    sysEmuOptions.machineMinionElfPath = MACHINE_MINION_ELF;
  }
  if (fs::exists(MASTER_MINION_ELF)) {
    sysEmuOptions.masterMinionElfPath = MASTER_MINION_ELF;
  }
  if (fs::exists(WORKER_MINION_ELF)) {
    sysEmuOptions.workerMinionElfPath = WORKER_MINION_ELF;
  }
  sysEmuOptions.executablePath = std::string(SYSEMU_INSTALL_DIR) + "sys_emu";
  sysEmuOptions.runDir = std::experimental::filesystem::current_path();
  sysEmuOptions.maxCycles = kSysEmuMaxCycles;
  sysEmuOptions.minionShiresMask = kSysEmuMinionShiresMask;
  sysEmuOptions.puUart0Path = sysEmuOptions.runDir + "/pu_uart0_tx.log";
  sysEmuOptions.puUart1Path = sysEmuOptions.runDir + "/pu_uart1_tx.log";
  sysEmuOptions.spUart0Path = sysEmuOptions.runDir + "/spio_uart0_tx.log";
  sysEmuOptions.spUart1Path = sysEmuOptions.runDir + "/spio_uart1_tx.log";
  sysEmuOptions.startGdb = false;
  return sysEmuOptions;
}

inline std::vector<std::byte> readFile(const std::string& path) {
  auto file = std::ifstream(path, std::ios_base::binary);
  EXPECT_TRUE(file.is_open());
  if (!file.is_open()) {
    return {};
  }
  auto iniF = file.tellg();
  file.seekg(0, std::ios::end);
  auto endF = file.tellg();
  auto size = endF - iniF;
  file.seekg(0, std::ios::beg);

  std::vector<std::byte> fileContent(static_cast<uint32_t>(size));
  file.read(reinterpret_cast<char*>(fileContent.data()), size);
  return fileContent;
}

template <typename TContainer> void randomize(TContainer& container, int init, int end) {
  static std::mt19937 gen(std::random_device{}());
  static std::uniform_int_distribution dis(init, end);
  for (auto& v : container) {
    v = static_cast<typename TContainer::value_type>(dis(gen));
  }
}
inline float getRand() {
  static std::default_random_engine e;
  static std::uniform_real_distribution<float> dis(0, 1.0f); // rage 0 - 1
  return dis(e);
}
inline rt::MemcpyList chunkTransfer(std::byte* src, std::byte* dst, size_t size, size_t chunks = 4) {
  rt::MemcpyList result;
  for (auto i = 0UL, chunkSize = size / chunks; i < chunks; ++i) {
    result.addOp(src + i * chunkSize, dst + i * chunkSize, chunkSize);
  }
  result.operations_.back().size_ += size % chunks;
  return result;
}

inline void stressMemThreadFunc(rt::IRuntime* runtime, uint32_t streams, uint32_t transactions, size_t bytes,
                                bool check_results, size_t deviceId, float memcpyListRatio = 0.0f) {
  ASSERT_LT(deviceId, runtime->getDevices().size());
  auto dev = runtime->getDevices()[deviceId];
  std::vector<rt::StreamId> streams_(streams);
  std::vector<std::vector<std::byte>> host_src(transactions);
  std::vector<std::vector<std::byte>> host_dst(transactions);
  std::vector<std::byte*> dev_mem(transactions);
  for (auto j = 0U; j < streams; ++j) {
    streams_[j] = runtime->createStream(dev);
    for (auto k = 0U; k < transactions / streams; ++k) {
      auto idx = k + j * transactions / streams;
      host_src[idx] = std::vector<std::byte>(bytes);
      host_dst[idx] = std::vector<std::byte>(bytes);
      if (check_results) { // no point in randomizing if we don't want to check results
        randomize(host_src[idx], 0, 256);
      }
      dev_mem[idx] = runtime->mallocDevice(dev, bytes);
      if (getRand() < memcpyListRatio) {
        auto list = chunkTransfer(host_src[idx].data(), dev_mem[idx], bytes);
        runtime->memcpyHostToDevice(streams_[j], list);
        list = chunkTransfer(dev_mem[idx], host_dst[idx].data(), bytes);
        runtime->memcpyDeviceToHost(streams_[j], list);
      } else {
        runtime->memcpyHostToDevice(streams_[j], host_src[idx].data(), dev_mem[idx], bytes);
        runtime->memcpyDeviceToHost(streams_[j], dev_mem[idx], host_dst[idx].data(), bytes);
      }
    }
  }
  for (auto j = 0U; j < streams; ++j) {
    runtime->waitForStream(streams_[j]);
    for (auto k = 0U; k < transactions / streams; ++k) {
      auto idx = k + j * transactions / streams;
      runtime->freeDevice(dev, dev_mem[idx]);
      if (check_results) {
        ASSERT_THAT(host_dst[idx], testing::ElementsAreArray(host_src[idx]));
      }
    }
    runtime->destroyStream(streams_[j]);
  }
}

inline void run_stress_mem(rt::IRuntime* runtime, size_t bytes, uint32_t transactions, uint32_t streams,
                           uint32_t threads, bool check_results = true, size_t deviceId = 0,
                           float memcpyListRatio = 0.0f) {
  std::vector<std::thread> threads_;
  using namespace testing;
  for (auto i = 0U; i < threads; ++i) {
    threads_.emplace_back(
      std::bind(stressMemThreadFunc, runtime, streams, transactions, bytes, check_results, deviceId, memcpyListRatio));
  }
  for (auto& t : threads_) {
    t.join();
  }
}
