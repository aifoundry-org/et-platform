/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
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

  registerThread();

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

void RemoteProfiler::recordNowOrAtStart(const ProfileEvent& event) {
  if (localProfiler_) {
    localProfiler_->recordNowOrAtStart(event);
  }
}

void RemoteProfiler::sendProfilingEvent(Worker* worker, const ProfileEvent& event) {
  assert(worker != nullptr);

  auto threadId = event.getNumericThreadId();

  // Check if the thread needs to be identified to the worker
  SpinLock lock(workerAccessedThreadsMutex_);
  auto& accessedThreads = workerAccessedThreads_[worker];
  auto [it, created] = accessedThreads.emplace(threadId, false);
  bool alreadyIdentified = it->second;
  lock.unlock();

  // Identify the thread to the worker if not already done
  if (created or (not alreadyIdentified)) {
    auto threadName = getThreadName(threadId);
    if (not threadName.empty()) {
      ProfileEvent identifyThreadEvent{Type::Instant, Class::IdentifyThread};
      identifyThreadEvent.setThreadId(threadId);
      identifyThreadEvent.setThreadName(std::move(threadName));
      worker->sendProfilerEvent(identifyThreadEvent);

      // Mark the thread as identified
      lock.lock();
      workerAccessedThreads_[worker][threadId] = true;
      lock.unlock();
    }
  }

  // Send the actual profiling event to the worker
  worker->sendProfilerEvent(event);
}

void RemoteProfiler::recordRequestProfilingEvent(EventId eventId, StreamId streamId, const ProfileEvent& event) {
  auto worker = getWorker(streamId);
  if (worker == nullptr) {
    RT_LOG(WARNING) << "Stream " << int(streamId) << " has no worker assigned";
    return;
  }

  // Emit the request profiling event
  sendProfilingEvent(worker, event);

  // Response profiling events do not have the stream id recorded
  SpinLock lock(eventToStreamAndEraliesMutex_);
  auto it = earlyProfilingEvents_.find(eventId);
  if (it != earlyProfilingEvents_.end()) {
    // Try to process any response profiling event that might have arrived too early
    auto const& earlyEvent = it->second;
    sendProfilingEvent(worker, earlyEvent);
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
    sendProfilingEvent(worker, event);
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

inline void RemoteProfiler::registerThread() {
  auto threadId = std::this_thread::get_id();

  SpinLock lock(threadIdNamesMutex_);
  auto it = threadIdNames_.find(threadId);
  if (it == threadIdNames_.end()) {
    threadIdNames_.emplace(threadId, threadName_);
  }
}

std::string RemoteProfiler::getThreadName(std::thread::id threadId) {
  if (threadId == std::this_thread::get_id()) {
    return threadName_;
  }

  SpinLock lock(threadIdNamesMutex_);
  auto it = threadIdNames_.find(threadId);
  if (it != threadIdNames_.end()) {
    return it->second;
  } else {
    return std::string();
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
