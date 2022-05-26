/*-------------------------------------------------------------------------
 * Copyright (C) 2022, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/
#include "Server.h"
#include "Utils.h"
#include "Worker.h"
#include "runtime/Types.h"
#include <hostUtils/threadPool/ThreadPool.h>
#include <mutex>
#include <signal.h>
#include <sys/param.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
using namespace rt;

Server::~Server() {
  RT_LOG(INFO) << "Destroying server.";
  running_ = false;
  SpinLock lock(mutex_);
  RT_LOG(INFO) << "Waiting for listener.";
  listener_.join();
  RT_LOG(INFO) << "Destroying all existing workers.";
  workers_.clear();
  close(socket_);
}

Server::Server(const std::string& socketPath, std::unique_ptr<dev::IDeviceLayer> deviceLayer, Options options) {
  deviceLayer_ = std::move(deviceLayer);

  runtime_ = IRuntime::create(deviceLayer_.get(), options);

  socket_ = socket(AF_UNIX, SOCK_SEQPACKET, 0);

  sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);
  remove(socketPath.c_str());
  if (bind(socket_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    RT_LOG(FATAL) << "Bind error: " << strerror(errno);
  }
  if (::listen(socket_, 10) < 0) {
    RT_LOG(FATAL) << "Listen error: " << strerror(errno);
  }
  RT_LOG(INFO) << "Listening on socket " << socketPath;
  listener_ = std::thread(&Server::listen, this);
  runtime_->setOnStreamErrorsCallback([this](EventId evt, const StreamError& error) {
    SpinLock lock(mutex_);
    for (auto& w : workers_) {
      w->onStreamError(evt, error);
    }
  });
}

void Server::listen() {

  while (running_) {
    pollfd pfd;
    pfd.events = POLLIN;
    pfd.fd = socket_;
    if (poll(&pfd, 1, 5) == 0) {
      continue;
    }
    auto cl = accept(socket_, nullptr, nullptr);
    if (cl < 0) {
      RT_LOG(WARNING) << "Accept error: " << strerror(errno) << ". Ignoring this client connection.";
      continue;
    }
    if (int val = 1; setsockopt(cl, SOL_SOCKET, SO_PASSCRED, &val, sizeof(val)) < 0) {
      RT_LOG(FATAL) << "Unable to set local passcred: " << strerror(errno)
                    << ". Be sure runtime daemon has CAP_SYS_PTRACE capability.";
    }
    socklen_t len = sizeof(ucred);
    ucred credentials;
    if (getsockopt(cl, SOL_SOCKET, SO_PEERCRED, &credentials, &len) == -1) {
      RT_LOG(FATAL) << "Unable to get peer credentials: " << strerror(errno)
                    << ". Be sure runtime daemon has CAP_SYS_PTRACE capability.";
    }
    RT_LOG(INFO) << " New client connection established from PID: " << credentials.pid << "(UID: " << credentials.uid
                 << " GID: " << credentials.gid << ").";

    // delegate the request processing for this client to a worker
    SpinLock lock(mutex_);
    workers_.emplace_back(std::make_unique<Worker>(cl, dynamic_cast<RuntimeImp&>(*runtime_), *this, credentials));
  }
}

void Server::removeWorker(Worker* worker) {
  tp_.pushTask([this, worker] {
    RT_VLOG(LOW) << "Removing worker: " << worker;
    SpinLock lock(mutex_);
    if (running_) {
      auto it =
        std::find_if(begin(workers_), end(workers_), [worker](const auto& item) { return item.get() == worker; });
      if (it != end(workers_)) {
        workers_.erase(it);
        RT_VLOG(LOW) << "Worker " << worker << " removed.";
      } else {
        RT_LOG(WARNING) << "Trying to destroy a non existant worker and server was still runnning.";
      }
    }
  });
}