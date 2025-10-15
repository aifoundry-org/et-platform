//******************************************************************************
// Copyright (c) 2025 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------

#include "TestUtils.h"
#include "benchmarkerRunner.h"
#include "common/Constants.h"
#include <device-layer/IDeviceLayer.h>
#include <gtest/gtest.h>
#include <hostUtils/logging/Logging.h>
#include <tools/IBenchmarker.h>

TEST(BenchmarkerTool, sysemu) {
  std::shared_ptr<dev::IDeviceLayer> deviceLayer =
    dev::IDeviceLayer::createSysEmuDeviceLayer(getSysemuDefaultOptions());
  rt::IBenchmarker::Options options;
  options.bytesD2H = 300 << 20;
  options.bytesH2D = 300 << 20;
  options.numWorkloadsPerThread = 8;
  options.numThreads = 1;
  options.useDmaBuffers = false;
  auto rt = rt::IRuntime::create(deviceLayer);
  runBenchmarker(rt.get(), options);
}

int main(int argc, char** argv) {
  logging::LoggerDefault logger_;
  g3::log_levels::disable(DEBUG);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
