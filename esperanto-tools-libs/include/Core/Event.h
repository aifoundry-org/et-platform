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

#include "Support/ErrorOr.h"

#include <memory>

namespace et_runtime {

class EtActionEvent;

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

class Event {
public:
  Event(bool disable_timing, bool blocking_sync)
      : disable_timing_(disable_timing), blocking_sync_(blocking_sync) {}

  ~Event() { assert(action_event_ == nullptr); }

  std::shared_ptr<et_runtime::EtActionEvent> &getAction() {
    return action_event_;
  }
  void resetAction(std::shared_ptr<et_runtime::EtActionEvent> action = nullptr);

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

private:
  bool disable_timing_;
  bool blocking_sync_;
  std::shared_ptr<et_runtime::EtActionEvent> action_event_ = nullptr;
};
} // namespace et_runtime

#endif // ET_RUNTIME_EVENT_H
