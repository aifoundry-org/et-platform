#include "Core/Stream.h"
#include "Core/Commands.h"

namespace et_runtime {

void Stream::init() {
  // fprintf(stderr, "Hello from EtStream::init()\n");
}

void Stream::addCommand(std::shared_ptr<et_runtime::EtAction> action) {
  actions_.push(action);
}

} // namespace et_runtime
