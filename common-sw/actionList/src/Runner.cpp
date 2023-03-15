/*-------------------------------------------------------------------------
 * Copyright (C) 2023, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include "actionList/Runner.h"
#include "Log.h"

using namespace actionList;

Runner::Runner(ActionList actionList)
  : actionList_(std::move(actionList)) {
  runner_ = std::thread([this] {
    while (running_) {
      AL_VLOG(MID) << "Calling update on actionList.";
      std::unique_lock lock(mutex_);
      actionList_.update();
      if (running_) {
        AL_VLOG(MID) << "Update finished. Locking the mutex.";
        cv_.wait(lock);
      }
    }
  });
}

Runner::~Runner() {
  AL_LOG(INFO) << "Destroying Runner.";
  running_ = false;
  update();
  runner_.join();
  AL_LOG_IF(WARNING, actionList_.getNumActions() > 0)
    << "Runner destroyed with " << actionList_.getNumActions() << " actions still in queue.";
}

void Runner::update() {
  AL_VLOG(MID) << "Updating Runner.";
  cv_.notify_one();
}

void Runner::addAction(std::unique_ptr<IAction> action) {
  std::unique_lock lock(mutex_);
  actionList_.addAction(std::move(action));
  lock.unlock();
  update();
}