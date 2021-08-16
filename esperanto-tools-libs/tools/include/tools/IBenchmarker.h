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
#include <device-layer/IDeviceLayer.h>
#include <runtime/IRuntime.h>
namespace rt {
class IBenchmarker {
public:
  enum class WorkloadType {
    H2D_K_D2H, //< Host to Device transfer, kernel, Device to Host transfer
    H2D_D2H,   //< Host to Device transfer, Device to Host transfer
    K          //< Only kernel
  };

  struct Options {
    WorkloadType type = WorkloadType::H2D_K_D2H;
    uint64_t numThreads = 1; //< total number of threads to execute the workloads (numWorkloads / numThreads)
    uint64_t numWorkloads =
      100; //< total number of iterations, depending on WorkloadType each iteration will be 1, 2 or 3 commands
    uint64_t numBytesPerTransferH2D = 4096; //< total number of bytes transferred from host to device
    uint64_t numBytesPerTransferD2H = 4096; //< total number of bytes transferred from device to host
    uint64_t numCyclesPerKernel = 1000;     //< estimated on a basis of ~10 cycles is a jump. In sysemu its completely
                                            // different, < since it doesnt estimate cycles accurately.
    bool useDmaBuffers = false;             //< indicates if we want to use dmaBuffers or not (dmaBuffers <-> zero-copy)
  };

  struct Results {
    float commandsSentPerSecond;
    float bytesSentPerSecond;
    float bytesReceivedPerSecond;
  };
  // factory method. kernelsDir parameter is where all elfs are located, from test_compute_kernels
  static std::unique_ptr<IBenchmarker> create(dev::IDeviceLayer* deviceLayer, const std::string& kernelsDir);

  // retrieve the runtime, useful to setup profiler for example
  virtual IRuntime* getRuntime() = 0;

  // runs the benchmark
  virtual Results run(Options options) = 0;

  virtual ~IBenchmarker() = default;
};
} // namespace rt