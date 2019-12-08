//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "esperanto/runtime/Core/Stream.h"
#include "DeviceAPI/Commands.h"
#include "esperanto/runtime/Core/Device.h"
#include "esperanto/runtime/Core/Event.h"

namespace et_runtime {

StreamID Stream::global_stream_count_ = 1;

Stream::StreamRegistry Stream::stream_registry_ = {};

Stream::Stream(Device &dev, bool is_blocking)
    : id_(global_stream_count_++), dev_(dev), is_blocking_(is_blocking) {
  stream_registry_[id_] = std::unique_ptr<Stream>(this);
}

Stream::~Stream() { assert(actions_.empty()); }

ErrorOr<Stream &> Stream::getStream(StreamID sid) {
  auto it = stream_registry_.find(sid);
  if (it == stream_registry_.end()) {
    return etrtErrorInvalidResourceHandle;
  }
  return *it->second;
}

etrtError Stream::destroyStream(StreamID id) {
  auto it = stream_registry_.find(id);
  if (it == stream_registry_.end()) {
    return etrtErrorInvalidResourceHandle;
  }
  stream_registry_.erase(it);
  return etrtSuccess;
}

etrtError Stream::synchronize() {
  auto event = std::make_shared<Event>();
  addCommand(std::dynamic_pointer_cast<device_api::CommandBase>(event));
  auto future = event->getFuture();
  auto response = future.get();
  return response.error();
}

void Stream::addCommand(
    std::shared_ptr<et_runtime::device_api::CommandBase> action) {
  // Push to the stream's command
  actions_.push(action);
  action->registerStream(this);

  // Push to the device's command queue
  auto res = dev_.addCommand(action);
  assert(res);

}

} // namespace et_runtime
