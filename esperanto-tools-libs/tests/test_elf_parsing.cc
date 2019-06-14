//******************************************************************************
// Copyright (C) 2018,2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "Core/ELFSupport.h"

#include "gtest/gtest.h"
#include <experimental/filesystem>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <iostream>
#include <libgen.h>

using namespace et_runtime;
using namespace std;
namespace fs = std::experimental::filesystem;

namespace {

// Test basic elf parsing functionality
TEST(ELFInfo, load_elf) {

  ELFInfo elf_info{"convolution"};

  // We expect that the elf we test with is installed next to the test-binary
  // Find the absolute pasth of the test binary
  fs::path p = "/proc/self/exe";
  auto test_real_path = fs::read_symlink(p);
  auto dir_name = test_real_path.remove_filename();
  auto conv_elf = dir_name / "convolution.elf";

  EXPECT_TRUE(elf_info.loadELF(conv_elf));
}

// Test parsing the elf using data in a preloaded vector.
TEST(ELFInfo, load_elf_vector) {

  ELFInfo elf_info{"convolution"};

  // We expect that the elf we test with is installed next to the test-binary
  // Find the absolute pasth of the test binary
  fs::path p = "/proc/self/exe";
  auto test_real_path = fs::read_symlink(p);
  auto dir_name = test_real_path.remove_filename();
  auto conv_elf = dir_name / "convolution.elf";

  ifstream testFile(conv_elf, std::ios::binary);
  std::vector<char> fileContents((std::istreambuf_iterator<char>(testFile)),
                                 std::istreambuf_iterator<char>());

  EXPECT_TRUE(elf_info.loadELF(fileContents));
}

// Test kernel elf parsing functionality
TEST(KernelELFInfo, parse_kernel_elf) {

  KernelELFInfo elf_info{"convolution"};

  // We expect that the elf we test with is installed next to the test-binary
  // Find the absolute pasth of the test binary
  fs::path p = "/proc/self/exe";
  auto test_real_path = fs::read_symlink(p);
  auto dir_name = test_real_path.remove_filename();
  auto conv_elf = dir_name / "convolution.elf";

  EXPECT_TRUE(elf_info.loadELF(conv_elf));

  EXPECT_TRUE(elf_info.rawKernelExists("convolution"));

  EXPECT_EQ(0x17f4, elf_info.rawKernelOffset("convolution"));
}

extern std::string FLAGS_dev_target;

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  google::SetCommandLineOption("GLOG_minloglevel", "0");
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

} // namespace
