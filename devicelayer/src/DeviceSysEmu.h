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
#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <future>
#include <sw-sysemu/ISysEmu.h>
#include <thread>
#include <unordered_map>

namespace dev {

struct CircBuffCb {
  uint64_t head_offset; /**< Offset of the circular buffer to write data to */
  uint64_t tail_offset; /**< Offset of the circular buffer to read data from */
  uint64_t length;      /**< Total length (in bytes) of the circular buffer */
  uint8_t buffer_ptr[]; /**< Flexible array to access circular buffer memory located just after circ_buff_cb */
} __attribute__((__packed__));

class DeviceSysEmu : public IDeviceLayer {
public:
  explicit DeviceSysEmu(const emu::SysEmuOptions& options);
  ~DeviceSysEmu();

  // IDeviceAsync
  bool sendCommandMasterMinion(int device, int sqIdx, std::byte* command, size_t commandSize) override;
  void setSqThresholdMasterMinion(int device, int sqIdx, uint32_t bytesNeeded) override;
  void waitForEpollEventsMasterMinion(int device, uint64_t& sqBitmap, bool& cqAvailable,
                                      std::chrono::seconds timeout = std::chrono::seconds(10)) override;
  bool receiveResponseMasterMinion(int device, std::vector<std::byte>& response) override;

  bool sendCommandServiceProcessor(int device, std::byte* command, size_t commandSize) override;
  void setSqThresholdServiceProcessor(int device, uint32_t bytesNeeded) override;
  void waitForEpollEventsServiceProcessor(int device, bool& sqAvailable, bool& cqAvailable,
                                          std::chrono::seconds timeout = std::chrono::seconds(10)) override;
  bool receiveResponseServiceProcessor(int device, std::vector<std::byte>& response) override;

  // IDeviceSync
  int getDevicesCount() const override;
  int getSubmissionQueuesCount(int device) const override;
  size_t getSubmissionQueueSizeMasterMinion(int device) const override;
  size_t getSubmissionQueueSizeServiceProcessor(int device) const override;
  int getDmaAlignment() const override;
  uint64_t getDramSize() const override;
  uint64_t getDramBaseAddress() const override;

private:
  struct QueueInfo {
    uint64_t bufferAddress_;
    size_t size_;
    uint32_t thresholdBytes_;
  };

  bool sendCommand(QueueInfo& queue, std::byte* command, size_t commandSize, bool& clearEvent);
  bool receiveResponse(QueueInfo& queue, std::vector<std::byte>& response, bool& clearEvent);

  bool checkForEventEPOLLIN(QueueInfo& queueInfo);
  bool checkForEventEPOLLOUT(QueueInfo& queueInfo);

  void startHostMemoryAccessThread();
  void setupMasterMinion();
  void setupServiceProcessor();

  std::atomic<bool> isRunning_ = true;
  std::unique_ptr<emu::ISysEmu> sysEmu_;
  SysEmuHostListener hostListener_;

  std::array<uint64_t, 8> barAddress_;

  std::mutex interruptMutex_;
  std::array<std::condition_variable, 32> interruptBlock_;
  std::thread interruptListener_;

  uint64_t spDevIntfRegAddr_;
  uint64_t mmDevIntfRegAddr_;

  std::atomic<uint64_t> mmSqBitmap_ = 0;
  std::atomic<bool> mmCqReady_ = false;

  std::atomic<bool> spSqReady_ = false;
  std::atomic<bool> spCqReady_ = false;

  MM_DEV_INTF_REG mmInfo_;
  SP_DEV_INTF_REG spInfo_;

  // TODO there should be a vector for each device ...
  std::vector<QueueInfo> submissionQueuesMM_;
  QueueInfo completionQueueMM_;

  QueueInfo submissionQueueSP_;
  QueueInfo completionQueueSP_;
};
} // namespace dev
