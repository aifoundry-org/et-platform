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

#include "esperanto/runtime/Core/DeviceHelpers.h"
#include "esperanto/runtime/Core/VersionCheckers.h"

#include <chrono>
#include <thread>

ABSL_FLAG(std::string, kernels_dir, "",
          "Directory where different kernel ELF files are located");

using namespace et_runtime;

TEST_F(DeviceFWTest, TestStreamCreate) {
  auto &defaultStream = dev_->defaultStream();

  // This test is really limited to one device
  ASSERT_EQ(defaultStream.id(), 1);

  auto res = dev_->streamCreate(false);

  ASSERT_EQ(res.getError(), etrtSuccess);

  auto &stream = res.get();

  ASSERT_EQ(stream.id(), 2);

  auto id = stream.id();

  auto find_res = Stream::getStream(id);

  ASSERT_EQ(find_res.getError(), etrtSuccess);

  auto del_res = dev_->destroyStream(id);

  ASSERT_EQ(del_res, etrtSuccess);
}

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
  google::SetCommandLineOption("GLOG_minloglevel", "0");
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  et_runtime::ParseCommandLineOptions(argc, argv);
  return RUN_ALL_TESTS();
}
