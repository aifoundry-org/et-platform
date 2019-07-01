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

#include "DeviceAPI/Response.h"

#include "Common/ErrorTypes.h"

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
  using IDty = int64_t;

  CommandBase();
  virtual ~CommandBase() = default;

  /// @brief Return the unique ID of this specific command
  IDty id() const { return command_id_; }

  /// @brief Execute the command on the Device
  virtual etrtError execute(et_runtime::Device *device_target) = 0;

protected:
  static IDty command_id_;
};

template <class Response> class Command : public CommandBase {
public:
  using Timestamp = typename Response::Timestamp;

  Command()
      : timestamp_(std::chrono::high_resolution_clock::now()), promise_() {}

  virtual ~Command() = default;

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

private:
  Timestamp timestamp_;            ///< Timestamp this command was created
  std::promise<Response> promise_; ///< Promise used to return a future with the
                                   ///< Repose value of this command
};
} // namespace device_api
} // namespace et_runtime

#endif // ET_RUNTIME_DEVICEAPI_COMMAND_H
