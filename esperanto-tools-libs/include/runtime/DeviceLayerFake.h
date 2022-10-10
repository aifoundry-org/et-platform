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
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <device-layer/IDeviceLayer.h>
#include <esperanto/device-apis/operations-api/device_ops_api_cxx.h>
#include <mutex>
#include <queue>
#include <string>

namespace dev {
class DeviceLayerFake : public IDeviceLayer {
  std::queue<device_ops_api::rsp_header_t> responsesMasterMinion_;
  std::queue<device_ops_api::dev_mgmt_rsp_header_t> responsesServiceProcessor_;
  std::condition_variable cvMm_;
  std::condition_variable cvSp_;
  std::mutex mmMutex_;
  std::mutex spMutex_;

public:
  bool sendCommandMasterMinion(int, int, std::byte* command, size_t, bool, bool) override {
    std::unique_lock lock(mmMutex_, std::defer_lock);
    while (!lock.try_lock()) {
      // spin-lock
    }
    auto cmd = reinterpret_cast<device_ops_api::cmn_header_t*>(command);
    device_ops_api::rsp_header_t rsp;
    rsp.rsp_hdr.tag_id = cmd->tag_id;
    switch (cmd->msg_id) {
    case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DMA_WRITELIST_CMD:
      rsp.rsp_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DMA_WRITELIST_RSP;
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
    case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONFIG_CMD:
      rsp.rsp_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONFIG_RSP;
      break;
    case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONTROL_CMD:
      rsp.rsp_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONTROL_RSP;
      break;
    case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_CM_RESET_CMD:
      rsp.rsp_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_CM_RESET_RSP;
      break;
    case device_ops_api::DEV_OPS_API_MID_CHECK_DEVICE_OPS_API_COMPATIBILITY_CMD:
      rsp.rsp_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_API_COMPATIBILITY_RSP;
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
                                      std::chrono::milliseconds timeout) override {
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

  bool sendCommandServiceProcessor(int, std::byte* command, size_t, bool) override {
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
                                          std::chrono::milliseconds timeout) override {
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

  DmaInfo getDmaInfo(int) const override {
    DmaInfo info;
    info.maxElementCount_ = 4;
    info.maxElementSize_ = 32 << 20;
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
  size_t getTraceBufferSizeMasterMinion(int, TraceBufferType traceType) override {
    if (traceType == TraceBufferType::TraceBufferMM) {
      return 1024 * 1024;
    } else if (traceType == TraceBufferType::TraceBufferCM) {
      return 8 * 1024 * 1024;
    }
    return 0;
  }
  bool getTraceBufferServiceProcessor(int, TraceBufferType, std::vector<std::byte>&) override {
    return false;
  }
  DeviceConfig getDeviceConfig(int) override {
    return DeviceConfig{
      DeviceConfig::FormFactor::PCIE, // Form factor
      25,                             // TDP
      32768,                          // Total L3 size in KBytes
      16384,                          // Total L2 size in KBytes
      81920,                          // Total L2scp size in KBytes
      64,                             // CacheLine alignment in Bytes
      4,                              // number of L2 cache banks
      128000,                         // ddr bandwidth
      1000,                           // Base frequency
      0xFFFFFFFF,                     // Compute minion mask
      32,                             // spare minion shire id
      0                               // arch revision (ETSOC)
    };
  }
  int getActiveShiresNum(int device) override {
    DeviceConfig config = getDeviceConfig(device);
    uint32_t shireMask = config.computeMinionShireMask_;
    uint32_t checkPattern = 0x00000001;
    size_t maskBits = 32;
    int cntActive = 0;

    for (size_t i = 0; i < maskBits; i++) {
      bool active = shireMask & (checkPattern << i);
      if (active) {
        cntActive++;
      }
    }
    return cntActive;
  }
  uint32_t getFrequencyMHz(int device) override {
    DeviceConfig config = getDeviceConfig(device);
    return config.minionBootFrequency_;
  }
  int updateFirmwareImage(int, std::vector<unsigned char>&) override {
    return 0;
  }
  size_t getFreeCmaMemory() const override {
    return 1ULL << 30;
  }
  std::string getDeviceAttribute(int, std::string) const override {
    return "";
  }
  void clearDeviceAttributes(int, std::string) const override {
    // No implementation
  }
  void hintInactivity(int) override {
    // No implementation
  }
};

} // namespace dev
