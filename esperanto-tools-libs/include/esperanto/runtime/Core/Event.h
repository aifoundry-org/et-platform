//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_EVENT_H
#define ET_RUNTIME_EVENT_H

#include "esperanto/runtime/Support/ErrorOr.h"

#include "esperanto/runtime/DeviceAPI/Command.h"

#include <memory>

namespace et_runtime {

class EventResponse final : public device_api::ResponseBase {
public:
  // dummy type
  using response_devapi_t = bool;
  EventResponse() = default;
};

///
/// @Brief Event holding class
///
/// Events are how concurrent operations (within and among Devices) are
/// synchronized. Events are explicitly created and deleted, have work (i.e.,
/// pending operations in a Stream) associated with them, and can be queried to
/// determine if that work has been completed.
///
/// The workflow for using Events is as follows:
/// - an Event is created
/// - the state of the outstanding operations within a Stream is captured into
/// the Event
/// - the snapshot of pending work captured in an Event can be queried
/// - the client code can be made to wait until all of the work in an Event has
/// completed execution
///
/// In addition to being used as mechanisms for synchronization, Events are also
/// used to capture the execution times associated with specific operations.

// @todo the semantics of this class need to be changed currently not used
// actively in the code
/// base. It is broken as it is.
class Event final : public device_api::Command<EventResponse> {
public:
  Event(bool disable_timing, bool blocking_sync)
      : disable_timing_(disable_timing), blocking_sync_(blocking_sync) {}

  Event() : Event(false, false) {}

  ~Event() {}

  bool isDisableTiming() { return disable_timing_; }
  bool isBlockingSync() { return blocking_sync_; }

  ///
  /// @brief  Return the state of an Event.
  ///
  /// Return a value of `etrtSuccess` if all of the operations associated with
  /// the Event have completed.  Otherwise, a value of `etrtErrorNotReady` is
  /// returned.
  ///
  /// @return  etrtSuccess, etrtErrorNotReady, etrtErrorInvalidValue,
  /// etrtErrorInvalidResourceHandle, etrtErrorLaunchFailure
  ///
  etrtError query();

  ///
  /// @brief  Wait for all of the work captured in an Event to complete.
  ///
  /// Blocks until all of the pending operations recorded in the Event have
  /// completed execution.  This will return immediately if the Event is empty.
  ///
  /// @return  etrtSuccess, etrtErrorInvalidValue,
  /// etrtErrorInvalidResourcHandle, etrtErrorLaunchFailure
  ////
  etrtError synchronize();

  etrtError update();

  /// @todo fixme the following will need to ve fixed at some point
  etrtError execute(Device *dev);

private:
  bool disable_timing_;
  bool blocking_sync_;
};
} // namespace et_runtime

#endif // ET_RUNTIME_EVENT_H
