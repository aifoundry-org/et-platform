/*-------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#pragma once

#include "runtime/IProfileEvent.h"
#include "runtime/IProfiler.h"
#include "runtime/IRuntime.h"

#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>

#include <mutex>
#include <variant>

namespace rt {
namespace profiling {

class ProfilerImp : public IProfiler {
public:
  // IProfiler interface
  void start(std::ostream& outputStream, OutputType outputType) override {
    std::lock_guard lock{mutex_};

    switch (outputType) {
    case OutputType::Json:
      archive_.emplace<cereal::JSONOutputArchive>(outputStream);
      break;
    case OutputType::Binary:
      archive_.emplace<cereal::PortableBinaryOutputArchive>(outputStream);
      break;
    default:
      throw Exception("Unknown profiler output type");
    }
  }
  void stop() override {
    std::lock_guard lock{mutex_};
    // emplacing monostate makes the profiler stop recording (see record function)
    archive_.emplace<std::monostate>();
  }

  void record(const ProfileEvent& event) {
    std::visit(
      [&event, this](auto&& archive) {
        using T = std::decay_t<decltype(archive)>;
        if constexpr (!std::is_same_v<T, std::monostate>) {
          std::lock_guard lock{mutex_};
          archive(event);
        }
      },
      archive_);
  }

private:
  std::variant<std::monostate, cereal::JSONOutputArchive, cereal::PortableBinaryOutputArchive> archive_;
  std::mutex mutex_;
};

} // namespace profiling
} // namespace rt
