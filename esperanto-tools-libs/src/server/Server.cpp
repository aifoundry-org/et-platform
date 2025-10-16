/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/
#include "Server.h"

#include "Constants.h"
#include "RemoteProfiler.h"
#include "Utils.h"
#include "Worker.h"

#include "runtime/Types.h"

#include <easy/profiler.h>
#include <hostUtils/threadPool/ThreadPool.h>

#include <linux/capability.h>
#include <signal.h>
#include <sys/capability.h>
#include <sys/param.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include <mutex>

using namespace rt;
namespace {
constexpr auto kSocketNeededSize = static_cast<int>(2 * sizeof(ErrorContext) * kNumErrorContexts);
}

Server::~Server() {
  RT_LOG(INFO) << "Destroying server.";
  running_ = false;
  SpinLock lock(mutex_);
  RT_LOG(INFO) << "Waiting for listener.";
  close(socket_);
  listener_.join();
  RT_LOG(INFO) << "Destroying all existing workers.";
  workers_.clear();
  close(socket_);
  if (auto p = getProfiler(); p != nullptr) {
    p->stop();
  }
}

Server::Server(const std::string& socketPath, std::shared_ptr<dev::IDeviceLayer> const& deviceLayer, Options options)
  : deviceLayer_{deviceLayer} {

  cap_t caps;
  std::array<cap_value_t, 1> capList = {CAP_SYS_PTRACE};
  cap_value_t* p_capList = capList.data();

  caps = cap_get_proc();
  if (caps == nullptr) {
    throw Exception("Can't get process capabilities." + std::string{strerror(errno)});
  }
  if (cap_set_flag(caps, CAP_EFFECTIVE, capList.size(), p_capList, CAP_SET) == -1) {
    throw Exception("Can't set flag for enabling CAP_SYS_PTRACE. " + std::string{strerror(errno)});
  }

  if (cap_set_proc(caps) == -1) {
    throw Exception("Can't set required CAP_SYS_PTRACE capability. Be sure this process is run as a root. " +
                    std::string{strerror(errno)});
  }

  if (cap_free(caps) == -1) {
    throw Exception("Couldn't free the caps struct. " + std::string{strerror(errno)});
  }
  CHECK(deviceLayer_ != nullptr) << "DeviceLayer can't be null";

  runtime_ = IRuntime::create(deviceLayer_, options);
  auto profiler = std::make_unique<rt::profiling::RemoteProfiler>();
  runtime_->setProfiler(std::move(profiler));

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
    for (const auto& w : workers_) {
      w->onStreamError(evt, error);
    }
  });
  runtime_->setOnKernelAbortedErrorCallback(
    [this](EventId event, std::byte* context, size_t size, std::function<void()> freeResources) {
      SpinLock lock(mutex_);
      for (const auto& w : workers_) {
        w->onKernelAborted(event, context, size, freeResources);
      }
    });
}

void Server::listen() {
  EASY_THREAD_SCOPE("Server::listener")

  profiling::IProfilerRecorder::setCurrentThreadName("Listener thread");

  while (running_) {
    pollfd pfd;
    pfd.events = POLLIN;
    pfd.fd = socket_;
    EASY_BLOCK("Server::listener::poll", profiler::colors::Blue)
    if (poll(&pfd, 1, 5) == 0) {
      continue;
    }
    EASY_END_BLOCK
    EASY_BLOCK("Server::listener::accept", profiler::colors::Blue)
    auto cl = accept(socket_, nullptr, nullptr);
    if (cl < 0) {
      RT_LOG(WARNING) << "Accept error: " << strerror(errno) << ". Ignoring this client connection.";
      continue;
    }
    if (auto val = kSocketNeededSize; setsockopt(cl, SOL_SOCKET, SO_SNDBUFFORCE, &val, sizeof(val)) < 0) {
      RT_LOG(FATAL) << "Unable to set send buffer size to required: " << strerror(errno)
                    << ". Be sure runtime daemon has CAP_NET_ADMIN capability.";
    }

    if (auto val = kSocketNeededSize; setsockopt(cl, SOL_SOCKET, SO_RCVBUFFORCE, &val, sizeof(val)) < 0) {
      RT_LOG(FATAL) << "Unable to set receive buffer size to required: " << strerror(errno)
                    << ". Be sure runtime daemon has CAP_NET_ADMIN capability.";
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
    EASY_THREAD("Server::removeWorker");
    RT_VLOG(LOW) << "Removing worker: " << worker;
    SpinLock lock(mutex_);
    if (running_) {
      auto it =
        std::find_if(begin(workers_), end(workers_), [worker](const auto& item) { return item.get() == worker; });
      if (it != end(workers_)) {
        dynamic_cast<RuntimeImp&>(*runtime_).detach(worker);
        workers_.erase(it);
        RT_VLOG(LOW) << "Worker " << worker << " removed.";
      } else {
        RT_LOG(WARNING) << "Trying to destroy a non existant worker and server was still runnning.";
      }
    }
  });
}
