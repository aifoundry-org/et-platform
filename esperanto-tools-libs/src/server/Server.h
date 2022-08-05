/*-------------------------------------------------------------------------
 * Copyright (C) 2022, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/
#pragma once
#include "runtime/IProfiler.h"
#include "runtime/IRuntime.h"
#include <deviceLayer/IDeviceLayer.h>
#include <hostUtils/threadPool/ThreadPool.h>
#include <string_view>
#include <thread>
#include <vector>
namespace rt {
class Worker;
class Server {
public:
  explicit Server(const std::string& socketPath, std::unique_ptr<dev::IDeviceLayer> deviceLayer, Options options);
  ~Server();
  void removeWorker(Worker* worker);
  profiling::IProfilerRecorder* getProfiler() const {
    return runtime_->getProfiler();
  }

private:
  void listen();

  int socket_;
  bool running_ = true;
  std::mutex mutex_;
  std::thread listener_;
  std::unique_ptr<dev::IDeviceLayer> deviceLayer_;
  std::unique_ptr<IRuntime> runtime_;
  std::vector<std::unique_ptr<Worker>> workers_;
  threadPool::ThreadPool tp_{1}; // used to remove workers
};
} // namespace rt