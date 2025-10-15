
/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/
#include "hostUtils/actionList/ActionList.h"
#include <chrono>

using namespace actionList;

void ActionList::addAction(std::unique_ptr<IAction> action) {
  actions_.push_back(std::move(action));
}

size_t ActionList::getNumActions() const {
  return actions_.size();
}

void ActionList::update() {
  if (actions_.empty()) {
    return;
  }
  while (!actions_.empty() && actions_.front()->update()) {
    actions_.front()->onFinish();
    actions_.pop_front();
  }
}
