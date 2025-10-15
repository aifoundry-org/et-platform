//******************************************************************************
// Copyright (c) 2025 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------
#pragma once
#include "runtime/IRuntime.h"
#include "server/Client.h"
#include "server/Server.h"
#include <hostUtils/logging/Logger.h>

// this class is not thread-safe
class MpOrchestrator {
public:
  MpOrchestrator() = default;
  ~MpOrchestrator();
  using DeviceLayerCreatorFunc = std::function<std::unique_ptr<dev::IDeviceLayer>()>;

  void createServer(const DeviceLayerCreatorFunc& deviceLayerCreator, rt::Options options);
  void createClient(const std::function<void(rt::IRuntime* runtime)>&);
  const std::string& getSocketPath() const { // to be able to create local clients.
    return socketPath_;
  }
  void clearClients();

private:
  MpOrchestrator(const MpOrchestrator&) = delete;
  MpOrchestrator& operator=(const MpOrchestrator&) = delete;
  bool useExternalServer_ = false;
  int efdToServer_;
  int efdFromServer_;
  std::string socketPath_;
  pid_t server_ = -1;
  std::vector<pid_t> clients_;
};
