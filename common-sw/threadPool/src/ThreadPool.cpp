
/*-------------------------------------------------------------------------
 * Copyright (C) 2021, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/
#include "threadPool/ThreadPool.h"
#include <g3log/loglevels.hpp>
#include <logging/Logger.h>
#include <logging/Logging.h>

#define TP_LOG(severity) ET_LOG(THREADPOOL, severity)
#define TP_DLOG(severity) ET_DLOG(THREADPOOL, severity)
#define TP_VLOG(severity) ET_VLOG(THREADPOOL, severity)
#define TP_LOG_IF(severity, condition) ET_LOG_IF(THREADPOOL, severity, condition)

using namespace threadPool;
ThreadPool::ThreadPool(size_t numThreads)
  : running_(true) {
  for (auto i = 0U; i < numThreads; ++i) {
    threads_.emplace_back(std::bind(&ThreadPool::workerFunc, this));
  }
}

void ThreadPool::pushTask(Task task) {
  TP_VLOG(HIGH) << "Pushing a new task into threadpool " << std::hex << this;
  std::unique_lock lock(mutex_);
  tasks_.emplace_back(std::move(task));
  lock.unlock();
  condVar_.notify_one();
}

ThreadPool::~ThreadPool() {
  TP_LOG(INFO) << "Destroying threadpool " << std::hex << this;
  std::unique_lock lock(mutex_);
  tasks_.clear();
  running_ = false;
  lock.unlock();
  condVar_.notify_all();
  TP_VLOG(LOW) << "Waiting for all threads in threadpool " << std::hex << this;
  for (auto& t : threads_) {
    t.join();
  }
  TP_VLOG(HIGH) << "Threadpool " << std::hex << this << " destroyed.";
}

void ThreadPool::workerFunc() {
  while (running_) {
    std::unique_lock lock(mutex_);
    if (tasks_.empty()) {
      TP_VLOG(HIGH) << "No tasks to execute, waiting for next task.";
      condVar_.wait(lock, [this] { return !(running_ && tasks_.empty()); });
    } else {
      TP_VLOG(HIGH) << "Executing task.";
      auto task = tasks_.back();
      tasks_.pop_back();
      lock.unlock();
      task();
    }
  }
}