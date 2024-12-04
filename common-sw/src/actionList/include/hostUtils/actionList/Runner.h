/*-------------------------------------------------------------------------
 * Copyright (C) 2023, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#pragma once
#include "ActionList.h"

#include <condition_variable>
#include <thread>
namespace actionList {

/// \brief Runner instance manages a  thread that executes actions from an ActionList; it will execute an update cycle
/// and then it will block.
class Runner {
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