#include "Core/Stream.h"
#include "DeviceAPI/Commands.h"

namespace et_runtime {

void Stream::init() {
  // fprintf(stderr, "Hello from EtStream::init()\n");
}

void Stream::addCommand(
    std::shared_ptr<et_runtime::device_api::CommandBase> action) {
  actions_.push(action);
}

} // namespace et_runtime
