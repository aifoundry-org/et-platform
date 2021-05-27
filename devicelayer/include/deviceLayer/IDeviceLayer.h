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
#include <sw-sysemu/SysEmuOptions.h>
#include <vector>

/// \defgroup device_api Device API
///
/// The device API provides an asynchronous API (command send and response) and a synchronous API (configuration
/// discovery methods).
/// The asynchronous API is meant to be used to send commands to a device in non-blocking way.
/// The synchronous API offers different methods which will return inmediatly, these methods are queries about hardware
/// configuration.
///
/// Thread safety: the API implementation shall support sending commands / receiving responses from the same thread to
/// master minion and potentially a different thread for the service processor. Hence, the API implementation will
/// guarantee thread safety only between 2 threads, each one using the master minion services and service processor
/// services. Further clarification: if 2+ threads consume services for the master minion, this can lead to data races.
/// Same for service processor.
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
  /// \brief Sends a command to the master minion. If the method returns false, the caller should try later when the
  /// queue has enough space indicated by availability from `waitForEpollEventsMasterMinion()`
  ///
  /// @param[in] device indicating which device to send the command.
  /// @param[in] sqIdx indicates which submission queue to send the command to.
  /// @param[in] command its a buffer which contains the command itself.
  /// @param[in] commandSize the size of the command + payload buffer.
  /// @param[in] isDma indicates if the command involves a DMA operation. Needed for PCIe deviceLayer implementations
  ///
  /// @returns false if there was not enough space to send the command, true otherwise
  ///
  virtual bool sendCommandMasterMinion(int device, int sqIdx, std::byte* command, size_t commandSize, bool isDma) = 0;

  /// \brief Set the submission queue availability threshold. Submission queue epoll event will be generated only if
  /// space on submission queue is greater or equal to this threshold set. Default threshold value is one forth of size
  /// of submission queue buffer returned by `getSubmissionQueueSizeMasterMinion()`.
  ///
  /// @param[in] device indicating which device to send the command.
  /// @param[in] sqIdx indicates which submission queue to set the threshold for.
  /// @param[in] bytesNeeded the number of bytes needed free on Submission Queue. Should be positive and within the size
  /// returned by `getSubmissionQueueSizeMasterMinion()`
  ///
  virtual void setSqThresholdMasterMinion(int device, int sqIdx, uint32_t bytesNeeded) = 0;

  /// \brief Blocks the caller thread until submission queues or Completion Queue is available for use.
  /// The operation will be aborted if no event happen in given timeout. In such case, sq_bitmap and cq_available will
  /// be both 0 and false.
  ///
  /// @param[in] device indicating which device to wait on for EPOLL events.
  /// @param[out] sq_bitmap indicates which submission queues are available.
  /// @param[out] cq_available indicates if Completion Queue is available.
  /// @param[in] timeout the operation will be aborted if no event happen in given timeout
  ///
  virtual void waitForEpollEventsMasterMinion(int device, uint64_t& sq_bitmap, bool& cq_available,
                                              std::chrono::seconds timeout = std::chrono::seconds(10)) = 0;

  /// \brief Receives a response from the device, a non-blocking interface. If the method returns false, the caller
  /// should try later when the queue has response available indicated by `waitForEpollEventsMasterMinion()`
  ///
  /// @param[in] device indicating which device to receive the response from.
  /// @param[out] response buffer containing the response.
  ///
  /// @returns false if there was no response to be received.
  ///
  virtual bool receiveResponseMasterMinion(int device, std::vector<std::byte>& response) = 0;

  /// \brief Sends a command to the service processor. If the method returns false, the caller should try later when the
  /// queue has enough space indicated by availability from `waitForEpollEventsServiceProcessor()`
  ///
  /// @param[in] device indicating which device to send the command.
  /// @param[in] command its a buffer which contains the command itself.
  /// @param[in] commandSize the size of the command + payload buffer.
  ///
  /// @returns false if there was not enough space to send the command, true otherwise
  ///
  virtual bool sendCommandServiceProcessor(int device, std::byte* command, size_t commandSize) = 0;

  /// \brief Set the submission queue availability threshold. Submission queue epoll event will be generated only if
  /// space on submission queue is greater or equal to this threshold set. Default threshold value is one forth of size
  /// of submission queue buffer returned by `getSubmissionQueueSizeServiceProcessor()`.
  ///
  /// @param[in] device indicating which device to send the command.
  /// @param[in] sqIdx indicates which submission queue to set the threshold for.
  /// @param[in] bytesNeeded the number of bytes needed free on Submission Queue. Should be positive and within the size
  /// returned by `getSubmissionQueueSizeServiceProcessor()`
  ///
  virtual void setSqThresholdServiceProcessor(int device, uint32_t bytesNeeded) = 0;

  /// \brief Blocks the caller thread until submission queue or completion queue is available for use.
  /// The operation will be aborted if no event happen in given timeout. In such case, sq_bitmap and cq_available will
  /// be both 0 and false.
  ///
  /// @param[in] device indicating which device to wait on for EPOLL events.
  /// @param[out] sq_available indicates if Submission Queue is available.
  /// @param[out] cq_available indicates if Completion Queue is available.
  /// @param[in] timeout the operation will be aborted if no event happen in given timeout
  ///
  virtual void waitForEpollEventsServiceProcessor(int device, bool& sq_available, bool& cq_available,
                                                  std::chrono::seconds timeout = std::chrono::seconds(10)) = 0;

  /// \brief Receives a response from the device, a non-blocking interface. If the method returns false, the caller
  /// should try later when the queue has response available indicated by `waitForEpollEventsServiceProcessor()`
  ///
  /// @param[in] device indicating which device to receive the response from.
  /// @param[out] response buffer containing the response.
  ///
  /// @returns false if there was no response to be received.
  ///
  virtual bool receiveResponseServiceProcessor(int device, std::vector<std::byte>& response) = 0;

  /// \brief Virtual Destructor to enable polymorphic release of the IDeviceAsync instances.
  virtual ~IDeviceAsync() = default;
};

/// This should be a command Interface class which is shared between Ops and Management, although will be different.

class IDeviceSync {
public:
  /// \brief Returns the number of esperanto devices connected to the host
  ///
  /// @returns the number of devices
  ///
  virtual int getDevicesCount() const = 0;

  /// \brief Returns the number of submission queues associated to given device
  ///
  /// @returns the number of virtual queues
  ///
  virtual int getSubmissionQueuesCount(int device) const = 0;

  /// \brief Returns the submission queue buffer size in bytes associated to given device
  /// all submission queues on Master Minion are equal in size
  ///
  /// @returns the submission queue size in bytes
  ///
  virtual size_t getSubmissionQueueSizeMasterMinion(int device) const = 0;

  /// \brief Returns the submission queue buffer size in bytes associated to given device
  ///
  /// @returns the submission queue size in bytes
  ///
  virtual size_t getSubmissionQueueSizeServiceProcessor(int device) const = 0;

  /// \brief Returns the DMA alignment requirement
  ///
  /// @returns User DRAM alignment(in bits)
  ///
  virtual int getDmaAlignment() const = 0;

  /// \brief Returns the DRAM available size (in bytes)
  ///
  /// @returns DRAM size (in bytes)
  ///
  virtual uint64_t getDramSize() const = 0;

  /// \brief Returns the DRAM base address
  ///
  /// @returns DRAM Host Managed base address
  ///
  virtual uint64_t getDramBaseAddress() const = 0;

  /// \brief Virtual Destructor to enable polymorphic release of the IDeviceOpsSync
  /// instances
  virtual ~IDeviceSync() = default;


  /// \brief Allocates a consecutive chunk of memory, able to do DMA. This chunk of memory must be deallocated properly 
  ///  using the freeDmaBuffer function.
  ///
  /// @param[in] sizeInBytes size, in bytes, for the memory allocation. 
  /// @param[in] writeable indicates if the memory should be writeable or, if false, readonly.
  ///
  /// @returns a chunk of memory which is suitable to be used in DMA operations.
  ///
  virtual void* allocDmaBuffer(int device, size_t sizeInBytes, bool writeable) = 0; 

  /// \brief Deallocates a previously allocated dmaBuffer.
  ///
  /// @param[in] dmaBuffer the buffer to be deallocated.
  ///
  virtual void freeDmaBuffer(void* dmaBuffer) = 0;

};

class IDeviceLayer : public IDeviceAsync, public IDeviceSync {
public:
  /// \brief Factory method to instantiate a IDeviceApi implementation, based on sysemu backend
  ///
  /// @returns std::unique_ptr<IDeviceApi> is the IDeviceApi implementation
  ///
  static std::unique_ptr<IDeviceLayer> createSysEmuDeviceLayer(const emu::SysEmuOptions& options);

  /// \brief Factory method to instantiate a IDeviceApi implementation, based on Pcie backend
  ///
  /// @param[in] enableMasterMinion if this is true, deviceLayer will attempt to open the operations port (Master
  /// Minion). If this is not enabled, then all Master Minion operations will fail.
  /// @param[in] enableServiceProcessor if this is true, deviceLayer will attempt to open the management port (Service
  /// Processor). If this is not enabled, then all Service Processor operations will fail.
  /// @returns std::unique_ptr<IDeviceApi> is the IDeviceApi implementation
  ///
  static std::unique_ptr<IDeviceLayer> createPcieDeviceLayer(bool enableMasterMinion = true,
                                                             bool enableServiceProcessor = true);

  /// \brief Virtual Destructor to enable polymorphic release of the IDeviceApi
  /// instances
  virtual ~IDeviceLayer() = default;
};

} // namespace dev
