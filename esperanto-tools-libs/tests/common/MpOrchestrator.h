//******************************************************************************
// Copyright (C) 2022, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------
#pragma once
#include "runtime/IRuntime.h"
#include "server/Client.h"
#include "server/Server.h"
#include <hostUtils/logging/Logger.h>

// this class is not thread-safe
class MpOrchestrator {
public:
  ~MpOrchestrator();
  using DeviceLayerCreatorFunc = std::function<std::unique_ptr<dev::IDeviceLayer>()>;

  void createServer(DeviceLayerCreatorFunc deviceLayerCreator, rt::Options options);
  void createClient(const std::function<void(rt::IRuntime* runtime)>&);
  const std::string& getSocketPath() const { // to be able to create local clients.
    return socketPath_;
  }
  void clearClients();

private:
  bool useExternalServer_ = false;
  int efdToServer_;
  int efdFromServer_;
  std::string socketPath_;
  pid_t server_ = -1;
  std::vector<pid_t> clients_;
};
