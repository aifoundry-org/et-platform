//******************************************************************************
// Copyright (C) 2018,2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "Core/CommandQueue.h"
#include "esperanto/runtime/DeviceAPI/Command.h"
#include "esperanto/runtime/DeviceAPI/Response.h"

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>

namespace et_runtime {

class TestResponse : public device_api::ResponseBase {
public:
  // dummy type
  using response_devapi_t = bool;
};

class TestCommand : public device_api::Command<TestResponse> {
  using Response = TestResponse;

  etrtError execute(et_runtime::Device *deviice) override {
    return etrtSuccess;
  }
};

TEST(CommandQueue, Example) {

  CommandQueue<device_api::CommandBase> queue;

  // Pseudo command processing thread;
  std::thread set_response([&queue] {
    auto command_base = queue.pop();
    auto command = std::dynamic_pointer_cast<TestCommand>(command_base);
    command->setResponse(TestResponse());
    command_base = queue.pop();
    command = std::dynamic_pointer_cast<TestCommand>(command_base);
    command->setResponse(TestResponse());
  });

  auto command1 = std::make_shared<TestCommand>();
  queue.push(std::dynamic_pointer_cast<device_api::CommandBase>(command1));
  auto command2 = std::make_shared<TestCommand>();
  queue.push(std::dynamic_pointer_cast<device_api::CommandBase>(command2));
  queue.push(command2);

  auto response_future = command1->getFuture();
  response_future.wait();
  auto response = response_future.get();
  EXPECT_TRUE(response);

  response_future = command2->getFuture();
  response_future.wait();
  response = response_future.get();
  EXPECT_TRUE(response);

  set_response.join();
}

} // namespace et_runtime
