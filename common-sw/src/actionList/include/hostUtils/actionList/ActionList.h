/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/

#pragma once

#include <hostUtils/actionList/ActionListExport.h>

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <list>

namespace actionList {

struct ACTION_LIST_API IAction {
  virtual bool update() = 0;    // returns true if the action is finished
  virtual void onFinish(){};    // called when the action is finished, defaults to do nothing
  virtual ~IAction() = default; // virtual destructor
};

class ACTION_LIST_API ActionList {
public:
  void addAction(std::unique_ptr<IAction> action); // add an action to the list
  void update(); // update first action, remove it if finished and call the next one, until one is not finished
  size_t getNumActions() const; // get number of actions in the list

private:
  // using list to allow pushing new actions without invalidating iterators
  std::list<std::unique_ptr<IAction>> actions_;
};

} // namespace actionList
