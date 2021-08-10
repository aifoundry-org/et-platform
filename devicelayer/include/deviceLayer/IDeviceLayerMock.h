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
#include <gmock/gmock.h>

namespace dev {
class IDeviceLayerMock : public IDeviceLayer {
public:
  MOCK_METHOD5(sendCommandMasterMinion,
               bool(int device, int sqIdx, std::byte* command, size_t commandSize, bool isDma));
  MOCK_METHOD3(setSqThresholdMasterMinion, void(int device, int sqIdx, uint32_t bytesNeeded));
  MOCK_METHOD3(waitForEpollEventsMasterMinion, void(int device, uint64_t& sq_bitmap, bool& cq_available));
  MOCK_METHOD4(waitForEpollEventsMasterMinion,
               void(int device, uint64_t& sq_bitmap, bool& cq_available, std::chrono::seconds timeout));
  MOCK_METHOD2(receiveResponseMasterMinion, bool(int device, std::vector<std::byte>& response));
  MOCK_METHOD3(sendCommandServiceProcessor, bool(int device, std::byte* command, size_t commandSize));
  MOCK_METHOD2(setSqThresholdServiceProcessor, void(int device, uint32_t bytesNeeded));
  MOCK_METHOD3(waitForEpollEventsServiceProcessor, void(int device, bool& sq_available, bool& cq_available));
  MOCK_METHOD4(waitForEpollEventsServiceProcessor,
               void(int device, bool& sq_available, bool& cq_available, std::chrono::seconds timeout));
  MOCK_METHOD2(receiveResponseServiceProcessor, bool(int device, std::vector<std::byte>& response));
  MOCK_CONST_METHOD1(getSubmissionQueuesCount, int(int device));
  MOCK_CONST_METHOD1(getSubmissionQueueSizeMasterMinion, size_t(int device));
  MOCK_CONST_METHOD1(getSubmissionQueueSizeServiceProcessor, size_t(int device));
  MOCK_CONST_METHOD0(getDmaAlignment, int());
  MOCK_CONST_METHOD0(getDramSize, uint64_t());
  MOCK_CONST_METHOD0(getDramBaseAddress, uint64_t());
  MOCK_CONST_METHOD0(getDevicesCount, int());
  MOCK_METHOD3(allocDmaBuffer, void*(int, size_t sizeInBytes, bool));
  MOCK_METHOD1(freeDmaBuffer, void(void*));
  MOCK_METHOD2(getTraceBufferServiceProcessor, bool(int, std::vector<std::byte>&));
  MOCK_METHOD1(getDeviceInfo, DeviceInfo(int));
};
} // namespace dev