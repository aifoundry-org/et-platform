/*------------------------------------------------------------------------------
 * Copyright (C) 2019, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 ------------------------------------------------------------------------------ */

#ifndef ET_RUNTIME_TRACING_H
#define ET_RUNTIME_TRACING_H

#include "Tracing/TracingCommonGen.h"

// FIXME the following defice should be coming form the project configuration

#define ETRT_ENABLE_TRACING 1
#include "Tracing/TracingGen.h"
#include "Tracing/etrt-trace.pb.h"
#include "Tracing/DeviceFWTrace.h"

namespace et_runtime {
namespace tracing {

class RuntimeTraceEntry;

bool saveMessageInTrace(const RuntimeTraceEntry &entry);

} // namespace tracing
} // namespace et_runtime

#endif //ET_RUNTIME_TRACING_H
