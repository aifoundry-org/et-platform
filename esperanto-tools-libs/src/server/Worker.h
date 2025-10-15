/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/
#pragma once
#include "Protocol.h"
#include "RuntimeImp.h"
#include "runtime/Types.h"
#include "server/Protocol.h"

#include <set>
#include <sys/socket.h>
#include <thread>

namespace rt {

class Server;

namespace profiling {
class RemoteProfiler;
}

class Worker : public patterns::Observer<EventId> {
public:
  explicit Worker(int socket, RuntimeImp& runtime, Server& server, ucred credentials);
  ~Worker() override;

  void update(EventId event) override;

  void onStreamError(EventId event, const StreamError& error);

  void onKernelAborted(EventId event, std::byte* context, size_t size, std::function<void()> freeResources);

  void sendProfilerEvent(const profiling::ProfileEvent& event) {
    resp::Response response{resp::Type::TRACING_EVENT, req::ASYNC_RUNTIME_EVENT, event};
    std::get<profiling::ProfileEvent>(response.payload_).setServerPID(pid_);
    sendResponse(response);
  }

private:
  void requestProcessor(pid_t clientPID);
  void freeResources();
  void processRequest(const req::Request& request);

  void sendResponse(const resp::Response& response);

  rt::profiling::RemoteProfiler* getProfiler();

  struct Allocation {
    DeviceId device_;
    std::byte* ptr_;
    bool operator<(const Allocation& other) const {
      return device_ < other.device_ || (device_ == other.device_ && ptr_ < other.ptr_);
    }
  };

  inline static std::atomic<size_t> workerId_{0};

  RuntimeImp& runtime_;
  CmaCopyFunction cmaCopyFunction_;
  std::unordered_map<EventId, std::function<void()>> kernelAbortedFreeResources_;
  std::set<Allocation> allocations_;
  std::set<StreamId> streams_;
  std::set<KernelId> kernels_;
  std::set<EventId> events_;
  std::thread runner_;
  Server& server_;
  std::recursive_mutex mutex_;
  int socket_;
  bool running_ = true;

  pid_t pid_;
};
} // namespace rt
