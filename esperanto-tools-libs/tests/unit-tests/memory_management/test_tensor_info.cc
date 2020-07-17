//******************************************************************************
// Copyright (C) 2018,2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "MemoryManagement/BufferInfo.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <set>

using namespace et_runtime::device::memory_management;

TEST(BufferInfo, FreeRegion) {
  BufferInfo<FreeRegion> code(1, 1);
  ASSERT_EQ(code.type(), BufferType::Free);
  auto permissions = code.permissions();
  EXPECT_EQ(permissions,
            static_cast<BufferPermissionsTy>(BufferPermissions::None));
}

TEST(BufferInfo, Code) {
  BufferInfo<CodeBuffer> code(1, 1);
  ASSERT_EQ(code.type(), BufferType::Code);
  auto permissions = code.permissions();
  EXPECT_EQ(permissions, BufferPermissions::Read | BufferPermissions::Execute);
}

TEST(BufferInfo, Constant) {
  BufferInfo<ConstantBuffer> constant(1, 1);
  ASSERT_EQ(constant.type(), BufferType::Constant);
  auto permissions = constant.permissions();
  EXPECT_EQ(permissions,
            static_cast<BufferPermissionsTy>(BufferPermissions::Read));
}

TEST(BufferInfo, Placeholder) {
  BufferInfo<PlaceholderBuffer> placeholder(1, 1);
  ASSERT_EQ(placeholder.type(), BufferType::Placeholder);
  auto permissions = placeholder.permissions();
  EXPECT_EQ(permissions, BufferPermissions::Read | BufferPermissions::Write);
}

TEST(BufferInfo, Logging) {
  BufferInfo<LoggingBuffer> logging(1, 1);
  ASSERT_EQ(logging.type(), BufferType::Logging);
  auto permissions = logging.permissions();
  EXPECT_EQ(permissions, BufferPermissions::Read | BufferPermissions::Write);
}

TEST(BufferInfo, tensor_comparison) {
  {
    BufferInfo<CodeBuffer> code(1, 1);
    BufferInfo<ConstantBuffer> constant(2, 1);
    EXPECT_TRUE(code < constant);
  }
  {
    BufferInfo<CodeBuffer> code(1, 1);
    BufferInfo<ConstantBuffer> constant(2, 1);
    EXPECT_FALSE(constant < code);
  }
  {
    BufferInfo<CodeBuffer> code(0, 1);
    BufferInfo<ConstantBuffer> constant(1, 1);
    EXPECT_TRUE(code < constant);
  }
  {
    BufferInfo<CodeBuffer> code(0, 10);
    BufferInfo<ConstantBuffer> constant(1, 1);
    EXPECT_FALSE(code < constant);
  }
}
