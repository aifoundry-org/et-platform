/*------------------------------------------------------------------------------
 * Copyright (C) 2019, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 ------------------------------------------------------------------------------ */

#include "Tracing.h"

#include "Core/CoreState.h"
#include "esperanto/runtime/Core/CommandLineOptions.h"

#include "Tracing/etrt-trace.pb.h"
#include <fstream>
#include <google/protobuf/util/delimited_message_util.h>
#include <memory>

ABSL_FLAG(std::string, etrt_trace, "", "Path to runtime Protobuf trace");

namespace et_runtime {
namespace tracing {

static std::ostream *getFileStream() {
  auto &core_state = getCoreState();
  auto &ostream = core_state.runtime_trace;

  if (ostream.get() == nullptr) {
    if (auto path = absl::GetFlag(FLAGS_etrt_trace); !path.empty()) {
      ostream = std::make_unique<std::fstream>(
        path, std::ios::out | std::ios::app | std::ios::ate | std::ios::binary);
    }
  }
  return ostream.get();
}

bool saveMessageInTrace(const RuntimeTraceEntry &mentry) {
  auto *stream = getFileStream();
  if (stream) {
    return google::protobuf::util::SerializeDelimitedToOstream(mentry, stream);
  }
  return true;
}

} // namespace tracing
} // namespace et_runtime
