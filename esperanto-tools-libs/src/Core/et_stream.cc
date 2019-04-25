#include "Core/et_stream.h"
#include "Core/Commands.h"

void EtStream::init() {
  // fprintf(stderr, "Hello from EtStream::init()\n");
}

void EtStream::addCommand(et_runtime::EtAction *action) {
  actions_.push(action);
}
