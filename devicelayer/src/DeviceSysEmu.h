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
#include "DevInfReg/MM_DIR.h"
#include "DevInfReg/SP_DIR.h"
#include "SysEmuHostListener.h"
#include "deviceLayer/IDeviceLayer.h"

#include <chrono>
#include <sw-sysemu/ISysEmu.h>

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <future>
#include <thread>
#include <unordered_map>

namespace dev {

struct CircBuffCb {
  uint64_t head_offset; /**< Offset of the circular buffer to write data to */
  uint64_t tail_offset; /**< Offset of the circular buffer to read data from */
  uint64_t length;      /**< Total length (in bytes) of the circular buffer */
  uint64_t pad;         /**< Padding to make the struct 32-bytes aligned */
} __attribute__((__packed__));

class DeviceSysEmu : public IDeviceLayer {
public:
  explicit DeviceSysEmu(const emu::SysEmuOptions& options);
  ~DeviceSysEmu();

  // IDeviceAsync
  bool sendCommandMasterMinion(int device, int sqIdx, std::byte* command, size_t commandSize, CmdFlagMM flags) override;
  void setSqThresholdMasterMinion(int device, int sqIdx, uint32_t bytesNeeded) override;
  void waitForEpollEventsMasterMinion(int device, uint64_t& sqBitmap, bool& cqAvailable,
                                      std::chrono::milliseconds timeout = std::chrono::seconds(10)) override;
  bool receiveResponseMasterMinion(int device, std::vector<std::byte>& response) override;

  bool sendCommandServiceProcessor(int device, std::byte* command, size_t commandSize, CmdFlagSP flags) override;
  void setSqThresholdServiceProcessor(int device, uint32_t bytesNeeded) override;
  void waitForEpollEventsServiceProcessor(int device, bool& sqAvailable, bool& cqAvailable,
                                          std::chrono::milliseconds timeout = std::chrono::seconds(10)) override;
  bool receiveResponseServiceProcessor(int device, std::vector<std::byte>& response) override;

  // IDeviceSync
  DmaInfo getDmaInfo(int device) const override;
  int getDevicesCount() const override;
  int getSubmissionQueuesCount(int device) const override;
  DeviceState getDeviceStateMasterMinion(int device) const override;
  DeviceState getDeviceStateServiceProcessor(int device) const override;
  size_t getSubmissionQueueSizeMasterMinion(int device) const override;
  size_t getSubmissionQueueSizeServiceProcessor(int device) const override;
  int getDmaAlignment() const override;
  uint64_t getDramSize(int device) const override;
  uint64_t getDramBaseAddress(int device) const override;
  void* allocDmaBuffer(int device, size_t sizeInBytes, bool writeable) override;
  void freeDmaBuffer(void* dmaBuffer) override;
  bool getTraceBufferServiceProcessor(int device, TraceBufferType traceType, std::vector<std::byte>& traceBuf) override;
  size_t getTraceBufferSizeMasterMinion(int device, TraceBufferType traceType) override;
  DeviceConfig getDeviceConfig(int device) override;
  int getActiveShiresNum(int device) override;
  uint32_t getFrequencyMHz(int device) override;
  int updateFirmwareImage(int device, std::vector<unsigned char>& fwImage) override;
  size_t getFreeCmaMemory() const override;
  std::string getDeviceAttribute(int device, std::string relAttrPath) const override;
  void clearDeviceAttributes(int device, std::string relGroupPath) const override;
  void reinitDeviceInstance(int device, bool masterMinionOnly, std::chrono::milliseconds timeout) override;
  void hintInactivity(int device) override;
  bool checkP2pDmaCompatibility(int deviceA, int deviceB) const override;

private:
  struct QueueInfo {
    uint64_t bufferAddress_;
    size_t size_;
    uint32_t thresholdBytes_;
    CircBuffCb cb_;
  };

  friend struct Checker;

  struct Checker {
    explicit Checker(const DeviceSysEmu& parent)
      : parent_(parent){};

    ~Checker() {
      parent_.checkSysemuLastError();
    }

    const DeviceSysEmu& parent_;
  };

  void checkSysemuLastError() const;

  bool sendCommand(QueueInfo& queue, std::byte* command, size_t commandSize, bool& clearEvent);
  bool receiveResponse(QueueInfo& queue, std::vector<std::byte>& response, bool& clearEvent);

  bool checkForEventEPOLLIN(const QueueInfo& queueInfo) const;
  bool checkForEventEPOLLOUT(const QueueInfo& queueInfo) const;
  bool foundEventsMasterMinion(uint64_t& sqBitmap, bool& cqAvailable);
  bool foundEventsServiceProcessor(bool& sqAvailable, bool& cqAvailable);

  void startHostMemoryAccessThread();
  void setupMasterMinion();
  void setupServiceProcessor();

  std::atomic<bool> isRunning_ = true;
  std::unique_ptr<emu::ISysEmu> sysEmu_;
  SysEmuHostListener hostListener_;

  std::array<uint64_t, 8> barAddress_;

  std::mutex mutex_;
  std::thread interruptListener_;

  uint64_t spDevIntfRegAddr_;
  uint64_t mmDevIntfRegAddr_;

  std::condition_variable mmEpollBlock_;
  uint32_t mmIntrptBitmap_ = 0;
  std::atomic<uint64_t> mmSqBitmap_ = 0;
  std::atomic<bool> mmCqReady_ = false;

  std::condition_variable spEpollBlock_;
  uint32_t spIntrptBitmap_ = 0;
  std::atomic<bool> spSqReady_ = false;
  std::atomic<bool> spCqReady_ = false;

  MM_DEV_INTF_REG mmInfo_;
  SP_DEV_INTF_REG spInfo_;

  std::vector<QueueInfo> submissionQueuesMM_;
  std::vector<QueueInfo> hpSubmissionQueuesMM_;
  QueueInfo completionQueueMM_;

  QueueInfo submissionQueueSP_;
  QueueInfo completionQueueSP_;
};
} // namespace dev
