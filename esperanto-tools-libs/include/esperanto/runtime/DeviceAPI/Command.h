//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_DEVICEAPI_COMMAND_H
#define ET_RUNTIME_DEVICEAPI_COMMAND_H

#include "esperanto/runtime/DeviceAPI/Response.h"

#include "esperanto/runtime/Common/ErrorTypes.h"

#include <esperanto/device-api/device_api.h>
#include <chrono>
#include <cstdint>
#include <future>

namespace et_runtime {

class Device;

namespace device_api {

///
/// @brief Empty CommandBase class so that derived classes can be held inside a
/// container
class CommandBase {
public:
  using IDty = uint64_t;

  CommandBase();
  virtual ~CommandBase() = default;

  /// @brief Return the unique ID of this specific command
  IDty id() const { return command_id_; }

  /// @brief Execute the command on the Device
  virtual etrtError execute(et_runtime::Device *device_target) = 0;

protected:
  static IDty command_id_;
};


///
/// @brief Dummy Command info.
///
/// Class used as a place holder for now until we fully populate the different
/// Types of commands and responses we have across the DeviceAPI and the PCIE Device
class DummyCommandInfo {
public:
  DummyCommandInfo() = default;
};

///
/// @brief Command sent to the device.
///
/// The Command issued to the device can be either issued at the level of the
/// of the DeviceAPI or it could be an interaaction with the PCIE driver.
template <class Response, class CommandInfo=DummyCommandInfo> class Command : public CommandBase {
public:
  using Timestamp = typename ResponseBase::Timestamp;
  using response_devapi_t = typename Response::response_devapi_t;

  Command(const CommandInfo &i)
    : CommandBase(), cmd_info_(i), timestamp_(std::chrono::high_resolution_clock::now()),
        promise_() {
  }

  // FIXME the following constructor should be eventually removed
  Command() : Command(CommandInfo()) {}

  virtual ~Command() = default;

  uint64_t commandID() const { return command_id_; }

  /// @brief Return the underlying command
  const CommandInfo &cmd_info() const { return cmd_info_; }

  /// @brief Return the type of the command
  uint64_t commandType() const { return cmd_info_.message_id; }

  /// @brief Return the command information
  const ::device_api::command_header_t &cmd_header() const {
    return cmd_info_.command_info;
  }

  /// @brief Returns std::future<Response> to wait on until the Response of
  /// this Command is ready.
  std::future<Response> getFuture() { return promise_.get_future(); }

  ///
  /// @brief Set the response of this command
  ///
  /// @param[in] Response that holds the results of the error after executing
  /// this Command
  void setResponse(const Response &resp) { promise_.set_value(resp); }

  ///
  /// @brief Return timestamp the Command was created
  Timestamp startTime() const { return timestamp_; }

protected:
  CommandInfo cmd_info_; ///< Object holding the basic command information
  Timestamp timestamp_;            ///< Timestamp this command was created
  std::promise<Response> promise_; ///< Promise used to return a future with the
                                   ///< Repose value of this command
};
} // namespace device_api
} // namespace et_runtime

#endif // ET_RUNTIME_DEVICEAPI_COMMAND_H
