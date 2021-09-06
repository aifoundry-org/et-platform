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
#include "runtime/IDmaBuffer.h"
#include "runtime/Types.h"
#include <chrono>
#include <fstream>
#include <future>
#include <hostUtils/logging/Logging.h>
using namespace rt;

namespace {
inline std::vector<std::byte> readFile(const std::string& path) {
  auto file = std::ifstream(path, std::ios_base::binary);
  CHECK(file.is_open()) << "Can't open file path: " << path;
  auto iniF = file.tellg();
  file.seekg(0, std::ios::end);
  auto endF = file.tellg();
  auto size = endF - iniF;
  file.seekg(0, std::ios::beg);

  std::vector<std::byte> fileContent(size);
  file.read(reinterpret_cast<char*>(fileContent.data()), size);
  return fileContent;
}
} // namespace

BenchmarkerImp::~BenchmarkerImp() {
  runtime_->unloadCode(jumpLoop_);
  if (dH2DBuffer_) {
    runtime_->freeDevice(device_, dH2DBuffer_);
    dH2DBuffer_ = nullptr;
  }
  if (dD2HBuffer_) {
    runtime_->freeDevice(device_, dD2HBuffer_);
    dD2HBuffer_ = nullptr;
  }
}

BenchmarkerImp::BenchmarkerImp(dev::IDeviceLayer* deviceLayer, const std::string& kernelsDirPath)
  : deviceLayer_(std::move(deviceLayer)) {
  auto jumpLoopLoc = kernelsDirPath + "/jump_loop.elf";
  runtime_ = IRuntime::create(deviceLayer_);
  auto devices = runtime_->getDevices();
  CHECK(!devices.empty()) << "No devices";
  device_ = devices.front();
  auto kernelContent = readFile(jumpLoopLoc);
  jumpLoop_ = runtime_->loadCode(device_, kernelContent.data(), kernelContent.size());
}

BenchmarkerImp::Results BenchmarkerImp::run(Options options) {
  // allocate buffers
  std::vector<std::byte> hH2DBuffer, hD2HBuffer;
  std::unique_ptr<IDmaBuffer> pDmaBufferH2D;
  std::unique_ptr<IDmaBuffer> pDmaBufferD2H;
  auto dmaBufferD2H = runtime_->allocateDmaBuffer(device_, options.numBytesPerTransferD2H, true);
  std::byte* hH2D = nullptr;
  std::byte* hD2H = nullptr;
  if (options.numBytesPerTransferH2D > 0) {
    dH2DBuffer_ = runtime_->mallocDevice(device_, options.numBytesPerTransferH2D);
    if (options.useDmaBuffers) {
      pDmaBufferH2D = runtime_->allocateDmaBuffer(device_, options.numBytesPerTransferH2D, true);
      hH2D = pDmaBufferH2D->getPtr();
    } else {
      hH2DBuffer.resize(options.numBytesPerTransferH2D);
      hH2D = hH2DBuffer.data();
    }
  }
  if (options.numBytesPerTransferD2H > 0) {
    dD2HBuffer_ = runtime_->mallocDevice(device_, options.numBytesPerTransferD2H);
    if (options.useDmaBuffers) {
      pDmaBufferD2H = runtime_->allocateDmaBuffer(device_, options.numBytesPerTransferD2H, true);
      hD2H = pDmaBufferD2H->getPtr();
    } else {
      hD2HBuffer.resize(options.numBytesPerTransferD2H);
      hD2H = hD2HBuffer.data();
    }
  }
  ET_LOG(BENCHMARK, INFO) << "Launching threads...";
  std::vector<std::future<rt::StreamId>> futures;
  auto wlPerThread = std::max(1UL, options.numWorkloads / options.numThreads);
  ET_LOG_IF(BENCHMARK, WARNING, options.numWorkloads % options.numThreads != 0)
    << "numWorkloads should be multiple of numThreads. Real executed workloads will be "
    << wlPerThread * options.numThreads;

  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < options.numThreads; ++i) {
    futures.emplace_back(std::async(std::launch::async, [options, hH2D, hD2H, wlPerThread, this] {
      auto stream = runtime_->createStream(device_);
      for (int i = 0; i < wlPerThread; ++i) {
        if (dH2DBuffer_) {
          runtime_->memcpyHostToDevice(stream, hH2D, dH2DBuffer_, options.numBytesPerTransferH2D);
        }
        if (options.type == IBenchmarker::WorkloadType::H2D_K_D2H || options.type == IBenchmarker::WorkloadType::K) {
          // num cycles per iter should be approx ~11 (10 for the jump and 1 for the addi)
          uint64_t num_iters = options.numCyclesPerKernel / 11;
          runtime_->kernelLaunch(stream, jumpLoop_, reinterpret_cast<std::byte*>(&num_iters), sizeof num_iters, 0x3);
        }
        if (dD2HBuffer_) {
          runtime_->memcpyDeviceToHost(stream, dD2HBuffer_, hD2H, options.numBytesPerTransferD2H);
        }
      }
      return stream;
    }));
  }
  for (auto& f : futures) {
    f.wait();
  }
  auto et = std::chrono::high_resolution_clock::now() - start;
  auto us = std::chrono::duration_cast<std::chrono::microseconds>(et);
  auto secs = us.count() / 1e6f;
  Results r;
  int cmdPerWl;
  if (options.type == IBenchmarker::WorkloadType::H2D_D2H) {
    cmdPerWl = 2;
  } else if (options.type == IBenchmarker::WorkloadType::H2D_K_D2H) {
    cmdPerWl = 3;
  } else {
    cmdPerWl = 1;
  }
  r.commandsSentPerSecond = cmdPerWl * wlPerThread * options.numThreads / secs;

  ET_LOG(BENCHMARK, INFO) << "Waiting for streams...";
  for (auto& f : futures) {
    auto stream = f.get();
    runtime_->waitForStream(stream);
    runtime_->destroyStream(stream);
  }
  ET_LOG(BENCHMARK, INFO) << "Work finished.";
  et = std::chrono::high_resolution_clock::now() - start;
  us = std::chrono::duration_cast<std::chrono::microseconds>(et);
  secs = us.count() / 1e6f;
  r.bytesReceivedPerSecond = options.numBytesPerTransferD2H * wlPerThread * options.numThreads / secs;
  r.bytesSentPerSecond = options.numBytesPerTransferH2D * wlPerThread * options.numThreads / secs;
  return r;
}

std::unique_ptr<IBenchmarker> IBenchmarker::create(dev::IDeviceLayer* deviceLayer, const std::string& kernelsDir) {
  return std::make_unique<BenchmarkerImp>(deviceLayer, kernelsDir);
}