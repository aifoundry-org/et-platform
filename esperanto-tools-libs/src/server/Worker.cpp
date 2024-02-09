/*-------------------------------------------------------------------------
 * Copyright (C) 2021,2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include "Worker.h"

#include "runtime/Types.h"

#include "NetworkException.h"
#include "ProfilerImp.h"
#include "Protocol.h"
#include "RemoteProfiler.h"
#include "ScopedProfileEvent.h"
#include "Server.h"
#include "Utils.h"

#include <cereal/archives/portable_binary.hpp>
#include <easy/profiler.h>
#include <g3log/loglevels.hpp>

#include <cstring>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <variant>

using namespace rt;

struct MemStream : public std::streambuf {
  MemStream(char* s, std::size_t n) {
    setg(s, s, s + n);
  }
};

void Worker::update(EventId event) {
  SpinLock lock(mutex_);

  if (events_.erase(event) > 0) {
    RT_DLOG(INFO) << "Dispatched at worker event " << static_cast<int>(event);
    sendResponse({resp::Type::EVENT_DISPATCHED, req::ASYNC_RUNTIME_EVENT, resp::Event{event}});
  }
}

void Worker::sendResponse(const resp::Response& resp) {
  EASY_FUNCTION(profiler::colors::Green)
  SpinLock lock(mutex_);
  RT_VLOG(MID) << "Sending response. Type: " << static_cast<uint32_t>(resp.type_) << "(" << resp::getStr(resp.type_)
               << ") Id: " << resp.id_;
  EASY_BLOCK("Serialize response")
  std::stringstream response;
  cereal::PortableBinaryOutputArchive archive(response);
  archive(resp);
  auto str = response.str();
  EASY_END_BLOCK
  EASY_BLOCK("Write socket")
  if (auto res = write(socket_, str.data(), str.size()); res < static_cast<long>(str.size())) {
    auto errorMsg = std::string{strerror(errno)};
    RT_VLOG(LOW) << "Write socket error: " << errorMsg;
    throw NetworkException("Write socket error: " + errorMsg);
  }
}

Worker::Worker(int socket, RuntimeImp& runtime, Server& server, ucred credentials)
  : runtime_(runtime)
  , server_(server)
  , socket_(socket) {

  runtime_.attach(this);
  cmaCopyFunction_ = [pid = credentials.pid](const std::byte* src, std::byte* dst, size_t size, CmaCopyType type) {
    EASY_BLOCK("CMA copy", profiler::colors::Green)
    iovec local;
    iovec remote;
    local.iov_len = remote.iov_len = size;

    ssize_t res;
    if (type == CmaCopyType::TO_CMA) {
      remote.iov_base = const_cast<std::byte*>(src);
      local.iov_base = dst;
      res = process_vm_readv(pid, &local, 1, &remote, 1, 0);
    } else {
      local.iov_base = const_cast<std::byte*>(src);
      remote.iov_base = dst;
      res = process_vm_writev(pid, &local, 1, &remote, 1, 0);
    }
    if (res != static_cast<ssize_t>(size)) {
      RT_LOG(WARNING) << "Error copying/writing from/to remote process with PID: " + std::to_string(pid) + " Src: "
                      << std::hex << src << " Dst: " << dst << std::string{" error: "} + strerror(errno);
    }
  };
  runner_ = std::thread(&Worker::requestProcessor, this);
}

Worker::~Worker() {
  RT_VLOG(LOW) << "Destroying worker " << this;
  running_ = false;
  SpinLock lock(mutex_);
  close(socket_);
  runner_.join();

  // Unregister all streams from the Remote Profiler
  auto profiler = getProfiler();
  if (profiler != nullptr) {
    for (auto stream : streams_) {
      profiler->unassignRemoteWorkerFromStream(stream);
    }
  }

  freeResources();

  RT_VLOG(LOW) << "Worker dtor ended. Ptr:" << this;
}

rt::profiling::RemoteProfiler* Worker::getProfiler() {
  return dynamic_cast<rt::profiling::RemoteProfiler*>(runtime_.getProfiler());
}

void Worker::requestProcessor() {
  using namespace std::string_literals;
  auto threadName = "Server::Worker" + std::to_string(workerId_++);

  EASY_THREAD_SCOPE(threadName.c_str())
  constexpr size_t kMaxRequestSize = req::kMaxKernelSize + 4096; // 4096 is for all metadata
  auto requestBuffer = std::vector<char>(kMaxRequestSize);

  auto profiler = getProfiler();
  if (profiler == nullptr) {
    RT_LOG(FATAL) << "Remote profiler not set";
    return;
  }

  // Associate the current thread to the this Worker object within the RemoteProfiler
  profiler->setThisThreadsWorker(this);

  try {
    while (running_) {
      EASY_BLOCK("requestProcessor::loop", profiler::colors::Blue)
      RT_VLOG(MID) << "Reading next request";
      pollfd pfd;
      pfd.events = POLLIN;
      pfd.fd = socket_;
      EASY_BLOCK("poll")
      if (poll(&pfd, 1, 5) == 0) {
        continue;
      }
      EASY_END_BLOCK

      EASY_BLOCK("read")
      auto res = read(socket_, requestBuffer.data(), requestBuffer.size());
      if (res < 0) {
        auto msg = std::string{"Read socket error: "} + strerror(errno);
        RT_VLOG(LOW) << msg;
        throw NetworkException(msg);
      } else if (res > 0) {
        EASY_END_BLOCK

        EASY_BLOCK("decodeRequest")
        auto ms = MemStream{requestBuffer.data(), static_cast<size_t>(res)};
        std::istream is(&ms);
        req::Id id{req::INVALID_REQUEST_ID};
        try {
          cereal::PortableBinaryInputArchive archive{is};
          req::Request request;
          archive >> request;
          id = request.id_; // save in case runtime triggers an exception to answer with correct id
          EASY_END_BLOCK

          processRequest(request);
        } catch (const Exception& e) {
          if (running_) {
            RT_VLOG(LOW) << "Got a runtime exception. Passing that exception to the client.";
            sendResponse({resp::Type::RUNTIME_EXCEPTION, id, resp::RuntimeException{e}});
          }
        }
      } else if (running_) {
        running_ = false;
        server_.removeWorker(this);
      }
    } // while running_
  } catch (const NetworkException& e) {
    RT_VLOG(LOW) << "Got a network exception. " << e.what();
  }

  profiler->releaseThisThreadsWorker();
}

void Worker::processRequest(const req::Request& request) {
  EASY_FUNCTION(profiler::colors::LightGreen)
  SpinLock lock(mutex_);
  RT_VLOG(MID) << "Processing request. Type: " << static_cast<uint32_t>(request.type_) << " Id: " << request.id_;
  switch (request.type_) {

  case req::Type::VERSION: {
    sendResponse({resp::Type::VERSION, request.id_, resp::Version{Protocol::MAJOR, Protocol::MINOR}});
    break;
  }

  case req::Type::MALLOC: {
    auto& req = std::get<req::Malloc>(request.payload_);
    auto ptr = runtime_.mallocDevice(req.device_, req.size_, req.alignment_);
    allocations_.insert(Allocation{req.device_, ptr});
    sendResponse({resp::Type::MALLOC, request.id_, resp::Malloc{reinterpret_cast<AddressT>(ptr)}});
    break;
  }

  case req::Type::FREE: {
    auto& req = std::get<req::Free>(request.payload_);
    auto addr = reinterpret_cast<std::byte*>(req.address_);
    if (allocations_.erase(Allocation{req.device_, addr}) != 1) {
      RT_LOG(WARNING) << "Trying to deallocate a non previous allocated buffer.";
      throw Exception("Trying to deallocate a non previous allocated buffer.");
    }
    runtime_.freeDevice(DeviceId{req.device_}, addr);
    sendResponse({resp::Type::FREE, request.id_, std::monostate{}});
    break;
  }

  case req::Type::MEMCPY_H2D: {
    auto& req = std::get<req::Memcpy>(request.payload_);
    auto src = reinterpret_cast<std::byte*>(req.src_);
    auto dst = reinterpret_cast<std::byte*>(req.dst_);
    auto evt = runtime_.memcpyHostToDevice(req.stream_, src, dst, req.size_, req.barrier_, cmaCopyFunction_);
    events_.emplace(evt);
    sendResponse({resp::Type::MEMCPY_H2D, request.id_, resp::Event{evt}});
    break;
  }

  case req::Type::MEMCPY_D2H: {
    auto& req = std::get<req::Memcpy>(request.payload_);
    auto src = reinterpret_cast<std::byte*>(req.src_);
    auto dst = reinterpret_cast<std::byte*>(req.dst_);
    auto evt = runtime_.memcpyDeviceToHost(req.stream_, src, dst, req.size_, req.barrier_, cmaCopyFunction_);
    events_.emplace(evt);
    sendResponse({resp::Type::MEMCPY_D2H, request.id_, resp::Event{evt}});
    break;
  }

  case req::Type::MEMCPY_LIST_H2D: {
    auto& req = std::get<req::MemcpyList>(request.payload_);
    auto evt = runtime_.memcpyHostToDevice(req.stream_, MemcpyList(req), req.barrier_, cmaCopyFunction_);
    events_.emplace(evt);
    sendResponse({resp::Type::MEMCPY_LIST_H2D, request.id_, resp::Event{evt}});
    break;
  }

  case req::Type::MEMCPY_LIST_D2H: {
    auto& req = std::get<req::MemcpyList>(request.payload_);
    auto evt = runtime_.memcpyDeviceToHost(req.stream_, MemcpyList(req), req.barrier_, cmaCopyFunction_);
    events_.emplace(evt);
    sendResponse({resp::Type::MEMCPY_LIST_D2H, request.id_, resp::Event{evt}});
    break;
  }

  case req::Type::CREATE_STREAM: {
    auto& req = std::get<req::CreateStream>(request.payload_);
    auto st = runtime_.createStream(req.device_);
    auto profiler = getProfiler();
    profiler->assignRemoteWorkerToStream(st);
    streams_.insert(st);
    sendResponse({resp::Type::CREATE_STREAM, request.id_, resp::CreateStream{st}});
    break;
  }

  case req::Type::DESTROY_STREAM: {
    auto& req = std::get<req::DestroyStream>(request.payload_);
    auto st = req.stream_;
    if (streams_.erase(st) != 1) {
      RT_LOG(WARNING) << "Trying to destroy a non previous created stream.";
      throw Exception("Trying to destroy a non previous created stream.");
    }
    auto profiler = getProfiler();
    profiler->unassignRemoteWorkerFromStream(st);
    runtime_.destroyStream(st);
    sendResponse({resp::Type::DESTROY_STREAM, request.id_, std::monostate{}});
    break;
  }

  case req::Type::LOAD_CODE: {
    auto& req = std::get<req::LoadCode>(request.payload_);
    std::vector<std::byte> tmpBuffer(req.elfSize_);
    // there is no problem using a tmpBuffer in the stack because the loadCode function does not return until it has
    // already copied the data into a CMA buffer. A further improvement could be to make the copy directly to the CMA
    // buffer here (or in a specialized loadCode function).
    profiling::ScopedProfileEvent pevent(profiling::Class::CmaCopy, *runtime_.getProfiler());
    cmaCopyFunction_(reinterpret_cast<const std::byte*>(req.elfData_), tmpBuffer.data(), req.elfSize_,
                     CmaCopyType::TO_CMA);

    auto resp = runtime_.loadCode(req.stream_, tmpBuffer.data(), tmpBuffer.size());
    events_.emplace(resp.event_);
    pevent.setEventId(resp.event_);
    pevent.setParentId(resp.event_);
    kernels_.emplace(resp.kernel_);
    sendResponse({resp::Type::LOAD_CODE, request.id_,
                  resp::LoadCode{resp.event_, resp.kernel_, reinterpret_cast<AddressT>(resp.loadAddress_)}});
    break;
  }

  case req::Type::UNLOAD_CODE: {
    auto& req = std::get<req::UnloadCode>(request.payload_);
    if (kernels_.erase(req.kernel_) != 1) {
      RT_LOG(WARNING) << "Trying to unload a non previously loaded kernel.";
      throw Exception("Trying to unload a non previously loaded kernel.");
    }
    runtime_.unloadCode(req.kernel_);
    sendResponse({resp::Type::UNLOAD_CODE, request.id_, std::monostate{}});
    break;
  }

  case req::Type::KERNEL_LAUNCH: {
    auto& req = std::get<req::KernelLaunch>(request.payload_);

    KernelLaunchOptions kernelLaunchOptions = runtime_.createKernelLaunchOptions(req.kernelOptionsImp_);

    auto evt = runtime_.kernelLaunch(req.stream_, req.kernel_, req.kernelArgs_.data(), req.kernelArgs_.size(),
                                     kernelLaunchOptions);
    events_.emplace(evt);

    RT_DLOG(INFO) << "Registered at worker event " << static_cast<int>(evt);
    sendResponse({resp::Type::KERNEL_LAUNCH, request.id_, resp::Event{evt}});
    break;
  }

  case req::Type::GET_DEVICES: {
    auto devices = runtime_.getDevices();
    sendResponse({resp::Type::GET_DEVICES, request.id_, resp::GetDevices{devices}});
    break;
  }

  case req::Type::ABORT_COMMAND: {
    auto event = std::get<EventId>(request.payload_);
    auto evt = runtime_.abortCommand(event);
    events_.emplace(evt);
    sendResponse({resp::Type::ABORT_COMMAND, request.id_, resp::Event{evt}});
    break;
  }

  case req::Type::ABORT_STREAM: {
    auto& req = std::get<req::AbortStream>(request.payload_);
    auto evt = runtime_.abortStream(req.streamId_);
    events_.emplace(evt);
    sendResponse({resp::Type::ABORT_STREAM, request.id_, resp::Event{evt}});
    break;
  }

  case req::Type::DMA_INFO: {
    auto device = std::get<DeviceId>(request.payload_);
    auto dmaInfo = runtime_.getDmaInfo(device);
    sendResponse({resp::Type::DMA_INFO, request.id_, dmaInfo});
    break;
  }

  case req::Type::DEVICE_PROPERTIES: {
    auto device = std::get<DeviceId>(request.payload_);
    auto dc = runtime_.getDeviceProperties(device);
    sendResponse({resp::Type::DEVICE_PROPERTIES, request.id_, dc});
    break;
  }

  case req::Type::KERNEL_ABORT_RELEASE_RESOURCES: {
    auto evt = std::get<EventId>(request.payload_);
    if (auto it = kernelAbortedFreeResources_.find(evt); it == end(kernelAbortedFreeResources_)) {
      RT_LOG(WARNING) << "Received a request to release kernel abort resources but this event " << static_cast<int>(evt)
                      << " was not registered. Ignoring it.";
    } else {
      it->second();
      kernelAbortedFreeResources_.erase(it);
    }
    sendResponse({resp::Type::KERNEL_ABORT_RELEASE_RESOURCES, request.id_, std::monostate{}});
    break;
  }

  case req::Type::GET_FREE_MEMORY: {
    sendResponse({resp::Type::GET_FREE_MEMORY, request.id_, resp::FreeMemory{runtime_.getFreeMemory()}});
    break;
  }

  case req::Type::GET_CURRENT_CLIENTS: {
    sendResponse({resp::Type::GET_CURRENT_CLIENTS, request.id_, resp::NumClients{server_.getNumWorkers()}});
    break;
  }

  case req::Type::GET_WAITING_COMMANDS: {
    sendResponse({resp::Type::GET_WAITING_COMMANDS, request.id_, resp::WaitingCommands{runtime_.getWaitingCommands()}});
    break;
  }

  case req::Type::GET_ALIVE_EVENTS: {
    sendResponse({resp::Type::GET_ALIVE_EVENTS, request.id_, resp::AliveEvents{runtime_.getAliveEvents()}});
    break;
  }

  case req::Type::GET_P2P_COMPATIBILITY: {
    resp::P2PCompatibility result;

    auto devices = runtime_.doGetDevices();
    result.compatibilityArray_.resize(devices.size());
    // build the matrix here ..
    for (auto i = 0UL, count = devices.size(); i < count; ++i) {
      for (auto j = i + 1; j < count; ++j) {
        if (runtime_.doIsP2PEnabled(DeviceId(i), DeviceId(j))) {
          result.compatibilityArray_[i] |= (1ULL << j);
        }
      }
    }
    sendResponse({resp::Type::GET_P2P_COMPATIBILITY, request.id_, result});
    break;
  }

  case req::Type::MEMCPY_P2P_WRITE: {
    auto& req = std::get<req::MemcpyP2P>(request.payload_);
    auto src = reinterpret_cast<std::byte*>(req.src_);
    auto dst = reinterpret_cast<std::byte*>(req.dst_);
    auto evt = runtime_.memcpyDeviceToDevice(req.stream_, req.device_, src, dst, req.size_, req.barrier_);
    events_.emplace(evt);
    sendResponse({resp::Type::MEMCPY_P2P_WRITE, request.id_, resp::Event{evt}});
    break;
  }

  case req::Type::MEMCPY_P2P_READ: {
    auto& req = std::get<req::MemcpyP2P>(request.payload_);
    auto src = reinterpret_cast<std::byte*>(req.src_);
    auto dst = reinterpret_cast<std::byte*>(req.dst_);
    auto evt = runtime_.memcpyDeviceToDevice(req.device_, req.stream_, src, dst, req.size_, req.barrier_);
    events_.emplace(evt);
    sendResponse({resp::Type::MEMCPY_P2P_READ, request.id_, resp::Event{evt}});
    break;
  }

  case req::Type::ENABLE_TRACING: {
    auto profiler = getProfiler();
    if (profiler != nullptr) {
      profiler->enableRemote();
    }
    sendResponse({resp::Type::ENABLE_TRACING, request.id_, std::monostate()});
    break;
  }

  case req::Type::DISABLE_TRACING: {
    auto profiler = getProfiler();
    if (profiler != nullptr) {
      profiler->disableRemote();
    }
    sendResponse({resp::Type::DISABLE_TRACING, request.id_, std::monostate()});
    break;
  }

  default:
    RT_LOG(WARNING) << "Unknown request: " << static_cast<int>(request.type_) << " id: " << request.id_;
    throw Exception("Unknown request: " + std::to_string(static_cast<int>(request.type_)));
  }
}

void Worker::freeResources() {
  RT_LOG(INFO) << "Freeing resources";
  for (auto alloc : allocations_) {
    runtime_.freeDevice(alloc.device_, alloc.ptr_);
  }
  allocations_.clear();

  for (auto st : streams_) {
    runtime_.destroyStream(st);
  }
  streams_.clear();

  for (auto k : kernels_) {
    runtime_.unloadCode(k);
  }
  kernels_.clear();
  for (const auto& [evt, cb] : kernelAbortedFreeResources_) {
    RT_LOG(WARNING) << "Resources for event " << static_cast<int>(evt)
                    << " were not freed. Freeing them automatically.";
    if (cb) {
      cb();
    }
  }
  kernelAbortedFreeResources_.clear();
}

void Worker::onStreamError(EventId event, const StreamError& error) {
  SpinLock lock(mutex_);
  if (events_.find(event) != end(events_)) {
    lock.unlock();
    // this response is sent before the server sends the distpatch event; so the client can avoid dispatching until the
    // callback has been executed
    sendResponse({resp::Type::STREAM_ERROR, req::ASYNC_RUNTIME_EVENT, resp::StreamError{event, error}});
  } else {
    RT_LOG(WARNING) << "Got streamError for event " << static_cast<int>(event)
                    << " but the event is not registered; ignoring the StreamError.";
  }
}

void Worker::onKernelAborted(EventId event, std::byte* context, size_t size, std::function<void()> freeResources) {
  SpinLock lock(mutex_);
  if (events_.find(event) != end(events_)) {
    if (auto it = kernelAbortedFreeResources_.find(event); it != end(kernelAbortedFreeResources_)) {
      RT_LOG(WARNING) << "Kernel abort for an event which was already aborted but resources not freed. Freeing now.";
      it->second();
      it->second = std::move(freeResources);
    } else {
      kernelAbortedFreeResources_[event] = std::move(freeResources);
    }
    lock.unlock();
    // this response is sent before the server sends the distpatch event; so the client can avoid dispatching until the
    // callback has been executed
    sendResponse({resp::Type::KERNEL_ABORTED, req::ASYNC_RUNTIME_EVENT,
                  resp::KernelAborted{size, reinterpret_cast<AddressT>(context), event}});
  }
}
