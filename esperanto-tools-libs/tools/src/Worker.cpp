/*-------------------------------------------------------------------------
 * Copyright (C) 2022, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/
#include "Worker.h"
#include "Logging.h"
#include "runtime/Types.h"
#include "tools/IBenchmarker.h"
#include <chrono>
#include <fstream>
#include <hostUtils/logging/Logging.h>
using namespace rt;

Worker::Worker(size_t bytesH2D, size_t bytesD2H, size_t numH2D, size_t numD2H, DeviceId device, IRuntime& runtime,
               const std::string& kernelPath)
  : runtime_(runtime)
  , device_(device)
  , numH2D_(numH2D)
  , numD2H_(numD2H) {
  auto devices = runtime.getDevices();
  BM_LOG_IF(FATAL, std::find(begin(devices), end(devices), device_) == end(devices)) << "Invalid DeviceId";
  BM_LOG_IF(FATAL, bytesH2D == 0 && bytesD2H == 0) << "H2D and D2H can't be both zero";
  if (numH2D_ > 1) {
    bytesH2D = bytesH2D + numH2D_ - 1;
  }
  if (numD2H_ > 1) {
    bytesD2H = bytesD2H + numD2H_ - 1;
  }
  if (bytesH2D > 0) {
    hH2D_.resize(bytesH2D);
    dH2D_ = runtime_.mallocDevice(device_, bytesH2D);
  }
  if (bytesD2H > 0) {
    hD2H_.resize(bytesD2H);
    dD2H_ = runtime_.mallocDevice(device_, bytesD2H);
  }
  stream_ = runtime_.createStream(device_);
  result_.device = device_;

  // load code if any
  if (!kernelPath.empty()) {
    std::ifstream file(kernelPath);
    if (file.is_open()) {
      throw Exception("Couldn't open kernel file.");
    }
    auto iniF = file.tellg();
    file.seekg(0, std::ios::end);
    auto endF = file.tellg();
    auto size = endF - iniF;
    file.seekg(0, std::ios::beg);

    std::vector<std::byte> fileContent(static_cast<uint32_t>(size));
    file.read(reinterpret_cast<char*>(fileContent.data()), size);
    auto res = runtime_.loadCode(stream_, fileContent.data(), fileContent.size());
    kernel_ = res.kernel_;
    runtime_.waitForEvent(res.event_);
    if (!runtime_.retrieveStreamErrors(stream_).empty()) {
      throw Exception("There were some errors in runtime");
    }
    parameters_.src = dH2D_;
    parameters_.srcSize = hH2D_.size();
    parameters_.dst = dD2H_;
    parameters_.dstSize = hD2H_.size();
  }
}

void Worker::doIteration(bool computeOpStats, uint64_t shireMask, std::vector<rt::MemcpyList>& listH2D,
                         std::vector<rt::MemcpyList>& listD2H, std::vector<OpStats>& opstats) {
  if (dH2D_) {
    if (numH2D_ > 1) {
      for (auto& op : listH2D) {
        auto evt = runtime_.memcpyHostToDevice(stream_, op);
        if (computeOpStats) {
          opstats.emplace_back(OpStats{evt});
        }
      }
    } else {
      auto evt = runtime_.memcpyHostToDevice(stream_, hH2D_.data(), dH2D_, hH2D_.size());
      if (computeOpStats) {
        opstats.emplace_back(OpStats{evt});
      }
    }
  }
  if (kernel_) {
    auto evt = runtime_.kernelLaunch(stream_, kernel_.value(), reinterpret_cast<std::byte*>(&parameters_),
                                     sizeof(parameters_), shireMask);
    if (computeOpStats) {
      opstats.emplace_back(OpStats{evt});
    }
  }
  if (dD2H_) {
    if (numD2H_ > 1) {
      for (auto& op : listD2H) {
        auto evt = runtime_.memcpyDeviceToHost(stream_, op);
        if (computeOpStats) {
          opstats.emplace_back(OpStats{evt});
        }
      }
    } else {
      auto evt = runtime_.memcpyDeviceToHost(stream_, dD2H_, hD2H_.data(), hD2H_.size());
      if (computeOpStats) {
        opstats.emplace_back(OpStats{evt});
      }
    }
  }
}

void Worker::start(int numIterations, bool computeOpStats, bool discardFirst) {
  runner_ = std::thread([this, numIterations, computeOpStats, discardFirst] {
    using OpStats = rt::IBenchmarker::OpStats;
    auto shireMask = runtime_.getDeviceProperties(device_).computeMinionShireMask_;
    std::vector<OpStats> opstats;

    std::vector<rt::MemcpyList> listH2D;
    std::vector<rt::MemcpyList> listD2H;

    if (discardFirst) {
      doIteration(computeOpStats, shireMask, listH2D, listD2H, opstats);
    }
    opstats.clear();

    auto start = std::chrono::high_resolution_clock::now();

    for (auto i = 0U; i < numH2D_; ++i) {
      auto size = hH2D_.size() / numH2D_;
      if (i % 4 == 0) {
        listH2D.emplace_back(rt::MemcpyList{});
      }
      listH2D.back().addOp(hH2D_.data() + i * size, dH2D_ + i * size, size);
    }
    for (auto i = 0U; i < numD2H_; ++i) {
      auto size = hD2H_.size() / numD2H_;
      if (i % 4 == 0) {
        listD2H.emplace_back(rt::MemcpyList{});
      }
      listD2H.back().addOp(dD2H_ + i * size, hD2H_.data() + i * size, size);
    }
    for (int i = 0; i < numIterations; ++i) {
      BM_VLOG(MID) << "Doing interation: " << i;
      doIteration(computeOpStats, shireMask, listH2D, listD2H, opstats);
    }
    if (computeOpStats) {
      for (auto& e : opstats) {
        runtime_.waitForEvent(e.evt_);
        e.setEnd();
      }
    } else {
      BM_VLOG(MID) << "End submitting work, wait to finish";
      runtime_.waitForStream(stream_);
    }
    auto et = std::chrono::high_resolution_clock::now() - start;
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(et);
    auto secs = us.count() / 1e6f;
    result_.bytesReceivedPerSecond = hD2H_.size() * numIterations / secs;
    result_.bytesSentPerSecond = hH2D_.size() * numIterations / secs;
    result_.workloadsPerSecond = numIterations / secs;
    result_.opStats_ = std::move(opstats);
  });
}

rt::IBenchmarker::WorkerResult Worker::wait() {
  runner_.join();
  return result_;
}

Worker::~Worker() {
  if (dD2H_) {
    runtime_.freeDevice(device_, dD2H_);
  }
  if (dH2D_) {
    runtime_.freeDevice(device_, dH2D_);
  }
  runtime_.destroyStream(stream_);
}