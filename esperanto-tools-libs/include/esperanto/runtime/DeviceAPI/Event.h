//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_DEVICEAPI_EVENT_H
#define ET_RUNTIME_DEVICEAPI_EVENT_H

#include <esperanto/device-api/device_api.h>

#include <chrono>
#include <cstdint>

namespace et_runtime {
namespace device_api {

///
/// @brief Base Event class that enables us to store events it generic
/// containers
///
class EventBase {
public:
  virtual ~EventBase() = default;
};

///
/// @brief Device Event
///
/// Class hold information about an event that has originated in the device.
/// The source of the event can be communicated either through the DeviceAPI
/// or could be an error that is generated/reported by the PCIE driver
///
template <class EventType> class Event {
public:
  Event(const EventType &e) : event_(e) {}

  /// @brief Return the type of the event
  ::device_api::device_api_msg_e type() const { return event_.event_info.message_id; }

  /// @brief Return the device timestamp the event was generated
  uint64_t device_timestamp() const { return event_.event_info.device_timestamp; }

  /// @brief return the event data
  const EventType &event() const { return event_; }

protected:
  EventType event_; ///< Event information
};

} // namespace device_api
} // namespace et_runtime

#endif // ET_RUNTIME_DEVICEAPI_EVENT_H
