/*-------------------------------------------------------------------------
 * Copyright (C) 2021,2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/
#include "Protocol.h"
#include "RuntimeImp.h"
#include "runtime/Types.h"

#include <set>
#include <sys/socket.h>
#include <thread>

#pragma once
namespace rt {
class Server;
class Worker : public patterns::Observer<EventId> {
public:
  explicit Worker(int socket, RuntimeImp& runtime, Server& server, ucred credentials);
  ~Worker();

  void update(EventId event) override;

private:
  void requestProcessor();
  void freeResources();
  void processRequest(const req::Request& request);

  void sendResponse(const resp::Response& response);

  struct Allocation {
    DeviceId device_;
    std::byte* ptr_;
    bool operator<(const Allocation& other) const {
      return device_ < other.device_ || (device_ == other.device_ && ptr_ < other.ptr_);
    }
  };

  RuntimeImp::CmaCopyFunction cmaCopyFunction_;
  RuntimeImp& runtime_;
  std::set<Allocation> allocations_;
  std::set<StreamId> streams_;
  std::set<KernelId> kernels_;
  std::set<EventId> events_;
  std::thread runner_;
  Server& server_;
  int socket_;
  bool running_ = true;
};
} // namespace rt