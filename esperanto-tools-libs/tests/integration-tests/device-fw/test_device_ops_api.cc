//******************************************************************************
// Copyright (C) 2018,2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "CodeManagement/CodeModule.h"
#include "CodeManagement/ELFSupport.h"
#include "CodeManagement/ModuleManager.h"
#include "DeviceAPI/Commands.h"
#include "DeviceAPI/CommandsGen.h"
#include "RPCDevice/SysEmuTarget_MM.h"
#include "esperanto/runtime/Core/CommandLineOptions.h"
#include "esperanto/runtime/Core/Device.h"
#include "esperanto/runtime/Core/DeviceManager.h"
#include "esperanto/runtime/Core/DeviceTarget.h"
#include "esperanto/runtime/Support/Logging.h"
#include <esperanto/device-apis/operations-api/device_ops_api_cxx.h>

#include <array>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <experimental/filesystem>
#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <shared_mutex>
#include <string>
#include <thread>
#include <atomic>

using namespace et_runtime;
using namespace et_runtime::device;
using namespace std;

namespace {
const uint16_t kCommandsToProcess = 6;
const uint8_t kDevFWMajor = DEVICE_OPS_API_MAJOR;
const uint8_t kDevFWMinor = DEVICE_OPS_API_MINOR;
const uint8_t kDevFWPatch = DEVICE_OPS_API_PATCH;
const uint32_t kEchoPayload = 0xDEADBEEF;
const uint64_t kDmaMinBufSize = 64;
} // namespace

class TestDeviceOpsAPI : public ::testing::Test {
public:
  void fExecutor(uint8_t queueId, device_ops_api::msg_id_t msg_id,
                 TimeDuration wait_time) {
    auto start = Clock::now();
    auto end = start + wait_time;

    auto* target_device_ptr = &dev_->getTargetDevice();
    auto* target_device = dynamic_cast<device::RPCTargetMM*>(target_device_ptr);
    ASSERT_TRUE(target_device != nullptr);

    int i = 0;
    while (i < kCommandsToProcess) {
      if (end < Clock::now()) {
        break;
      }

      if (!pushCmd(queueId, msg_id)) {
        {
          std::shared_lock<std::shared_mutex> lk(sq_bitmap_mtx_);
          sq_bitmap_cv_.wait(lk, [this, queueId]() -> bool { return sq_bitmap_ & (0x1U << queueId); });
          sq_bitmap_ &= ~(0x1U << queueId);
        }
      } else {
        i++;
      }
    }
  }

  void fListener(TimeDuration wait_time) {
    auto start = Clock::now();
    auto end = start + wait_time;

    auto* target_device_ptr = &dev_->getTargetDevice();
    auto* target_device = dynamic_cast<device::RPCTargetMM*>(target_device_ptr);
    ASSERT_TRUE(target_device != nullptr);

    int i = 0;
    while (i < target_device->virtQueueCount() * kCommandsToProcess) {
      uint32_t sq_bitmap = 0;
      uint32_t cq_bitmap = 0;

      if (end < Clock::now()) {
        EXPECT_GE(i, target_device->virtQueueCount() * kCommandsToProcess);
        break;
      }

      if (target_device->waitForEpollEvents(sq_bitmap, cq_bitmap)) {
        if (cq_bitmap > 0) {
          for (uint8_t queueId = 0; queueId < target_device->virtQueueCount(); queueId++) {
            if (cq_bitmap & (0x1U << queueId)) {
              while (popRsp(queueId)) {
                i++;
              }
            }
          }
        }
        if (sq_bitmap > 0) {
          {
            std::lock_guard<std::shared_mutex> lk(sq_bitmap_mtx_);
            sq_bitmap_ |= sq_bitmap;
          }
          sq_bitmap_cv_.notify_all();
        }
      }
    }
  }

protected:
  void SetUp() override {
    bool success;

    tag_id_ = 0xa1;

    auto device_manager = et_runtime::getDeviceManager();
    auto ret_value = device_manager->registerDevice(0);
    dev_ = ret_value.get();

    auto worker_minion = absl::GetFlag(FLAGS_worker_minion_elf);
    auto machine_minion = absl::GetFlag(FLAGS_machine_minion_elf);
    auto master_minion = absl::GetFlag(FLAGS_master_minion_elf);

    // Start the simulator and load device-fw in memory
    ASSERT_TRUE(dev_->setFWFilePaths({master_minion, machine_minion, worker_minion}));

    // Do nothing the test fixture should do the above
    auto* target_device_ptr = &dev_->getTargetDevice();
    auto* target_device = dynamic_cast<device::RPCTargetMM*>(target_device_ptr);
    ASSERT_TRUE(target_device != nullptr);

    auto res = target_device->init();
    ASSERT_TRUE(res);

    res = dev_->mem_manager().init();
    ASSERT_TRUE(res);

    success = dev_->loadFirmwareOnDevice();
    ASSERT_TRUE(success == etrtSuccess);

    success = dev_->configureFirmware();
    ASSERT_TRUE(success == etrtSuccess);

    success = dev_->bootFirmware();
    ASSERT_TRUE(success == etrtSuccess);

    dmaWriteCmdCnt_ = 0;
    dramBaseAddress_ = target_device->dramBaseAddr();
  }

  void TearDown() override {
    // Stop the simulator
    ASSERT_TRUE(dev_->getTargetDevice().deinit());
  }

  void getDmaWriteCmdInfo(uint64_t &dmaWriteAddr , uint64_t &dmaBufSize) {
    if (dmaWriteCmdCnt_ <= 0) {
      dmaBufSize = kDmaMinBufSize;
      dmaWriteAddr = dramBaseAddress_;
    } else {
       dmaBufSize = kDmaMinBufSize * (0x1 << dmaWriteCmdCnt_);
       dmaWriteAddr = dramBaseAddress_ + kDmaMinBufSize * ((0x1 << dmaWriteCmdCnt_) - 1);
    }
    dmaWriteCmdCnt_ = (dmaWriteCmdCnt_ + 1) % 10; // wrap address & sizes after 10 cmds
  }

  void getDmaReadCmdInfo(uint64_t &dmaWriteAddr , uint64_t &dmaBufSize) {
    if (dmaReadCmdCnt_ <= 0) {
      dmaBufSize = kDmaMinBufSize;
      dmaWriteAddr = dramBaseAddress_;
    } else {
       dmaBufSize = kDmaMinBufSize * (0x1 << dmaReadCmdCnt_);
       dmaWriteAddr = dramBaseAddress_ + kDmaMinBufSize * ((0x1 << dmaReadCmdCnt_) - 1);
    }
    dmaReadCmdCnt_ = (dmaReadCmdCnt_ + 1) % 10; // wrap address & sizes after 10 cmds
  }

  device_ops_api::tag_id_t getNextTagId() {
    return tag_id_++;
  }

  std::tuple<device_ops_api::tag_id_t, void *, uint64_t>
  addDmaAllocBufPtr(device_ops_api::tag_id_t tag_id, void *dmaBufPtr, uint64_t &dmaBufSize) {
    auto elem = std::find_if(dmaAllocBufPtrStorage_.begin(), dmaAllocBufPtrStorage_.end(),
                             [tag_id](decltype(dmaAllocBufPtrStorage_)::value_type &e) {
                               return std::get<0>(e) == tag_id;
                             });
    if (elem != dmaAllocBufPtrStorage_.end()) {
      auto &[id, ptr, size] = *elem;
      return {id, ptr, size};
    }

    auto val = std::move(std::make_tuple(tag_id, dmaBufPtr, dmaBufSize));
    dmaAllocBufPtrStorage_.emplace_back(std::move(val));
    return {tag_id, dmaBufPtr, dmaBufSize};
  }

  void getDmaAllocBufPtr(device_ops_api::tag_id_t tag_id, void **dmaBufPptr, uint64_t &dmaBufSize) {
    auto elem = std::find_if(dmaAllocBufPtrStorage_.begin(), dmaAllocBufPtrStorage_.end(),
                             [tag_id](decltype(dmaAllocBufPtrStorage_)::value_type &e) {
                               return std::get<0>(e) == tag_id;
                             });
    if (elem == dmaAllocBufPtrStorage_.end()) {
      *dmaBufPptr = nullptr;
      dmaBufSize = 0;
      return;
    }
    *dmaBufPptr = std::get<1>(*elem);
    dmaBufSize = std::get<2>(*elem);
  }

  bool delDmaAllocBufPtr(device_ops_api::tag_id_t tag_id) {
    dmaAllocBufPtrStorage_.erase(
        std::remove_if(dmaAllocBufPtrStorage_.begin(), dmaAllocBufPtrStorage_.end(),
                       [tag_id](const decltype(dmaAllocBufPtrStorage_)::value_type &e) {
                         return std::get<0>(e) == tag_id;
                       }),
        dmaAllocBufPtrStorage_.end());
    return true;
  }

  bool pushCmd(uint8_t queueId, device_ops_api::msg_id_t msg_id) {
    auto* target_device_ptr = &dev_->getTargetDevice();

    auto* target_device = dynamic_cast<device::RPCTargetMM*>(target_device_ptr);
    if (target_device == nullptr) {
      return false;
    }
    switch (msg_id) {
    case device_ops_api::DEV_OPS_API_MID_CHECK_DEVICE_OPS_API_COMPATIBILITY_CMD: {
      device_ops_api::check_device_ops_api_compatibility_cmd_t cmd = {0};
      cmd.command_info.cmd_hdr.tag_id = getNextTagId();
      cmd.command_info.cmd_hdr.msg_id = msg_id;
      cmd.major = DEVICE_OPS_API_MAJOR;
      cmd.minor = DEVICE_OPS_API_MINOR;
      cmd.patch = DEVICE_OPS_API_PATCH;
      std::cout << "HOST_INFO: cmd.major: " << (int)cmd.major << " cmd.major: " << (int)cmd.minor
                << " cmd.patch: " << (int)cmd.patch << endl;
      return target_device->virtQueueWrite(&cmd, sizeof(cmd), queueId);
    }
    case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DEVICE_FW_VERSION_CMD: {
      device_ops_api::device_ops_device_fw_version_cmd_t cmd = {0};
      cmd.command_info.cmd_hdr.tag_id = getNextTagId();
      cmd.command_info.cmd_hdr.msg_id = msg_id;
      cmd.firmware_type = 1;
      return target_device->virtQueueWrite(&cmd, sizeof(cmd), queueId);
    }
    case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_ECHO_CMD: {
      device_ops_api::device_ops_echo_cmd_t cmd = {0};
      cmd.command_info.cmd_hdr.tag_id = getNextTagId();
      cmd.command_info.cmd_hdr.msg_id = msg_id;
      cmd.echo_payload = kEchoPayload;
      return target_device->virtQueueWrite(&cmd, sizeof(cmd), queueId);
    }
    case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_CMD: {
      device_ops_api::device_ops_kernel_launch_cmd_t cmd = {0};
      cmd.command_info.cmd_hdr.tag_id = getNextTagId();
      cmd.command_info.cmd_hdr.msg_id = msg_id;
      // Sending dummay data for kernel launch
      cmd.code_start_address = 0xA5A5A5A5A5A5A5A5;
      cmd.pointer_to_args = 0xA5A5A5A5A5A5A5A5;
      return target_device->virtQueueWrite(&cmd, sizeof(cmd), queueId);
    }
    case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_KERNEL_ABORT_CMD: {
      device_ops_api::device_ops_kernel_abort_cmd_t cmd = {0};
      // Sending dummay data for kernel launch
      cmd.command_info.cmd_hdr.tag_id = getNextTagId();
      cmd.command_info.cmd_hdr.msg_id = msg_id;
      // Sending dummay data for kernel launch
      cmd.kernel_id = 0xA5;
      return target_device->virtQueueWrite(&cmd, sizeof(cmd), queueId);
    }
    case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_KERNEL_STATE_CMD: {
      device_ops_api::device_ops_kernel_state_cmd_t cmd = {0};
      cmd.command_info.cmd_hdr.tag_id = getNextTagId();
      cmd.command_info.cmd_hdr.msg_id = msg_id;
      // Sending dummay data for kernel launch
      cmd.kernel_id = 0xA5;
      return target_device->virtQueueWrite(&cmd, sizeof(cmd), queueId);
    }
    case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_CMD: {
      dataReadCmdMtx_.lock();
      device_ops_api::device_ops_data_read_cmd_t cmd = {0};
      uint64_t dmaReadAddr, dmaBufSize;
      getDmaReadCmdInfo(dmaReadAddr, dmaBufSize);
      cout << "dmaReadAddr: " << std::hex << dmaReadAddr << " dmaBufSize: " << dmaBufSize << std::endl;
      cmd.command_info.cmd_hdr.tag_id = getNextTagId();
      cmd.command_info.cmd_hdr.msg_id = msg_id;
      cmd.src_device_phy_addr = dmaReadAddr;
      void *dmaBufPtr = malloc(dmaBufSize);
      if (!dmaBufPtr) {
        dataReadCmdMtx_.unlock();
        return false;
      }
      cmd.dst_host_virt_addr = reinterpret_cast<uint64_t>(dmaBufPtr);
      cmd.dst_host_phy_addr = reinterpret_cast<uint64_t>(dmaBufPtr);
      cmd.size = dmaBufSize;
      if (target_device->virtQueueWrite(&cmd, sizeof(cmd), queueId)) {
        addDmaAllocBufPtr(cmd.command_info.cmd_hdr.tag_id, dmaBufPtr, dmaBufSize);
        dataReadCmdMtx_.unlock();
        return true;
      } else {
        dmaReadCmdCnt_--;
        free(dmaBufPtr);
        dataReadCmdMtx_.unlock();
        return false;
      }
    }
    case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_CMD: {
      // TODO: Remove sleep; DMA on multiple cmds fail without sleep
      sleep(0.5);
      dataWriteCmdMtx_.lock();
      device_ops_api::device_ops_data_write_cmd_t cmd = {0};
      uint64_t dmaWriteAddr, dmaBufSize;
      getDmaWriteCmdInfo(dmaWriteAddr, dmaBufSize);
      cout << "dmaWriteAddr: " << std::hex << dmaWriteAddr << " dmaBufSize: " << dmaBufSize << std::endl;
      cmd.command_info.cmd_hdr.tag_id = getNextTagId();
      cmd.command_info.cmd_hdr.msg_id = msg_id;
      cmd.dst_device_phy_addr = dmaWriteAddr;
      void *dmaBufPtr = malloc(dmaBufSize);
      if (!dmaBufPtr) {
        dataWriteCmdMtx_.unlock();
        return false;
      }
      uint8_t *buf = reinterpret_cast<uint8_t*>(dmaBufPtr);
      for (int i = 0; i < dmaBufSize; i++, buf++) {
        *buf = (uint8_t)(i % 0x100);
      }
      cmd.src_host_virt_addr = reinterpret_cast<uint64_t>(dmaBufPtr);
      cmd.src_host_phy_addr = reinterpret_cast<uint64_t>(dmaBufPtr);
      cmd.size = dmaBufSize;
      if (target_device->virtQueueWrite(&cmd, sizeof(cmd), queueId)) {
        addDmaAllocBufPtr(cmd.command_info.cmd_hdr.tag_id, dmaBufPtr, dmaBufSize);
        dataWriteCmdMtx_.unlock();
        return true;
      } else {
        dmaWriteCmdCnt_--;
        free(dmaBufPtr);
        dataWriteCmdMtx_.unlock();
        return false;
      }
    }
    }
  }

  bool popRsp(uint8_t queueId) {
    auto* target_device_ptr = &dev_->getTargetDevice();
    auto* target_device = dynamic_cast<device::RPCTargetMM*>(target_device_ptr);
    if (target_device == nullptr) {
      return false;
    }
    std::vector<uint8_t> message(target_device->virtQueueMsgMaxSize(), 0);
    auto res = target_device->virtQueueRead(message.data(), message.size(), queueId);
    if (!res) {
      return res;
    }

    auto response_header = reinterpret_cast<device_ops_api::rsp_header_t*>(message.data());
    auto rsp_msg_id = response_header->rsp_hdr.msg_id;
    auto rsp_tag_id = response_header->rsp_hdr.tag_id;

    if (rsp_msg_id == device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_API_COMPATIBILITY_RSP) {
      auto response = reinterpret_cast<device_ops_api::device_ops_api_compatibility_rsp_t*>(message.data());

      EXPECT_TRUE(response->major == kDevFWMajor);
      EXPECT_TRUE(response->minor == kDevFWMinor);
      EXPECT_TRUE(response->patch == kDevFWPatch);

    } else if (rsp_msg_id == device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_FW_VERSION_RSP) {
      auto response = reinterpret_cast<device_ops_api::device_ops_fw_version_rsp_t*>(message.data());

      EXPECT_TRUE(response->major == 1);
      EXPECT_TRUE(response->minor == 0);
      EXPECT_TRUE(response->patch == 0);

    } else if (rsp_msg_id == device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_ECHO_RSP) {
      auto response = reinterpret_cast<device_ops_api::device_ops_echo_rsp_t*>(message.data());

      res = (response->echo_payload == kEchoPayload);

      EXPECT_TRUE(response->echo_payload == kEchoPayload);

    } else if (rsp_msg_id == device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_RSP) {
      auto response = reinterpret_cast<device_ops_api::device_ops_kernel_launch_rsp_t*>(message.data());

      EXPECT_TRUE(response->cmd_wait_time == 0xdeadbeef);
      EXPECT_TRUE(response->cmd_execution_time == 0xdeadbeef);
      EXPECT_TRUE(response->status == device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_STATUS_RESULT_OK);

    } else if (rsp_msg_id == device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_KERNEL_ABORT_RSP) {
      auto response = reinterpret_cast<device_ops_api::device_ops_kernel_abort_rsp_t*>(message.data());

      EXPECT_TRUE(response->kernel_id == 0xA5);
      EXPECT_TRUE(response->status == device_ops_api::DEV_OPS_API_KERNEL_ABORT_RESPONSE_OK);

    } else if (rsp_msg_id == device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_KERNEL_STATE_RSP) {
      auto response = reinterpret_cast<device_ops_api::device_ops_kernel_state_rsp_t*>(message.data());

      EXPECT_TRUE(response->status == device_ops_api::DEV_OPS_API_KERNEL_STATE_COMPLETE);

    } else if (rsp_msg_id == device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_RSP) {
      auto response = reinterpret_cast<device_ops_api::device_ops_data_read_rsp_t*>(message.data());

      EXPECT_TRUE(response->cmd_wait_time == 0xdeadbeef);
      EXPECT_TRUE(response->cmd_execution_time == 0xdeadbeef);
      EXPECT_TRUE(response->status == device_ops_api::ETSOC_DMA_STATE_DONE);

      void *dmaBufPtr;
      uint64_t dmaBufSize;
      getDmaAllocBufPtr(rsp_tag_id, &dmaBufPtr, dmaBufSize);
      uint8_t *buf = reinterpret_cast<uint8_t*>(dmaBufPtr);
      for (int i = 0; i < dmaBufSize; i++, buf++) {
        EXPECT_EQ(*buf, i % 0x100);
      }
      free(dmaBufPtr);
      EXPECT_TRUE(delDmaAllocBufPtr(rsp_tag_id));

    } else if (rsp_msg_id == device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_RSP) {
      auto response = reinterpret_cast<device_ops_api::device_ops_data_write_rsp_t*>(message.data());

      EXPECT_TRUE(response->cmd_wait_time == 0xdeadbeef);
      EXPECT_TRUE(response->cmd_execution_time == 0xdeadbeef);
      EXPECT_TRUE(response->status == device_ops_api::ETSOC_DMA_STATE_DONE);

      void *dmaBufPtr;
      uint64_t dmaBufSize;
      getDmaAllocBufPtr(rsp_tag_id, &dmaBufPtr, dmaBufSize);
      free(dmaBufPtr);
      EXPECT_TRUE(delDmaAllocBufPtr(rsp_tag_id));
    } else {
      cout << "Unknown response!" << endl;
      return false;
    }

    return res;
  }

  std::atomic<device_ops_api::tag_id_t> tag_id_;
  uint16_t dmaWriteCmdCnt_;
  uint16_t dmaReadCmdCnt_;
  uint64_t dramBaseAddress_;
  std::mutex dataWriteCmdMtx_;
  std::mutex dataReadCmdMtx_;
  std::shared_ptr<Device> dev_;
  std::shared_mutex sq_bitmap_mtx_;
  std::condition_variable_any sq_bitmap_cv_;
  std::atomic_uint32_t sq_bitmap_ = 0xffffffff;
  std::vector<std::thread> threadVector_;
  std::vector<std::tuple<device_ops_api::tag_id_t, void*, uint64_t>> dmaAllocBufPtrStorage_;
};

TEST_F(TestDeviceOpsAPI, postFWLoadInit) {
  // Do nothing the test fixture should do the above
  auto* target_device_ptr = &dev_->getTargetDevice();
  auto* target_device = dynamic_cast<device::RPCTargetMM*>(target_device_ptr);
  ASSERT_TRUE(target_device != nullptr);

  auto res = target_device->postFWLoadInit();
  ASSERT_TRUE(res);
}

TEST_F(TestDeviceOpsAPI, ApiCompatibiltyCmd) {
  // Do nothing the test fixture should do the above
  auto* target_device_ptr = &dev_->getTargetDevice();
  auto* target_device = dynamic_cast<device::RPCTargetMM*>(target_device_ptr);
  ASSERT_TRUE(target_device != nullptr);

  auto res = target_device->postFWLoadInit();
  ASSERT_TRUE(res);

  // Create one listener thread
  threadVector_.push_back(
    std::thread(std::bind(&TestDeviceOpsAPI::fListener, this, std::chrono::milliseconds(100))));

  // Create submission threads equal to virtQueueCount()
  for (uint8_t queueId = 0; queueId < target_device->virtQueueCount(); queueId++) {
    threadVector_.push_back(std::thread(std::bind(
      &TestDeviceOpsAPI::fExecutor, this, queueId,
      device_ops_api::DEV_OPS_API_MID_CHECK_DEVICE_OPS_API_COMPATIBILITY_CMD, std::chrono::milliseconds(100))));
    // Wait before next iteration. This ensures thread started with expected queueId
    sleep(1);
  }

  for (auto& t : threadVector_) {
    if (t.joinable()) {
      t.join();
    }
  }
}

TEST_F(TestDeviceOpsAPI, FwVersionCmd) {
  // Do nothing the test fixture should do the above
  auto* target_device_ptr = &dev_->getTargetDevice();
  auto* target_device = dynamic_cast<device::RPCTargetMM*>(target_device_ptr);
  ASSERT_TRUE(target_device != nullptr);

  auto res = target_device->postFWLoadInit();
  ASSERT_TRUE(res);

  // Create one listener thread
  threadVector_.push_back(
    std::thread(std::bind(&TestDeviceOpsAPI::fListener, this, std::chrono::milliseconds(100))));

  // Create submission threads equal to virtQueueCount()
  for (uint8_t queueId = 0; queueId < target_device->virtQueueCount(); queueId++) {
    threadVector_.push_back(std::thread(std::bind(&TestDeviceOpsAPI::fExecutor, this, queueId,
                                                  device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DEVICE_FW_VERSION_CMD,
                                                  std::chrono::milliseconds(100))));
    //  Wait before next iteration. This ensures thread started with expected queueId
    sleep(1);
  }

  for (auto& t : threadVector_) {
    if (t.joinable()) {
      t.join();
    }
  }
}

TEST_F(TestDeviceOpsAPI, EchoCmd) {
  // Do nothing the test fixture should do the above
  auto* target_device_ptr = &dev_->getTargetDevice();
  auto* target_device = dynamic_cast<device::RPCTargetMM*>(target_device_ptr);
  ASSERT_TRUE(target_device != nullptr);

  auto res = target_device->postFWLoadInit();
  ASSERT_TRUE(res);

  // Create one listener thread
  threadVector_.push_back(
    std::thread(std::bind(&TestDeviceOpsAPI::fListener, this, std::chrono::milliseconds(100))));

  // Create submission threads equal to virtQueueCount()
  for (uint8_t queueId = 0; queueId < target_device->virtQueueCount(); queueId++) {
    threadVector_.push_back(
      std::thread(std::bind(&TestDeviceOpsAPI::fExecutor, this, queueId,
                            device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_ECHO_CMD, std::chrono::milliseconds(100))));
    // Wait before next iteration. This ensures thread started with expected queueId
    sleep(1);
  }

  for (auto& t : threadVector_) {
    if (t.joinable()) {
      t.join();
    }
  }
}

TEST_F(TestDeviceOpsAPI, DmaWriteAndReadCmds) {
  // Do nothing the test fixture should do the above
  auto* target_device_ptr = &dev_->getTargetDevice();
  auto* target_device = dynamic_cast<device::RPCTargetMM*>(target_device_ptr);
  ASSERT_TRUE(target_device != nullptr);

  auto res = target_device->postFWLoadInit();
  ASSERT_TRUE(res);

  // Create one listener thread
  threadVector_.push_back(
    std::thread(std::bind(&TestDeviceOpsAPI::fListener, this, std::chrono::seconds(10))));

  // Create submission threads equal to virtQueueCount()
  for (uint8_t queueId = 0; queueId < target_device->virtQueueCount(); queueId++) {
    threadVector_.push_back(
      std::thread(std::bind(&TestDeviceOpsAPI::fExecutor, this, queueId,
                            device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_CMD, std::chrono::seconds(10))));
    // Wait before next iteration. This ensures thread started with expected queueId
    sleep(1);
  }

  // Wait for threads to join to ensure that DMA write is complete
  for (auto& t : threadVector_) {
    if (t.joinable()) {
      t.join();
    }
  }

  // Create one listener thread
  threadVector_.push_back(
    std::thread(std::bind(&TestDeviceOpsAPI::fListener, this, std::chrono::seconds(10))));

  // Create submission threads equal to virtQueueCount()
  for (uint8_t queueId = 0; queueId < target_device->virtQueueCount(); queueId++) {
    threadVector_.push_back(
      std::thread(std::bind(&TestDeviceOpsAPI::fExecutor, this, queueId,
                            device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_CMD, std::chrono::seconds(10))));
    // Wait before next iteration. This ensures thread started with expected queueId
    sleep(1);
  }

  for (auto& t : threadVector_) {
    if (t.joinable()) {
      t.join();
    }
  }
}

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  et_runtime::ParseCommandLineOptions(argc, argv);
  return RUN_ALL_TESTS();
}
