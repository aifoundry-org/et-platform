/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/

#pragma once

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "ProfilerImp.h" // IProfilerRecorder

namespace rt::profiling {

class ProfilerMock : public IProfilerRecorder {
public:
  MOCK_METHOD2(start, void(std::ostream&, OutputType));
  MOCK_METHOD0(stop, void());
  MOCK_METHOD1(record, void(const ProfileEvent&));
};

} // namespace rt::profiling