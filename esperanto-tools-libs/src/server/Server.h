/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/
#pragma once
#include "runtime/IProfiler.h"
#include "runtime/IRuntime.h"

#include <device-layer/IDeviceLayer.h>
#include <hostUtils/threadPool/ThreadPool.h>

#include <memory>
#include <string_view>
#include <thread>
#include <vector>

namespace rt {
class Worker;
class ETRT_API Server {
public:
  explicit Server(const std::string& socketPath, std::shared_ptr<dev::IDeviceLayer> const& deviceLayer,
                  Options options);
  ~Server();
  void removeWorker(Worker* worker);
  void setProfiler(std::unique_ptr<profiling::IProfilerRecorder>&& profiler) {
    runtime_->setProfiler(std::move(profiler));
  }
  profiling::IProfilerRecorder* getProfiler() const {
    return runtime_->getProfiler();
  }

  size_t getNumWorkers() const {
    return workers_.size();
  }

private:
  void listen();

  int socket_;
  bool running_ = true;
  std::mutex mutex_;
  std::thread listener_;
  std::shared_ptr<dev::IDeviceLayer> deviceLayer_;
  std::unique_ptr<IRuntime> runtime_;
  std::vector<std::unique_ptr<Worker>> workers_;
  threadPool::ThreadPool tp_{1}; // used to remove workers
};
} // namespace rt
