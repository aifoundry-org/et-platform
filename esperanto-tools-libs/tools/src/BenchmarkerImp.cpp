/*-------------------------------------------------------------------------
 * Copyright (C) 2021, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/
#include "BenchmarkerImp.h"
#include "Logging.h"
#include "Worker.h"
#include "runtime/IDmaBuffer.h"
#include "runtime/IProfiler.h"
#include "runtime/Types.h"
#include "tools/IBenchmarker.h"
#include <chrono>
#include <fstream>
#include <future>
#include <g3log/loglevels.hpp>
#include <hostUtils/logging/Logging.h>
#include <stdio.h>

using namespace rt;

namespace {
inline std::vector<std::byte> readFile(const std::string& path) {
  auto file = std::ifstream(path, std::ios_base::binary);
  CHECK(file.is_open()) << "Can't open file path: " << path;
  auto iniF = file.tellg();
  file.seekg(0, std::ios::end);
  auto endF = file.tellg();
  auto size = static_cast<uint32_t>(endF - iniF);
  file.seekg(0, std::ios::beg);

  std::vector<std::byte> fileContent(size);
  file.read(reinterpret_cast<char*>(fileContent.data()), size);
  return fileContent;
}
} // namespace

BenchmarkerImp::~BenchmarkerImp() = default;

BenchmarkerImp::BenchmarkerImp(IRuntime* runtime)
  : runtime_(runtime) {
  runtime_->setOnStreamErrorsCallback([](auto eventId, const auto& streamError) {
    BM_LOG(FATAL) << "Got device response error: " << static_cast<int>(eventId)
                  << " error msg: " << streamError.getString();
  });
  auto devices = runtime_->getDevices();
  BM_LOG_IF(FATAL, devices.empty()) << "No devices";
}

IBenchmarker::SummaryResults BenchmarkerImp::run(Options options, DeviceMask mask) {
  BM_LOG_IF(FATAL, options.useDmaBuffers) << "Use dma buffers is not yet supported.";
  BM_LOG_IF(FATAL, !options.kernelPath.empty()) << "Kernel based workloads is not yet supported.";
  auto& tracePath = options.runtimeTracePath;
  std::ofstream traceOutput;
  if (!tracePath.empty()) {
    BM_LOG(INFO) << "Storing runtime trace at path " << tracePath;
    std::ifstream f(tracePath);
    if (f.good()) {
      BM_LOG(WARNING) << "Erasing previous file in path " << tracePath;
      BM_LOG_IF(FATAL, remove(tracePath.c_str()) != 0) << "Couldn't remove previous trace file.";
    }
    traceOutput.open(tracePath);
    auto profiler = runtime_->getProfiler();
    profiler->start(traceOutput, rt::IProfiler::OutputType::Json);
  }
  BM_LOG(INFO) << "Creating workers...";
  std::vector<std::unique_ptr<Worker>> workers;
  for (auto d : runtime_->getDevices()) {
    if (mask.isEnabled(d)) {
      BM_LOG(INFO) << "\t Device " << static_cast<int>(d) << " is enabled. Creating workers.";
      for (int i = 0; i < options.numThreads; ++i) {
        workers.emplace_back(std::make_unique<Worker>(options.bytesH2D, options.bytesD2H, d, *runtime_));
      }
    }
  }
  BM_LOG_IF(FATAL, workers.empty()) << "There are no workers. Check parameters and num devices";

  BM_LOG(INFO) << "Starting the run.";
  auto start = std::chrono::high_resolution_clock::now();
  for (auto& w : workers) {
    w->start(options.numWorkloadsPerThread);
  }
  BM_LOG(INFO) << "Waiting until all workers have ended.";
  SummaryResults summary;
  for (auto& w : workers) {
    summary.workerResults.emplace_back(w->wait());
  }
  auto et = std::chrono::high_resolution_clock::now() - start;
  auto us = std::chrono::duration_cast<std::chrono::microseconds>(et);
  auto secs = us.count() / 1e6f;
  auto totalWl = workers.size() * options.numWorkloadsPerThread;
  summary.bytesReceivedPerSecond = options.bytesD2H * totalWl / secs;
  summary.bytesSentPerSecond = options.bytesH2D * totalWl / secs;
  return summary;
}

std::unique_ptr<IBenchmarker> IBenchmarker::create(IRuntime* runtime) {
  return std::make_unique<BenchmarkerImp>(runtime);
}

IBenchmarker::DeviceMask IBenchmarker::DeviceMask::enableAll() {
  DeviceMask result;
  for (auto i = 0U; i < kMaxDevices; ++i) {
    result.mask_.set(i);
  }
  return result;
}
void IBenchmarker::DeviceMask::enableDevice(DeviceId device) {
  auto idx = static_cast<size_t>(device);
  BM_LOG_IF(FATAL, idx >= mask_.size()) << "Max devices are: " << kMaxDevices;
  mask_.set(idx);
}

void IBenchmarker::DeviceMask::disableDevice(DeviceId device) {
  auto idx = static_cast<size_t>(device);
  BM_LOG_IF(FATAL, idx >= mask_.size()) << "Max devices are: " << kMaxDevices;
  mask_.set(idx, false);
}

bool IBenchmarker::DeviceMask::isEnabled(DeviceId device) {
  auto idx = static_cast<size_t>(device);
  BM_LOG_IF(FATAL, idx >= mask_.size()) << "Max devices are: " << kMaxDevices;
  return mask_.test(idx);
}