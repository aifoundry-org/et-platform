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

#include "Utils.h"
#include "runtime/IProfileEvent.h"
#include "runtime/IProfiler.h"
#include "runtime/IRuntime.h"

#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>

#include <mutex>
#include <queue>
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
    if (recording_) {
      throw Exception("Profiler was already started");
    }
    recording_ = true;
    ioThread_ = std::thread(std::bind(&ProfilerImp::ioThread, this, outputType, &outputStream));
  }

  void stop() override {
    if (recording_) {
      recording_ = false;
      ioThread_.join();
    }
  }

  void record(const ProfileEvent& event) override {
    if (!recording_)
      return;
    SpinLock lock{mutex_};
    auto wasEmpty = events_.empty();
    events_.push(event);
    lock.unlock();
    if (wasEmpty) {
      cv_.notify_one();
    }
  }

  ~ProfilerImp() override {
    if (recording_) {
      stop();
    }
  }

  void ioThread(OutputType outputType, std::ostream* stream) {
    std::variant<std::monostate, cereal::JSONOutputArchive, cereal::PortableBinaryOutputArchive> archive;
    switch (outputType) {
    case OutputType::Json:
      archive.emplace<cereal::JSONOutputArchive>(*stream);
      break;
    case OutputType::Binary:
      archive.emplace<cereal::PortableBinaryOutputArchive>(*stream);
      break;
    default:
      throw Exception("Unknown profiler output type");
    }
    SpinLock lock{mutex_};
    while (recording_ || !events_.empty()) {
      if (events_.empty()) {
        cv_.wait(lock, [this] { return !events_.empty() || !recording_; });
      }
      if (!events_.empty()) {
        auto evt = events_.front();
        events_.pop();
        lock.unlock();
        std::visit(
          [&evt](auto&& arch) {
            using T = std::decay_t<decltype(arch)>;
            if constexpr (!std::is_same_v<T, std::monostate>) {
              arch(evt);
            }
          },
          archive);
        lock.lock();
      }
    }
  }

private:
  std::mutex mutex_;
  std::queue<ProfileEvent> events_;
  std::condition_variable cv_;
  std::thread ioThread_;
  bool recording_ = false;
};

} // namespace rt::profiling
