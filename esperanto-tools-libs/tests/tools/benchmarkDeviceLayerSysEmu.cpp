//******************************************************************************
// Copyright (C) 2021, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "TestUtils.h"
#include "benchmarkerRunner.h"
#include "common/Constants.h"
#include <device-layer/IDeviceLayer.h>
#include <gtest/gtest.h>
#include <hostUtils/logging/Logging.h>
#include <tools/IBenchmarker.h>

TEST(BenchmarkerTool, sysemu) {
  auto deviceLayer = dev::IDeviceLayer::createSysEmuDeviceLayer(getDefaultOptions());
  rt::IBenchmarker::Options options;
  options.bytesD2H = 300 << 20;
  options.bytesH2D = 300 << 20;
  options.numWorkloadsPerThread = 8;
  options.numThreads = 1;
  options.useDmaBuffers = false;
  auto rt = rt::IRuntime::create(deviceLayer.get());
  runBenchmarker(rt.get(), options);
}

int main(int argc, char** argv) {
  logging::LoggerDefault logger_;
  g3::log_levels::disable(DEBUG);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}