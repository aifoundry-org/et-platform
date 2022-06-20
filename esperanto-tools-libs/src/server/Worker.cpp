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
#include "NetworkException.h"
#include "Protocol.h"
#include "Server.h"
#include "Utils.h"
#include "runtime/Types.h"
#include <cereal/archives/portable_binary.hpp>
#include <cstring>
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
    sendResponse({resp::Type::EVENT_DISPATCHED, req::ASYNC_RUNTIME_EVENT, resp::Event{event}});
  }
}

void Worker::sendResponse(const resp::Response& resp) {
  SpinLock lock(mutex_);
  RT_VLOG(MID) << "Sending response. Type: " << static_cast<int>(resp.type_) << " Id: " << resp.id_;
  std::stringstream response;
  cereal::PortableBinaryOutputArchive archive(response);
  archive(resp);
  auto str = response.str();
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
  cmaCopyFunction_ = [pid = credentials.pid](const std::byte* src, std::byte* dst, size_t size,
                                             RuntimeImp::CmaCopyType type) {
    iovec local;
    iovec remote;
    local.iov_len = remote.iov_len = size;

    ssize_t res;
    if (type == RuntimeImp::CmaCopyType::TO_CMA) {
      remote.iov_base = const_cast<std::byte*>(src);
      local.iov_base = dst;
      res = process_vm_readv(pid, &local, 1, &remote, 1, 0);
    } else {
      local.iov_base = const_cast<std::byte*>(src);
      remote.iov_base = dst;
      res = process_vm_writev(pid, &local, 1, &remote, 1, 0);
    }
    // TODO this exception will backfire since it can be thrown only on server side and it could potentially destroy the
    // daemon! So remove this exception but keep printing the error ...
    if (res != static_cast<ssize_t>(size)) {
      throw NetworkException("Error copying/writing from/to remote process with PID: " + std::to_string(pid) +
                             " error: " + strerror(errno));
    };
  };
  runner_ = std::thread(&Worker::requestProcessor, this);
}

Worker::~Worker() {
  RT_VLOG(LOW) << "Destroying worker " << this;
  running_ = false;
  SpinLock lock(mutex_);
  runner_.join();
  freeResources();
  RT_VLOG(LOW) << "Worker dtor ended. Ptr:" << this;
}

void Worker::requestProcessor() {
  constexpr size_t kMaxRequestSize = 4096;
  auto requestBuffer = std::vector<char>(kMaxRequestSize);
  try {
    while (running_) {
      RT_VLOG(MID) << "Reading next request";
      if (auto res = read(socket_, requestBuffer.data(), requestBuffer.size()); res < 0) {
        auto msg = std::string{"Read socket error: "} + strerror(errno);
        RT_VLOG(LOW) << msg;
        throw NetworkException(msg);
      } else if (res > 0) {
        auto ms = MemStream{requestBuffer.data(), static_cast<size_t>(res)};
        std::istream is(&ms);
        req::Id id{req::INVALID_REQUEST_ID};
        try {
          cereal::PortableBinaryInputArchive archive{is};
          req::Request request;
          archive >> request;
          id = request.id_; // save in case runtime triggers an exception to answer with correct id
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
    }
  } catch (const NetworkException& e) {
    RT_VLOG(LOW) << "Got a network exception. " << e.what();
  }
}

void Worker::processRequest(const req::Request& request) {
  SpinLock lock(mutex_);
  RT_VLOG(MID) << "Processing request. Type: " << static_cast<uint32_t>(request.type_) << " Id: " << request.id_;
  switch (request.type_) {

  case req::Type::VERSION: {
    sendResponse({resp::Type::VERSION, request.id_, resp::Version{1}}); // current version is "1"
    break;
  }

  case req::Type::MALLOC: {
    auto& req = std::get<req::Malloc>(request.payload_);
    auto ptr = runtime_.mallocDeviceWithoutProfiling(req.device_, req.size_, req.alignment_);
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
    runtime_.freeDeviceWithoutProfiling(DeviceId{req.device_}, addr);
    sendResponse({resp::Type::FREE, request.id_, std::monostate{}});
    break;
  }

  case req::Type::MEMCPY_H2D: {
    auto& req = std::get<req::Memcpy>(request.payload_);
    auto src = reinterpret_cast<std::byte*>(req.src_);
    auto dst = reinterpret_cast<std::byte*>(req.dst_);
    auto evt =
      runtime_.memcpyHostToDeviceWithoutProfiling(req.stream_, src, dst, req.size_, req.barrier_, cmaCopyFunction_);
    events_.emplace(evt);
    sendResponse({resp::Type::MEMCPY_H2D, request.id_, resp::Event{evt}});
    break;
  }

  case req::Type::MEMCPY_D2H: {
    auto& req = std::get<req::Memcpy>(request.payload_);
    auto src = reinterpret_cast<std::byte*>(req.src_);
    auto dst = reinterpret_cast<std::byte*>(req.dst_);
    auto evt =
      runtime_.memcpyDeviceToHostWithoutProfiling(req.stream_, src, dst, req.size_, req.barrier_, cmaCopyFunction_);
    events_.emplace(evt);
    sendResponse({resp::Type::MEMCPY_D2H, request.id_, resp::Event{evt}});
    break;
  }

  case req::Type::MEMCPY_LIST_H2D: {
    auto& req = std::get<req::MemcpyList>(request.payload_);
    auto evt =
      runtime_.memcpyHostToDeviceWithoutProfiling(req.stream_, MemcpyList(req), req.barrier_, cmaCopyFunction_);
    events_.emplace(evt);
    sendResponse({resp::Type::MEMCPY_LIST_H2D, request.id_, resp::Event{evt}});
    break;
  }

  case req::Type::MEMCPY_LIST_D2H: {
    auto& req = std::get<req::MemcpyList>(request.payload_);
    auto evt =
      runtime_.memcpyDeviceToHostWithoutProfiling(req.stream_, MemcpyList(req), req.barrier_, cmaCopyFunction_);
    events_.emplace(evt);
    sendResponse({resp::Type::MEMCPY_LIST_D2H, request.id_, resp::Event{evt}});
    break;
  }

  case req::Type::CREATE_STREAM: {
    auto& req = std::get<req::CreateStream>(request.payload_);
    auto st = runtime_.createStreamWithoutProfiling(req.device_);
    streams_.insert(st);
    sendResponse({resp::Type::CREATE_STREAM, request.id_, resp::CreateStream{st}});
    break;
  }

  case req::Type::DESTROY_STREAM: {
    auto& req = std::get<req::DestroyStream>(request.payload_);
    if (streams_.erase(req.stream_) != 1) {
      RT_LOG(WARNING) << "Trying to destroy a non previous created stream.";
      throw Exception("Trying to destroy a non previous created stream.");
    }
    runtime_.destroyStreamWithoutProfiling(req.stream_);
    sendResponse({resp::Type::DESTROY_STREAM, request.id_, std::monostate{}});
    break;
  }

  case req::Type::LOAD_CODE: {
    auto& req = std::get<req::LoadCode>(request.payload_);
    auto resp = runtime_.loadCode(req.stream_, req.elf_.data(), req.elf_.size());
    events_.emplace(resp.event_);
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
    auto evt = runtime_.kernelLaunch(req.stream_, req.kernel_, req.kernelArgs_.data(), req.kernelArgs_.size(),
                                     req.shireMask_, req.barrier_, req.flushL3_, req.userTrace_);
    events_.emplace(evt);
    sendResponse({resp::Type::KERNEL_LAUNCH, request.id_, resp::Event{evt}});
    break;
  }

  case req::Type::GET_DEVICES: {
    auto devices = runtime_.getDevices();
    sendResponse({resp::Type::GET_DEVICES, request.id_, resp::GetDevices{devices}});
    break;
  }

  case req::Type::ABORT_STREAM: {
    auto& req = std::get<req::AbortStream>(request.payload_);
    auto evt = runtime_.abortStream(req.streamId_);
    events_.emplace(evt);
    sendResponse({resp::Type::ABORT_STREAM, request.id_, resp::Event{evt}});
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
}

void Worker::onStreamError(EventId event, const StreamError& error) {
  SpinLock lock(mutex_);
  if (events_.find(event) != end(events_)) {
    sendResponse({resp::Type::STREAM_ERROR, req::ASYNC_RUNTIME_EVENT, error});
  }
}