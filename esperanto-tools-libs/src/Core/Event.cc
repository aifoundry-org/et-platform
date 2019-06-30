#include "Core/Event.h"

#include "DeviceAPI/Commands.h"
#include <assert.h>
#include <memory.h>
#include <stdio.h>

using namespace et_runtime;

void Event::resetAction(std::shared_ptr<EtActionEvent> action) {
  action_event_ = action;
}
