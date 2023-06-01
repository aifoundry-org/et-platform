/*-------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include "ProfilerImp.h"
#include "RuntimeImp.h"
#include "ScopedProfileEvent.h"
#include "runtime/IMonitor.h"
#include "runtime/IRuntime.h"
#include "runtime/Types.h"
#include "server/Client.h"
#include <easy/arbitrary_value.h>
#include <easy/details/profiler_colors.h>
#include <easy/profiler.h>

namespace rt {
using namespace profiling;

IRuntime::~IRuntime() {
  if (profiler::isEnabled()) {
    auto path = getenv("ET_EASY_PROFILER_DUMP_PATH");
    auto blocks = profiler::dumpBlocksToFile(path ? path : "runtime.prof");
    RT_LOG(INFO) << "Dumped " << blocks << " blocks to " << (path ? path : "runtime.prof");
  }
}

IRuntime::IRuntime()
  : profiler_{std::make_unique<DummyProfiler>()} {EASY_PROFILER_ENABLE EASY_THREAD_SCOPE("Runtime")}

  std::vector<DeviceId> IRuntime::getDevices() {
  EASY_FUNCTION()
  ScopedProfileEvent profileEvent(Class::GetDevices, *profiler_);
  return doGetDevices();
}

DeviceProperties IRuntime::getDeviceProperties(DeviceId device) const {
  EASY_FUNCTION()
  ScopedProfileEvent profileEvent(Class::GetDeviceProperties, *profiler_, device);
  auto prop = doGetDeviceProperties(device);
  profileEvent.setDeviceProperties(prop);
  return prop;
}

std::byte* IRuntime::mallocDevice(DeviceId device, size_t size, uint32_t alignment) {
  EASY_FUNCTION()
  ScopedProfileEvent profileEvent(Class::MallocDevice, *profiler_, device);
  return doMallocDevice(device, size, alignment);
}

void IRuntime::freeDevice(DeviceId device, std::byte* buffer) {
  EASY_FUNCTION()
  ScopedProfileEvent profileEvent(Class::FreeDevice, *profiler_, device);
  doFreeDevice(device, buffer);
}

StreamId IRuntime::createStream(DeviceId device) {
  EASY_FUNCTION()
  ScopedProfileEvent profileEvent(Class::CreateStream, *profiler_, device);
  auto st = doCreateStream(device);
  profileEvent.setStream(st);
  return st;
}

void IRuntime::destroyStream(StreamId stream) {
  EASY_FUNCTION()
  ScopedProfileEvent profileEvent(Class::DestroyStream, *profiler_, stream);
  doDestroyStream(stream);
}

EventId IRuntime::memcpyHostToDevice(StreamId stream, const std::byte* h_src, std::byte* d_dst, size_t size,
                                     bool barrier, const CmaCopyFunction& cmaCopyFunction) {
  EASY_FUNCTION()
  ScopedProfileEvent profileEvent(Class::MemcpyHostToDevice, *profiler_, stream);
  auto eventId = doMemcpyHostToDevice(stream, h_src, d_dst, size, barrier, cmaCopyFunction);
  profileEvent.setEventId(eventId);
  return eventId;
}

LoadCodeResult IRuntime::loadCode(StreamId stream, const std::byte* elf, size_t elf_size) {
  EASY_FUNCTION()
  ScopedProfileEvent profileEvent(Class::LoadCode, *profiler_, stream);
  auto res = doLoadCode(stream, elf, elf_size);
  profileEvent.setEventId(res.event_);
  profileEvent.setLoadAddress(reinterpret_cast<uint64_t>(res.loadAddress_));
  profileEvent.setKernelId(res.kernel_);
  profileEvent.recordNow();
  return res;
}

EventId IRuntime::memcpyDeviceToHost(StreamId stream, const std::byte* d_src, std::byte* h_dst, size_t size,
                                     bool barrier, const CmaCopyFunction& cmaCopyFunction) {
  EASY_FUNCTION()
  ScopedProfileEvent profileEvent(Class::MemcpyDeviceToHost, *profiler_, stream);
  auto eventId = doMemcpyDeviceToHost(stream, d_src, h_dst, size, barrier, cmaCopyFunction);
  profileEvent.setEventId(eventId);
  return eventId;
}

EventId IRuntime::memcpyHostToDevice(StreamId stream, MemcpyList memcpyList, bool barrier,
                                     const CmaCopyFunction& cmaCopyFunction) {
  EASY_FUNCTION()
  ScopedProfileEvent profileEvent(Class::MemcpyHostToDevice, *profiler_, stream);
  auto eventId = doMemcpyHostToDevice(stream, memcpyList, barrier, cmaCopyFunction);
  profileEvent.setEventId(eventId);
  return eventId;
}

EventId IRuntime::memcpyDeviceToHost(StreamId stream, MemcpyList memcpyList, bool barrier,
                                     const CmaCopyFunction& cmaCopyFunction) {
  EASY_FUNCTION()
  ScopedProfileEvent profileEvent(Class::MemcpyDeviceToHost, *profiler_, stream);
  auto eventId = doMemcpyDeviceToHost(stream, memcpyList, barrier, cmaCopyFunction);
  profileEvent.setEventId(eventId);
  return eventId;
}

EventId IRuntime::kernelLaunch(StreamId stream, KernelId kernel, const std::byte* kernel_args, size_t kernel_args_size,
                               uint64_t shire_mask, bool barrier, bool flushL3,
                               std::optional<UserTrace> userTraceConfig) {
  EASY_FUNCTION()
  ScopedProfileEvent profileEvent(Class::KernelLaunch, *profiler_, stream, kernel, -1ULL);
  auto evt = doKernelLaunch(stream, kernel, kernel_args, kernel_args_size, shire_mask, barrier, flushL3,
                            std::move(userTraceConfig));
  profileEvent.setEventId(evt);
  return evt;
}

bool IRuntime::waitForEvent(EventId event, std::chrono::seconds timeout) {
  EASY_FUNCTION(profiler::colors::Red300)
  EASY_VALUE("Event", static_cast<int>(event));
  ScopedProfileEvent profileEvent(Class::WaitForEvent, *profiler_, event);
  return doWaitForEvent(event, timeout);
}

bool IRuntime::waitForStream(StreamId stream, std::chrono::seconds timeout) {
  EASY_FUNCTION(profiler::colors::Red)
  ScopedProfileEvent profileEvent(Class::WaitForStream, *profiler_, stream);
  return doWaitForStream(stream, timeout);
}

void IRuntime::setOnStreamErrorsCallback(StreamErrorCallback callback) {
  EASY_FUNCTION()
  doSetOnStreamErrorsCallback(std::move(callback));
}

void IRuntime::setOnKernelAbortedErrorCallback(const KernelAbortedCallback& callback) {
  EASY_FUNCTION()
  doSetOnKernelAbortedErrorCallback(callback);
}

std::vector<StreamError> IRuntime::retrieveStreamErrors(StreamId stream) {
  EASY_FUNCTION()
  return doRetrieveStreamErrors(stream);
}

EventId IRuntime::abortCommand(EventId commandId, std::chrono::milliseconds timeout) {
  EASY_FUNCTION()
  return doAbortCommand(commandId, timeout);
}

EventId IRuntime::abortStream(StreamId streamId) {
  EASY_FUNCTION()
  return doAbortStream(streamId);
}

DmaInfo IRuntime::getDmaInfo(DeviceId deviceId) const {
  EASY_FUNCTION()
  return doGetDmaInfo(deviceId);
}

void IRuntime::unloadCode(KernelId kernel) {
  EASY_FUNCTION()
  ScopedProfileEvent profileEvent(Class::UnloadCode, *profiler_, kernel);
  doUnloadCode(kernel);
}

RuntimePtr IRuntime::create(dev::IDeviceLayer* deviceLayer, rt::Options options) {
  EASY_FUNCTION()
  auto res = std::make_unique<RuntimeImp>(deviceLayer, options);
  res->setProfiler(std::make_unique<profiling::ProfilerImp>());
  return res;
}

RuntimePtr IRuntime::create(const std::string& socketPath) {
  EASY_FUNCTION()
  auto res = std::make_unique<Client>(socketPath);
  res->setProfiler(std::make_unique<profiling::ProfilerImp>());
  return res;
}

std::unique_ptr<IMonitor> IMonitor::create(const std::string& socketPath) {
  EASY_FUNCTION()
  return std::make_unique<Client>(socketPath);
}

bool IRuntime::isP2PEnabled(DeviceId one, DeviceId other) const {
  EASY_FUNCTION()
  return doIsP2PEnabled(one, other);
}

EventId IRuntime::memcpyDeviceToDevice(StreamId streamSrc, DeviceId deviceDst, const std::byte* d_src, std::byte* d_dst,
                                       size_t size, bool barrier) {
  EASY_FUNCTION()
  return doMemcpyDeviceToDevice(streamSrc, deviceDst, d_src, d_dst, size, barrier);
}

EventId IRuntime::memcpyDeviceToDevice(DeviceId deviceSrc, StreamId streamDst, const std::byte* d_src, std::byte* d_dst,
                                       size_t size, bool barrier) {
  EASY_FUNCTION()
  return doMemcpyDeviceToDevice(deviceSrc, streamDst, d_src, d_dst, size, barrier);
}

} // namespace rt
