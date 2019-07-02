#include "Core/Event.h"

#include "DeviceAPI/Commands.h"
#include <assert.h>
#include <memory.h>
#include <stdio.h>

using namespace et_runtime;

etrtError Event::execute(Device *dev) {
  setResponse(EventResponse());
  return etrtSuccess;
}
