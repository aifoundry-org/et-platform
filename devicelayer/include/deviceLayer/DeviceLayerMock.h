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

#include "IDeviceLayer.h"

#include <chrono>
#include <gmock/gmock.h>
#include <type_traits>

using ::testing::A;

namespace dev {

class DeviceLayerMock : public IDeviceLayer {
public:
  explicit DeviceLayerMock() = default;
  explicit DeviceLayerMock(IDeviceLayer* delegate)
    : delegate_(delegate) {
  }
  MOCK_METHOD6(sendCommandMasterMinion,
               bool(int device, int sqIdx, std::byte* command, size_t commandSize, bool isDma, bool isHighPriority));
  MOCK_METHOD3(setSqThresholdMasterMinion, void(int device, int sqIdx, uint32_t bytesNeeded));
  MOCK_METHOD3(waitForEpollEventsMasterMinion, void(int device, uint64_t& sq_bitmap, bool& cq_available));
  MOCK_METHOD4(waitForEpollEventsMasterMinion,
               void(int device, uint64_t& sq_bitmap, bool& cq_available, std::chrono::milliseconds timeout));
  MOCK_METHOD2(receiveResponseMasterMinion, bool(int device, std::vector<std::byte>& response));
  MOCK_METHOD4(sendCommandServiceProcessor, bool(int device, std::byte* command, size_t commandSize, bool isMmReset));
  MOCK_METHOD2(setSqThresholdServiceProcessor, void(int device, uint32_t bytesNeeded));
  MOCK_METHOD3(waitForEpollEventsServiceProcessor, void(int device, bool& sq_available, bool& cq_available));
  MOCK_METHOD4(waitForEpollEventsServiceProcessor,
               void(int device, bool& sq_available, bool& cq_available, std::chrono::milliseconds timeout));
  MOCK_METHOD2(receiveResponseServiceProcessor, bool(int device, std::vector<std::byte>& response));
  MOCK_CONST_METHOD1(getSubmissionQueuesCount, int(int device));
  MOCK_CONST_METHOD1(getSubmissionQueueSizeMasterMinion, size_t(int device));
  MOCK_CONST_METHOD1(getSubmissionQueueSizeServiceProcessor, size_t(int device));
  MOCK_CONST_METHOD1(getDeviceStateMasterMinion, DeviceState(int device));
  MOCK_CONST_METHOD1(getDeviceStateServiceProcessor, DeviceState(int device));
  MOCK_CONST_METHOD0(getDmaAlignment, int());
  MOCK_CONST_METHOD0(getDramSize, uint64_t());
  MOCK_CONST_METHOD0(getDramBaseAddress, uint64_t());
  MOCK_CONST_METHOD0(getDevicesCount, int());
  MOCK_METHOD3(allocDmaBuffer, void*(int, size_t sizeInBytes, bool));
  MOCK_METHOD1(freeDmaBuffer, void(void*));
  MOCK_METHOD2(getTraceBufferSizeMasterMinion, size_t(int, TraceBufferType));
  MOCK_METHOD3(getTraceBufferServiceProcessor, bool(int, TraceBufferType, std::vector<std::byte>&));
  MOCK_METHOD1(getDeviceConfig, DeviceConfig(int));
  MOCK_METHOD1(getActiveShiresNum, int(int));
  MOCK_METHOD1(getFrequencyMHz, uint32_t(int));
  MOCK_METHOD2(updateFirmwareImage, int(int, std::vector<unsigned char>&));
  MOCK_CONST_METHOD0(getFreeCmaMemory, size_t());
  MOCK_CONST_METHOD1(getDmaInfo, DmaInfo(int));

  void Delegate() {
    ON_CALL(*this, sendCommandMasterMinion)
      .WillByDefault(
        [this](int device, int sqIdx, std::byte* command, size_t commandSize, bool isDma, bool isHighPriority) {
          return delegate_->sendCommandMasterMinion(device, sqIdx, command, commandSize, isDma, isHighPriority);
        });
    ON_CALL(*this, setSqThresholdMasterMinion).WillByDefault([this](int device, int sqIdx, uint32_t bytesNeeded) {
      delegate_->setSqThresholdMasterMinion(device, sqIdx, bytesNeeded);
    });
    ON_CALL(*this, waitForEpollEventsMasterMinion(A<int>(), A<uint64_t&>(), A<bool&>()))
      .WillByDefault([this](int device, uint64_t& sq_bitmap, bool& cq_available) {
        delegate_->waitForEpollEventsMasterMinion(device, sq_bitmap, cq_available, std::chrono::seconds(1));
      });
    ON_CALL(*this, waitForEpollEventsMasterMinion(A<int>(), A<uint64_t&>(), A<bool&>(), A<std::chrono::milliseconds>()))
      .WillByDefault([this](int device, uint64_t& sq_bitmap, bool& cq_available, std::chrono::milliseconds timeout) {
        delegate_->waitForEpollEventsMasterMinion(device, sq_bitmap, cq_available, timeout);
      });
    ON_CALL(*this, receiveResponseMasterMinion).WillByDefault([this](int device, std::vector<std::byte>& response) {
      return delegate_->receiveResponseMasterMinion(device, response);
    });
    ON_CALL(*this, sendCommandServiceProcessor)
      .WillByDefault([this](int device, std::byte* command, size_t commandSize, bool isMmReset) {
        return delegate_->sendCommandServiceProcessor(device, command, commandSize, isMmReset);
      });
    ON_CALL(*this, setSqThresholdServiceProcessor).WillByDefault([this](int device, uint32_t bytesNeeded) {
      delegate_->setSqThresholdServiceProcessor(device, bytesNeeded);
    });
    ON_CALL(*this, waitForEpollEventsServiceProcessor(A<int>(), A<bool&>(), A<bool&>()))
      .WillByDefault([this](int device, bool& sq_available, bool& cq_available) {
        delegate_->waitForEpollEventsServiceProcessor(device, sq_available, cq_available, std::chrono::seconds(1));
      });
    ON_CALL(*this, waitForEpollEventsServiceProcessor(A<int>(), A<bool&>(), A<bool&>(), A<std::chrono::milliseconds>()))
      .WillByDefault([this](int device, bool& sq_available, bool& cq_available, std::chrono::milliseconds timeout) {
        delegate_->waitForEpollEventsServiceProcessor(device, sq_available, cq_available, timeout);
      });
    ON_CALL(*this, receiveResponseServiceProcessor).WillByDefault([this](int device, std::vector<std::byte>& response) {
      return delegate_->receiveResponseServiceProcessor(device, response);
    });
    ON_CALL(*this, getSubmissionQueuesCount).WillByDefault([this](int device) {
      return delegate_->getSubmissionQueuesCount(device);
    });
    ON_CALL(*this, getSubmissionQueueSizeMasterMinion).WillByDefault([this](int device) {
      return delegate_->getSubmissionQueueSizeMasterMinion(device);
    });
    ON_CALL(*this, getSubmissionQueueSizeServiceProcessor).WillByDefault([this](int device) {
      return delegate_->getSubmissionQueueSizeServiceProcessor(device);
    });
    ON_CALL(*this, getDeviceStateMasterMinion).WillByDefault([this](int device) {
      return delegate_->getDeviceStateMasterMinion(device);
    });
    ON_CALL(*this, getDeviceStateServiceProcessor).WillByDefault([this](int device) {
      return delegate_->getDeviceStateServiceProcessor(device);
    });
    ON_CALL(*this, getDmaAlignment).WillByDefault([this]() { return delegate_->getDmaAlignment(); });
    ON_CALL(*this, getDramSize).WillByDefault([this]() { return delegate_->getDramSize(); });
    ON_CALL(*this, getDramBaseAddress).WillByDefault([this]() { return delegate_->getDramBaseAddress(); });
    ON_CALL(*this, getDevicesCount).WillByDefault([this]() { return delegate_->getDevicesCount(); });
    ON_CALL(*this, allocDmaBuffer).WillByDefault([this](int device, size_t sizeInBytes, bool writeable) {
      return delegate_->allocDmaBuffer(device, sizeInBytes, writeable);
    });
    ON_CALL(*this, freeDmaBuffer).WillByDefault([this](void* dmaBuffer) { delegate_->freeDmaBuffer(dmaBuffer); });
    ON_CALL(*this, getTraceBufferSizeMasterMinion).WillByDefault([this](int device, TraceBufferType traceType) {
      return delegate_->getTraceBufferSizeMasterMinion(device, traceType);
    });
    ON_CALL(*this, getTraceBufferServiceProcessor)
      .WillByDefault([this](int device, TraceBufferType traceType, std::vector<std::byte>& traceBuf) {
        return delegate_->getTraceBufferServiceProcessor(device, traceType, traceBuf);
      });
    ON_CALL(*this, getDeviceConfig).WillByDefault([this](int device) { return delegate_->getDeviceConfig(device); });
    ON_CALL(*this, updateFirmwareImage).WillByDefault([this](int device, std::vector<unsigned char>& fwImage) {
      return delegate_->updateFirmwareImage(device, fwImage);
    });
    ON_CALL(*this, getFreeCmaMemory).WillByDefault([this]() { return delegate_->getFreeCmaMemory(); });
    ON_CALL(*this, getDmaInfo).WillByDefault([this](int device) { return delegate_->getDmaInfo(device); });
  }

private:
  IDeviceLayer* delegate_ = nullptr;
};
} // namespace dev
