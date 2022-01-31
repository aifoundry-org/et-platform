/*-------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include "ProfilerImp.h"
#include "RuntimeImp.h"
#include "runtime/IRuntime.h"

namespace rt {
RuntimePtr IRuntime::create(dev::IDeviceLayer* deviceLayer, rt::Options options) {
  return std::make_unique<RuntimeImp>(deviceLayer, std::make_unique<profiling::ProfilerImp>(), options);
}
}
