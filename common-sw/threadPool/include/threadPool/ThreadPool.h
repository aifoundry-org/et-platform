/*-------------------------------------------------------------------------
 * Copyright (C) 2021, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#pragma once
#include "function2.hpp"
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>
namespace threadPool {

class ThreadPool {
public:
  using Task = fu2::unique_function<void()>;
  explicit ThreadPool(size_t numThreads);
  void pushTask(Task task);
  ~ThreadPool();

private:
  void workerFunc();
  std::mutex mutex_;
  bool running_;
  std::condition_variable condVar_;
  std::vector<Task> tasks_;
  std::vector<std::thread> threads_;
};
} // namespace threadPool
