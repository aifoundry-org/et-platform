#include "Core/Event.h"

#include "Core/Commands.h"
#include <assert.h>
#include <memory.h>
#include <stdio.h>

using namespace et_runtime;

void Event::resetAction(EtActionEvent *action /*= nullptr*/) {
  if (action_event_) {
    EtAction::decRefCounter(action_event_);
  }

  action_event_ = action;

  if (action_event_) {
    action_event_->incRefCounter();
  }
}
