/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/

#include "RuntimeImp.h"
#include "Constants.h"
#include "ExecutionContextCache.h"
#include "MemoryManager.h"
#include "ScopedProfileEvent.h"
#include "StreamManager.h"
#include "Utils.h"
#include "dma/CmaManager.h"
#include "dma/DmaBufferImp.h"
#include "dma/IDmaBuffer.h"
#include "runtime/IRuntime.h"
#include "runtime/Types.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <device-layer/IDeviceLayer.h>
#include <easy/arbitrary_value.h>
#include <easy/details/profiler_colors.h>
#include <easy/profiler.h>
#include <elfio/elfio.hpp>
#include <esperanto/device-apis/device_apis_message_types.h>
#include <esperanto/device-apis/operations-api/device_ops_api_cxx.h>
#include <esperanto/device-apis/operations-api/device_ops_api_rpc_types.h>
#include <hostUtils/threadPool/ThreadPool.h>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>
#include <tuple>
#include <type_traits>

using namespace rt;
using namespace rt::profiling;

void relocateELF(std::byte* runtimeBaseAddress, ELFIO::elfio& elf, std::byte* elfContents,
                 ELFIO::Elf64_Addr elfBaseAddr);
void relocateSection(std::byte* runtimeBaseAddress, std::byte* elfContents,
                     ELFIO::relocation_section_accessor& reloc_sec, ELFIO::Elf64_Addr elfBaseAddr);
std::tuple<ELFIO::Elf64_Addr, size_t> getELFBaseAddr(const ELFIO::elfio& elf);

void recordMemoryStats(IProfilerRecorder& profiler, DeviceId device, size_t free_bytes,
                       size_t max_free_contiguous_bytes, size_t allocated_memory);

RuntimeImp::~RuntimeImp() {
  RT_LOG(INFO) << "Destroying runtime";
  for (auto d : devices_) {
    setMemoryManagerDebugMode(d, false);
  }
  running_ = false;
}

DmaInfo RuntimeImp::doGetDmaInfo(DeviceId deviceId) const {
  DmaInfo res;
  auto r = deviceLayer_->getDmaInfo(static_cast<int>(deviceId));
  res.maxElementCount_ = r.maxElementCount_;
  res.maxElementSize_ = r.maxElementSize_;
  return res;
}
void RuntimeImp::onProfilerChanged() {
  for (auto& it : commandSenders_) {
    it.second.setProfiler(getProfiler());
  }
}

RuntimeImp::RuntimeImp(std::shared_ptr<dev::IDeviceLayer> const& deviceLayer, Options options)
  : deviceLayer_{deviceLayer} {

  RT_LOG(INFO) << "Profiler enabled? " << (profiler::isEnabled() ? "True" : "False");
  checkMemcpyDeviceAddress_ = options.checkMemcpyDeviceOperations_;
  auto devicesCount = deviceLayer_->getDevicesCount();
  CHECK(devicesCount > 0);

  for (int device = 0; device < devicesCount; ++device) {
    auto deviceId = DeviceId{device};
    devices_.emplace_back(deviceId);
    auto sqCount = deviceLayer->getSubmissionQueuesCount(device);
    streamManager_.addDevice(deviceId, sqCount);
    for (int sq = 0; sq < sqCount; ++sq) {
      commandSenders_.try_emplace(getCommandSenderIdx(device, sq), *deviceLayer_, getProfiler(), device, sq);
    }
  }

  auto maxElementCount = 0UL;
  auto totalElementSize = 0UL;

  for (auto& d : devices_) {
    auto devInt = static_cast<int>(d);
    auto tracingBufferSize = deviceLayer_->getTraceBufferSizeMasterMinion(devInt, dev::TraceBufferType::TraceBufferCM) +
                             deviceLayer_->getTraceBufferSizeMasterMinion(devInt, dev::TraceBufferType::TraceBufferMM);
    auto dramBaseAddress = deviceLayer_->getDramBaseAddress(devInt);
    auto dramSize = deviceLayer_->getDramSize(devInt);
    RT_LOG(INFO) << std::hex << "Runtime initialization device " << devInt << ": Dram base addr: " << dramBaseAddress
                 << " Dram size: " << dramSize
                 << " Check memcpy operations: " << (checkMemcpyDeviceAddress_ ? "True" : "False");

    memoryManagers_.try_emplace(d, dramBaseAddress, dramSize, kBlockSize);
    deviceTracing_.try_emplace(
      d, DeviceFwTracing{std::make_unique<DmaBufferImp>(devInt, tracingBufferSize, true, *deviceLayer_), nullptr,
                         nullptr});
    auto dmaInfo = deviceLayer_->getDmaInfo(devInt);
    maxElementCount = std::max(maxElementCount, dmaInfo.maxElementCount_);
    totalElementSize += dmaInfo.maxElementSize_;
    threadPools_.try_emplace(DeviceId{d}, std::make_unique<threadPool::ThreadPool>(4));
    errorHandlingThreadPools_.try_emplace(DeviceId{d}, std::make_unique<threadPool::ThreadPool>(1));
    abortSync_.try_emplace(DeviceId{d});
  }
  auto desiredCma = maxElementCount * totalElementSize * kAllocFactorTotalMaxMemory;
  auto envCma = getenv("ET_CMA_SIZE");
  if (envCma) {
    auto mem = std::max(std::stoull(envCma), static_cast<unsigned long long>(uint32_t(devicesCount) * kBlockSize));
    RT_LOG(INFO) << "Overriding default calculated CMA size of " << desiredCma << " with a CMA size of " << mem;
    desiredCma = mem;
  }
  auto cmaPerDevice = desiredCma / static_cast<uint32_t>(devicesCount);
  while (cmaPerDevice >= kBlockSize) {
    try {
      for (const auto& d : devices_) {
        auto devInt = static_cast<int>(d);
        auto dmaInfo = deviceLayer_->getDmaInfo(devInt);
        cmaManagers_.try_emplace(
          d, std::make_unique<CmaManager>(std::make_unique<DmaBufferImp>(devInt, cmaPerDevice, true, *deviceLayer_),
                                          dmaInfo.maxElementCount_ * dmaInfo.maxElementSize_));
      }
      break; // if we were able to do the required allocations, end the loop; if not try asking for less memory.
    } catch (const dev::Exception&) {
      cmaManagers_.clear();
      auto newCmaPerDevice = 3 * cmaPerDevice / 4;
      RT_LOG(WARNING) << "There is not enough CMA memory to allocate desired " << cmaPerDevice
                      << " bytes per device. Trying with " << newCmaPerDevice << " bytes per device.";
      cmaPerDevice = newCmaPerDevice;
    }
  }
  RT_LOG_IF(FATAL, cmaPerDevice < kBlockSize) << "Error: need at least " << kBlockSize << "B of CMA per device to work";
  responseReceiver_ = std::make_unique<ResponseReceiver>(*deviceLayer_, this);

  // initialization sequence, need to send abort command to ensure the device is in a proper state
  for (int d = 0; d < devicesCount; ++d) {
    RT_LOG(INFO) << "Initializing device: " << d;
    abortDevice(DeviceId{d});
    if (options.checkDeviceApiVersion_) {
      running_ = true;
      RT_LOG(INFO) << "Checking device api version for device: " << d;
      checkDeviceApi(DeviceId{d});
    }
    deviceLayer_->hintInactivity(d);
    RT_LOG(INFO) << "Device: " << d << " initialized.";
  }
  eventManager_.setThrowOnMissingEvent(true);
  running_ = true;
  executionContextCache_ = std::make_unique<ExecutionContextCache>(
    this, kNumExecutionCacheBuffers, align(kExceptionBufferSize + kBlockSize, kBlockSize));
  responseReceiver_->startDeviceChecker();
  RT_LOG(INFO) << "Runtime initialized.";
}

std::vector<DeviceId> RuntimeImp::doGetDevices() {
  return devices_;
}

DeviceProperties RuntimeImp::doGetDeviceProperties(DeviceId device) const {
  auto deviceInt = static_cast<int>(device);
  auto dc = deviceLayer_->getDeviceConfig(deviceInt);
  rt::DeviceProperties prop{};

  prop.frequency_ = dc.minionBootFrequency_;
  prop.availableShires_ = static_cast<uint32_t>(deviceLayer_->getActiveShiresNum(deviceInt));
  prop.memoryBandwidth_ = dc.ddrBandwidth_;
  prop.memorySize_ = deviceLayer_->getDramSize(static_cast<int>(device));
  prop.l3Size_ = static_cast<uint16_t>(dc.totalL3Size_ / 1024);
  prop.l2shireSize_ = static_cast<uint16_t>(dc.totalL2Size_ / 1024);
  prop.l2scratchpadSize_ = static_cast<uint16_t>(dc.totalScratchPadSize_ / 1024);
  prop.cacheLineSize_ = dc.cacheLineSize_;
  prop.l2CacheBanks_ = dc.numL2CacheBanks_;
  prop.computeMinionShireMask_ = dc.computeMinionShireMask_;
  prop.spareComputeMinionShireId_ = dc.spareComputeMinionShireId_;
  switch (dc.archRevision_) {
  case dev::DeviceConfig::ArchRevision::ETSOC1:
    prop.deviceArch_ = DeviceProperties::ArchRevision::ETSOC1;
    break;
  case dev::DeviceConfig::ArchRevision::GEPARDO:
    prop.deviceArch_ = DeviceProperties::ArchRevision::GEPARDO;
    break;
  case dev::DeviceConfig::ArchRevision::PANTERO:
    prop.deviceArch_ = DeviceProperties::ArchRevision::PANTERO;
    break;
  default:
    RT_LOG(WARNING) << "Unknown architecture revision.";
    prop.deviceArch_ = DeviceProperties::ArchRevision::UNKNOWN;
    break;
  }
  prop.formFactor_ = static_cast<DeviceProperties::FormFactor>(dc.formFactor_);
  prop.tdp_ = dc.tdp_;

  prop.localScpFormat0BaseAddress_ = dc.localScpFormat0BaseAddress_;
  prop.localScpFormat1BaseAddress_ = dc.localScpFormat1BaseAddress_;
  prop.localDRAMBaseAddress_ = dc.localDRAMBaseAddress_;
  prop.onPkgScpFormat2BaseAddress_ = dc.onPkgScpFormat2BaseAddress_;
  prop.onPkgDRAMBaseAddress_ = dc.onPkgDRAMBaseAddress_;
  prop.onPkgDRAMInterleavedBaseAddress_ = dc.onPkgDRAMInterleavedBaseAddress_;

  prop.localDRAMSize_ = dc.localDRAMSize_;
  prop.minimumAddressAlignmentBits_ = dc.minimumAddressAlignmentBits_;
  prop.numChiplets_ = dc.numChiplets_;

  prop.localScpFormat0ShireLSb_ = dc.localScpFormat0ShireLSb_;
  prop.localScpFormat0ShireBits_ = dc.localScpFormat0ShireBits_;
  prop.localScpFormat0LocalShire_ = dc.localScpFormat0LocalShire_;

  prop.localScpFormat1ShireLSb_ = dc.localScpFormat1ShireLSb_;
  prop.localScpFormat1ShireBits_ = dc.localScpFormat1ShireBits_;

  prop.onPkgScpFormat2ShireLSb_ = dc.onPkgScpFormat2ShireLSb_;
  prop.onPkgScpFormat2ShireBits_ = dc.onPkgScpFormat2ShireBits_;
  prop.onPkgScpFormat2ChipletLSb_ = dc.onPkgScpFormat2ChipletLSb_;
  prop.onPkgScpFormat2ChipletBits_ = dc.onPkgScpFormat2ChipletBits_;

  prop.onPkgDRAMChipletLSb_ = dc.onPkgDRAMChipletLSb_;
  prop.onPkgDRAMChipletBits_ = dc.onPkgDRAMChipletBits_;

  prop.onPkgDRAMInterleavedChipletLSb_ = dc.onPkgDRAMInterleavedChipletLSb_;
  prop.onPkgDRAMInterleavedChipletBits_ = dc.onPkgDRAMInterleavedChipletBits_;

  return prop;
}

LoadCodeResult RuntimeImp::doLoadCode(StreamId stream, const std::byte* data, size_t size) {
  SpinLock lock(mutex_);

  auto stInfo = streamManager_.getStreamInfo(stream);

  std::vector<std::byte> dataCopy(size);
  std::byte* elfContents = dataCopy.data();
  std::copy(data, data + size, elfContents);

  auto memStream = std::istringstream(std::string(reinterpret_cast<const char*>(elfContents), size), std::ios::binary);

  ELFIO::elfio elf;
  if (!elf.load(memStream)) {
    throw Exception("Error parsing elf");
  }

  auto [elfBaseAddr, extraSize] = getELFBaseAddr(elf);

  // we need to add all the diff between fileSize and memSize to the final size
  // allocate a buffer in the device to load the code

  auto deviceBuffer = doMallocDevice(DeviceId{stInfo.device_}, size + extraSize, kCacheLineSize);

  // Handle the elf relocations
  relocateELF(deviceBuffer, elf, elfContents, elfBaseAddr);

  // copy the execution code into the device
  // iterate over all the LOAD segments, writing them to device memory
  uint64_t basePhysicalAddress;
  bool basePhysicalAddressCalculated = false;
  auto entry = elf.get_entry();

  std::vector<EventId> events;
  for (auto&& segment : elf.segments) {
    if (segment->get_type() & PT_LOAD) {
      auto offset = segment->get_offset();
      auto loadAddress = segment->get_physical_address();
      auto fileSize = segment->get_file_size();
      auto memSize = segment->get_memory_size();
      if (memSize == 0) {
        RT_LOG(WARNING) << "Segment " << segment->get_index() << " is 0-sized; skipping it.";
        continue;
      }
      auto addr = reinterpret_cast<uint64_t>(deviceBuffer) + offset;
      CHECK(memSize >= fileSize);
      if (!basePhysicalAddressCalculated) {
        basePhysicalAddress = loadAddress - offset;
        basePhysicalAddressCalculated = true;
      }
      // allocate a dmabuffer to do the copy
      std::vector<std::byte> currentBuffer{memSize};
      // first fill with fileSize
      std::copy(elfContents + offset, elfContents + offset + fileSize, currentBuffer.data());
      if (memSize > fileSize) {
        RT_VLOG(LOW) << "Memsize of segment " << segment->get_index() << " is larger than fileSize. Filling with 0s";
        std::fill_n(reinterpret_cast<uint8_t*>(currentBuffer.data()) + fileSize, memSize - fileSize, 0);
      }
      RT_VLOG(LOW) << "S: " << segment->get_index() << std::hex << " O: 0x" << offset << " PA: 0x" << loadAddress
                   << " MS: 0x" << memSize << " FS: 0x" << fileSize << " @: 0x" << addr << " E: 0x" << entry << "\n";
      events.emplace_back(doMemcpyHostToDevice(stream, currentBuffer.data(), reinterpret_cast<std::byte*>(addr),
                                               memSize, false, defaultCmaCopyFunction));
      eventManager_.addOnDispatchCallback({{events.back()}, [buffer = std::move(currentBuffer)] {
                                             // do nothing, it will release the buffer
                                           }});
    }
  }
  if (!basePhysicalAddressCalculated) {
    throw Exception("Error calculating kernel entrypoint");
  }

  auto kernel = std::make_unique<Kernel>(DeviceId{stInfo.device_}, deviceBuffer, entry - basePhysicalAddress);

  // store the ref
  auto kernelId = static_cast<KernelId>(nextKernelId_++);
  auto it = kernels_.find(kernelId);
  if (it != end(kernels_)) {
    throw Exception("Can't create kernel");
  }
  kernels_.emplace(kernelId, std::move(kernel));

  // fill the struct results
  LoadCodeResult loadCodeResult;
  loadCodeResult.loadAddress_ = deviceBuffer;
  loadCodeResult.kernel_ = kernelId;

  loadCodeResult.event_ = eventManager_.getNextId();
  streamManager_.addEvent(stream, loadCodeResult.event_);
  eventManager_.addOnDispatchCallback({std::move(events), [this, evt = loadCodeResult.event_] {
                                         RT_VLOG(LOW) << "Load code ended.";
                                         dispatch(evt);
                                       }});
  coreDumper_.addCodeAddress(DeviceId{stInfo.device_}, deviceBuffer);
  return loadCodeResult;
}

void RuntimeImp::doUnloadCode(KernelId kernel) {
  SpinLock lock(mutex_);
  auto it = find(kernels_, kernel);
  auto deviceId = it->second->deviceId_;
  auto deviceBuffer = it->second->deviceBuffer_;
  RT_VLOG(LOW) << "Unloading kernel from deviceId " << static_cast<std::underlying_type_t<DeviceId>>(deviceId)
               << " buffer: " << deviceBuffer;

  // free the buffer
  doFreeDevice(deviceId, it->second->deviceBuffer_);

  // and remove the kernel
  kernels_.erase(it);
  coreDumper_.removeCodeAddress(deviceId, deviceBuffer);
}

void recordMemoryStats(IProfilerRecorder& profiler, DeviceId device, const size_t free_bytes,
                       const size_t max_free_contiguous_bytes, const size_t allocated_memory) {
  ProfileEvent evt(Type::Counter, Class::MemoryStats);
  evt.setTimeStamp();
  evt.setThreadId();
  evt.setDeviceId(device);
  evt.setAllocatedMemory(allocated_memory);
  evt.setMaxContiguousFreeMemory(max_free_contiguous_bytes);
  evt.setFreeMemory(free_bytes);
  profiler.record(evt);
}

std::byte* RuntimeImp::doMallocDevice(DeviceId device, size_t size, uint32_t alignment) {
  RT_VLOG(LOW) << "Malloc requested device " << std::hex << static_cast<std::underlying_type_t<DeviceId>>(device)
               << " size: " << size << " alignment: " << alignment;

  if (__builtin_popcount(alignment) != 1) {
    throw Exception("Alignment must be power of two");
  }

  std::unique_lock lock(mutex_);
  auto it = find(memoryManagers_, device);
  auto ptr = it->second.malloc(size, alignment);
  const size_t free_bytes = it->second.getFreeBytes();
  const size_t max_free_contiguous_bytes = it->second.getFreeContiguousBytes();
  const size_t allocated_memory = it->second.getTotalMemoryBytes() - free_bytes;
  lock.unlock();
  recordMemoryStats(*getProfiler(), device, free_bytes, max_free_contiguous_bytes, allocated_memory);
  return ptr;
}

void RuntimeImp::doFreeDevice(DeviceId device, std::byte* buffer) {
  RT_VLOG(LOW) << "Free at device: " << static_cast<std::underlying_type_t<DeviceId>>(device)
               << " buffer address: " << std::hex << buffer;
  std::unique_lock lock(mutex_);
  auto it = find(memoryManagers_, device);
  it->second.free(buffer);
  const size_t free_bytes = it->second.getFreeBytes();
  const size_t max_free_contiguous_bytes = it->second.getFreeContiguousBytes();
  const size_t allocated_memory = it->second.getTotalMemoryBytes() - free_bytes;
  lock.unlock();
  recordMemoryStats(*getProfiler(), device, free_bytes, max_free_contiguous_bytes, allocated_memory);
}

StreamId RuntimeImp::doCreateStream(DeviceId device) {
  RT_VLOG(LOW) << "Creating stream at device: " << static_cast<std::underlying_type_t<DeviceId>>(device);
  return streamManager_.createStream(device);
}

void RuntimeImp::doDestroyStream(StreamId stream) {
  RT_VLOG(LOW) << "Destroying stream: " << static_cast<std::underlying_type_t<StreamId>>(stream);
  streamManager_.destroyStream(stream);
}

bool RuntimeImp::doWaitForEvent(EventId event, std::chrono::seconds timeout) {
  EASY_FUNCTION()
  if (!running_) {
    RT_LOG(WARNING) << "Trying to wait for an event but runtime is not running anymore, returning.";
    return true;
  }

  RT_VLOG(LOW) << "Waiting for event " << static_cast<int>(event) << " to be dispatched.";
  auto res = eventManager_.blockUntilDispatched(event, timeout);
  RT_VLOG(LOW) << "Finished wait for event " << static_cast<int>(event) << " timed out? " << (res ? "false" : "true");
  return res;
}

bool RuntimeImp::doWaitForStream(StreamId stream, std::chrono::seconds timeout) {
  auto start = std::chrono::steady_clock::now();

  auto events = streamManager_.getLiveEvents(stream);

#ifdef NDEBUG
  std::stringstream ss;
  for (auto e : events) {
    ss << static_cast<int>(e) << " ";
  }
  RT_VLOG(HIGH) << "WaitForStream: number of events to wait for: " << events.size() << ". Events: " << ss.str();
#endif

  decltype(events) remainingEvents;

  if (!events.empty()) {
    std::sort(events.begin(), events.end());

    auto maxEvent = *events.rbegin();
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start);
    auto remainingTime = timeout - elapsed;

    // Just wait for the highest numbered event
    RT_VLOG(LOW) << "WaitForStream: Waiting for event " << static_cast<int>(maxEvent) << " for "
                 << remainingTime.count() << " seconds";
    if (!doWaitForEvent(maxEvent, remainingTime)) {
      return false;
    }

    auto currentEvents = streamManager_.getLiveEvents(stream);
    std::sort(currentEvents.begin(), currentEvents.end());

    // remainingEvents := intersection between events and currentEvents
    std::set_intersection(events.begin(), events.end(), currentEvents.begin(), currentEvents.end(),
                          std::back_inserter(remainingEvents));
  }

  // If there are still any remaining events from the initial set, wait for them individually
  for (auto e : remainingEvents) {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start);
    auto remainingTime = timeout - elapsed;

    RT_VLOG(LOW) << "WaitForStream: Waiting for event " << static_cast<int>(e) << " for " << remainingTime.count()
                 << " seconds";
    if (!doWaitForEvent(e, remainingTime)) {
      return false;
    }
  }

  return true;
}

void RuntimeImp::doSetOnStreamErrorsCallback(StreamErrorCallback callback) {
  streamManager_.setErrorCallback(std::move(callback));
}

void RuntimeImp::doSetOnKernelAbortedErrorCallback(const KernelAbortedCallback& callback) {
  SpinLock lock(mutex_);
  kernelAbortedCallback_ = callback;
}

void RuntimeImp::handleKernelAbortedCallback(EventId event) {
  auto buffer = executionContextCache_->getReservedBuffer(event);
  auto exceptionContext = (buffer != nullptr ? buffer->getExceptionContextPtr() : nullptr);

  RT_LOG(INFO) << "Executing kernel abort callback";
  auto th = std::thread([event, exceptionContext, this] {
    static std::atomic<int> threadId = 0;
    profiling::IProfilerRecorder::setCurrentThreadName("Kernel aborted callback thread " + std::to_string(threadId++));

    kernelAbortedCallback_(event, exceptionContext, kExceptionBufferSize,
                           [this, event] { executionContextCache_->releaseBuffer(event); });
    RT_LOG(INFO) << "Dispatching event " << int(event);
    dispatch(event);
  });
  th.detach();
}

void RuntimeImp::processResponseError(DeviceId device, const ResponseError& responseError) {
  EASY_FUNCTION()
  using namespace std::chrono_literals;
  RT_LOG(INFO) << "processResponseError Event: " << int(responseError.event_);
  errorHandlingThreadPools_.at(device)->pushTask([this, device, responseError] {
    waitUntilAllowedToProcessResponseErrors(device);

    bool dispatchNow = true;
    // here we have to check if there is an associated errorbuffer with the event; if so, copy the buffer from
    // devicebuffer into dmabuffer; then do the callback
    auto [errorCode, event, kernelExtra] = responseError;
    RT_LOG(INFO) << "processResponseError lambda Event: " << int(responseError.event_)
                 << " CapturedEvent: " << int(event);
    StreamError streamError(errorCode, device);
    auto stInfo = streamManager_.getStreamInfo(event);
    if (stInfo) {
      streamError.stream_ = stInfo->id_;
    }
    if (kernelExtra) {
      streamError.cmShireMask_ = kernelExtra->cm_shire_mask;
    }

    if (executionContextCache_) {
      if (auto buffer = executionContextCache_->getReservedBuffer(event); buffer != nullptr) {
        if (errorCode == DeviceErrorCode::KernelAbortHostAborted) {
          RT_LOG(INFO) << "Error code is a KernelAbortHostAborted";
          if (streamError.stream_) {
            RT_LOG(INFO) << "stream found";
            auto st = *streamError.stream_;
            auto evts = streamManager_.getLiveEvents(st);
            for (auto e : evts) {
              RT_LOG(INFO) << "Waiting for event " << int(e);
              if (e != event && !doWaitForEvent(e, 1s)) { // if some event is not yet finished, we will try later
                RT_LOG(INFO) << "Event was not ready, requeue";
                errorHandlingThreadPools_.at(device)->pushTask(
                  [this, device, responseError] { processResponseError(device, responseError); });
                return;
              }
            }
          }
        }
        if (errorCode == DeviceErrorCode::KernelLaunchHostAborted) {
          RT_LOG(INFO) << "Error code is a KernelLaunchHostAborted";
          if (kernelAbortedCallback_) {
            handleKernelAbortedCallback(event);
            dispatchNow = false;
          }
        }
        if (auto st = streamError.stream_; st.has_value()) {
          // if we reach here, there are no more events in associated stream so we can do the copy
          auto errorContexts = std::vector<ErrorContext>(kNumErrorContexts);
          auto copyStream = doCreateStream(buffer->device_);
          auto e = memcpyDeviceToHost(copyStream, buffer->getExceptionContextPtr(),
                                      reinterpret_cast<std::byte*>(errorContexts.data()), kExceptionBufferSize, false);
          doWaitForEvent(e);
          streamError.errorContext_.emplace(std::move(errorContexts));
          SpinLock mmlock(mutex_);
          auto allocs = memoryManagers_.at(device).getAllocations();
          mmlock.unlock();
          coreDumper_.dump(event, allocs, streamError, *this);
        }
        if (dispatchNow) {
          executionContextCache_->releaseBuffer(event);
        }
      }
    }
    auto afterCb = [this, evt = event, dispatchNow] {
      if (dispatchNow) {
        dispatch(evt);
      }
    };

    if (!streamManager_.executeCallback(event, streamError, afterCb)) {
      // the callback was not set, so add the error to the error list
      streamManager_.addError(event, std::move(streamError));
      if (dispatchNow) {
        dispatch(event);
      }
    }
  });
}

void RuntimeImp::lockProcessingResponseErrors(DeviceId device, EventId eventId) {
  auto& sync = abortSync_[device];

  SpinLock lock(sync.mutex_);
  RT_LOG(INFO) << "Abort phase begins. Tag id: " << static_cast<int>(eventId);
  sync.numBlockers_++;
}

void RuntimeImp::unlockProcessingResponseErrors(DeviceId device, EventId eventId) {
  auto& sync = abortSync_[device];

  SpinLock lock(sync.mutex_);
  RT_LOG(INFO) << "Abort phase finished. Tag id: " << static_cast<int>(eventId);
  sync.numBlockers_--;
  if (sync.numBlockers_ == 0) {
    sync.condVar_.notify_one();
  } else if (sync.numBlockers_ < 0) {
    RT_LOG(FATAL) << "Unbalanced number of abort unblockers.";
  }
}

void RuntimeImp::waitUntilAllowedToProcessResponseErrors(DeviceId device) {
  auto& sync = abortSync_[device];

  SpinLock lock(sync.mutex_);
  sync.condVar_.wait(lock, [&sync] { return (sync.numBlockers_ == 0); });
}

void RuntimeImp::onResponseReceived(DeviceId device, const std::vector<std::byte>& response) {
  EASY_FUNCTION()
  // check the response header
  auto header = reinterpret_cast<const rsp_header_t*>(response.data());
  auto eventId = EventId{header->rsp_hdr.tag_id};

  auto recordEvent = [](auto& profiler, const auto& rsp, const auto& evt, ResponseType rspT) {
    RT_VLOG(HIGH) << std::hex << " Start time: " << rsp.device_cmd_start_ts << " Wait time: " << rsp.device_cmd_wait_dur
                  << " Execution time: " << rsp.device_cmd_execute_dur;
    ProfileEvent event(Type::Instant, Class::ResponseReceived);
    event.setEvent(evt);
    event.setResponseType(rspT);
    event.setDeviceCmdStartTs(rsp.device_cmd_start_ts);
    event.setDeviceCmdWaitDur(rsp.device_cmd_wait_dur);
    event.setDeviceCmdExecDur(rsp.device_cmd_execute_dur);
    profiler.record(event);
  };

  RT_VLOG(MID) << "Response received eventId: " << static_cast<int>(eventId)
               << " Message Id: " << header->rsp_hdr.msg_id;
  bool responseWasOk = true;
  bool skipDispatch = false;
  switch (header->rsp_hdr.msg_id) {
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_API_COMPATIBILITY_RSP:
    if (auto r = reinterpret_cast<const device_ops_api::device_ops_api_compatibility_rsp_t*>(response.data());
        r->status != device_ops_api::DEV_OPS_API_COMPATIBILITY_RESPONSE_SUCCESS) {
      responseWasOk = false;
      RT_LOG(WARNING) << "Error on device api check version: " << r->status
                      << ". Tag id: " << static_cast<int>(eventId);
      processResponseError(device, {convert(header->rsp_hdr.msg_id, r->status), eventId});
    } else {
      SpinLock lock(mutex_);
      deviceApiVersion_.major = r->major;
      deviceApiVersion_.minor = r->minor;
      deviceApiVersion_.patch = r->patch;
    }
    break;
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_KERNEL_ABORT_RSP:
    unlockProcessingResponseErrors(device, eventId);
    if (auto r = reinterpret_cast<const device_ops_api::device_ops_kernel_abort_rsp_t*>(response.data());
        r->status != device_ops_api::DEV_OPS_API_KERNEL_ABORT_RESPONSE_SUCCESS) {
      responseWasOk = false;
      RT_LOG(WARNING) << "Error on kernel abort: " << r->status << ". Tag id: " << static_cast<int>(eventId);
      processResponseError(device, {convert(header->rsp_hdr.msg_id, r->status), eventId});
    } else if (kernelAbortedCallback_) {
      handleKernelAbortedCallback(eventId);
      skipDispatch = true;
    }
    break;
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DMA_READLIST_RSP: {
    auto r = reinterpret_cast<const device_ops_api::device_ops_dma_readlist_rsp_t*>(response.data());
    recordEvent(*getProfiler(), *r, eventId, ResponseType::DMARead);
    if (r->status != device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE) {
      responseWasOk = false;
      RT_LOG(WARNING) << "Error on DMA read: " << r->status << ". Tag id: " << static_cast<int>(eventId);
      processResponseError(device, {convert(header->rsp_hdr.msg_id, r->status), eventId});
    }
    break;
  }
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_ABORT_RSP:
    unlockProcessingResponseErrors(device, eventId);
    if (auto r = reinterpret_cast<const device_ops_api::device_ops_abort_rsp_t*>(response.data());
        r->status != device_ops_api::DEV_OPS_API_ABORT_RESPONSE_SUCCESS) {
      responseWasOk = false;
      RT_LOG(WARNING) << "Error on abort command: " << r->status << ". Tag id: " << static_cast<int>(eventId);
      processResponseError(device, {convert(header->rsp_hdr.msg_id, r->status), eventId});
    }
    break;
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DMA_WRITELIST_RSP: {
    auto r = reinterpret_cast<const device_ops_api::device_ops_dma_writelist_rsp_t*>(response.data());
    recordEvent(*getProfiler(), *r, eventId, ResponseType::DMAWrite);
    if (r->status != device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE) {
      responseWasOk = false;
      RT_LOG(WARNING) << "Error on DMA write: " << r->status << ". Tag id: " << static_cast<int>(eventId);
      processResponseError(device, {convert(header->rsp_hdr.msg_id, r->status), eventId});
    }
    break;
  }
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_RSP: {
    auto r = reinterpret_cast<const device_ops_api::device_ops_kernel_launch_rsp_t*>(response.data());
    recordEvent(*getProfiler(), *r, eventId, ResponseType::Kernel);
    RT_LOG(INFO) << "KernelLaunch Reponse Event: " << int(eventId);
    if (r->status !=
        device_ops_api::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE::DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_KERNEL_COMPLETED) {
      responseWasOk = false;
      RT_LOG(WARNING) << "Error on kernel launch: " << r->status << ". Tag id: " << static_cast<int>(eventId);
      ResponseError re{convert(header->rsp_hdr.msg_id, r->status), eventId};
      if (auto regularPayloadSize = sizeof(device_ops_api::device_ops_kernel_launch_rsp_t) - sizeof(rsp_header_t);
          r->response_info.rsp_hdr.size >= regularPayloadSize) {
        CHECK(r->response_info.rsp_hdr.size == regularPayloadSize + sizeof(device_ops_api::kernel_rsp_error_ptr_t))
          << "Incorrect response size";
        auto extra = reinterpret_cast<const device_ops_api::kernel_rsp_error_ptr_t*>(r->kernel_rsp_error_ptr);
        re.kernelLaunchErrorExtra_ = {reinterpret_cast<std::byte*>(extra->umode_exception_buffer_ptr),
                                      reinterpret_cast<std::byte*>(extra->umode_trace_buffer_ptr),
                                      extra->cm_shire_mask};
      }
      processResponseError(device, re);
    } else {
      if (executionContextCache_) {
        // it can happen on runtime initialization that executionContextCache does not exist yet and we are receiving
        // responses from previous executions
        executionContextCache_->releaseBuffer(eventId);
      }
    }
    break;
  }
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONFIG_RSP:
    if (auto r = reinterpret_cast<const device_ops_api::device_ops_trace_rt_config_rsp_t*>(response.data());
        r->status != device_ops_api::DEV_OPS_TRACE_RT_CONFIG_RESPONSE::DEV_OPS_TRACE_RT_CONFIG_RESPONSE_SUCCESS) {
      responseWasOk = false;
      RT_LOG(WARNING) << "Error on firmware trace configure: " << r->status
                      << ". Tag id: " << static_cast<int>(eventId);
      processResponseError(device, {convert(header->rsp_hdr.msg_id, r->status), eventId});
    }
    break;
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONTROL_RSP:
    if (auto r = reinterpret_cast<const device_ops_api::device_ops_trace_rt_control_rsp_t*>(response.data());
        r->status != device_ops_api::DEV_OPS_TRACE_RT_CONTROL_RESPONSE::DEV_OPS_TRACE_RT_CONTROL_RESPONSE_SUCCESS) {
      responseWasOk = false;
      RT_LOG(WARNING) << "Error on firmware trace control (start/stop): " << r->status
                      << ". Tag id: " << static_cast<int>(eventId);
      processResponseError(device, {convert(header->rsp_hdr.msg_id, r->status), eventId});
    }
    break;
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DEVICE_FW_ERROR: {
    auto r = reinterpret_cast<const device_ops_api::device_ops_device_fw_error_t*>(response.data());
    RT_LOG(WARNING) << "Reported asynchronous ERROR event from firmware: " << r->error_type;
    processResponseError(device, {convert(header->rsp_hdr.msg_id, r->error_type), eventId});
    break;
  }
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_TRACE_BUFFER_FULL_EVENT: {
    auto r = reinterpret_cast<const device_ops_api::device_ops_trace_buffer_full_event_t*>(response.data());
    RT_LOG(WARNING) << "Reported asynchronous event from firmware: Trace buffer full. This is ignored by host runtime. "
                       "Trace buffer type: "
                    << r->buffer_type;
    break;
  }
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_P2PDMA_READLIST_RSP:
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_P2PDMA_WRITELIST_RSP: {
    auto r = reinterpret_cast<const device_ops_api::device_ops_p2pdma_writelist_rsp_t*>(response.data());
    recordEvent(*getProfiler(), *r, eventId, ResponseType::DMAP2P);
    if (r->status != device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE) {
      responseWasOk = false;
      RT_LOG(WARNING) << "Error on memcpyDeviceToDevice (P2P) op: " << r->status
                      << ". Tag id: " << static_cast<int>(eventId);
      processResponseError(device, {convert(header->rsp_hdr.msg_id, r->status), eventId});
    }
    break;
  }
  default:
    RT_LOG(WARNING) << "Unknown response msg id: " << header->rsp_hdr.msg_id;
    break;
  }
  // If response wasn't ok, then processResponseError will clean events.
  // In addition, abort callbacks delay dispatching to the end of the callback.
  if (responseWasOk and !skipDispatch) {
    dispatch(eventId);
  }
}

void RuntimeImp::setMemoryManagerDebugMode(DeviceId device, bool enable) {
  RT_LOG(INFO) << "Setting memory manager debug mode: " << (enable ? "True" : "False");
  find(memoryManagers_, device)->second.setDebugMode(enable);
}

void RuntimeImp::setSentCommandCallback(DeviceId device, CommandSender::CommandSentCallback callback) {
  auto deviceInt = static_cast<int>(device);
  auto sqCount = deviceLayer_->getSubmissionQueuesCount(deviceInt);
  for (auto sq = 0; sq < sqCount; ++sq) {
    auto& cs = find(commandSenders_, getCommandSenderIdx(deviceInt, sq))->second;
    cs.setOnCommandSentCallback(callback);
  }
}

bool RuntimeImp::areEventsOnFly(DeviceId device) const {
  return streamManager_.hasEventsOnFly(device);
}

std::vector<StreamError> RuntimeImp::doRetrieveStreamErrors(StreamId stream) {
  return streamManager_.retrieveErrors(stream);
}
void RuntimeImp::checkDeviceApi(DeviceId device) {
  auto st = doCreateStream(device);
  auto streamInfo = streamManager_.getStreamInfo(st);
  auto evt = eventManager_.getNextId();
  streamManager_.addEvent(st, evt);
  std::vector<std::byte> cmd(sizeof(device_ops_api::check_device_ops_api_compatibility_cmd_t));
  auto cmdPtr = reinterpret_cast<device_ops_api::check_device_ops_api_compatibility_cmd_t*>(cmd.data());
  cmdPtr->command_info.cmd_hdr.tag_id = static_cast<uint16_t>(evt);
  cmdPtr->command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_CHECK_DEVICE_OPS_API_COMPATIBILITY_CMD;
  cmdPtr->command_info.cmd_hdr.size = static_cast<msg_size_t>(cmd.size());
  auto& commandSender = find(commandSenders_, getCommandSenderIdx(streamInfo.device_, streamInfo.vq_))->second;
  commandSender.send(Command{cmd, commandSender, evt, evt, st, false, true});
  doWaitForStream(st);
  doDestroyStream(st);
  // now the responsereceiver will put the data into deviceapi field, check that
  if (!deviceApiVersion_.isValid()) {
    throw Exception("Runtime couldn't retrieve a valid device-api version.");
  }
  RT_LOG(INFO) << "Device API version: " << deviceApiVersion_.major << "." << deviceApiVersion_.minor << "."
               << deviceApiVersion_.patch;
  if (deviceApiVersion_.major != 2) {
    throw Exception("Incompatible device-api version. This runtime version supports device-api 2.X.Y.");
  }
}

EventId RuntimeImp::doAbortCommand(EventId commandId, std::chrono::milliseconds timeout) {
  using namespace std::chrono_literals;
  auto stInfo = streamManager_.getStreamInfo(commandId);
  auto evt = eventManager_.getNextId();
  if (!stInfo) {
    RT_LOG(WARNING) << "Trying to abort a command which was already finished. Runtime won't do anything.";
    dispatch(evt);
  } else {
    lockProcessingResponseErrors(DeviceId{stInfo->device_}, evt);

    device_ops_api::device_ops_abort_cmd_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.tag_id = static_cast<uint16_t>(commandId);
    cmd.command_info.cmd_hdr.size = sizeof(cmd);
    cmd.command_info.cmd_hdr.flags = device_ops_api::CMD_FLAGS_BARRIER_ENABLE;
    cmd.command_info.cmd_hdr.msg_id = device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_ABORT_CMD;
    cmd.command_info.cmd_hdr.tag_id = static_cast<uint16_t>(evt);
    auto deadLine = std::chrono::steady_clock::now() + timeout;
    // check what device contains that eventid
    dev::CmdFlagMM flags;
    flags.isDma_ = false;
    flags.isHpSq_ = true;
    while (!deviceLayer_->sendCommandMasterMinion(stInfo->device_, stInfo->vq_, reinterpret_cast<std::byte*>(&cmd),
                                                  sizeof(cmd), flags)) {
      if (std::chrono::steady_clock::now() > deadLine) {
        throw rt::Exception("Couldn't use the HPSQ. Perhaps the Master Minion is hanged?");
      }
      RT_LOG(WARNING) << "Trying to abort a command but special HPSQ is full. Retrying...";
      std::this_thread::sleep_for(1ms);
    }
    streamManager_.addEvent(stInfo->id_, evt);
  }
  Sync(evt);
  return evt;
}

EventId RuntimeImp::doAbortStream(StreamId streamId) {
  auto events = streamManager_.getLiveEvents(streamId);
  if (events.empty()) {
    RT_LOG(WARNING) << "Trying to abort stream " << static_cast<int>(streamId) << " but it had no outstanding events.";
    auto evt = eventManager_.getNextId();
    eventManager_.dispatch(evt);
    return evt;
  } else {
    auto lastEvt = EventId{};
    for (auto e : events) {
      lastEvt = abortCommand(e);
    }
    RT_VLOG(LOW) << "Abort command event: " << static_cast<int>(lastEvt);
    Sync(lastEvt);
    return lastEvt;
  }
}

void RuntimeImp::dispatch(EventId event) {
  EASY_FUNCTION(profiler::colors::Green)
  EASY_VALUE("Event", static_cast<int>(event))
  if (!running_) {
    RT_LOG(WARNING) << "Trying to dispatch an event but runtime is not running. Ignoring the dispatch."
                    << static_cast<int>(event);
    return;
  }
  ProfileEvent evt(Type::Instant, Class::DispatchEvent);
  evt.setEvent(event);
  getProfiler()->record(evt);
  streamManager_.removeEvent(event);
  eventManager_.dispatch(event);
  notify(event);
}

void RuntimeImp::checkDevice(DeviceId device) {
  auto state = deviceLayer_->getDeviceStateMasterMinion(static_cast<int>(device));
  RT_VLOG(LOW) << "Device state: " << static_cast<int>(state) << " Runtime running: " << (running_ ? "True" : "False");
  if (running_ && state == dev::DeviceState::PendingCommands) {
    return;
  }
  if (state != dev::DeviceState::Ready) {
    RT_LOG(WARNING) << "Device " << static_cast<int>(device)
                    << " is not ready. Current state: " << static_cast<int>(state)
                    << ". Runtime will issue abort command to all SQs of that device.";
    abortDevice(device);
  }
}

void RuntimeImp::abortDevice(DeviceId device) {
  EASY_FUNCTION()
  using namespace std::chrono_literals;

  // we need to ensure runtime is in running state to allow dispatch and waitForStream to work properly
  auto oldRunningState = running_;
  running_ = true;
  for (auto sq = 0, sqCount = deviceLayer_->getSubmissionQueuesCount(static_cast<int>(device)); sq < sqCount; ++sq) {
    auto st = doCreateStream(device);
    auto fakeEvt = eventManager_.getNextId();
    streamManager_.addEvent(st, fakeEvt);
    abortCommand(fakeEvt, 5s);
    dispatch(fakeEvt);
    doWaitForStream(st);
    doDestroyStream(st);
  }
  running_ = oldRunningState;
}

void RuntimeImp::checkList(int device, const MemcpyList& list) const {
  EASY_FUNCTION()
  auto dmaInfo = deviceLayer_->getDmaInfo(device);
  if (list.operations_.size() > dmaInfo.maxElementCount_) {
    throw Exception("Invalid element count in memcpy list. Max elements allowed is:" +
                    std::to_string(dmaInfo.maxElementCount_));
  }
  size_t totalSize = 0;
  for (auto& op : list.operations_) {
    totalSize += op.size_;
    if (op.size_ > dmaInfo.maxElementSize_) {
      throw Exception("Invalid size in memcpy list. Max size available is: " + std::to_string(dmaInfo.maxElementSize_));
    }
  }
  if (totalSize > cmaManagers_.at(DeviceId{device})->getTotalSize()) {
    throw Exception("Required total size for the list is: " + std::to_string(totalSize) +
                    " which is larger of maximum allowed of " +
                    std::to_string(cmaManagers_.at(DeviceId{device})->getTotalSize()) + " bytes.");
  }
}

std::unordered_map<DeviceId, uint64_t> RuntimeImp::getFreeMemory() const {
  std::unordered_map<DeviceId, uint64_t> res;
  SpinLock lock(mutex_);
  for (auto d : devices_) {
    res.emplace(d, memoryManagers_.at(d).getFreeBytes());
  }
  return res;
}

std::unordered_map<DeviceId, uint32_t> RuntimeImp::getWaitingCommands() const {
  std::unordered_map<DeviceId, uint32_t> res;

  for (auto d : devices_) {
    auto dev = static_cast<int>(d);
    auto sqCount = deviceLayer_->getSubmissionQueuesCount(static_cast<int>(d));
    auto totalCommands = 0U;
    for (int sq = 0; sq < sqCount; ++sq) {
      totalCommands += static_cast<uint32_t>(commandSenders_.at(getCommandSenderIdx(dev, sq)).getCurrentSize());
    }
    res.emplace(d, totalCommands);
  }
  return res;
}

std::unordered_map<DeviceId, uint32_t> RuntimeImp::getAliveEvents() const {
  return streamManager_.getEventCount();
}

bool RuntimeImp::doIsP2PEnabled(DeviceId one, DeviceId other) const {
  return deviceLayer_->checkP2pDmaCompatibility(static_cast<int>(one), static_cast<int>(other));
}

void relocateELF(std::byte* runtimeBaseAddress, ELFIO::elfio& elf, std::byte* elfContents,
                 ELFIO::Elf64_Addr elfBaseAddr) {
  for (const auto& section : elf.sections) {
    if (section->get_type() == SHT_RELA) {
      if (section->get_name().find(".rela.debug") != std::string::npos) {
        continue; // SW-20381: Handle debug relocations
      } else {
        ELFIO::relocation_section_accessor reloc_sec(elf, section);
        relocateSection(runtimeBaseAddress, elfContents, reloc_sec, elfBaseAddr);
      }
    }
  }
}

void relocateSection(std::byte* runtimeBaseAddress, std::byte* elfContents,
                     ELFIO::relocation_section_accessor& reloc_sec, ELFIO::Elf64_Addr elfBaseAddr) {
  static constexpr uint32_t RISCV_64_RELOCATION_TYPE = 2;

  // SW-20451: A base offset of 0x1000 is taken up when the ELF is loaded on the device
  static constexpr uint32_t BASE_OFFSET = 0x1000;
  for (ELFIO::Elf_Xword i = 0; i < reloc_sec.get_entries_num(); ++i) {
    ELFIO::Elf64_Addr offset;
    ELFIO::Elf_Word symbol_index;
    ELFIO::Elf_Word type;
    ELFIO::Elf_Sxword addend;
    reloc_sec.get_entry(i, offset, symbol_index, type, addend);
    if (type == RISCV_64_RELOCATION_TYPE) {
      // Resolve the relocation here
      auto target_ptr = reinterpret_cast<uint64_t*>(elfContents + offset - elfBaseAddr + BASE_OFFSET);
      *target_ptr += reinterpret_cast<uint64_t>(runtimeBaseAddress) - elfBaseAddr + BASE_OFFSET;
    }
  }
}

std::tuple<ELFIO::Elf64_Addr, size_t> getELFBaseAddr(const ELFIO::elfio& elf) {
  ELFIO::Elf64_Addr elfBaseAddr = std::numeric_limits<ELFIO::Elf64_Addr>::max();
  auto extraSize = 0UL;
  for (auto& segment : elf.segments) {
    if (segment->get_type() & PT_LOAD) {
      if (segment->get_physical_address() < elfBaseAddr) {
        elfBaseAddr = segment->get_physical_address();
      }
      auto fileSize = segment->get_file_size();
      auto memSize = segment->get_memory_size();
      extraSize += memSize - fileSize;
    }
  }
  return std::make_tuple(elfBaseAddr, extraSize);
}
