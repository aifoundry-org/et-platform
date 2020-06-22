//******************************************************************************
// Copyright (C) 2018,2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "CodeManagement/ELFSupport.h"

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

  // We expect that the elf we test with is installed next to the test-binary
  // Find the absolute pasth of the test binary
  fs::path p = "/proc/self/exe";
  auto test_real_path = fs::read_symlink(p);
  auto dir_name = test_real_path.remove_filename();
  auto conv_elf = dir_name / "../convolution.elf";

  // FIXME SW-1369 do not consume the downloaded ELF file
  ELFInfo elf_info{"convolution", conv_elf.string()};

  EXPECT_TRUE(elf_info.loadELF());
}

// Test parsing the elf using data in a preloaded vector.
TEST(ELFInfo, load_elf_vector) {

  // We expect that the elf we test with is installed next to the test-binary
  // Find the absolute pasth of the test binary
  fs::path p = "/proc/self/exe";
  auto test_real_path = fs::read_symlink(p);
  auto dir_name = test_real_path.remove_filename();
  auto conv_elf = dir_name / "../convolution.elf";

  ELFInfo elf_info{"convolution", conv_elf.string()};

  ifstream testFile(conv_elf, std::ios::binary);
  std::vector<char> fileContents((std::istreambuf_iterator<char>(testFile)),
                                 std::istreambuf_iterator<char>());

  EXPECT_TRUE(elf_info.loadELF(fileContents));
}

// Test kernel elf parsing functionality
TEST(KernelELFInfo, parse_kernel_elf) {

  // We expect that the elf we test with is installed next to the test-binary
  // Find the absolute pasth of the test binary
  fs::path p = "/proc/self/exe";
  auto test_real_path = fs::read_symlink(p);
  auto dir_name = test_real_path.remove_filename();
  auto conv_elf = dir_name / "../convolution.elf";

  KernelELFInfo elf_info{"convolution", conv_elf.string()};

  EXPECT_TRUE(elf_info.loadELF());

  EXPECT_TRUE(elf_info.rawKernelExists("convolution"));

  EXPECT_EQ(0x17f4, elf_info.rawKernelOffset("convolution"));
}

TEST(KernelELFInfo, parse_multisegment_elf) {

  // We expect that the elf we test with is installed next to the test-binary
  // Find the absolute pasth of the test binary
  fs::path p = "/proc/self/exe";
  auto test_real_path = fs::read_symlink(p);
  auto dir_name = test_real_path.remove_filename();
  auto conv_elf = dir_name / "../etsocmaxsplat.elf";

  KernelELFInfo elf_info{"etsocmaxsplat.elf", conv_elf.string()};

  EXPECT_TRUE(elf_info.loadELF());

  EXPECT_TRUE(elf_info.rawKernelExists("etsocmaxsplat"));

  EXPECT_EQ(0x10a0, elf_info.rawKernelOffset("etsocmaxsplat"));
}

} // namespace

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
  // Force logging in stderr and set min logging level
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
