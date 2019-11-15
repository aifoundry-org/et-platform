//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_DEVICE_EMUMAILBOXDEV_H
#define ET_RUNTIME_DEVICE_EMUMAILBOXDEV_H

#include "Core/DeviceFwTypes.h"
#include "Support/TimeHelpers.h"

#include <cassert>
#include <cstdlib>
#include <stdint.h>
#include <unistd.h>

namespace et_runtime {
namespace device {

class RPCTarget;

enum class RingBufferType : uint8_t {
  NONE = 0,
  RX = 1,
  TX = 2,
};

#if ENABLE_DEVICE_FW
  /// @brief Implementation of the ringbuffer that device-fw supports
  /// over the mailbox.
  class RingBuffer {
  public:
    RingBuffer(RingBufferType type, RPCTarget &target);
    const device_fw::ringbuffer_s &state() const { return ringbuffer_; }
    void setState(const device_fw::ringbuffer_s &rb) { ringbuffer_ = rb; }
    int64_t write(const void *const data_ptr, uint32_t length);
    int64_t read(void *const data_ptr, uint32_t length);
    uint64_t free();
    uint64_t used();
    /// @brief Initialize the ring buffer to empty
    bool init();
    bool empty();
    bool full();

    // NOTE: the mailbox class is responsible for pulling the lastest state from
    // the simulator before we operate on the ringbuffer, and writing the
    // ringbuffer's updated state back to the simulator once it is done updating
    // it.
    /// @brief Update the state of the ring buffer from the "source" fo truth
    /// that is the target simulator
    bool readRingBufferState();
    /// @brief Write-back the mailbox status
    bool writeRingBufferState();

  private:
    RingBufferType type_;
    struct device_fw::ringbuffer_s ringbuffer_;
    RPCTarget &rpcDev_;
  };

#else

  class RingBuffer {
  public:
    RingBuffer(RingBufferType type, RPCTarget &target) {  }
  };

#endif


///
/// @brief Helper class that impements the mailbox protocol.
///
/// This class provides the same interface as the MailBoxDev that wraps
/// the character device, but internally it fully implements the mailbox
/// protocol over the SimualorAPI. This is used when we communicate to
/// Device-FW over the SimulatorAPI to SysEmu or VCS
class EmuMailBoxDev {
public:
  /// @brief Construct a MailBox device passing the path to it
  EmuMailBoxDev(RPCTarget &rpcDevice);
  EmuMailBoxDev(EmuMailBoxDev &) = delete;

  /// @brief Query if the mailbox devie is ready
  bool ready(TimeDuration wait_time = TimeDuration::max());

  /// @brief Reret the mailbox device and discard any pending mailbox messages
  /// from the device
  bool reset(TimeDuration wait_time = TimeDuration::max());

  /// @brief Return the maximum size of a mailbox message
  ssize_t mboxMaxMsgSize() const;

  /// @brief Write message pointed by pointer data of size "size"
  bool write(const void *data, ssize_t size);

  /// @brief Read message of size "size" in buffer data. Wait until wait_time
  /// expires otherwise return false.
  ssize_t read(void *data, ssize_t size,
               TimeDuration wait_time = TimeDuration::max());

protected:
  RingBuffer tx_ring_buffer_; ///<
  RingBuffer rx_ring_buffer_; ///<
  uint32_t master_status_; ///< master status for the mailbox. WE SHOULD NEVER
                           ///< MODIFY DIRECTLY THE MASTER STATUS
  /// HERE WE HAVE A READ ONLY COPY
  // The host is always the mailbox slave
  uint32_t slave_status_; ///< slave status for the mailbox
  RPCTarget &rpcDev_;

  bool readRemoteStatus();
  bool writeRemoteStatus();
  bool mboxDestroy();
  bool mboxReady();
  bool mboxReset();
  bool waitForHostInterrupt(TimeDuration wait_time);
  bool raiseDevicePuPlicPcieMessageInterrupt();
};
} // namespace device

} // namespace et_runtime

#endif // ET_RUNTIME_DEVICE_EMUMAILBOXDEV_H
