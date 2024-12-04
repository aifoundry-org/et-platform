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
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <list>
namespace actionList {

struct IAction {
  virtual bool update() = 0;    // returns true if the action is finished
  virtual void onFinish(){};    // called when the action is finished, defaults to do nothing
  virtual ~IAction() = default; // virtual destructor
};

class ActionList {
public:
  void addAction(std::unique_ptr<IAction> action); // add an action to the list
  void update(); // update first action, remove it if finished and call the next one, until one is not finished
  size_t getNumActions() const; // get number of actions in the list

private:
  // using list to allow pushing new actions without invalidating iterators
  std::list<std::unique_ptr<IAction>> actions_;
};
} // namespace actionList
