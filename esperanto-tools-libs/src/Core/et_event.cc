#include "et_event.h"
#include <assert.h>
#include <memory.h>
#include <stdio.h>

void EtEvent::resetAction(EtActionEvent *action /*= nullptr*/) {
  if (action_event_) {
    EtAction::decRefCounter(action_event_);
  }

  action_event_ = action;

  if (action_event_) {
    action_event_->incRefCounter();
  }
}
