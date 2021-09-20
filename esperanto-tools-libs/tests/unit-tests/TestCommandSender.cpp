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

  auto header = reinterpret_cast<cmn_header_t*>(commandData.data());
  // dummy msg_id to make it work on deviceLayerFake
  header->msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DMA_WRITELIST_CMD;
  auto numCommands = 3 * 1e4;
  std::vector<Command*> commands;
  dev::IDeviceLayerFake deviceLayer;
  CommandSender cs(deviceLayer, 0, 0);
  for (tag_id_t i = 0; i < numCommands; ++i) {
    header->tag_id = i + 1;
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
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
  // check that we indeed got a response
  ASSERT_FALSE(response.empty());
  // check the tag (should be 1)
  auto rsp = reinterpret_cast<rsp_header_t*>(response.data());
  EXPECT_EQ(rsp->rsp_hdr.tag_id, 1);

  // if we enable the last one, it shouldnt produce anything
  commands.back()->enable();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  EXPECT_FALSE(deviceLayer.receiveResponseMasterMinion(0, response));
  EXPECT_EQ(rsp, reinterpret_cast<rsp_header_t*>(response.data()));

  // if we enable all, we should expect all tag_ids following
  for (auto c : commands) {
    c->enable();
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  for (auto i = 1; i < numCommands; ++i) {
    EXPECT_TRUE(deviceLayer.receiveResponseMasterMinion(0, response));
    EXPECT_EQ(rsp, reinterpret_cast<rsp_header_t*>(response.data()));
    EXPECT_EQ(rsp->rsp_hdr.tag_id, i + 1);
  }
}

int main(int argc, char** argv) {
  logging::LoggerDefault logger_;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}