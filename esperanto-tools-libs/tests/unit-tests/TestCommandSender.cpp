/*-------------------------------------------------------------------------
 * Copyright (C) 2021, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include <chrono>
#include <device-layer/IDeviceLayerFake.h>
#include <esperanto/device-apis/device_apis_message_types.h>
#include <hostUtils/logging/Logger.h>
#include <thread>
#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wkeyword-macro"
#endif
#define private public
#pragma GCC diagnostic pop
#include "CommandSender.h"
#undef private
#include <gtest/gtest.h>

using namespace rt;

TEST(CommandSender, checkConsistency) {

  std::vector<std::byte> commandData(64);

  auto header = reinterpret_cast<device_ops_api::cmn_header_t*>(commandData.data());
  // dummy msg_id to make it work on deviceLayerFake
  header->msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DMA_WRITELIST_CMD;
  auto numCommands = 3 * 1e4;
  std::vector<Command*> commands;
  dev::IDeviceLayerFake deviceLayer;
  profiling::DummyProfiler profiler;
  CommandSender cs(deviceLayer, profiler, 0, 0);
  for (device_ops_api::tag_id_t i = 0; i < numCommands; ++i) {
    header->tag_id = device_ops_api::tag_id_t(i + 1);
    commands.emplace_back(cs.send(Command{commandData, cs}));
  }
  // no command is enabled now, so a call to receiveResponseMasterMinion should return false
  std::vector<std::byte> response;
  EXPECT_FALSE(deviceLayer.receiveResponseMasterMinion(0, response));

  // now, lets enable the first command sent
  commands.front()->enable();

  // we should expect a response now, due to multithreading we could wait for a bit. Let's try 5 times in a row
  for (int i = 0; i < 5; ++i) {
    if (deviceLayer.receiveResponseMasterMinion(0, response)) {
      break;
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
  }
  // check that we indeed got a response
  ASSERT_FALSE(response.empty());
  // check the tag (should be 1)
  auto rsp = reinterpret_cast<device_ops_api::rsp_header_t*>(response.data());
  ASSERT_EQ(rsp->rsp_hdr.tag_id, 1);

  // if we enable the last one, it shouldnt produce anything
  commands.back()->enable();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ASSERT_FALSE(deviceLayer.receiveResponseMasterMinion(0, response));
  ASSERT_EQ(rsp, reinterpret_cast<device_ops_api::rsp_header_t*>(response.data()));

  // if we enable all (except first and last one since they were already enabled) we should expect all tag_ids following
  for (auto i = 1U; i < numCommands - 1; ++i) {
    commands[i]->enable();
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  for (auto i = 1; i < numCommands; ++i) {
    ASSERT_TRUE(deviceLayer.receiveResponseMasterMinion(0, response));
    ASSERT_EQ(rsp, reinterpret_cast<device_ops_api::rsp_header_t*>(response.data()));
    ASSERT_EQ(rsp->rsp_hdr.tag_id, i + 1);
  }
}

TEST(CommandSender, checkSendBefore) {
  std::vector<std::byte> commandData(64);

  auto header = reinterpret_cast<device_ops_api::cmn_header_t*>(commandData.data());
  // dummy msg_id to make it work on deviceLayerFake
  header->msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DMA_WRITELIST_CMD;
  auto numCommands = 100;
  std::vector<Command*> commands;
  dev::IDeviceLayerFake deviceLayer;
  profiling::DummyProfiler profiler;
  CommandSender cs(deviceLayer, profiler, 0, 0);
  // first emplace commands with odd tag_ids
  for (device_ops_api::tag_id_t i = 0; i < numCommands / 2; ++i) {
    header->tag_id = device_ops_api::tag_id_t(i * 2 + 1);
    commands.emplace_back(cs.send(Command{commandData, cs}));
  }
  ASSERT_EQ(numCommands / 2, commands.size());
  // now emplace commands with even tag_ids
  auto copyCommands = commands;
  for (auto c : copyCommands) {
    auto sentCommandHeader = reinterpret_cast<device_ops_api::cmn_header_t*>(c->commandData_.data());
    header->tag_id = device_ops_api::tag_id_t(sentCommandHeader->tag_id - 1);
    commands.emplace_back(cs.sendBefore(c, Command{commandData, cs}));
  }

  // lets enable all commands
  for (auto c : commands) {
    c->enable();
  }
  std::vector<std::byte> response;

  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  for (auto i = 0; i < numCommands; ++i) {
    ASSERT_TRUE(deviceLayer.receiveResponseMasterMinion(0, response));
    auto rsp = reinterpret_cast<device_ops_api::rsp_header_t*>(response.data());
    ASSERT_EQ(rsp->rsp_hdr.tag_id, i);
  }
}

int main(int argc, char** argv) {
  logging::LoggerDefault logger_;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}