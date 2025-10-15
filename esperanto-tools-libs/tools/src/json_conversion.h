/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/

#include "tools/IBenchmarker.h"
#include <nlohmann/json.hpp>
namespace rt {

void to_json(nlohmann::json& j, const IBenchmarker::Options& options) {
  j = nlohmann::json{{"bytesD2H", options.bytesD2H},
                     {"bytesH2D", options.bytesH2D},
                     {"numThreads", options.numThreads},
                     {"workloadsPerThread", options.numWorkloadsPerThread},
                     {"runtimeTracePath", options.runtimeTracePath},
                     {"useDmaBuffers", options.useDmaBuffers},
                     {"computeOpStats", options.computeOpStats},
                     {"kernelPath", options.kernelPath},
                     {"numH2D", options.numH2D},
                     {"numD2H", options.numD2H}};
}
void to_json(nlohmann::json& j, const IBenchmarker::OpStats& result) {
  using namespace std::chrono;
  auto start = duration_cast<microseconds>(result.start_.time_since_epoch()).count();
  auto end = duration_cast<microseconds>(result.end_.time_since_epoch()).count();
  j = nlohmann::json{
    {"event", static_cast<int>(result.evt_)}, {"start_us", start}, {"end_us", end}, {"duration_us", end - start}};
}
void to_json(nlohmann::json& j, const IBenchmarker::WorkerResult& result) {
  j = nlohmann::json{{"MBpsReceived", result.bytesReceivedPerSecond / static_cast<float>(1 << 20)},
                     {"MBpsSent", result.bytesSentPerSecond / static_cast<float>(1 << 20)},
                     {"WLps", result.workloadsPerSecond},
                     {"DeviceId", result.device},
                     {"OpStats", result.opStats_}};
}
void to_json(nlohmann::json& j, const IBenchmarker::SummaryResults& result) {
  j = nlohmann::json{{"TotalMBpsReceived", result.bytesReceivedPerSecond / static_cast<float>(1 << 20)},
                     {"TotalMBpsSent", result.bytesSentPerSecond / static_cast<float>(1 << 20)},
                     {"TotalWLps", result.workloadsPerSecond},
                     {"WorkersResults", result.workerResults}};
}
} // namespace rt