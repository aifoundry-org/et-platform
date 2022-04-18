/*-------------------------------------------------------------------------
 * Copyright (C) 2022, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
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
                     {"kernelPath", options.kernelPath}};
}
void to_json(nlohmann::json& j, const IBenchmarker::WorkerResult& result) {
  j = nlohmann::json{{"MBpsReceived", result.bytesReceivedPerSecond / static_cast<float>(1 << 20)},
                     {"MBpsSent", result.bytesSentPerSecond / static_cast<float>(1 << 20)},
                     {"DeviceId", result.device}};
}
void to_json(nlohmann::json& j, const IBenchmarker::SummaryResults& result) {
  j = nlohmann::json{{"TotalMBpsReceived", result.bytesReceivedPerSecond / static_cast<float>(1 << 20)},
                     {"TotalMBpsSent", result.bytesSentPerSecond / static_cast<float>(1 << 20)},
                     {"WorkersResults", result.workerResults}};
}
} // namespace rt