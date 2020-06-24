//******************************************************************************
// Copyright (C) 2020, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "client/linux/handler/exception_handler.h"

#include <chrono>
#include <experimental/filesystem>
#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <thread>

namespace fs = std::experimental::filesystem;

static bool dumpCallback(const google_breakpad::MinidumpDescriptor &descriptor,
                         void *context, bool succeeded) {
  printf("Dump path: %s\n", descriptor.path());
  return succeeded;
}

google_breakpad::MinidumpDescriptor descriptor("/tmp");
google_breakpad::ExceptionHandler eh(descriptor, NULL, dumpCallback, NULL, true,
                                     -1);

class SentryTest : public ::testing::Test {

protected:
  SentryTest() {}

  void SetUp() override {
    const ::testing::TestInfo *const test_info =
        ::testing::UnitTest::GetInstance()->current_test_info();

    printf("We are in test %s of test suite %s.\n", test_info->name(),
           test_info->test_case_name());

    fs::path p = "/proc/self/exe";
    auto test_crash_path = fs::read_symlink(p);
    test_crash_path += "_dump";
    test_crash_path /= test_info->name();

    auto res = fs::create_directories(test_crash_path);

    google_breakpad::MinidumpDescriptor descriptor(test_crash_path.string());
    eh.set_minidump_descriptor(descriptor);
  }
};

// Crash because of a segfault
TEST_F(SentryTest, segfault) {

  volatile int *a = (int *)(NULL);
  *a = 1;
}

// Assert failure
TEST_F(SentryTest, assert_failure) { assert(false); }

// The following should create a failure under the address sanitizer
TEST_F(SentryTest, address_sanitizer_failure) {

  int *a = new int[10];
  int b = a[12];
}

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  google::SetCommandLineOption("GLOG_minloglevel", "0");
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
