//******************************************************************************
// Copyright (c) 2025 Ainekko, Co.
// SPDX-License-Identifier: Apache-2.0
//------------------------------------------------------------------------------
#pragma once

#include "common/Constants.h"
#include "tools/IBenchmarker.h"
#include <device-layer/IDeviceLayer.h>
#include <hostUtils/logging/Logging.h>
inline void runBenchmarker(rt::IRuntime* runtime, rt::IBenchmarker::Options options) {

  auto benchmarker = rt::IBenchmarker::create(runtime);
  auto res = benchmarker->run(options);
  options.useDmaBuffers = false;
  ET_LOG(BENCHMARKER, INFO) << "Results without using DMA Buffers (non-zero copy): "
                            << "\n\tBytes sent per second: " << res.bytesSentPerSecond
                            << "\n\tBytes received per second: " << res.bytesReceivedPerSecond;

#if 0 // not enabled because useDmaBuffers is not supported yet
  options.useDmaBuffers = true;
  res = benchmarker->run(options);
  ET_LOG(BENCHMARKER, INFO) << "Results using DMA Buffers (zero copy): "
                            << "\n\tBytes sent per second: " << res.bytesSentPerSecond
                            << "\n\tBytes received per second: " << res.bytesReceivedPerSecond;
#endif
}