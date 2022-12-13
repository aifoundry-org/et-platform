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
#include "runtime/Types.h"
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
  std::unordered_map<int, std::queue<device_ops_api::rsp_header_t>> responsesMasterMinion_;
  std::unordered_map<int, std::queue<device_ops_api::dev_mgmt_rsp_header_t>> responsesServiceProcessor_;
  std::condition_variable cvMm_;
  std::condition_variable cvSp_;
  std::mutex mmMutex_;
  std::mutex spMutex_;
  const int numDevices_;

  void checkDevice(int device) const {
    if (device >= numDevices_ || device < 0) {
      throw Exception("Invalid device");
    }
  }

public:
  explicit DeviceLayerFake(int numDevices = 1)
    : numDevices_(numDevices) {
    if (numDevices <= 0) {
      throw Exception("Num devices needs to be > 0");
    }
    for (int i = 0; i < numDevices_; ++i) {
      responsesMasterMinion_[i] = {};
      responsesServiceProcessor_[i] = {};
    }
  }
  bool sendCommandMasterMinion(int device, int, std::byte* command, size_t, dev::CmdFlagMM) override {
    checkDevice(device);
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
    responsesMasterMinion_[device].push(rsp);
    return true;
  }

  void setSqThresholdMasterMinion(int, int, uint32_t) override {
    // does nothing in fake
  }

  void waitForEpollEventsMasterMinion(int device, uint64_t& sq_bitmap, bool& cq_available,
                                      std::chrono::milliseconds timeout) override {
    checkDevice(device);
    std::unique_lock<std::mutex> lock(mmMutex_, std::defer_lock);
    while (!lock.try_lock()) {
      // spin-lock
    }
    cvMm_.wait_for(lock, timeout, [this, device] { return !responsesMasterMinion_[device].empty(); });
    cq_available = true;
    sq_bitmap = 0xFFFFFFFFFFFFFFFF;
  }

  bool receiveResponseMasterMinion(int device, std::vector<std::byte>& response) override {
    checkDevice(device);
    std::unique_lock<std::mutex> lock(mmMutex_, std::defer_lock);
    while (!lock.try_lock()) {
      // spin-lock
    }

    if (!responsesMasterMinion_[device].empty()) {
      response.resize(sizeof(device_ops_api::rsp_header_t));
      std::memcpy(response.data(), &responsesMasterMinion_[device].front(), sizeof(device_ops_api::rsp_header_t));
      responsesMasterMinion_[device].pop();
      return true;
    }
    return false;
  }

  bool sendCommandServiceProcessor(int device, std::byte* command, size_t, CmdFlagSP) override {
    checkDevice(device);
    std::unique_lock<std::mutex> lock(spMutex_, std::defer_lock);
    while (!lock.try_lock()) {
      // spin-lock
    }
    auto cmd = reinterpret_cast<device_ops_api::cmn_header_t*>(command);
    device_ops_api::dev_mgmt_rsp_header_t rsp;
    rsp.rsp_hdr.tag_id = cmd->tag_id;
    responsesServiceProcessor_[device].push(rsp);
    return true;
  }

  void setSqThresholdServiceProcessor(int, uint32_t) override{};

  void waitForEpollEventsServiceProcessor(int device, bool& sq_available, bool& cq_available,
                                          std::chrono::milliseconds timeout) override {
    checkDevice(device);
    std::unique_lock lock(spMutex_);
    cvMm_.wait_for(lock, timeout, [this, device] { return !responsesServiceProcessor_[device].empty(); });
    sq_available = cq_available = true;
  }

  bool receiveResponseServiceProcessor(int device, std::vector<std::byte>& response) override {
    checkDevice(device);
    std::unique_lock<std::mutex> lock(spMutex_, std::defer_lock);
    while (!lock.try_lock()) {
      // spin-lock
    }
    if (!responsesServiceProcessor_.empty()) {
      response.resize(sizeof(device_ops_api::rsp_header_t));
      std::memcpy(response.data(), &responsesServiceProcessor_[device].front(), sizeof(device_ops_api::rsp_header_t));
      responsesServiceProcessor_[device].pop();
      return true;
    }
    return false;
  };

  int getDevicesCount() const override {
    return numDevices_;
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
      DeviceConfig::FormFactor::PCIE,    // Form factor
      25,                                // TDP
      32768,                             // Total L3 size in KBytes
      16384,                             // Total L2 size in KBytes
      81920,                             // Total L2scp size in KBytes
      64,                                // CacheLine alignment in Bytes
      4,                                 // number of L2 cache banks
      128000,                            // ddr bandwidth
      1000,                              // Base frequency
      0xFFFFFFFF,                        // Compute minion mask
      32,                                // spare minion shire id
      DeviceConfig::ArchRevision::ETSOC1 // arch revision (ETSOC)
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
