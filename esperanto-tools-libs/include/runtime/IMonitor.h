/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/
#pragma once

#include "Types.h"
#include <runtime/IRuntimeExport.h>

/// \defgroup runtime_monitor_api Runtime Monitoring API
///
/// The Monitor API provides an interface to query about the multiprocess server status.
///
/// @{
namespace rt {
/// \brief Facade Monitor interface declaration, all monitoring interactions should be made using this interface. There
/// is a static method \ref create to make monitoring instances.
///
class ETRT_API IMonitor {
public:
  /// \brief Returns the number of clients currently connected to the server.
  virtual size_t getCurrentClients() = 0;

  /// \brief Returns the free memory in bytes for each device.
  virtual std::unordered_map<DeviceId, uint64_t> getFreeMemory() = 0;

  /// \brief Returns how many commands are waiting to be executed for each device. These commands are queued in the host
  /// runtime and still not sent to the device.
  virtual std::unordered_map<DeviceId, uint32_t> getWaitingCommands() = 0;

  /// \brief Returns how many events are alive, this number correlates with the number of commands which are currently
  /// sent to the device and waiting to get the response from.
  virtual std::unordered_map<DeviceId, uint32_t> getAliveEvents() = 0;

  /// \brief create a new instance of the monitor.
  /// \param socketPath the socket to connect to the multiprocess server.
  static std::unique_ptr<IMonitor> create(const std::string& socketPath);

  /// \brief Defaulted destructor to enable polymorphic destruction.
  virtual ~IMonitor() = default;
};
} // namespace rt