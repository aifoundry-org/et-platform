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
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <esperanto/device-apis/operations-api/device_ops_api_cxx.h>
#include <mutex>
#include <queue>
#include <string>
namespace dev {
class IDeviceLayerFake : public IDeviceLayer {
  std::queue<device_ops_api::rsp_header_t> responsesMasterMinion_;
  std::queue<device_ops_api::dev_mgmt_rsp_header_t> responsesServiceProcessor_;
  std::condition_variable cvMm_;
  std::condition_variable cvSp_;
  std::mutex mmMutex_;
  std::mutex spMutex_;

public:
  bool sendCommandMasterMinion(int, int, std::byte* command, size_t, bool, bool) override {
    std::lock_guard<std::mutex> lock(mmMutex_);
    auto cmd = reinterpret_cast<device_ops_api::cmn_header_t*>(command);
    device_ops_api::rsp_header_t rsp;
    rsp.rsp_hdr.tag_id = cmd->tag_id;
    switch (cmd->msg_id) {
    case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DMA_WRITELIST_CMD:
      rsp.rsp_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_RSP;
      break;
    case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_CMD:
      rsp.rsp_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DMA_WRITELIST_RSP;
      break;
    case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_CMD:
      rsp.rsp_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_RSP;
      break;
    case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DMA_READLIST_CMD:
      rsp.rsp_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DMA_READLIST_RSP;
      break;
    case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_CMD:
      rsp.rsp_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_RSP;
      break;
    case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_ABORT_CMD:
      rsp.rsp_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_ABORT_RSP;
      break;
    case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_KERNEL_ABORT_CMD:
      rsp.rsp_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_KERNEL_ABORT_RSP;
      break;
    default:
      throw Exception("Please, add command with msg_id: " + std::to_string(cmd->msg_id));
    }
    responsesMasterMinion_.push(rsp);
    return true;
  }

  void setSqThresholdMasterMinion(int, int, uint32_t) override {
  }

  void waitForEpollEventsMasterMinion(int, uint64_t& sq_bitmap, bool& cq_available,
                                      std::chrono::seconds timeout) override {
    std::unique_lock<std::mutex> lock(mmMutex_, std::defer_lock);
    while (!lock.try_lock()) {
      // spin-lock
    }
    cvMm_.wait_for(lock, timeout, [this] { return !responsesMasterMinion_.empty(); });
    cq_available = true;
    sq_bitmap = 0xFFFFFFFFFFFFFFFF;
  }

  bool receiveResponseMasterMinion(int, std::vector<std::byte>& response) override {
    std::unique_lock<std::mutex> lock(mmMutex_, std::defer_lock);
    while (!lock.try_lock()) {
      // spin-lock
    }

    if (!responsesMasterMinion_.empty()) {
      response.resize(sizeof(device_ops_api::rsp_header_t));
      std::memcpy(response.data(), &responsesMasterMinion_.front(), sizeof(device_ops_api::rsp_header_t));
      responsesMasterMinion_.pop();
      return true;
    }
    return false;
  }

  bool sendCommandServiceProcessor(int, std::byte* command, size_t) override {
    std::unique_lock<std::mutex> lock(spMutex_, std::defer_lock);
    while (!lock.try_lock()) {
      // spin-lock
    }
    auto cmd = reinterpret_cast<device_ops_api::cmn_header_t*>(command);
    device_ops_api::dev_mgmt_rsp_header_t rsp;
    rsp.rsp_hdr.tag_id = cmd->tag_id;
    responsesServiceProcessor_.push(rsp);
    return true;
  }

  void setSqThresholdServiceProcessor(int, uint32_t) override{};

  void waitForEpollEventsServiceProcessor(int, bool& sq_available, bool& cq_available,
                                          std::chrono::seconds timeout) override {
    std::unique_lock lock(spMutex_);
    cvMm_.wait_for(lock, timeout, [this] { return !responsesServiceProcessor_.empty(); });
    sq_available = cq_available = true;
  }

  bool receiveResponseServiceProcessor(int, std::vector<std::byte>& response) override {
    std::unique_lock<std::mutex> lock(spMutex_, std::defer_lock);
    while (!lock.try_lock()) {
      // spin-lock
    }
    if (!responsesServiceProcessor_.empty()) {
      response.resize(sizeof(device_ops_api::rsp_header_t));
      std::memcpy(response.data(), &responsesServiceProcessor_.front(), sizeof(device_ops_api::rsp_header_t));
      responsesServiceProcessor_.pop();
      return true;
    }
    return false;
  };

  int getDevicesCount() const override {
    return 1;
  };

  DeviceState getDeviceStateMasterMinion(int) const override {
    return DeviceState::Ready;
  };

  DeviceState getDeviceStateServiceProcessor(int) const override {
    return DeviceState::Ready;
  };

  int getSubmissionQueuesCount(int) const override {
    return 1;
  };

  size_t getSubmissionQueueSizeMasterMinion(int) const override {
    return 1024 * 1024 * 1024;
  };

  size_t getSubmissionQueueSizeServiceProcessor(int) const override {
    return 1024 * 1024 * 1024;
  };

  int getDmaAlignment() const override {
    return 64;
  };

  DmaInfo getDmaInfo() const override {
    DmaInfo info;
    info.maxElementCount_ = 4;
    info.maxElementSize_ = 128 << 20;
    return info;
  }

  uint64_t getDramSize() const override {
    return 1UL << (10 + 10 + 10 + 4);
  };

  uint64_t getDramBaseAddress() const override {
    return 0x8000;
  }
  void* allocDmaBuffer(int, size_t sizeInBytes, bool) override {
    return malloc(sizeInBytes);
  }
  void freeDmaBuffer(void* dmaBuffer) override {
    free(dmaBuffer);
  }
  bool getTraceBufferServiceProcessor(int, TraceBufferType, std::vector<std::byte>&) override {
    return false;
  }
  DeviceConfig getDeviceConfig(int) override {
    return DeviceConfig{};
  }
  int updateFirmwareImage(int device, std::vector<unsigned char>& fwImage) override {
    return 0;
  }
  size_t getFreeCmaMemory() const override {
    return 1ULL << 30;
  }
};

} // namespace dev
