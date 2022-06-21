//******************************************************************************
// Copyright (C) 2022, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "TestUtils.h"
#include "Utils.h"
#include "common/MpOrchestrator.h"
#include "runtime/DeviceLayerFake.h"
#include "runtime/IRuntime.h"
#include "runtime/Types.h"
#include "server/Client.h"
#include "server/Server.h"
#include <cstdio>
#include <sys/wait.h>

using namespace rt;
namespace {
auto getTmpFileName() {
  std::string filename("/tmp/mptestXXXXXX");
  auto fd = mkstemp(filename.data());
  close(fd);
  return filename;
}
} // namespace
TEST(multiprocess, handshake) {

  int fd[2];
  ASSERT_EQ(pipe(fd), 0);
  auto socketName = getTmpFileName();
  auto pid = fork();
  logging::LoggerDefault logger_;
  if (pid > 0) {
    RT_LOG(INFO) << "Creating server";
    Options options;
    options.checkDeviceApiVersion_ = false;
    options.checkMemcpyDeviceOperations_ = true;
    auto server = Server{socketName, std::make_unique<dev::DeviceLayerFake>(), options};
    RT_LOG(INFO) << "Signaling child to continue";
    write(fd[1], "Y", 1);
    RT_LOG(INFO) << "Waiting child to end";
    int childStatus;
    ASSERT_NE(wait(&childStatus), -1);
    ASSERT_EQ(childStatus, 0);
    RT_LOG(INFO) << "End parent";
    ASSERT_EQ(std::remove(socketName.c_str()), 0);
  } else {
    char dummy;
    RT_LOG(INFO) << "Waiting for parent signal";
    read(fd[0], &dummy, 1);
    RT_LOG(INFO) << "Creating client";
    auto cl = Client(socketName);
    RT_LOG(INFO) << "End child";
    ASSERT_EQ(dummy, 'Y');
    exit(0);
  }
}

TEST(multiprocess_1000, handshake) {
#ifdef __SANITIZE_ADDRESS__
  {
    logging::LoggerDefault logger_;
    RT_LOG(INFO) << "Skipping this test as it seems the address sanitizer doesn't like forks :/";
    return;
  }
#endif
  int fd[2];
  ASSERT_EQ(pipe(fd), 0);
  auto socketName = getTmpFileName();
  int numChildren = 1000;
  std::vector<pid_t> children;
  for (int i = 0; i < numChildren; ++i) {
    auto pid = fork();
    ASSERT_GE(pid, 0);
    if (pid > 0) {
      children.push_back(pid);
    } else {
      logging::LoggerDefault logger_;
      char dummy;
      read(fd[0], &dummy, 1);
      ASSERT_EQ(dummy, 'Y');
      auto cl = Client(socketName);
      exit(0);
    }
  }
  logging::LoggerDefault logger_;
  Options options;
  options.checkDeviceApiVersion_ = false;
  options.checkMemcpyDeviceOperations_ = true;
  auto server = Server{socketName, std::make_unique<dev::DeviceLayerFake>(), options};
  RT_LOG(INFO) << "Signaling children to continue";
  for (int i = 0; i < numChildren; ++i) {
    write(fd[1], "Y", 1);
  }
  for (auto p : children) {
    int status;
    waitpid(p, &status, 0);
    ASSERT_EQ(status, 0);
  }
}

TEST(multiprocess, mp_orchestrator_hs) {
  MpOrchestrator orch;
  orch.createServer([] { return std::make_unique<dev::DeviceLayerFake>(); }, Options{true, false});
  orch.createClient([](auto) {
    // only do handshake
  });
}

TEST(multiprocess, mp_orchestrator_1000_hs) {
  MpOrchestrator orch;
  orch.createServer([] { return std::make_unique<dev::DeviceLayerFake>(); }, Options{true, false});
  for (int i = 0; i < 1000; ++i) {
    orch.createClient([](auto) {
      // only do handshake
    });
  }
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}