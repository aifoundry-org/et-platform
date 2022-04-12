//******************************************************************************
// Copyright (C) 2021, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
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

#if 0 //not enabled because useDmaBuffers is not supported yet
  options.useDmaBuffers = true;
  res = benchmarker->run(options);
  ET_LOG(BENCHMARKER, INFO) << "Results using DMA Buffers (zero copy): "
                            << "\n\tBytes sent per second: " << res.bytesSentPerSecond
                            << "\n\tBytes received per second: " << res.bytesReceivedPerSecond;
#endif                            
}