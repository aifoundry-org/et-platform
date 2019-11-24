//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_DEVICEAPI_RESPONSE_H
#define ET_RUNTIME_DEVICEAPI_RESPONSE_H

#include "esperanto/runtime/Common/ErrorTypes.h"

#include <esperanto/device-api/device_api.h>
#include <chrono>

namespace et_runtime {
namespace device_api {

///
/// @brief Base class for the different types of responses that we are going to
/// encounter
///
class ResponseBase {
public:
  using Timestamp = std::chrono::time_point<std::chrono::high_resolution_clock>;
  ///
  /// @param[in] error Error we encountered while executing the command
  ResponseBase(etrtError error)
      : timestamp_(std::chrono::high_resolution_clock::now()),
        error_type_(error) {}

  virtual ~ResponseBase() = default;

  ResponseBase() : ResponseBase(etrtSuccess) {}

  ///
  /// @brief Return true iff no error has been encountered
  explicit operator bool() const { return error_type_ == etrtSuccess; }

  ///
  /// @brief Return the error value of the response
  etrtError error() const { return error_type_; }

  ///
  /// @brief Return response creation timestamp
  Timestamp endTime() const { return timestamp_; }

private:
  Timestamp timestamp_;  ///< Creation timestamp
  etrtError error_type_; ///< Error type of an error encountered or etrtSuccess
                         ///< by default
};

///
/// @brief Response to a Command sent to the device.
///
/// The Command could be issued to the device either through a DeviceAPI
/// command or because of an interaction with the PCIE driver
template <class ResponseType> class Response {
public:
  using response_devapi_t = ResponseType;

  Response(const ResponseType &r) : rsp_(r) {}

  /// @brief Return the type of the response
  ::device_api::device_api_msg_e type() const {
    return rsp_.response_info.message_id;
  }

  /// @brief Return the information of the commmand the response matches
  const ::device_api::command_header_t &cmd_info() const {
    return rsp_.response_info.command_info;
  }

  /// @brief Return the device timestamp the response was generated
  uint64_t device_timestamp() const {
    return rsp_.response_info.device_timestamp;
  }

  /// @brief Return the response information
  const ResponseType &response() const { return rsp_; }

protected:
  ResponseType rsp_; ///< Actual response data
};

} // namespace device_api
} // namespace et_runtime

#endif // ET_RUNTIME_DEVICEAPI_RESPONSE_H
