/*-------------------------------------------------------------------------
 * Copyright (C) 2022, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include "runtime/Types.h"
#include "tools/IBenchmarker.h"
#include <chrono>
#include <runtime/IRuntime.h>
#include <thread>

class Worker {
public:
  explicit Worker(size_t bytesH2D, size_t bytesD2H, rt::DeviceId device, rt::IRuntime& runtime);
  void start(int numIterations);
  rt::IBenchmarker::WorkerResult wait();
  Worker(Worker&&) = default;
  ~Worker();

private:
  rt::IRuntime& runtime_;
  rt::DeviceId device_;
  rt::StreamId stream_;
  std::thread runner_;
  rt::IBenchmarker::WorkerResult result_;

  std::vector<std::byte> hH2D_;
  std::byte* dH2D_ = nullptr;

  std::vector<std::byte> hD2H_;
  std::byte* dD2H_ = nullptr;
};