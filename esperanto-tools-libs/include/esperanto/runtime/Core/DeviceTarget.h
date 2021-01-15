//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_DEVICE_TARGET_H
#define ET_RUNTIME_DEVICE_TARGET_H

/// @file

#include "esperanto/runtime/Core/DeviceInformation.h"
#include "esperanto/runtime/Support/TimeHelpers.h"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace et_runtime {
namespace device {

// Types
typedef struct {
    uint64_t tensor_a;     // Pointer to tensor A
    uint64_t tensor_b;     // Pointer to tensor B
    uint64_t tensor_c;     // Pointer to tensor C
    uint64_t tensor_d;     // Pointer to tensor D
    uint64_t tensor_e;     // Pointer to tensor E
    uint64_t tensor_f;     // Pointer to tensor F
    uint64_t tensor_g;     // Pointer to tensor G
    uint64_t tensor_h;     // Pointer to tensor H
    uint64_t kernel_id;    // Id for this Kernel
} layer_dynamic_info;


/// @class DeviceTarget
/// @brief Abtract class describing the interface to talk to different targets
///
/// The runtime can talk to a number of underlying device targers beyond the
/// PCIe device driver that will be in production. This class provides a generic
/// abstract interface to talk to the different targets.
///
/// # RIIA "Device Schemantics"
///
/// Interacting with PCIe drivers or any other simulator does not provide clean
/// RIIA semantics: e.g. closing files or connections can generate errors that
/// we need to be able to handle and recover from if necessary. As such we want
/// to avoid adding this logic inside the destructor where we cannot return an
/// error from and we should not be throwing exceptions either. As such we
/// provide init and deInit functions that will explicitly handle

class DeviceTarget {
public:
  /// @brief Type of different target devices we can have.
  enum class TargetType : uint8_t {
    None = 0,
    PCIe, ///< PCIE device type
    SysEmuGRPC, ///< Create an RPC device and connect to SysEmu
    SysEmuGRPC_VQ_MM, ///< Create an RPC device and connect to SysEmu using the VQ implementation (a duplicate of SysEmuGRPC to keep both mailbox and virtqueue functional at a time)
    DeviceGRPC, ///< Create a RPC device and connect to any simulator that simplements the SimulatorAPI
    FakeDevice, ///< @todo this type should be deprecated
  };

  static const std::map<std::string, TargetType> Str2TargetType;

  /// @brief Callback function type for device responses
  using ResponseCallback = std::function<bool()>;
  /// @brief Callback function for device events
  using EventCallback = std::function<bool()>;

  DeviceTarget() = default;
  /// @brief DeviceTargert contructor
  ///
  /// @params[in] Index of the device we are using
  DeviceTarget(int index);
  virtual ~DeviceTarget() = default;

  ///
  /// @brief static function that should enumerate all devices and return
  /// related information
  ///
  /// @return Vector with the DeviceInfomrmation of all devices we can detect on
  /// the system
  /// @todo Having it be a static function is not the best interface decision
  /// this should be revisited
  static std::vector<DeviceInformation> enumerateDevices() {
    abort();
    return {};
  }

  /// @brief Return the type of this device
  virtual TargetType type() = 0;
  /// @brief Initialize the target device
  virtual bool init() = 0;
  /// @brief Steps to take after fw is loaded on the device
  // FIXME this is ore of a place holder that assumes that the runtime loads the
  // FW on the device
  // which is true for SysEMU but will not be the case with the real device
  virtual bool postFWLoadInit() = 0;
  /// @brief De-Initialize the target device
  virtual bool deinit() = 0;
  /// @brief Get status information from the device
  virtual bool getStatus() = 0;

  /// @brief Return the absolute base DRAM address we can access
  virtual uintptr_t dramBaseAddr() const = 0;

  /// @brief Return the size of DRAM we can write to in bytes.
  virtual uintptr_t dramSize() const = 0;

  ///
  /// @brief Get static configuration information from the device
  ///
  /// Extract static configuration information from the device. The static
  /// information should be exposed at the level of the device driver and should
  /// not require exchanging dynamic commands with the device.
  virtual DeviceInformation getStaticConfiguration() = 0;
  ///
  /// @brief Submit a command to the device.
  ///
  /// This is an asynchronous call in respect with the response of the device.
  /// This call wil block only if the command queue to the device is full from
  /// the driver's perspective.
  virtual bool submitCommand() = 0;
  ///
  /// @brief Register callback for when a response arrives from the device.
  virtual bool registerResponseCallback() = 0;
  ///
  /// @brief Register callback for when an event arrives from the device
  virtual bool registerDeviceEventCallback() = 0;

  virtual bool alive() { return device_alive_; }

  /// @todo the following interface mimics the functionality of the origial card
  /// proxy this is transitional code that probably could be removed in the
  /// future.

  /// @brief Read device memory using MMIO
  virtual bool readDevMemMMIO(uintptr_t dev_addr, size_t size, void *buf) = 0;

  /// @brief Write device memory using MMIO
  virtual bool writeDevMemMMIO(uintptr_t dev_addr, size_t size, const void *buf) = 0;

  /// @brief Read device memory using MMIO
  virtual bool readDevMemDMA(uintptr_t dev_addr, size_t size, void *buf) = 0;

  /// @brief Write device memory using MMIO
  virtual bool writeDevMemDMA(uintptr_t dev_addr, size_t size, const void *buf) = 0;

  /// @brief Return the maximum size of a mailbox message
  virtual ssize_t mboxMsgMaxSize() const = 0;

  /// @brief Return the maximum size of message
  virtual uint16_t virtQueueMsgMaxSize() { return maxMsgSize_; }

  /// @brief Return the virtual queues count
  virtual uint8_t virtQueueCount() { return queueCount_; }

  /// @brief Return the buffer count in a queue
  virtual uint16_t virtQueueBufCount() { return queueBufCount_; }

  /// @brief Discover virtual queues info and update maxMsgSize_, queueCount_ and
  /// queueBufCount_ from remote
  /// @param[wait_time] wait_time. Time to wait for the discovery to complete
  ///
  /// @return True if discovery is successful
  virtual bool virtQueuesDiscover(TimeDuration wait_time) = 0;

  /// @brief Virtual Queue write message
  ///
  /// @param[in] data Pointer to queue message
  /// @param[in] size Size in bytes of the queue message to write
  /// @param[in] queueId index of the queue to write the message to
  ///
  /// @return True if write is successful
  virtual bool virtQueueWrite(const void *data, ssize_t size,
                              uint8_t queueId) = 0;

  /// @Brief Virtual Queue message read
  ///
  /// @param[inout] data Pointer to buffer that will hold the size of the
  /// message we want to read
  /// @param[in] size Size of the message to read.
  /// @param[in] queueId index of the queue to read the message from
  ///
  /// @return Size of the message read. Zero if the read was not succesful.
  virtual ssize_t virtQueueRead(void *data, ssize_t size, uint8_t queueId) = 0;

  /// @Brief Wait for Epoll Events
  ///
  /// @param[out] sq_bitmap Bitmap with bits indicating availability of submission
  /// queue
  /// @param[out] cq_bitmap Bitmap with bits indicating availability of completion
  /// queue
  ///
  /// @return True if epoll event occur
  virtual bool waitForEpollEvents(uint32_t &sq_bitmap, uint32_t &cq_bitmap) = 0;

  /// @brief MailBox write message
  ///
  /// @param[in] data Pointer to mailbox message
  /// @param[in] size Size in bytes of the mailbox message to write
  ///
  /// @return True iff write is successfull
  virtual bool mb_write(const void *data, ssize_t size) = 0;

  /// @Brief Mailbox message read
  ///
  /// @param[inout] data  Pointer to buffer that will hold the size of the
  /// message we want to read
  /// @param[in] size  Size of the mailbox message to read.
  ///
  /// @return Size of the message read. Zero iff the read was not succesfull.
  virtual ssize_t mb_read(void *data, ssize_t size) = 0;

  /// @brief Shutdown the device
  virtual bool shutdown() = 0;

  /// @brief Factory function that will generate the appropriate target device
  static std::unique_ptr<DeviceTarget> deviceFactory(TargetType target, int index);
  /// @brief Return the type of device to allocate as specified by the command
  /// line arguments
  /// FIXME this functions should not be released in production
  static TargetType deviceToCreate();

  /// @brief Force the type of device to create
  ///
  /// @param[in] String with the device type, allowed values are: pcie,
  /// sysemu_card_proxy, sysemu_grpc, device_grpc
  /// @returns True if valid value was passed
  static bool setDeviceType(const std::string &device_type);

protected:
  int index_; ///< Index fo the device
  bool device_alive_ = false; ///< Flag to guard that the device has been initialize correctly and
                              ///< gate any incorrect re-initialization or de-initialization
  uint16_t maxMsgSize_;  ///< Maximum msg size
  uint8_t queueCount_;       ///< Count of virtual queues
  uint16_t queueBufCount_;    ///< Count of buffers in a virtual queue
};
} // namespace device
} // namespace et_runtime

#endif // ET_RUNTIME_DEVICE_TARGET_H
