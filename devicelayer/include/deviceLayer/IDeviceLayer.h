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

#include <hostUtils/debug/StackException.h>
#include <sw-sysemu/SysEmuOptions.h>

#include <chrono>
#include <cstddef>
#include <memory>
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

/// NOTE: New enum/struct entities should be added in the end for backward compatibility

/// \brief This struct contains device configured information
struct DeviceConfig {
  enum class FormFactor { PCIE, M2 };
  enum class ArchRevision { ETSOC1, PANTERO, GEPARDO };
  FormFactor formFactor_;              ///< device form factor
  uint8_t tdp_;                        ///< TDP in Watts
  uint32_t totalL3Size_;               ///< total size of L3 cache in KBytes
  uint32_t totalL2Size_;               ///< total size of L2 cache in KBytes
  uint32_t totalScratchPadSize_;       ///< total scratchpad size in KBytes
  uint8_t cacheLineSize_;              ///< chache line size, in Bytes
  uint8_t numL2CacheBanks_;            ///< num L2 cache banks
  uint32_t ddrBandwidth_;              ///< DDR bandwidth in MBytes/s
  uint32_t minionBootFrequency_;       ///< minion boot frequency in Mhz
  uint32_t computeMinionShireMask_;    ///< mask which indicates what are the compute minion shires
  uint8_t spareComputeMinionoShireId_; ///< spare compute minion Shire ID
  ArchRevision archRevision_;          ///< architecture revision
  uint8_t physDeviceId_;               ///< physical device ID
};

/// \brief This struct contains the limitations / optimal DMA parameters.
struct DmaInfo {
  uint64_t maxElementSize_;  ///< maximum amount of memory that can be transfer per each DMA command entry
  uint64_t maxElementCount_; ///< max number of DMA entries per DMA command
};

/// \brief This enum contains possible device states
enum class DeviceState { Ready, PendingCommands, NotResponding, ResetInProgress, NotReady, Undefined };

/// \brief This struct contains possible command flags for SP
struct CmdFlagSP {
  bool isMmReset_ = false;
  bool isEtsocReset_ = false;
};

/// \brief This struct contains possible command flags for MM
struct CmdFlagMM {
  bool isDma_ = false;
  bool isHpSq_ = false;
  bool isP2pDma_ = false;
};

/// \brief This enum contains possible trace buffer types to extract from SP
enum class TraceBufferType {
  TraceBufferSP = 0,
  TraceBufferMM,
  TraceBufferCM,
  TraceBufferSPStats,
  TraceBufferMMStats,
  TraceBufferTypeNum
};

class Exception : public dbg::StackException {
  using dbg::StackException::StackException;
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
  /// @param[in] flags indicates command options as defined by `CmdFlagMM`. Needed for PCIe deviceLayer
  ///
  /// @returns false if there was not enough space to send the command, true otherwise
  ///
  virtual bool sendCommandMasterMinion(int device, int sqIdx, std::byte* command, size_t commandSize,
                                       CmdFlagMM flags) = 0;

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
  /// @param[out] sq_bitmap indicates which submission queue(s) became available since the last call.
  /// @param[out] cq_available indicates if Completion Queue became available since the last call.
  /// @param[in] timeout the operation will be aborted if no event happen in given timeout
  ///
  virtual void waitForEpollEventsMasterMinion(int device, uint64_t& sq_bitmap, bool& cq_available,
                                              std::chrono::milliseconds timeout = std::chrono::seconds(10)) = 0;

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
  /// @param[in] flags indicates command options as defined by `CmdFlagSO`. Needed for PCIe deviceLayer
  /// implementations
  ///
  /// @returns false if there was not enough space to send the command, true otherwise
  ///
  virtual bool sendCommandServiceProcessor(int device, std::byte* command, size_t commandSize, CmdFlagSP flags) = 0;

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
  /// @param[out] sq_available indicates if submission queue became available since the last call.
  /// @param[out] cq_available indicates if Completion Queue became available since the last call.
  /// @param[in] timeout the operation will be aborted if no event happen in given timeout
  ///
  virtual void waitForEpollEventsServiceProcessor(int device, bool& sq_available, bool& cq_available,
                                                  std::chrono::milliseconds timeout = std::chrono::seconds(10)) = 0;

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
  /// \brief Returns the DMA parameters.
  ///
  /// @param[in] device indicating which device to get the dma parameters from.
  ///
  /// @returns \ref DmaInfo contains the dma parameters.
  ///
  virtual DmaInfo getDmaInfo(int device) const = 0;

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

  /// \brief Gets device state associated to given Master Minion device and returns enum DeviceState
  /// If the caller didn't send any command in current session and received DeviceState::PendingCommands,
  /// then caller may choose to reset the device by sending abort commands and discard any previous
  /// responses. If received DeviceState::NotResponding, then caller should try to recover by sending
  /// abort commands. Caller should be careful in polling on this method because it directly gets status
  /// from device and hence can be expensive in I/O transactions with device.
  ///
  /// @returns enum DeviceState
  ///
  virtual DeviceState getDeviceStateMasterMinion(int device) const = 0;

  /// \brief Gets device state associated to given Service Processor device and returns enum DeviceState
  /// If the caller didn't send any command in current session and received DeviceState::PendingCommands,
  /// then caller may choose to reset the device by sending abort commands and discard any previous
  /// responses. If received DeviceState::NotResponding, then caller should try to recover by sending
  /// abort commands. Caller should be careful in polling on this method because it directly gets status
  /// from device and hence can be expensive in I/O transactions with device.
  ///
  /// @returns enum DeviceState
  ///
  virtual DeviceState getDeviceStateServiceProcessor(int device) const = 0;

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

  /// \brief Receives Master Minion's trace buffer sizes associated to given device
  ///
  /// @param[in] device indicating which device to receive the response from.
  /// @param[in] traceType indicating which trace buffer type to pull from device.
  /// Supported types are TraceBufferType::TraceBufferSP, TraceBufferType::TraceBufferMM,
  /// TraceBufferType::CM.
  ///
  /// @returns size of trace buffer.
  ///
  virtual size_t getTraceBufferSizeMasterMinion(int device, TraceBufferType traceType) = 0;

  /// \brief Receives Service Processor's trace buffer associated to given device in traceBuf
  ///
  /// @param[in] device indicating which device to receive the buffer from.
  /// @param[in] traceType indicating the type of trace buffer whose size is needed.
  /// Supported types are TraceBufferType::TraceBufferMM, TraceBufferType::TraceBufferCM.
  /// @param[out] traceBuf buffer containing Service Processor's trace buffer.
  ///
  /// @returns false if there was no buffer to be received.
  ///
  virtual bool getTraceBufferServiceProcessor(int device, TraceBufferType traceType,
                                              std::vector<std::byte>& traceBuf) = 0;

  /// \brief Writes firmware image on DRAM of given device
  ///
  /// @param[in] device indicating which device's DRAM to be written.
  /// @param[in] firmware image to be written
  ///
  /// @returns bytes written
  ///
  virtual int updateFirmwareImage(int device, std::vector<unsigned char>& fwImage) = 0;

  /// \brief Returns the DMA alignment requirement
  ///
  /// @returns User DRAM alignment(in bits)
  ///
  virtual int getDmaAlignment() const = 0;

  /// \brief Returns the DRAM available size (in bytes)
  ///
  /// @param[in] device the device which will be queried
  ///
  /// @returns DRAM size (in bytes)
  ///
  virtual uint64_t getDramSize(int device) const = 0;

  /// \brief Returns the DRAM base address
  ///
  /// @param[in] device the device which will be queried
  ///
  /// @returns DRAM Host Managed base address
  ///
  virtual uint64_t getDramBaseAddress(int device) const = 0;

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

  /// \brief returns a \ref DeviceConfig struct containing device configured details
  ///
  /// @param[in] device the device which will be queried
  ///
  virtual DeviceConfig getDeviceConfig(int device) = 0;

  /// \brief returns a number of active shires (number of bits set to 1 in shire mask)
  ///
  /// @param[in] device the device which will be queried
  ///
  virtual int getActiveShiresNum(int device) = 0;

  /// \brief returns device frequency
  ///
  /// @param[in] device the device which will be queried
  ///
  virtual uint32_t getFrequencyMHz(int device) = 0;

  /// \brief returns the available CMA memory in the system. That memory is allocated using \ref allocDmaBuffer
  ///
  /// @returns the size in bytes of allocatable CMA memory
  ///
  /// DEPRECATED: To be removed
  virtual size_t getFreeCmaMemory() const = 0;

  /// \brief Get device statistics by reading the attribute files
  ///
  /// @param[in] device the device which will be queried
  /// @param[in] relAttrPath the attribute path relative to directory:
  /// /sys/bus/pci/devices/[[[[<domain>]:]<bus>]:][<slot>][.[<func>]]
  ///
  /// @returns the content of requested attribute file
  ///
  virtual std::string getDeviceAttribute(int device, std::string relAttrPath) const = 0;

  /// \brief Clear device statistics in the attribute files
  ///
  /// @param[in] device the device which will be queried
  /// @param[in] relGroupPath the relative group path of attributes
  /// The attribute file /sys/bus/pci/devices/[[[[<domain>]:]<bus>]:][<slot>][.[<func>]]/<relGroupPath>/clear
  /// should exist and this call will clear all attributes in <relGroupPath> directory.
  ///
  virtual void clearDeviceAttributes(int device, std::string relGroupPath) const = 0;

  /// \brief Reinitialize the device master minion or both master minion and service processor
  /// NOTE: During master minion reinitialization other master minion APIs cannot be used. During both
  /// devices' reinitialization no other API should be used.
  ///
  /// @param[in] device the device to be reinitialized
  /// @param[in] masterMinionOnly if true only master minion will be reinitialized otherwise both master minion
  /// and service processor will be reinitialized.
  /// @param[in] timeout the operation will be aborted if time exceeds given timeout
  ///
  virtual void reinitDeviceInstance(int device, bool masterMinionOnly, std::chrono::milliseconds timeout) = 0;

  /// \brief Hints the underlying device that we don't have anything to process, so it could potentially go idle / low
  /// power consumption.
  ///
  /// @param[in] device the device to be hinted
  ///
  virtual void hintInactivity(int device) = 0;

  /// \brief Evaluates the Peer-to-Peer DMA compatibility of given devices.
  ///
  /// @param[in] deviceA the peer device A
  /// @param[in] deviceB the peer device B
  ///
  /// @returns true if deviceA and deviceB are compatible for P2P DMA, false otherwise
  ///
  virtual bool checkP2pDmaCompatibility(int deviceA, int deviceB) const = 0;
};

class IDeviceLayer : public IDeviceAsync, public IDeviceSync {
public:
  /// \brief Factory method to instantiate a IDeviceApi implementation, based on sysemu backend
  ///
  /// @param[in] options this contains all sysemu parametrizable options see \ref emu::SysEmuOptions
  /// @param[in] numDevices number of devices which one would like to emulate. Defaults to 1
  ///
  /// @returns std::unique_ptr<IDeviceApi> is the IDeviceApi implementation
  ///
  static std::unique_ptr<IDeviceLayer> createSysEmuDeviceLayer(const emu::SysEmuOptions& options,
                                                               uint8_t numDevices = 1);

  /// \brief Factory method to instantiate a IDeviceApi implementation, based on sysemu backend
  ///
  /// @param[in] options this contains all sysemu parametrizable options see \ref emu::SysEmuOptions. For each
  /// sysemuOptions it will create an emulation device. This method is intended to provide different sysemu parameters
  /// for each sysemu device simulated.
  ///
  /// @returns std::unique_ptr<IDeviceApi> is the IDeviceApi implementation
  ///
  static std::unique_ptr<IDeviceLayer> createSysEmuDeviceLayer(std::vector<emu::SysEmuOptions> options);

  /// \brief Factory method to instantiate a IDeviceApi implementation, based on Pcie backend
  ///
  /// @param[in] enableMasterMinion if this is true, deviceLayer will attempt to open the operations port (Master
  /// Minion). If this is not enabled, then all Master Minion operations will fail.
  /// @param[in] enableServiceProcessor if this is true, deviceLayer will attempt to open the management port (Service
  /// Processor). If this is not enabled, then all Service Processor operations will fail.
  /// @returns std::unique_ptr<IDeviceApi> is the IDeviceApi implementation
  ///
  static std::unique_ptr<IDeviceLayer> createPcieDeviceLayer(bool enableMasterMinion = true,
                                                             bool enableServiceProcessor = false);

  /// \brief Virtual Destructor to enable polymorphic release of the IDeviceApi
  /// instances
  virtual ~IDeviceLayer() = default;
};

} // namespace dev
