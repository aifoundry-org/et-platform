//******************************************************************************
// Copyright (C) 2018,2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "Core/CodeModule.h"
#include "Core/CommandLineOptions.h"
#include "Core/ELFSupport.h"
#include "Support/Logging.h"

#include <experimental/filesystem>
#include <glog/logging.h>
#include <gtest/gtest.h>
#include <iostream>
#include <libgen.h>

using namespace et_runtime;
using namespace std;
namespace fs = std::experimental::filesystem;

// Test device-fw elf parsing functionality
TEST(ELFInfo, parse_device_fw_elf) {

  KernelELFInfo elf_info{"master_minion"};

  // We expect that the elf we test with is installed next to the test-binary
  // Find the absolute pasth of the test binary
  fs::path p = "/proc/self/exe";
  auto test_real_path = fs::read_symlink(p);
  auto dir_name = test_real_path.remove_filename();
  auto master_minion = absl::GetFlag(FLAGS_master_minion_elf);

  EXPECT_TRUE(elf_info.loadELF(master_minion));

  // Check the load address of the Masterminion
  EXPECT_EQ(elf_info.loadAddr(), 0x8000800000);
}

ABSL_FLAG(std::string, empty_elf, "", "Path to the empty ELF binary");

// Test kernel elf parsing where we are we have only the
// ELF entrypoint and no magic annocated symbols
TEST(KernelELFInfo, parse_dev_fw_kernel_elf) {

  KernelELFInfo elf_info{"empty"};
  auto empty_elf = absl::GetFlag(FLAGS_empty_elf);

  EXPECT_TRUE(elf_info.loadELF(empty_elf));

  // For this kernel they are no raw kernel entrypoints
  EXPECT_TRUE(elf_info.rawKernelExists("empty"));

  EXPECT_EQ(0x8105000000, elf_info.loadAddr());
}

// Test kernel elf parsing where we are we have only the
// ELF entrypoint and no magic annocated symbols
TEST(KernelELFInfo, code_module_dev_fw) {

  KernelELFInfo elf_info{"empty"};
  auto empty_elf = absl::GetFlag(FLAGS_empty_elf);

  Module module(1, "empty");

  auto res = module.readELF(empty_elf);
  ASSERT_TRUE(res);

  RTDEBUG << "Elf Load Addr 0x" << std::hex << module.elfLoadAddr() << "\n";

  // For this kernel they are no raw kernel entrypoints
  EXPECT_EQ(0x8105000000, module.elfLoadAddr());
}

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  google::SetCommandLineOption("GLOG_minloglevel", "0");
  google::SetCommandLineOption("GLOG_logtostderr", "1");
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  et_runtime::ParseCommandLineOptions(argc, argv);
  return RUN_ALL_TESTS();
}
