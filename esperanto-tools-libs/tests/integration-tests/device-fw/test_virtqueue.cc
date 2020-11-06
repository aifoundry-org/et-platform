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
#include "RPCDevice/SysEmuTarget_MM.h"
#include "esperanto/runtime/Core/CommandLineOptions.h"
#include "esperanto/runtime/Core/Device.h"
#include "esperanto/runtime/Core/DeviceManager.h"
#include "esperanto/runtime/Core/DeviceTarget.h"
#include "DeviceAPI/Commands.h"
#include "DeviceAPI/CommandsGen.h"
#include "esperanto/runtime/Support/Logging.h"
#include <esperanto/device-api/device_api_cxx_non_privileged.h>

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <array>
#include <chrono>
#include <cstdio>
#include <experimental/filesystem>
#include <string>
#include <thread>
#include <shared_mutex>
#include <condition_variable>

using namespace et_runtime;
using namespace et_runtime::device;
using namespace et_runtime::device_api;
using namespace std;

namespace {
  const uint16_t kCommandsToProcess = 6;
}

class EmuVirtQueueTest : public ::testing::Test {
public:
  void fExecutor(uint8_t queueId, TimeDuration wait_time) {
    auto start = Clock::now();
    auto end = start + wait_time;

    auto *target_device_ptr = &dev_->getTargetDevice();
    auto *target_device = dynamic_cast<device::RPCTargetMM *>(target_device_ptr);
    ASSERT_TRUE(target_device != nullptr);

    int i = 0;
    while (i < kCommandsToProcess) {
      if (end < Clock::now()) {
        break;
      }

      if (!pushReflectCmd(queueId)) {
        {
          std::shared_lock<std::shared_mutex> lk(sq_bitmap_mtx_);
          sq_bitmap_cv_.wait(lk, [this, queueId]() ->
                             bool { return sq_bitmap_ & (0x1U << queueId); });
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

    auto *target_device_ptr = &dev_->getTargetDevice();
    auto *target_device = dynamic_cast<device::RPCTargetMM *>(target_device_ptr);
    ASSERT_TRUE(target_device != nullptr);

    int i = 0;
    while (i < target_device->virtQueueCount() * kCommandsToProcess) {
      uint32_t sq_bitmap = 0;
      uint32_t cq_bitmap = 0;

      if (end < Clock::now()) {
        break;
      }

      if (target_device->waitForEpollEvents(sq_bitmap, cq_bitmap)) {
        if (cq_bitmap > 0) {
          for (uint8_t queueId = 0; queueId < target_device->virtQueueCount(); queueId++) {
            if (cq_bitmap & (0x1U << queueId)) {
              while(popReflectRsp(queueId)) {
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

    auto device_manager = et_runtime::getDeviceManager();
    auto ret_value = device_manager->registerDevice(0);
    dev_ = ret_value.get();

    auto worker_minion = absl::GetFlag(FLAGS_worker_minion_elf);
    auto machine_minion = absl::GetFlag(FLAGS_machine_minion_elf);
    auto master_minion = absl::GetFlag(FLAGS_master_minion_elf);

    // Start the simulator and load device-fw in memory
    ASSERT_TRUE(dev_->setFWFilePaths({master_minion, machine_minion, worker_minion}));

    // Do nothing the test fixture should do the above
    auto *target_device_ptr = &dev_->getTargetDevice();
    auto *target_device = dynamic_cast<device::RPCTargetMM *>(target_device_ptr);
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
  }

  void TearDown() override {
    // Stop the simulator
    ASSERT_TRUE(dev_->getTargetDevice().deinit());
  }

  bool pushReflectCmd (uint8_t queueId) {
    auto *target_device_ptr = &dev_->getTargetDevice();
    auto *target_device = dynamic_cast<device::RPCTargetMM *>(target_device_ptr);
    if (target_device == nullptr) {
      return false;
    }

    ::device_api::non_privileged::reflect_test_cmd_t cmd = {0};
    cmd.command_info.message_id = ::device_api::MBOX_DEVAPI_NON_PRIVILEGED_MID_REFLECT_TEST_CMD;
    cmd.command_info.command_id = queueId * 1234;
    return target_device->virtQueueWrite(&cmd, sizeof(cmd), queueId);
  }

  bool popReflectRsp (uint8_t queueId) {
    auto *target_device_ptr = &dev_->getTargetDevice();
    auto *target_device = dynamic_cast<device::RPCTargetMM *>(target_device_ptr);
    if (target_device == nullptr) {
      return false;
    }
    std::vector<uint8_t> message(target_device->virtQueueMsgMaxSize(), 0);
    auto res = target_device->virtQueueRead(message.data(), message.size(), queueId);
    if (!res) {
      return res;
    }

    auto response =
        reinterpret_cast<::device_api::non_privileged::response_header_t *>(message.data());
    auto msgid = response->message_id;
    auto cmdid = response->command_info.command_id;

    // Check the response received is of reflect command
    res = msgid == ::device_api::MBOX_DEVAPI_NON_PRIVILEGED_MID_REFLECT_TEST_RSP;
    if (!res) {
      return res;
    }

    // Check the response is of the same command that we sent
    return cmdid == 1234 * queueId;
  }

  std::shared_ptr<Device> dev_;
  std::shared_mutex sq_bitmap_mtx_;
  std::condition_variable_any sq_bitmap_cv_;
  std::atomic_uint32_t sq_bitmap_ = 0xffffffff;
  std::vector <std::thread> threadVector_;
};

TEST_F(EmuVirtQueueTest, postFWLoadInit) {
  // Do nothing the test fixture should do the above
  auto *target_device_ptr = &dev_->getTargetDevice();
  auto *target_device = dynamic_cast<device::RPCTargetMM *>(target_device_ptr);
  ASSERT_TRUE(target_device != nullptr);

  auto res = target_device->postFWLoadInit();
  ASSERT_TRUE(res);
}

TEST_F(EmuVirtQueueTest, pushAndPopReflectCmd) {
  // Do nothing the test fixture should do the above
  auto *target_device_ptr = &dev_->getTargetDevice();
  auto *target_device = dynamic_cast<device::RPCTargetMM *>(target_device_ptr);
  ASSERT_TRUE(target_device != nullptr);

  auto res = target_device->postFWLoadInit();
  ASSERT_TRUE(res);

  // Create one listener thread
  threadVector_.push_back(std::thread(std::bind(&EmuVirtQueueTest::fListener,\
                                                this,
                                                std::chrono::milliseconds(100))));

  // Create submission threads equal to virtQueueCount()
  for(uint8_t queueId = 0; queueId < target_device->virtQueueCount(); queueId++) {
    threadVector_.push_back(std::thread(std::bind(&EmuVirtQueueTest::fExecutor,\
                                                  this, queueId,
                                                  std::chrono::milliseconds(100))));
    // Wait before next iteration. This ensures thread started with expected queueId
    sleep(1);
  }

  for(auto& t: threadVector_) {
    if (t.joinable()) {
      t.join();
    }
  }
}

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  et_runtime::ParseCommandLineOptions(argc, argv);
  return RUN_ALL_TESTS();
}
