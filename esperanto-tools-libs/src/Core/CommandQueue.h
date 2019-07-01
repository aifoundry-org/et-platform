//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_CORE_COMMAND_QUEUE_H
#define ET_RUNTIME_CORE_COMMAND_QUEUE_H

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>

namespace et_runtime {

///
/// @brief Simple thread-save Command Queue.
///
/// This queue is to be used between the main execution and each stream/device
/// thread. The actions that the main thread will execute will be broken
/// down to Commands that are going to be enqueued and executed in each device.
template <class T> class CommandQueue {
public:
  CommandQueue() = default;
  ~CommandQueue() = default;

  /// @brief Push an element to the queue and notify any waiting thread
  void push(const std::shared_ptr<T> item) {
    std::unique_lock<std::mutex> mlock(mutex_);
    queue_.push(item);
    mlock.unlock(); // unlock before notificiation to minimize mutex contention
    cond_.notify_one(); // notify one waiting thread
  }

  /// @brief Get the front element of the queue or block otherwise
  std::shared_ptr<T> front() {
    std::unique_lock<std::mutex> mlock(mutex_);
    while (queue_.empty()) {
      cond_.wait(mlock);
    }
    auto item = queue_.front();
    return item;
  }

  /// @brief Get and pop the front element of the queue or block if queue is
  /// empty
  std::shared_ptr<T> pop() {
    std::unique_lock<std::mutex> mlock(mutex_);
    while (queue_.empty()) {
      cond_.wait(mlock);
    }
    auto item = queue_.front();
    queue_.pop();
    return item;
  }

private:
  std::queue<std::shared_ptr<T>> queue_;
  std::mutex mutex_;
  std::condition_variable cond_;
};
} // namespace et_runtime

#endif // ET_RUNTIME_CORE_COMMAND_QUEUE_H
