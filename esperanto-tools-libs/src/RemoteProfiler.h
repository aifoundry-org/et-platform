/*-------------------------------------------------------------------------
 * Copyright (C) 2024, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#pragma once

#include "ProfilerImp.h"

#include "runtime/Types.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace rt {
class Worker;
}

namespace rt::profiling {

// Worker profiler: sends back profiling events to the client
class ETRT_API RemoteProfiler : public IProfilerRecorder {
public:
  RemoteProfiler();
  ~RemoteProfiler() override = default;

  void start(std::ostream& outputStream, OutputType outputType) override;
  void stop() override;
  void record(const ProfileEvent& event) override;
  void recordNowOrAtStart(const ProfileEvent& event) override;

  void setLocalProfiler(std::unique_ptr<IProfilerRecorder>&& localProfiler);
  void setThisThreadsWorker(Worker* worker);
  void releaseThisThreadsWorker();

  void assignRemoteWorkerToStream(StreamId stream);
  void unassignRemoteWorkerFromStream(StreamId stream);

  void enableRemote();
  void disableRemote();

private:
  inline void registerThread();
  std::string getThreadName(std::thread::id threadId);

  Worker* getWorker(StreamId streamId);

  void recordRequestProfilingEvent(EventId eventId, StreamId streamId, const ProfileEvent& event);
  void recordResponseProfilingEvent(EventId eventId, const ProfileEvent& event);

  void sendProfilingEvent(Worker* worker, const ProfileEvent& event);

  std::atomic<bool> enabled_;
  std::unique_ptr<IProfilerRecorder> localProfiler_;

  mutable std::mutex streamToWorkerMutex_;
  std::unordered_map<StreamId, Worker*> streamToWorker_;

  mutable std::mutex eventToStreamAndEraliesMutex_;
  std::unordered_map<EventId, StreamId> eventToStream_;
  std::unordered_map<EventId, ProfileEvent> earlyProfilingEvents_;

  std::mutex threadIdNamesMutex_;
  std::unordered_map<std::thread::id, std::string> threadIdNames_;

  std::mutex workerAccessedThreadsMutex_;
  // For each worker, the ID of each thread that has emited an event and wether it has already been identified
  std::unordered_map<Worker*, std::unordered_map<std::thread::id, bool>> workerAccessedThreads_;

  static thread_local Worker* threadsWorker_;
};

} // namespace rt::profiling
