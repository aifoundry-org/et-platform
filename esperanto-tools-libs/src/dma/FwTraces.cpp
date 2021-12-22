/*-------------------------------------------------------------------------
 * Copyright (C) 2021, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include "Constants.h"
#include "RuntimeImp.h"
#include <fstream>

using namespace rt;

void RuntimeImp::dumpFwTraces(DeviceId device) {
  auto& deviceTracing = find(deviceTracing_, device)->second;
  auto oldCmOutput = deviceTracing.cmOutput_;
  auto oldMmOutput = deviceTracing.mmOutput_;
  auto envCmTrace = getenv("ET_CM_TRACE_FILEPATH");
  auto envMmTrace = getenv("ET_MM_TRACE_FILEPATH");
  std::string cmFilePath = envCmTrace ? envCmTrace : kCmPrevExecutionPath;
  std::string mmFilePath = envMmTrace ? envMmTrace : kMmPrevExecutionPath;

  auto cmOutputStream = std::ofstream(cmFilePath + "_dev_" + std::to_string(static_cast<int>(device)) + ".trace");
  auto mmOutputStream = std::ofstream(mmFilePath + "_dev_" + std::to_string(static_cast<int>(device)) + ".trace");

  deviceTracing.cmOutput_ = &cmOutputStream;
  deviceTracing.mmOutput_ = &mmOutputStream;
  auto st = createStreamWithoutProfiling(device);
  stopDeviceTracing(st, true);
  waitForStreamWithoutProfiling(st);
  destroyStreamWithoutProfiling(st);
  deviceTracing.cmOutput_ = oldCmOutput;
  deviceTracing.mmOutput_ = oldMmOutput;
}