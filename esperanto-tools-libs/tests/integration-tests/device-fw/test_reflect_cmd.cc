//******************************************************************************
// Copyright (C) 2018,2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "device-fw-fixture.h"

#include "DeviceAPI/Commands.h"
#include "DeviceAPI/CommandsGen.h"
#include "esperanto/runtime/Support/Logging.h"

using namespace et_runtime::device_api;

TEST_F(DeviceFWTest, reflectTest) {
  auto reflect_cmd = std::make_shared<devfw_commands::ReflectTestCmd>(
      dev_->defaultStream().id(), true);
  dev_->defaultStream().addCommand(reflect_cmd);

  auto ft = reflect_cmd->getFuture();
  auto response = ft.get().response();
  ASSERT_EQ(response.response_info.message_id,
            ::device_api::MBOX_DEVAPI_MESSAGE_ID_REFLECT_TEST_RSP);
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
