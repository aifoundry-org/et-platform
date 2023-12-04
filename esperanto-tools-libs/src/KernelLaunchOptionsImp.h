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

#include <cereal/cereal.hpp>
#include <cereal/types/optional.hpp>
#include <cereal/types/string.hpp>

namespace rt {

constexpr size_t SIZE_4K = 4096;

struct StackConfiguration {
  uint64_t baseAddress_ = 0;
  uint64_t totalSize_ = 0;

  template <class Archive> void serialize(Archive& archive) {
    archive(baseAddress_, totalSize_);
  }
};

struct KernelLaunchOptionsImp {
  uint64_t shireMask_ = 0xFFFFFFFFUL;
  bool barrier_ = true;
  bool flushL3_ = false;
  std::optional<UserTrace> userTraceConfig_;
  std::string coreDumpFilePath_;
  std::optional<StackConfiguration> stackConfig_;

  template <class Archive> void serialize(Archive& archive) {
    archive(shireMask_, barrier_, flushL3_, userTraceConfig_, coreDumpFilePath_, stackConfig_);
  }
};

struct DefaultKernelOptions {
  static inline KernelLaunchOptionsImp defaultKernelOptions;
};

} // namespace rt

#endif // KERNEL_LAUNCH_OPTIONS_IMP
