/*-------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#ifndef _KERNEL_LAUNCH_OPTIONS_IMP_H
#define _KERNEL_LAUNCH_OPTIONS_IMP_H

#include <cstdint>
#include <optional>
#include <string>

#include "runtime/Types.h"

namespace rt {

struct stackConfiguration {
  std::byte* baseAddress_ = nullptr;
  uint64_t size_ = 0;
};

struct KernelLaunchOptionsImp {
  uint64_t shireMask_ = 0xFFFFFFFF; // made dependent of ETSOC device max shire.
  bool barrier_ = true;
  bool flushL3_ = false;
  std::optional<UserTrace> userTraceConfig_ = std::nullopt;
  std::string coreDumpFilePath_ = "";
  std::optional<stackConfiguration> stackConfig_ = std::nullopt;
};

struct DefaultKernelOptions {
  static inline KernelLaunchOptionsImp defaultKernelOptions;
};

} // namespace rt

#endif // KERNEL_LAUNCH_OPTIONS_IMP
