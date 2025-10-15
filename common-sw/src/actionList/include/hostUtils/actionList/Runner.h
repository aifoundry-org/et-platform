/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/

#pragma once
#include "ActionList.h"
#include <hostUtils/actionList/ActionListExport.h>

#include <condition_variable>
#include <thread>

namespace actionList {

/// \brief Runner instance manages a  thread that executes actions from an ActionList; it will execute an update cycle
/// and then it will block.
class ACTION_LIST_API Runner {
public:
  explicit Runner(ActionList actionList = {});
  ~Runner();

  /// instructs the thread to perform an update cycle. This will wake up the thread if it was blocked.
  void update();

  /// adds an action to the action list and performs an update cycle.
  void addAction(std::unique_ptr<IAction> action);

private:
  ActionList actionList_;
  std::thread runner_;
  std::mutex mutex_;
  std::condition_variable cv_;
  bool running_ = true;
};

} // namespace actionList