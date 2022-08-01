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

namespace rt::profiling {

// Dummy implementation that does nothing (for performance measuring without traces)
class DummyProfiler : public IProfilerRecorder {
public:
  void start(std::ostream&, OutputType) override {
    // Intentionally unimplemented
  }
  void stop() override {
    // Intentionally unimplemented
  }
  void record(const ProfileEvent&) override {
    // Intentionally unimplemented
  }
};

// Regluar implementation
class ProfilerImp : public IProfilerRecorder {
public:
  // IProfiler interface
  void start(std::ostream& outputStream, OutputType outputType) override {
    std::lock_guard lock{mutex_};
    recording_ = true;

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
    recording_ = false;
    // emplacing monostate makes the profiler stop recording (see record function)
    archive_.emplace<std::monostate>();
  }

  void record(const ProfileEvent& event) override {
    if (!recording_)
      return;
    std::lock_guard lock{mutex_};
    std::visit(
      [&event](auto&& archive) {
        using T = std::decay_t<decltype(archive)>;
        if constexpr (!std::is_same_v<T, std::monostate>) {
          archive(event);
        }
      },
      archive_);
  }

private:
  std::variant<std::monostate, cereal::JSONOutputArchive, cereal::PortableBinaryOutputArchive> archive_;
  std::mutex mutex_;
  bool recording_ = false;
};

} // namespace rt::profiling
