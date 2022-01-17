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
#include "runtime/Types.h"

#include <set>
#include <thread>

#pragma once
namespace rt {
class RuntimeServer;
class IRuntime;
class Worker {
public:
  explicit Worker(int socket, IRuntime& runtime, RuntimeServer& server);
  ~Worker();

private:
  void requestProcessor();
  void freeResources();
  // return false if there were some error
  bool processRequest(const Request::Header& request);

  void sendResponse(Response::Type type, EventId event);

  template <typename T> bool deserializeRequest(T& out, size_t size);

  struct Allocation {
    int device_;
    std::byte* ptr_;
    bool operator<(const Allocation& other) const {
      return device_ < other.device_ || (device_ == other.device_ && ptr_ < other.ptr_);
    }
  };

  std::set<Allocation> allocations_;
  std::thread runner_;
  IRuntime& runtime_;
  RuntimeServer& server_;
  int socket_;
  bool running_ = true;
};
} // namespace rt