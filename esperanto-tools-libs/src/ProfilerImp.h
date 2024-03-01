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
  bool isDummy() const override {
    return true;
  }
  void recordNowOrAtStart(const ProfileEvent& event) override {
  }
};

// Regular implementation
class ProfilerImp : public IProfilerRecorder {
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
