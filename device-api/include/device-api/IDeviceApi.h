/*-------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/
#pragma once

#include <chrono>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <vector>

/// \defgroup device_api Device API
///
/// The device API provides an asynchronous API (command send and response) and a synchronous API (configuration
/// discovery methods).
/// The asynchronous API is meant to be used to send commands to a device and block until receiving response from the
/// device or until a given timeout expires.
/// The synchronous API offers different methods which will return inmediatly, these methods are queries about hardware
/// configuration.
///
/// Any error will be reported throwing an Exception of kind dev::Exception. Except for device errors which can be
/// reported through the command response; hence in the asynchronous API one could expect an exception only if the
/// transport layer fails, not the command execution at the device.
/// @{
namespace dev {

class Exception : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

class IDeviceAsync {
public:
  /// \brief Sends a command to the device
  ///
  /// @param[in] device indicating which device to send the command
  /// @param[in] command its a buffer which contains the command itself
  /// @param[in] commandSize the size of the command + payload buffer
  ///
  ///
  virtual void sendCommand(int device, int sq_idx, const std::byte* command, size_t commandSize) = 0;

  /// \brief Receives a response from the device, blocking the caller thread till the response is ready or until the
  /// timeout expires
  ///
  /// @param[in] device indicating which device to receive the response from
  /// @param[out] response buffer containing the response
  /// @param[in] timeout when this timeout expires, the thread will be unblocked and the wait finishes, returning false
  ///
  /// @returns false if the timeout is expired, true otherwise; indicating we got a response from the device
  ///
  virtual bool receiveResponse(int device, int cq_idx, std::vector<std::byte>& response, std::chrono::milliseconds timeout) = 0;

  /// \brief Virtual Destructor to enable polymorphic release of the IDeviceAsync
  /// instances
  virtual ~IDeviceAsync() = default;
};

 /// This should be a command Interface class which is shared between Ops and Management, although will be different
 
class IDeviceSync {
public:
  /// \brief Returns the number of esperanto devices connected to the host
  ///
  /// @returns the number of devices
  ///
  virtual int getDevicesCount() const = 0;

  /// \brief Returns the number of virtual queues associated to given device
  ///
  /// @returns the number of virtual queues
  ///
  virtual int getVirtualQueuesCount(int device) const = 0;

  /// \brief Returns the DMA alignment requirement
  ///
  /// @returns DMA alignment
  ///
  virtual int getDmaAlignment() const = 0;

  /// \brief Returns the DRAM available size in bytes
  ///
  /// @returns DRAM size in bytes
  ///
  virtual size_t getDramSize() const = 0;

  /// \brief Returns the DRAM base address
  ///
  /// @returns DRAM Host Managed base address
  ///
  virtual uint64_t getDramBaseAddress() const = 0;

  /// \brief Discover commands to discover all available devices
  //
  /// @returns arrays of int of indicies containing all available ET SOC devices
  ///

  virtual std::array<int, 6> devices = {0,1,2,3,4,5};

  /// \brief Virtual Destructor to enable polymorphic release of the IDeviceOpsSync
  /// instances
  virtual ~IDeviceSync() = default;
};


class IDeviceApi : public IDeviceAsync, public IDeviceSync {

  /// \brief Indicates the kind of deviceAPI implementation for the factory method
  enum class Kind { SysEmu, Silicon };

  /// \brief Factory method to instantiate a IDeviceApi implementation
  ///
  ///
  /// @returns std::unique_ptr<IDeviceApi> is the IDeviceApi implementation
  ///
  static std::unique_ptr<IDeviceApi> create(Kind);


  /// \brief Virtual Destructor to enable polymorphic release of the IDeviceApi
  /// instances
  virtual ~IDeviceApi() = default;
};

} // namespace dev
