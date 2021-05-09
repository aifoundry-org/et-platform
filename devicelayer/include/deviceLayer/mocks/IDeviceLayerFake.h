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
#include <cstring>
#include <device-layer/IDeviceLayer.h>
#include <esperanto/device-apis/device_apis_message_types.h>
#include <esperanto/device-apis/operations-api/device_ops_api_cxx.h>
#include <mutex>
#include <queue>
#include <string>
namespace dev {
class IDeviceLayerFake : public IDeviceLayer {
  std::queue<rsp_header_t> responsesMasterMinion_;
  std::queue<dev_mgmt_rsp_header_t> responsesServiceProcessor_;
  std::condition_variable cvMm_;
  std::condition_variable cvSp_;
  std::mutex mmMutex_;
  std::mutex spMutex_;

public:
  bool sendCommandMasterMinion(int device, int sqIdx, std::byte* command, size_t commandSize) override {
    std::lock_guard<std::mutex> lock(mmMutex_);
    auto cmd = reinterpret_cast<cmn_header_t*>(command);
    rsp_header_t rsp;
    rsp.rsp_hdr.tag_id = cmd->tag_id;
    if (cmd->msg_id == device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_CMD) {
      rsp.rsp_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_RSP;
    } else if (cmd->msg_id == device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_CMD) {
      rsp.rsp_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_RSP;
    }
    else if (cmd->msg_id == device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_CMD){
      rsp.rsp_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_RSP;
    }
    else {
      throw Exception("Please, add command with msg_id: " + std::to_string(cmd->msg_id));
    }
    responsesMasterMinion_.push(rsp);
    return true;
  }

  void setSqThresholdMasterMinion(int device, int sqIdx, uint32_t bytesNeeded) override {
  }

  void waitForEpollEventsMasterMinion(int device, uint64_t& sq_bitmap, bool& cq_available,
                                      std::chrono::seconds timeout) override {
    std::unique_lock<std::mutex> lock(mmMutex_, std::defer_lock);
    while (!lock.try_lock()) {
      // spin-lock
    }
    cvMm_.wait_for(lock, timeout, [this] { return !responsesMasterMinion_.empty(); });
    cq_available = true;
    sq_bitmap = 0xFFFFFFFFFFFFFFFF;
  }

  bool receiveResponseMasterMinion(int device, std::vector<std::byte>& response) override {
    std::unique_lock<std::mutex> lock(mmMutex_, std::defer_lock);
    while (!lock.try_lock()) {
      // spin-lock
    }

    if (!responsesMasterMinion_.empty()) {
      response.resize(sizeof(rsp_header_t));
      std::memcpy(response.data(), &responsesMasterMinion_.front(), sizeof(rsp_header_t));
      responsesMasterMinion_.pop();
      return true;
    }
    return false;
  }

  bool sendCommandServiceProcessor(int device, std::byte* command, size_t commandSize) override {
    std::unique_lock<std::mutex> lock(spMutex_, std::defer_lock);
    while (!lock.try_lock()) {
      // spin-lock
    }
    auto cmd = reinterpret_cast<cmn_header_t*>(command);
    dev_mgmt_rsp_header_t rsp;
    rsp.rsp_hdr.tag_id = cmd->tag_id;
    responsesServiceProcessor_.push(rsp);
    return true;
  }

  void setSqThresholdServiceProcessor(int device, uint32_t bytesNeeded) override{};

  void waitForEpollEventsServiceProcessor(int device, bool& sq_available, bool& cq_available,
                                          std::chrono::seconds timeout) override {
    std::unique_lock lock(spMutex_);
    cvMm_.wait_for(lock, timeout, [this] { return !responsesServiceProcessor_.empty(); });
    sq_available = cq_available = true;
  }

  bool receiveResponseServiceProcessor(int device, std::vector<std::byte>& response) override {
    std::unique_lock<std::mutex> lock(spMutex_, std::defer_lock);
    while (!lock.try_lock()) {
      // spin-lock
    }
    if (!responsesServiceProcessor_.empty()) {
      response.resize(sizeof(rsp_header_t));
      std::memcpy(response.data(), &responsesServiceProcessor_.front(), sizeof(rsp_header_t));
      responsesServiceProcessor_.pop();
      return true;
    }
    return false;
  };

  int getDevicesCount() const override {
    return 1;
  };

  int getSubmissionQueuesCount(int device) const override {
    return 1;
  };

  size_t getSubmissionQueueSizeMasterMinion(int device) const override {
    return 1024 * 1024 * 1024;
  };

  size_t getSubmissionQueueSizeServiceProcessor(int device) const override {
    return 1024 * 1024 * 1024;
  };

  int getDmaAlignment() const override {
    return 64;
  };

  /// \brief Returns the DRAM available size in bytes
  ///
  /// @returns DRAM size in bytes
  ///
  uint64_t getDramSize() const override {
    return 1UL << (10 + 10 + 10 + 4);
  };

  /// \brief Returns the DRAM base address
  ///
  /// @returns DRAM Host Managed base address
  ///
  uint64_t getDramBaseAddress() const override {
    return 0x8000;
  }
};

} // namespace dev
