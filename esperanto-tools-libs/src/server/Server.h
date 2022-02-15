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
#include "Worker.h"
#include "device-layer/IDeviceLayer.h"
#include "runtime/IRuntime.h"
#include <string_view>
#include <thread>
#include <vector>
namespace rt {
class Server {
public:
  explicit Server(const std::string& socketPath, std::unique_ptr<dev::IDeviceLayer> deviceLayer);

  void removeWorker(Worker* worker) {
    std::remove_if(begin(workers_), end(workers_), [worker](const auto& item) { return item.get() == worker; });
  }

private:
  void listen();

  int socket_;
  bool running_ = true;
  std::vector<std::unique_ptr<Worker>> workers_;
  std::thread listener_;
  std::unique_ptr<IRuntime> runtime_;
  std::unique_ptr<dev::IDeviceLayer> deviceLayer_;
};
} // namespace rt