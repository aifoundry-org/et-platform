/*-------------------------------------------------------------------------
 * Copyright (C) 2021, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
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