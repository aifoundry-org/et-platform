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

enum class Status : uint64_t { INVALID, SERVER_READY, END_SERVER, SERVER_ERROR };

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
        RT_LOG(INFO) << "Creating server process (pid " << getpid() << ")";
        auto server = rt::Server{socketPath_, deviceLayerCreator(), options};
        Status s(Status::SERVER_READY);
        write(efdFromServer_, &s, sizeof(s));
        while (s != Status::END_SERVER) {
          read(efdToServer_, &s, sizeof(s));
        }
        RT_LOG(INFO) << "Exiting server process (pid " << getpid() << ")";
        logger.reset();
        exit(0);
      } else {
        Status s(Status::INVALID);
        while ((s != Status::SERVER_READY) and (s != Status::SERVER_ERROR)) {
          read(efdFromServer_, &s, sizeof(s));
        }

        if (s == Status::SERVER_ERROR) {
          ADD_FAILURE() << "Server failure. Exiting (pid " << getpid() << ")";
          exit(1);
        }
      }
    } catch (const std::exception& e) {
      if (server_ == 0) {
        Status s(Status::SERVER_ERROR);
        write(efdFromServer_, &s, sizeof(s));
      }
      ADD_FAILURE() << "Exception in server process: '" << e.what() << "'. Exiting (pid " << getpid() << ")";
      exit(1);
    } catch (...) {
      ADD_FAILURE() << "Unknown exception in server process. Exiting (pid " << getpid() << ")";
      exit(1);
    }
  }
}

void MpOrchestrator::createClient(const std::function<void(rt::IRuntime*)>& func) {
  auto pid = fork();
  if (pid == 0) {
    try {
      auto logger = std::make_unique<logging::LoggerDefault>();
      auto cl = rt::Client(socketPath_);
      func(&cl);
      RT_LOG(INFO) << "End client execution.";
      logger.reset();
      exit(0);
    } catch (const std::exception& e) {
      ADD_FAILURE() << "Exception in client process: " << e.what();
      exit(1);
    } catch (...) {
      ADD_FAILURE() << "Unknown exception in client process.";
      exit(1);
    }
  } else {
    clients_.push_back(pid);
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
