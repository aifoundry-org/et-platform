//******************************************************************************
// Copyright (C) 2018,2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "runtime/IRuntime.h"

#include <fstream>
#include <glog/logging.h>
#include <gtest/gtest.h>
#include <ios>

namespace {

class TestCodeLoading : public ::testing::Test {
public:
  void SetUp() override {
    runtime_ = rt::IRuntime::create(rt::IRuntime::Kind::SysEmu);
    devices_ = runtime_->getDevices();
    ASSERT_GE(devices_.size(), 1);
    auto elf_file = std::ifstream("../convolution.elf", std::ios::in | std::ios::binary);
    ASSERT_TRUE(elf_file.is_open());
    elf_file.seekg(0, std::ios::end);
    auto size = elf_file.tellg();
    ASSERT_GT(size, 0);
    convolutionContent_.resize(size);
    elf_file.seekg(0);
    elf_file.read(reinterpret_cast<char*>(convolutionContent_.data()), size);
  }

  void TearDown() override { runtime_.reset(); }

  rt::RuntimePtr runtime_;
  std::vector<std::byte> convolutionContent_;
  std::vector<rt::Device> devices_;
};

// Load and removal of a single kernel.
TEST_F(TestCodeLoading, LoadKernel) {

  rt::Kernel kernel;
  /*EXPECT_NO_THROW(kernel =
                    runtime_->loadCode(devices_.front(), convolutionContent_.data(), convolutionContent_.size()));
  EXPECT_NO_THROW(runtime_->unloadCode(kernel));
  // if we unload again the same kernel we should expect to throw an exception
  EXPECT_THROW(runtime_->unloadCode(kernel), rt::Exception);*/
}

} // namespace

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
  google::SetCommandLineOption("GLOG_minloglevel", "0");
  // Force logging in stderr and set min logging level
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
