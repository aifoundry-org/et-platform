/*-------------------------------------------------------------------------
 * Copyright (C) 2021,2022, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#pragma once
#include "runtime/Types.h"
#include <bitset>
#include <runtime/IRuntime.h>
namespace rt {
constexpr auto kMaxDevices = 32;
class IBenchmarker {
public:
  struct Options {
    // total number of bytes transferred from host to device. If 0 then the workload won't do H2D transfers
    size_t bytesH2D = 4096;
    // total number of bytes transferred from device to host. If 0 then the workload won't do D2H transfers
    size_t bytesD2H = 4096;
    // number of commands for each transfer -use memcpylist- H2D
    size_t numH2D = 1;
    // number of commands for each transfer -use memcpylist- H2D
    size_t numD2H = 1;
    // path of kernel to execute. If empty then there won't be kernel execution in the workload
    std::string kernelPath;
    // number of workloads executed per thread
    uint64_t numWorkloadsPerThread = 100;
    // total number of threads
    uint64_t numThreads = 1;
    // total number of bytes transferred from host to device. If 0 then the workload won't do H2D transfers
    // indicates if we want to use dmaBuffers or not (dmaBuffers <-> zero-copy)
    bool useDmaBuffers = false;
    // optionally get runtime traces, if this is not empty runtime will gather execution traces there.
    std::string runtimeTracePath;
    // if true, then results will have timestamps for each start and end operation
    bool computeOpStats = false;
  };

  // DeviceMask by default has all devices disabled
  struct DeviceMask {
    // returns a DeviceMask with all devices enabled
    static DeviceMask enableAll();

    void enableDevice(DeviceId device);
    void disableDevice(DeviceId device);
    bool isEnabled(DeviceId device);
    std::bitset<kMaxDevices> mask_;
  };

  struct OpStats {
    explicit OpStats(EventId evt)
      : start_(std::chrono::high_resolution_clock::now())
      , evt_(evt) {
    }
    void setEnd() {
      end_ = std::chrono::high_resolution_clock::now();
    }
    std::chrono::high_resolution_clock::time_point start_;
    std::chrono::high_resolution_clock::time_point end_;
    EventId evt_;
  };
  struct WorkerResult {
    std::vector<OpStats> opStats_; // this is optionally filled with detailed start and end timestamps for each
                                   // operation
    DeviceId device; // device corresponding to these results
    float bytesSentPerSecond;
    float bytesReceivedPerSecond;
    float workloadsPerSecond;
  };
  struct SummaryResults {
    float bytesSentPerSecond;
    float bytesReceivedPerSecond;
    float workloadsPerSecond;
    std::vector<WorkerResult> workerResults;
  };

  // factory method.
  static std::unique_ptr<IBenchmarker> create(rt::IRuntime* runtime);

  // TODO: preprocess, postprocess and kernelLaunch customization functions

  // runs the benchmark. On each enabled device will be executed options.numWorkloadsPerThread * options.numThreads
  virtual SummaryResults run(Options options, DeviceMask mask = DeviceMask::enableAll()) = 0;

  virtual ~IBenchmarker() = default;
};
} // namespace rt