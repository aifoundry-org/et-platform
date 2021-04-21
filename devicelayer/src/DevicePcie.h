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
#include "deviceLayer/IDeviceLayer.h"

namespace dev {

class DevicePcie : public IDeviceLayer {
public:
  explicit DevicePcie();
  ~DevicePcie();

  // IDeviceAsync
  bool sendCommandMasterMinion(int device, int sqIdx, std::byte* command, size_t commandSize) override;
  void setSqThresholdMasterMinion(int device, int sqIdx, uint32_t bytesNeeded) override;
  void waitForEpollEventsMasterMinion(int device, uint64_t& sq_bitmap, bool& cq_available,
                                      std::chrono::seconds timeout = std::chrono::seconds(10)) override;
  bool receiveResponseMasterMinion(int device, std::vector<std::byte>& response) override;

  bool sendCommandServiceProcessor(int device, std::byte* command, size_t commandSize) override;
  void setSqThresholdServiceProcessor(int device, uint32_t bytesNeeded) override;
  void waitForEpollEventsServiceProcessor(int device, bool& sq_available, bool& cq_available,
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
  int devIdx_;
  uint16_t mmSqCount_;
  uint16_t spSqMaxMsgSize_;
  uint16_t mmSqMaxMsgSize_;
  uint64_t dramBase_;
  uint64_t dramSize_;
  int fdOps_, epFdOps_;
  int fdMgmt_, epFdMgmt_;
};
} // namespace dev
