/*-------------------------------------------------------------------------
 * Copyright (C) 2024, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include "RemoteProfiler.h"

#include "Utils.h"
#include "server/Worker.h"

namespace rt::profiling {

thread_local Worker* RemoteProfiler::threadsWorker_ = nullptr;

RemoteProfiler::RemoteProfiler() {
}

void RemoteProfiler::setLocalProfiler(std::unique_ptr<IProfilerRecorder>&& localProfiler) {
  localProfiler_ = std::move(localProfiler);
}

void RemoteProfiler::setThisThreadsWorker(Worker* worker) {
  threadsWorker_ = worker;
}

void RemoteProfiler::releaseThisThreadsWorker() {
  threadsWorker_ = nullptr;
}

void RemoteProfiler::start(std::ostream& outputStream, OutputType outputType) {
  if (localProfiler_) {
    localProfiler_->start(outputStream, outputType);
  }
}

void RemoteProfiler::stop() {
  if (localProfiler_) {
    localProfiler_->stop();
  }
}

void RemoteProfiler::enableRemote() {
  enabled_ = true;
}

void RemoteProfiler::disableRemote() {
  enabled_ = false;
}

void RemoteProfiler::record(const ProfileEvent& event) {
  if (localProfiler_) {
    localProfiler_->record(event);
  }

  if (!enabled_ || !event.getEvent().has_value()) {
    return;
  }

  auto cl = event.getClass();
  if ((cl != Class::ResponseReceived) && (cl != Class::CommandSent) && (cl != Class::DispatchEvent)) {
    // All other classes are also measured at the client, so here we avoid having both events in the client
    return;
  }

  auto eventId = event.getEvent().value();

  if (event.getStream().has_value()) {
    // Request profiling events have the stream id of the request saved
    auto streamId = event.getStream().value();

    recordRequestProfilingEvent(eventId, streamId, event);
  } else {
    // Response profiling events do not have the stream id recorded
    recordResponseProfilingEvent(eventId, event);
  }
}

void RemoteProfiler::recordRequestProfilingEvent(EventId eventId, StreamId streamId, const ProfileEvent& event) {
  auto worker = getWorker(streamId);
  if (worker == nullptr) {
    RT_LOG(WARNING) << "Stream " << int(streamId) << " has no worker assigned";
    return;
  }

  // Emit the request profiling event
  worker->sendProfilerEvent(event);

  // Response profiling events do not have the stream id recorded
  SpinLock lock(eventToStreamAndEraliesMutex_);
  auto it = earlyProfilingEvents_.find(eventId);
  if (it != earlyProfilingEvents_.end()) {
    // Try to process any response profiling event that might have arrived too early
    auto const& earlyEvent = it->second;
    worker->sendProfilerEvent(earlyEvent);
    earlyProfilingEvents_.erase(it);
  } else {
    // Save the event to stream association for the response profiling event
    eventToStream_[eventId] = streamId;
  }
}

void RemoteProfiler::recordResponseProfilingEvent(EventId eventId, const ProfileEvent& event) {
  SpinLock lock(eventToStreamAndEraliesMutex_);

  auto it = eventToStream_.find(eventId);
  if (it != eventToStream_.end()) {
    // A response profiling event correclty ordered after its request
    auto streamId = it->second;

    auto worker = getWorker(streamId);
    if (worker == nullptr) {
      RT_LOG(WARNING) << "Stream " << int(streamId) << " has no worker assigned";
      return;
    }

    lock.unlock();
    worker->sendProfilerEvent(event);
  } else {
    // Delay this reponse profiling event since it arrived before its matching request profiling event
    auto [it2, inserted] = earlyProfilingEvents_.try_emplace(eventId, event);
    (void)it2;

    if (not inserted) {
      RT_LOG(WARNING) << "Dropping remote profiling event with unknown worker since there is already another one "
                         "with the same event id";
    }
  }
}

Worker* RemoteProfiler::getWorker(StreamId streamId) {
  if (threadsWorker_) {
    return threadsWorker_;
  }

  SpinLock lock(streamToWorkerMutex_);
  auto it = streamToWorker_.find(streamId);
  if (it == streamToWorker_.end()) {
    return nullptr;
  } else {
    return it->second;
  }
}

void RemoteProfiler::assignRemoteWorkerToStream(StreamId stream) {
  if (threadsWorker_ == nullptr) {
    throw Exception("The current threrad's worker is not set");
  }

  SpinLock lock(streamToWorkerMutex_);
  streamToWorker_[stream] = threadsWorker_;
}

void RemoteProfiler::unassignRemoteWorkerFromStream(StreamId stream) {
  SpinLock lock(streamToWorkerMutex_);
  auto it = streamToWorker_.find(stream);
  if (it != streamToWorker_.end()) {
    streamToWorker_.erase(it);
  }
}

} // namespace rt::profiling
