/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/

#include "runtime/Types.h"
#include "tools/IBenchmarker.h"
#include <chrono>
#include <runtime/IRuntime.h>
#include <thread>

class Worker {
public:
  explicit Worker(size_t bytesH2D, size_t bytesD2H, size_t numH2D, size_t numD2H, rt::DeviceId device,
                  rt::IRuntime& runtime, const std::string& kernelPath);
  void start(int numIterations, bool computeOpStats, bool discardFirst = true);
  rt::IBenchmarker::WorkerResult wait();
  ~Worker();

private:
  using OpStats = rt::IBenchmarker::OpStats;
  void doIteration(bool computeOpStats, uint64_t shireMask, std::vector<rt::MemcpyList>& listH2D,
                   std::vector<rt::MemcpyList>& listD2H, std::vector<OpStats>& opstats);
  rt::IRuntime& runtime_;
  rt::DeviceId device_;
  rt::StreamId stream_;
  std::thread runner_;
  rt::IBenchmarker::WorkerResult result_;
  size_t numH2D_;
  size_t numD2H_;
  std::optional<rt::KernelId> kernel_;

  std::vector<std::byte> hH2D_;
  std::byte* dH2D_ = nullptr;

  std::vector<std::byte> hD2H_;
  std::byte* dD2H_ = nullptr;

  struct Parameters {
    std::byte* src;
    size_t srcSize;
    std::byte* dst;
    size_t dstSize;
  } parameters_;
};