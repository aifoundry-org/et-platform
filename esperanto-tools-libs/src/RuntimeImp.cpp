/*-------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
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

#include <cstdint>
#include <device-layer/IDeviceLayer.h>
#include <esperanto/device-apis/device_apis_message_types.h>
#include <esperanto/device-apis/operations-api/device_ops_api_cxx.h>

#include <elfio/elfio.hpp>

#include <chrono>
#include <esperanto/device-apis/operations-api/device_ops_api_rpc_types.h>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>
#include <type_traits>

using namespace rt;
using namespace rt::profiling;

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

RuntimeImp::RuntimeImp(dev::IDeviceLayer* deviceLayer, Options options)
  : deviceLayer_{deviceLayer} {

  checkMemcpyDeviceAddress_ = options.checkMemcpyDeviceOperations_;
  auto devicesCount = deviceLayer_->getDevicesCount();
  CHECK(devicesCount > 0);
  for (int i = 0; i < devicesCount; ++i) {
    auto id = DeviceId{i};
    devices_.emplace_back(id);
    auto sqCount = deviceLayer->getSubmissionQueuesCount(i);
    streamManager_.addDevice(id, sqCount);
    for (int sq = 0; sq < sqCount; ++sq) {
      commandSenders_.try_emplace(getCommandSenderIdx(i, sq), *deviceLayer_, getProfiler(), i, sq);
    }
  }
  auto dramBaseAddress = deviceLayer_->getDramBaseAddress();
  auto dramSize = deviceLayer_->getDramSize();
  RT_LOG(INFO) << std::hex << "Runtime initialization. Dram base addr: " << dramBaseAddress
               << " Dram size: " << dramSize;

  auto maxElementCount = 0UL;
  auto totalElementSize = 0UL;

  for (auto& d : devices_) {
    auto tracingBufferSize =
      deviceLayer_->getTraceBufferSizeMasterMinion(static_cast<int>(d), dev::TraceBufferType::TraceBufferCM) +
      deviceLayer_->getTraceBufferSizeMasterMinion(static_cast<int>(d), dev::TraceBufferType::TraceBufferMM);
    memoryManagers_.try_emplace(d, dramBaseAddress, dramSize, kBlockSize);
    auto devInt = static_cast<int>(d);
    deviceTracing_.try_emplace(
      d,
      DeviceFwTracing{std::make_unique<DmaBufferImp>(devInt, tracingBufferSize, true, deviceLayer_), nullptr, nullptr});
    auto dmaInfo = deviceLayer_->getDmaInfo(devInt);
    maxElementCount = std::max(maxElementCount, dmaInfo.maxElementCount_);
    totalElementSize += dmaInfo.maxElementSize_;
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
        cmaManagers_.try_emplace(d, std::make_unique<CmaManager>(std::make_unique<DmaBufferImp>(
                                      static_cast<int>(d), cmaPerDevice, true, deviceLayer_)));
      }
      break; // if we were able to do the required allocations, end the loop; if not try asking for less memory.
    } catch (const dev::Exception&) {
      auto newCmaPerDevice = 3 * cmaPerDevice / 4;
      RT_LOG(WARNING) << "There is not enough CMA memory to allocate desired " << cmaPerDevice
                      << " bytes per device. Trying with " << newCmaPerDevice << " bytes per device.";
      cmaPerDevice = newCmaPerDevice;
    }
  }
  RT_LOG_IF(FATAL, cmaPerDevice < kBlockSize) << "Error: need at least " << kBlockSize << "B of CMA per device to work";
  responseReceiver_ = std::make_unique<ResponseReceiver>(deviceLayer_, this);

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
  prop.memorySize_ = deviceLayer_->getDramSize();
  prop.l3Size_ = static_cast<uint16_t>(dc.totalL3Size_ / 1024);
  prop.l2shireSize_ = static_cast<uint16_t>(dc.totalL2Size_ / 1024);
  prop.l2scratchpadSize_ = static_cast<uint16_t>(dc.totalScratchPadSize_ / 1024);
  prop.cacheLineSize_ = dc.cacheLineSize_;
  prop.l2CacheBanks_ = dc.numL2CacheBanks_;
  prop.computeMinionShireMask_ = dc.computeMinionShireMask_;
  prop.spareComputeMinionoShireId_ = dc.spareComputeMinionoShireId_;
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

  return prop;
}

LoadCodeResult RuntimeImp::doLoadCode(StreamId stream, const std::byte* data, size_t size) {
  SpinLock lock(mutex_);

  auto stInfo = streamManager_.getStreamInfo(stream);

  auto memStream = std::istringstream(std::string(reinterpret_cast<const char*>(data), size), std::ios::binary);

  ELFIO::elfio elf;
  if (!elf.load(memStream)) {
    throw Exception("Error parsing elf");
  }

  auto extraSize = 0UL;
  for (auto& segment : elf.segments) {
    if (segment->get_type() & PT_LOAD) {
      auto fileSize = segment->get_file_size();
      auto memSize = segment->get_memory_size();
      extraSize += memSize - fileSize;
    }
  }

  // we need to add all the diff between fileSize and memSize to the final size
  // allocate a buffer in the device to load the code

  auto deviceBuffer = doMallocDevice(DeviceId{stInfo.device_}, size + extraSize, kCacheLineSize);

  // copy the execution code into the device
  // iterate over all the LOAD segments, writing them to device memory
  uint64_t basePhysicalAddress;
  bool basePhysicalAddressCalculated = false;
  auto entry = elf.get_entry();

  std::vector<EventId> events;
  std::vector<std::byte*> cmaBuffers;
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
      cmaBuffers.emplace_back(cmaManagers_.at(DeviceId{stInfo.device_})->alloc(memSize));
      auto currentBuffer = cmaBuffers.back();
      // first fill with fileSize
      std::copy(data + offset, data + offset + fileSize, currentBuffer);
      if (memSize > fileSize) {
        RT_VLOG(LOW) << "Memsize of segment " << segment->get_index() << " is larger than fileSize. Filling with 0s";
        std::fill_n(reinterpret_cast<uint8_t*>(currentBuffer) + fileSize, memSize - fileSize, 0);
      }
      RT_VLOG(LOW) << "S: " << segment->get_index() << std::hex << " O: 0x" << offset << " PA: 0x" << loadAddress
                   << " MS: 0x" << memSize << " FS: 0x" << fileSize << " @: 0x" << addr << " E: 0x" << entry << "\n";
      events.emplace_back(doMemcpyHostToDevice(stream, currentBuffer, reinterpret_cast<std::byte*>(addr), memSize,
                                               false, defaultCmaCopyFunction));
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

  // add another thread to dispatch the buffers once the copy is done
  eventManager_.addOnDispatchCallback(
    {std::move(events),
     [this, buffers = std::move(cmaBuffers), dev = DeviceId{stInfo.device_}, evt = loadCodeResult.event_] {
       RT_VLOG(LOW) << "Load code ended. Releasing buffers.";
       for (auto b : buffers) {
         cmaManagers_.at(dev)->free(b);
       }
       dispatch(evt);
     }});
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
}

std::byte* RuntimeImp::doMallocDevice(DeviceId device, size_t size, uint32_t alignment) {
  RT_VLOG(LOW) << "Malloc requested device " << std::hex << static_cast<std::underlying_type_t<DeviceId>>(device)
               << " size: " << size << " alignment: " << alignment;

  if (__builtin_popcount(alignment) != 1) {
    throw Exception("Alignment must be power of two");
  }

  std::lock_guard lock(mutex_);
  auto it = find(memoryManagers_, device);
  return it->second.malloc(size, alignment);
}

void RuntimeImp::doFreeDevice(DeviceId device, std::byte* buffer) {
  RT_VLOG(LOW) << "Free at device: " << static_cast<std::underlying_type_t<DeviceId>>(device)
               << " buffer address: " << std::hex << buffer;
  std::lock_guard lock(mutex_);
  find(memoryManagers_, device)->second.free(buffer);
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
  auto events = streamManager_.getLiveEvents(stream);
  std::stringstream ss;
  for (auto e : events) {
    ss << static_cast<int>(e) << " ";
  }
  RT_VLOG(HIGH) << "WaitForStream: number of events to wait for: " << events.size() << ". Events: " << ss.str();
  for (auto e : events) {
    RT_VLOG(LOW) << "WaitForStream: Waiting for event " << static_cast<int>(e);
    if (!doWaitForEvent(e, timeout)) {
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

void RuntimeImp::processResponseError(const ResponseError& responseError) {
  tp_.pushTask([this, responseError] {
    bool dispatchNow = true;
    // here we have to check if there is an associated errorbuffer with the event; if so, copy the buffer from
    // devicebuffer into dmabuffer; then do the callback
    auto [errorCode, event, kernelExtra] = responseError;
    StreamError streamError(errorCode);
    if (executionContextCache_) {
      if (auto buffer = executionContextCache_->getReservedBuffer(event); buffer != nullptr) {
        // TODO remove this when ticket https://esperantotech.atlassian.net/browse/SW-9617 is fixed
        if (errorCode != DeviceErrorCode::KernelLaunchHostAborted) {
          // do the copy
          auto st = doCreateStream(buffer->device_);
          auto errorContexts = std::vector<ErrorContext>(kNumErrorContexts);
          auto e = memcpyDeviceToHost(st, buffer->getExceptionContextPtr(),
                                      reinterpret_cast<std::byte*>(errorContexts.data()), kExceptionBufferSize, false);
          doWaitForEvent(e);
          streamError.errorContext_.emplace(std::move(errorContexts));
          executionContextCache_->releaseBuffer(event);
        } else {
          SpinLock lock(mutex_);
          if (kernelAbortedCallback_) {
            dispatchNow = false;
            auto th = std::thread([cb = this->kernelAbortedCallback_, evt = responseError.event_, buffer, this] {
              cb(evt, buffer->getExceptionContextPtr(), kExceptionBufferSize,
                 [this, evt] { executionContextCache_->releaseBuffer(evt); });
              dispatch(evt);
            });
            th.detach();
          } else {
            executionContextCache_->releaseBuffer(event);
          }
        }
      }
    }
    if (kernelExtra) {
      streamError.cmShireMask_ = kernelExtra->cm_shire_mask;
    }
    if (!streamManager_.executeCallback(event, streamError, [this, evt = event, dispatchNow] {
          if (dispatchNow) {
            dispatch(evt);
          }
        })) {
      // the callback was not set, so add the error to the error list
      streamManager_.addError(event, std::move(streamError));
      if (dispatchNow) {
        dispatch(event);
      }
    }
  });
}

void RuntimeImp::onResponseReceived(const std::vector<std::byte>& response) {

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
  switch (header->rsp_hdr.msg_id) {
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_API_COMPATIBILITY_RSP:
    if (auto r = reinterpret_cast<const device_ops_api::device_ops_api_compatibility_rsp_t*>(response.data());
        r->status != device_ops_api::DEV_OPS_API_COMPATIBILITY_RESPONSE_SUCCESS) {
      responseWasOk = false;
      RT_LOG(WARNING) << "Error on device api check version: " << r->status
                      << ". Tag id: " << static_cast<int>(eventId);
      processResponseError({convert(header->rsp_hdr.msg_id, r->status), eventId});
    } else {
      SpinLock lock(mutex_);
      deviceApiVersion_.major = r->major;
      deviceApiVersion_.minor = r->minor;
      deviceApiVersion_.patch = r->patch;
    }
    break;
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_KERNEL_ABORT_RSP:
    if (auto r = reinterpret_cast<const device_ops_api::device_ops_kernel_abort_rsp_t*>(response.data());
        r->status != device_ops_api::DEV_OPS_API_KERNEL_ABORT_RESPONSE_SUCCESS) {
      responseWasOk = false;
      RT_LOG(WARNING) << "Error on kernel abort: " << r->status << ". Tag id: " << static_cast<int>(eventId);
      processResponseError({convert(header->rsp_hdr.msg_id, r->status), eventId});
    }
    break;
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DMA_READLIST_RSP: {
    auto r = reinterpret_cast<const device_ops_api::device_ops_dma_readlist_rsp_t*>(response.data());
    recordEvent(*getProfiler(), *r, eventId, ResponseType::DMARead);
    if (r->status != device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE) {
      responseWasOk = false;
      RT_LOG(WARNING) << "Error on DMA read: " << r->status << ". Tag id: " << static_cast<int>(eventId);
      processResponseError({convert(header->rsp_hdr.msg_id, r->status), eventId});
    }
    break;
  }
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_ABORT_RSP:
    if (auto r = reinterpret_cast<const device_ops_api::device_ops_abort_rsp_t*>(response.data());
        r->status != device_ops_api::DEV_OPS_API_ABORT_RESPONSE_SUCCESS) {
      responseWasOk = false;
      RT_LOG(WARNING) << "Error on abort command: " << r->status << ". Tag id: " << static_cast<int>(eventId);
      processResponseError({convert(header->rsp_hdr.msg_id, r->status), eventId});
    }
    break;
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DMA_WRITELIST_RSP: {
    auto r = reinterpret_cast<const device_ops_api::device_ops_dma_writelist_rsp_t*>(response.data());
    recordEvent(*getProfiler(), *r, eventId, ResponseType::DMAWrite);
    if (r->status != device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE) {
      responseWasOk = false;
      RT_LOG(WARNING) << "Error on DMA write: " << r->status << ". Tag id: " << static_cast<int>(eventId);
      processResponseError({convert(header->rsp_hdr.msg_id, r->status), eventId});
    }
    break;
  }
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_RSP: {
    auto r = reinterpret_cast<const device_ops_api::device_ops_kernel_launch_rsp_t*>(response.data());
    recordEvent(*getProfiler(), *r, eventId, ResponseType::Kernel);
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
      processResponseError(re);
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
      processResponseError({convert(header->rsp_hdr.msg_id, r->status), eventId});
    }
    break;
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONTROL_RSP:
    if (auto r = reinterpret_cast<const device_ops_api::device_ops_trace_rt_control_rsp_t*>(response.data());
        r->status != device_ops_api::DEV_OPS_TRACE_RT_CONTROL_RESPONSE::DEV_OPS_TRACE_RT_CONTROL_RESPONSE_SUCCESS) {
      responseWasOk = false;
      RT_LOG(WARNING) << "Error on firmware trace control (start/stop): " << r->status
                      << ". Tag id: " << static_cast<int>(eventId);
      processResponseError({convert(header->rsp_hdr.msg_id, r->status), eventId});
    }
    break;
  case device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DEVICE_FW_ERROR: {
    auto r = reinterpret_cast<const device_ops_api::device_ops_device_fw_error_t*>(response.data());
    RT_LOG(WARNING) << "Reported asynchronous ERROR event from firmware: " << r->error_type;
    processResponseError({convert(header->rsp_hdr.msg_id, r->error_type), eventId});
    break;
  }
  default:
    RT_LOG(WARNING) << "Unknown response msg id: " << header->rsp_hdr.msg_id;
    break;
  }
  // if response wasn't ok, then processResponseError will clean events
  if (responseWasOk) {
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

void RuntimeImp::getDevicesWithEventsOnFly(std::vector<int>& outResult) const {
  outResult.clear();
  std::for_each(begin(devices_), end(devices_), [this, &outResult](const auto& device) {
    if (streamManager_.hasEventsOnFly(device)) {
      outResult.emplace_back(static_cast<int>(device));
    }
  });
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
  commandSender.send(Command{cmd, commandSender, evt, evt, false, true});
  doWaitForStream(st);
  doDestroyStream(st);
  // now the responsereceiver will put the data into deviceapi field, check that
  if (!deviceApiVersion_.isValid()) {
    throw Exception("Runtime couldn't retrieve a valid device-api version.");
  }
  RT_LOG(INFO) << "Device API version: " << deviceApiVersion_.major << "." << deviceApiVersion_.minor << "."
               << deviceApiVersion_.patch;
  if (deviceApiVersion_.major != 1) {
    throw Exception("Incompatible device-api version. This runtime version supports device-api 1.X.Y.");
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
    /* TODO
    for (auto e : events) {
      lastEvt = abortCommand(e);
    }*/
    // until this ticket get resolved, we need to abort just one command:
    // https://esperantotech.atlassian.net/browse/SW-9617
    lastEvt = abortCommand(events.back());
    RT_VLOG(LOW) << "Abort command event: " << static_cast<int>(lastEvt);
    Sync(lastEvt);
    return lastEvt;
  }
}

void RuntimeImp::dispatch(EventId event) {
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

void RuntimeImp::checkDevice(int device) {
  auto state = deviceLayer_->getDeviceStateMasterMinion(device);
  RT_VLOG(LOW) << "Device state: " << static_cast<int>(state) << " Runtime running: " << (running_ ? "True" : "False");
  if (running_ && state == dev::DeviceState::PendingCommands) {
    return;
  }
  if (state != dev::DeviceState::Ready) {
    RT_LOG(WARNING) << "Device " << device << " is not ready. Current state: " << static_cast<int>(state)
                    << ". Runtime will issue abort command to all SQs of that device.";
    abortDevice(DeviceId(device));
  }
}

void RuntimeImp::abortDevice(DeviceId device) {
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
