//******************************************************************************
// Copyright (C) 2018,2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "esperanto/runtime/Core/CommandLineOptions.h"
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

using namespace et_runtime;
using namespace et_runtime::device_api;

class TestResponse : public ResponseBase {
public:
  // dummy type
  using response_devapi_t = bool;

  static constexpr MBOXMessageTypeID responseTypeID() { return 0; };
};

class TestCommand : public Command<TestResponse> {
  using Response = TestResponse;

  etrtError execute(et_runtime::Device *deviice) override {
    return etrtSuccess;
  }
};

TEST(Command, Example) {

  TestCommand command;
  std::thread set_response([&command] { command.setResponse(TestResponse()); });
  auto response_future = command.getFuture();
  response_future.wait();
  auto response = response_future.get();
  EXPECT_TRUE(response);

  set_response.join();
}

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
  // Force logging in stderr and set min logging level
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  et_runtime::ParseCommandLineOptions(argc, argv,
                                      {"test_launch_uberkernel.cc"});
  return RUN_ALL_TESTS();
}
