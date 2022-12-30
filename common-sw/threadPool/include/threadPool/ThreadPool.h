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
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <list>
#include <mutex>
#include <queue>
#include <thread>
namespace threadPool {

class ThreadPool {
public:
  using Task = fu2::unique_function<void()>;
  // if resizable, the threadpool will automatically grow if all threads are busy when pushing a new task
  // if waitPendingTasks when destroying the threadpool it will block the caller until all task have been executed. If
  // not, it will clear the pending tasks and wait only for the running tasks.
  explicit ThreadPool(size_t numThreads, bool resizable = false, bool waitPendingTasks = false);
  void pushTask(Task task);
  ~ThreadPool();

  // this will block the caller until the threadpool has no more tasks
  void blockUntilDrained();

private:
  void addThreads(size_t numThreads);
  void workerFunc();

  std::list<std::thread> threads_;
  mutable std::mutex mutex_;
  std::condition_variable condVar_;
  std::queue<Task> tasks_;
  bool running_;
  bool resizable_;
  bool waitPendingTasks_;
};
} // namespace threadPool
