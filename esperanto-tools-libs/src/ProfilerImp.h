/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
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
  void start([[maybe_unused]] std::ostream&, [[maybe_unused]] OutputType) override {
    // Intentionally unimplemented
  }
  void stop() override {
    // Intentionally unimplemented
  }
  void record([[maybe_unused]] const ProfileEvent&) override {
    // Intentionally unimplemented
  }
  bool isDummy() const override {
    return true;
  }
  void recordNowOrAtStart([[maybe_unused]] const ProfileEvent& event) override {
  }
};

// Regular implementation
class ETRT_API ProfilerImp : public IProfilerRecorder {
public:
  // IProfiler interface
  void start(std::ostream& outputStream, OutputType outputType) override;
  void stop() override;
  void record(const ProfileEvent& event) override;
  void recordNowOrAtStart(const ProfileEvent& event) override;
  ~ProfilerImp() override;

private:
  void ioThread(OutputType outputType, std::ostream* stream);
  std::optional<ProfileEvent> identifyThread();

  std::mutex mutex_;
  std::queue<ProfileEvent> events_;
  std::queue<ProfileEvent> delayedEvents_;
  std::condition_variable cv_;
  std::thread ioThread_;
  bool recording_ = false;

  std::mutex identifiedThreadsMutex_;
  std::unordered_set<std::thread::id> identifiedThreads_;
};

} // namespace rt::profiling
