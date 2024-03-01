/*-------------------------------------------------------------------------
 * Copyright (C) 2024, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include "ProfilerImp.h"

namespace rt::profiling {

// IProfiler interface
void ProfilerImp::start(std::ostream& outputStream, OutputType outputType) {
  if (recording_) {
    throw Exception("Profiler was already started");
  }
  recording_ = true;
  ioThread_ = std::thread(std::bind(&ProfilerImp::ioThread, this, outputType, &outputStream));
  ProfileEvent evt(Type::Instant, Class::StartProfiling);
  evt.setExtras({{"version", kCurrentVersion}});
  record(std::move(evt));

  SpinLock lock{mutex_};
  while (!delayedEvents_.empty()) {
    auto& event = delayedEvents_.front();
    events_.emplace(std::move(event));
    delayedEvents_.pop();
  }
}

void ProfilerImp::stop() {
  if (recording_) {
    ProfileEvent evt(Type::Instant, Class::EndProfiling);
    record(std::move(evt));
    recording_ = false;
    cv_.notify_one();
    ioThread_.join();
  }
}

void ProfilerImp::record(const ProfileEvent& event) {
  if (!recording_) {
    return;
  }

  auto identifyThreadEvent = identifyThread();

  bool wasEmpty;
  {
    SpinLock lock{mutex_};
    wasEmpty = events_.empty();

    events_.push(event);

    if (identifyThreadEvent.has_value()) {
      events_.emplace(std::move(identifyThreadEvent.value()));
    }
  }

  if (wasEmpty) {
    cv_.notify_one();
  }
}

void ProfilerImp::recordNowOrAtStart(const ProfileEvent& event) {
  if (!recording_) {
    SpinLock lock{mutex_};
    delayedEvents_.emplace(event);
  } else {
    record(event);
  }
}

ProfilerImp::~ProfilerImp() {
  if (recording_) {
    stop();
  }
}

std::optional<ProfileEvent> ProfilerImp::identifyThread() {
  if (threadName_.empty()) {
    return std::nullopt;
  }

  SpinLock lock(identifiedThreadsMutex_);
  auto [it, inserted] = identifiedThreads_.emplace(std::this_thread::get_id());
  (void)it;
  lock.unlock();

  if (inserted) {
    ProfileEvent identifyThreadEvent{Type::Instant, Class::IdentifyThread};
    identifyThreadEvent.setThreadName(threadName_);
    return identifyThreadEvent;
  } else {
    return std::nullopt;
  }
}

void ProfilerImp::ioThread(OutputType outputType, std::ostream* stream) {
  profiling::IProfilerRecorder::setCurrentThreadName("Profiler IO thread");

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

} // namespace rt::profiling
