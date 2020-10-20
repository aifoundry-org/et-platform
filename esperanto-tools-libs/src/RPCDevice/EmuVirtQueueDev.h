//******************************************************************************
// Copyright (C) 2020, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_DEVICE_EMUVIRTQUEUEDEV_H
#define ET_RUNTIME_DEVICE_EMUVIRTQUEUEDEV_H

#include "Core/DeviceFwTypes.h"
#include "esperanto/runtime/Support/TimeHelpers.h"

#include <cassert>
#include <cstdlib>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <memory>

namespace et_runtime {
namespace device {

class RPCGenerator;

/// @brief Implementation of the circqueue that device-fw supports over the virtual queue.
class CircQueue {
public:
  CircQueue(const std::shared_ptr<RPCGenerator> &rpcGen, uint64_t queueBaseAddr,
            uint64_t queueHeaderAddr, uint16_t queueBufCount, uint16_t queueBufSize);
  static std::unique_ptr<CircQueue> queueFactory(const std::shared_ptr<RPCGenerator> &rpcGen,
                                                 uint64_t queueBaseAddr,
                                                 uint64_t queueHeaderAddr,
                                                 uint16_t queueBufCount,
                                                 uint16_t queueBufSize);
  /// @brief Initialize the circular queue to empty
  bool init();
  bool empty();
  bool full();

  uint16_t queueUsedBufCount();
  uint16_t queueFreeBufCount();

  int64_t write(const void *const data_ptr, uint32_t length, bool setHeadBufWritten);
  int64_t read(void *const data_ptr, uint32_t length, bool setTailBufRead);

  // NOTE: the virtual queue class is responsible for pulling the latest state from
  // the simulator before we operate on the circular queue, and writing the
  // circqueue's updated state back to the simulator once it is done updating it.

  /// @brief Update the state of the circular queue from the "source" of truth (target simulator)
  bool readCircQueueIndices();

  /// @brief Write-back the queue head index to the simulator
  bool writeCircQueueHeadIndex();

  /// @brief Write-back the queue tail index to the simulator
  bool writeCircQueueTailIndex();

private:
  std::shared_ptr<RPCGenerator> rpcGen_;
  const uint64_t queueBaseAddr_;
  const uint64_t queueHeadAddr_;
  const uint64_t queueTailAddr_;
  const uint16_t queueBufCount_;
  const uint16_t queueBufSize_;
  uint16_t head_;
  uint16_t tail_;
  uint16_t headBufPos_;
  uint16_t tailBufPos_;
};


///
/// @brief Helper class that impements the virtual queue unit.
///
/// Virtual queue unit is composed of a pair of submission and
/// completion queue
class EmuVirtQueueDev {
public:
  /// @brief Construct a Virtual Queue device
  EmuVirtQueueDev(const std::shared_ptr<RPCGenerator> &rpcGen, uint8_t queueId, uint64_t virtQueueAddr,
                  uint64_t virtQueueInfoAddr, uint16_t queueBufCount, uint16_t queueBufSize, bool isSP);
  EmuVirtQueueDev(EmuVirtQueueDev &) = delete;

  /// @brief Query if the virtual Queue device is ready
  bool ready(TimeDuration wait_time = TimeDuration::max());

  /// @brief Reset the virtual queue device and discard any pending virtual queue messages from the device
  bool reset(TimeDuration wait_time = TimeDuration::max());

  /// @brief Write message pointed by pointer data of size "size"
  bool write(const void *data, ssize_t size);

  /// @brief Read message of size "size" in buffer data. Wait until wait_time
  /// expires otherwise return false.
  ssize_t read(void *data, ssize_t size, TimeDuration wait_time = TimeDuration::max());

  /// @brief Returns true if EPOLLIN event is available
  bool checkForEventEPOLLIN();

  /// @brief Returns true if EPOLLOUT event is available
  bool checkForEventEPOLLOUT();

  /// @brief Handshake the master and slave status
  bool virtQueueStatusHandShake();

private:
  std::shared_ptr<RPCGenerator> rpcGen_;
  const uint8_t queueId_;
  const uint64_t masterStatusAddr_;
  const uint64_t slaveStatusAddr_;
  std::unique_ptr<CircQueue> submissionQueue_;
  std::unique_ptr<CircQueue> completionQueue_;
  const uint16_t queueBufSize_;
  const bool isSP_;
  bool isReady_;

  /// @brief Read status from remote i.e. SysEMU
  bool readRemoteStatus(uint8_t &master_status, uint8_t &slave_status);

  /// @brief Write slave status to remote i.e. SysEMU
  bool writeRemoteSlaveStatus(uint8_t slave_status);

  /// @brief Raise target device interrupt (SP or MM)
  bool raiseTargetDeviceInterrupt();

  /// @brief Return true if virtual queue is ready
  bool virtQueueReady();

  /// @brief Resets the virtual queue
  bool virtQueueReset();

  /// @brief Set the virtual queue as inactive
  bool virtQueueDestroy();
};

} // namespace device
} // namespace et_runtime

#endif // ET_RUNTIME_DEVICE_EMUVIRTQUEUEDEV_H
