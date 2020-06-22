//******************************************************************************
// Copyright (C) 2018,2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "MemoryManagement/TensorInfo.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <set>

using namespace et_runtime::device::memory_management;

TEST(TensorInfo, FreeRegion) {
  TensorInfo<FreeRegion> code(1, 1);
  ASSERT_EQ(code.type(), TensorType::Free);
  auto permissions = code.permissions();
  EXPECT_EQ(permissions,
            static_cast<TensorPermissionsTy>(TensorPermissions::None));
}

TEST(TensorInfo, Code) {
  TensorInfo<CodeBuffer> code(1, 1);
  ASSERT_EQ(code.type(), TensorType::Code);
  auto permissions = code.permissions();
  EXPECT_EQ(permissions, TensorPermissions::Read | TensorPermissions::Execute);
}

TEST(TensorInfo, Constant) {
  TensorInfo<ConstantTensor> constant(1, 1);
  ASSERT_EQ(constant.type(), TensorType::Constant);
  auto permissions = constant.permissions();
  EXPECT_EQ(permissions,
            static_cast<TensorPermissionsTy>(TensorPermissions::Read));
}

TEST(TensorInfo, Placeholder) {
  TensorInfo<PlaceholderTensor> placeholder(1, 1);
  ASSERT_EQ(placeholder.type(), TensorType::Placeholder);
  auto permissions = placeholder.permissions();
  EXPECT_EQ(permissions, TensorPermissions::Read | TensorPermissions::Write);
}

TEST(TensorInfo, Logging) {
  TensorInfo<LoggingBuffer> logging(1, 1);
  ASSERT_EQ(logging.type(), TensorType::Logging);
  auto permissions = logging.permissions();
  EXPECT_EQ(permissions, TensorPermissions::Read | TensorPermissions::Write);
}

TEST(TensorInfo, tensor_comparison) {
  {
    TensorInfo<CodeBuffer> code(1, 1);
    TensorInfo<ConstantTensor> constant(2, 1);
    EXPECT_TRUE(code < constant);
  }
  {
    TensorInfo<CodeBuffer> code(1, 1);
    TensorInfo<ConstantTensor> constant(2, 1);
    EXPECT_FALSE(constant < code);
  }
  {
    TensorInfo<CodeBuffer> code(0, 1);
    TensorInfo<ConstantTensor> constant(1, 1);
    EXPECT_TRUE(code < constant);
  }
  {
    TensorInfo<CodeBuffer> code(0, 10);
    TensorInfo<ConstantTensor> constant(1, 1);
    EXPECT_FALSE(code < constant);
  }
}
