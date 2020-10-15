//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "runtime/IRuntime.h"
#include <gtest/gtest-death-test.h>
#include <ios>
#define private public
#include "NewCore/MemoryManager.h"
#include "NewCore/RuntimeImp.h"
#include <chrono>
#include <cstdio>
#include <glog/logging.h>
#include <gtest/gtest.h>

using namespace rt;

TEST(StreamsLifeCycle, simple) {
  auto runtime = IRuntime::create(IRuntime::Kind::SysEmu);
  auto devices = runtime->getDevices();
  ASSERT_GE(devices.size(), 0);
  EXPECT_NO_THROW({
    auto streamId = runtime->createStream(devices[0]);
    runtime->destroyStream(streamId);
  });
}

TEST(StreamsLifeCycle, create_and_destroy_10k_streams) {
  auto runtime = IRuntime::create(IRuntime::Kind::SysEmu);
  auto devices = runtime->getDevices();
  ASSERT_GE(devices.size(), 0);
  std::vector<StreamId> streams_;
  for (int i = 0; i < 10000; ++i) {
    streams_.emplace_back(runtime->createStream(devices[0]));
    if (i > 0) {
      EXPECT_GT(static_cast<uint32_t>(streams_[i]), static_cast<uint32_t>(streams_[i - 1]));
    }
  }
  // check these are the same as inside the runtime
  auto rt = static_cast<RuntimeImp*>(runtime.get());
  EXPECT_EQ(rt->streams_.size(), streams_.size());
  for (auto s : streams_) {
    EXPECT_NE(rt->streams_.find(s), end(rt->streams_));
  }
  // destroy all streams and expect there are no streams inside the runtime
  for (auto s : streams_) {
    runtime->destroyStream(s);
  }
  EXPECT_TRUE(rt->streams_.empty());
}

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
  google::SetCommandLineOption("GLOG_minloglevel", "0");
  google::SetCommandLineOption("GLOG_logtostderr", "1");
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
