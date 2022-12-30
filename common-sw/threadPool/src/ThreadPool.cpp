
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
#include <chrono>
#include <g3log/loglevels.hpp>
#include <logging/Logger.h>
#include <logging/Logging.h>
#include <mutex>
#include <thread>

#define TP_LOG(severity) ET_LOG(THREADPOOL, severity)
#define TP_DLOG(severity) ET_DLOG(THREADPOOL, severity)
#define TP_VLOG(severity) ET_VLOG(THREADPOOL, severity)
#define TP_LOG_IF(severity, condition) ET_LOG_IF(THREADPOOL, severity, condition)

using namespace threadPool;
ThreadPool::ThreadPool(size_t numThreads, bool resizable, bool waitPendingTasks)
  : running_(true)
  , resizable_(resizable)
  , waitPendingTasks_(waitPendingTasks) {
  addThreads(numThreads);
}

void ThreadPool::pushTask(Task task) {
  if (threads_.empty()) {
    TP_VLOG(MID) << "Running thread pool with no threads (debugging), so execute the task directly";
    task();
  } else {
    TP_VLOG(MID) << "Pushing a new task into threadpool " << std::hex << this;
    std::unique_lock lock(mutex_);
    if (resizable_ && !tasks_.empty()) {
      TP_VLOG(MID) << "All threads busy, adding a new thread to the resizable thread pool. Prev num threads: "
                   << threads_.size();
      addThreads(1U);
    }
    tasks_.emplace(std::move(task));
    lock.unlock();
    condVar_.notify_one();
  }
}

void ThreadPool::blockUntilDrained() {
  std::unique_lock lock(mutex_);
  while (!tasks_.empty()) {
    TP_VLOG(MID) << "Waiting until tasks are drained: " << tasks_.size();
    lock.unlock();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  TP_VLOG(MID) << "All tasks are drained.";
}

ThreadPool::~ThreadPool() {
  TP_LOG(INFO) << "Destroying threadpool " << std::hex << this;
  if (waitPendingTasks_) {
    blockUntilDrained();
  }
  std::unique_lock lock(mutex_);
  while (!tasks_.empty()) {
    tasks_.pop();
  }
  running_ = false;
  lock.unlock();
  condVar_.notify_all();
  TP_VLOG(LOW) << "Waiting for all threads in threadpool " << std::hex << this;
  for (auto& t : threads_) {
    t.join();
  }
  threads_.clear();
  TP_VLOG(LOW) << "Threadpool " << std::hex << this << " destroyed.";
}

void ThreadPool::addThreads(size_t numThreads) {
  for (auto i = 0U; i < numThreads; i++) {
    threads_.emplace_back(std::bind(&ThreadPool::workerFunc, this));
  }
}

void ThreadPool::workerFunc() {
  while (running_) {
    std::unique_lock lock(mutex_);
    if (tasks_.empty()) {
      TP_VLOG(MID) << "No tasks to execute, waiting for next task.";
      condVar_.wait(lock, [this] { return !(running_ && tasks_.empty()); });
    } else {
      TP_VLOG(MID) << "Executing task.";
      auto task = std::move(tasks_.front());
      tasks_.pop();
      lock.unlock();
      task();
    }
  }
}