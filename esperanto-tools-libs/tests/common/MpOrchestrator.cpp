//******************************************************************************
// Copyright (C) 2022, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------
#include "MpOrchestrator.h"
#include "runtime/Types.h"
#include <gmock/gmock-more-actions.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <hostUtils/logging/Logger.h>
#include <sys/eventfd.h>
#include <unistd.h>

namespace {
auto getTmpFileName() {
  std::string filename("/tmp/mptestXXXXXX");
  auto fd = mkstemp(filename.data());
  close(fd);
  return filename;
}

enum class Status : uint64_t { INVALID, SERVER_READY, END_SERVER };

} // namespace

void MpOrchestrator::createServer(const DeviceLayerCreatorFunc& deviceLayerCreator, rt::Options options) {
  RT_LOG_IF(FATAL, server_ != -1) << "Server already created!";

  efdToServer_ = eventfd(0, 0);
  efdFromServer_ = eventfd(0, 0);

  if (auto socketPath = getenv("ET_SOCKET_PATH"); socketPath != nullptr) {
    socketPath_ = socketPath;
    Status s(Status::SERVER_READY);
    write(efdFromServer_, &s, sizeof(s));
    useExternalServer_ = true;
  } else {
    // find a socketpath
    socketPath_ = getTmpFileName();
    try {
      // instantiate the server
      server_ = fork();
      if (server_ == 0) {
        auto logger = std::make_unique<logging::LoggerDefault>();
        RT_LOG(INFO) << "Creating server process";
        auto server = rt::Server{socketPath_, deviceLayerCreator(), options};
        Status s(Status::SERVER_READY);
        write(efdFromServer_, &s, sizeof(s));
        while (s != Status::END_SERVER) {
          read(efdToServer_, &s, sizeof(s));
        }
        RT_LOG(INFO) << "Exiting server process.";
        logger.reset();
        exit(0);
      } else {
        Status s(Status::INVALID);
        while (s != Status::SERVER_READY) {
          read(efdFromServer_, &s, sizeof(s));
        }
      }
    } catch (const std::exception& e) {
      FAIL() << "Exception in server process: " << e.what();
    } catch (...) {
      FAIL() << "Unknown exception in server process.";
    }
  }
}

void MpOrchestrator::createClient(const std::function<void(rt::IRuntime*)>& func) {
  try {
    if (auto pid = fork(); pid == 0) {
      auto logger = std::make_unique<logging::LoggerDefault>();
      auto cl = rt::Client(socketPath_);
      func(&cl);
      RT_LOG(INFO) << "End client execution.";
      logger.reset();
      exit(0);
    } else {
      clients_.push_back(pid);
    }
  } catch (const std::exception& e) {
    FAIL() << "Exception in client process: " << e.what();
  } catch (...) {
    FAIL() << "Unknown exception in client process.";
  }
}

void MpOrchestrator::clearClients() {
  RT_LOG(INFO) << "Num clients: " << clients_.size();
  for (auto pid : clients_) {
    CHECK(pid > 0) << "bad PID";
    if (pid > 0) {
      int status;
      waitpid(pid, &status, 0);
      RT_LOG_IF(FATAL, status != 0) << "Child client did not ended properly";
    }
  }
  clients_.clear();
}

MpOrchestrator::~MpOrchestrator() {
  clearClients();
  Status s(Status::END_SERVER);
  write(efdToServer_, &s, sizeof(s));

  if (!useExternalServer_) {
    int status;
    waitpid(server_, &status, 0);
    RT_LOG_IF(FATAL, status != 0) << "Child server did not ended properly";
    std::remove(socketPath_.c_str());
  }
}
