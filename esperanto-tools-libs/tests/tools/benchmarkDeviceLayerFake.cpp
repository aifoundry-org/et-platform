//******************************************************************************
// Copyright (c) 2025 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------

#include "benchmarkerRunner.h"
#include "runtime/DeviceLayerFake.h"
#include "runtime/Types.h"

#include <gtest/gtest.h>
#include <hostUtils/logging/Logging.h>
#include <tools/IBenchmarker.h>

TEST(BenchmarkerTool, fake) {
  auto deviceLayer = std::shared_ptr<dev::IDeviceLayer>(new dev::DeviceLayerFake);
  rt::IBenchmarker::Options options;
  options.bytesD2H = 4 << 20;
  options.bytesH2D = 4 << 20;
  options.numWorkloadsPerThread = 100;
  options.numThreads = 8;
  options.useDmaBuffers = false;
  auto rt = rt::IRuntime::create(deviceLayer, rt::Options{true, false});
  runBenchmarker(rt.get(), options);
}

int main(int argc, char** argv) {
  logging::LoggerDefault logger_;
  g3::log_levels::disable(DEBUG);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
