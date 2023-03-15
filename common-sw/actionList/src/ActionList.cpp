
/*-------------------------------------------------------------------------
 * Copyright (C) 2021, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/
#include "actionList/ActionList.h"
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
