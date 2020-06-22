//******************************************************************************
// Copyright (C) 2018,2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "esperanto/runtime/Support/ErrorOr.h"
#include "gtest/gtest.h"

#include <cstdint>
#include <vector>

using namespace et_runtime;

namespace {

class TestClass {
public:
  TestClass(int val) : val_(val) {}
  const int get_val() { return val_; }

private:
  int val_;
};

ErrorOr<TestClass> test_function_return_val() { return TestClass{12}; }

/**
 * Example error we are testing success
 */
TEST(ErrorOr, success) {
  auto res = test_function_return_val();
  ASSERT_TRUE(res);
  auto error = res.getError();
  ASSERT_EQ(error, etrtSuccess);
  auto val = res->get_val();
  ASSERT_EQ(val, 12);
}

ErrorOr<TestClass> test_function_return_error() {
  return etrtErrorMissingConfiguration;
}

/**
 * Example error we are testing an error
 */
TEST(ErrorOr, failure) {
  auto res = test_function_return_error();
  ASSERT_FALSE(res);
  auto error = res.getError();
  ASSERT_EQ(error, etrtErrorMissingConfiguration);
}
};
