/*-------------------------------------------------------------------------
 * Copyright (C) 2022, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#include "Client.h"
#include "Utils.h"
#include "server/NetworkException.h"
#include "server/Protocol.h"
#include <cereal/archives/portable_binary.hpp>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <poll.h>
#include <sstream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <variant>

using namespace rt;

namespace {
constexpr size_t kMaxMessageSize = 4096;
struct MemStream : public std::streambuf {
  MemStream(char* s, std::size_t n) {
    setg(s, s, s + n);
  }
};
} // namespace
void Client::setOnStreamErrorsCallback(std::function<void(EventId, StreamError const&)> callback) {
  SpinLock lock(mutex_);
  streamErrorCallback_ = std::move(callback);
}
Client::~Client() {
  RT_LOG(INFO) << "Destroying client.";
  running_ = false;
  listener_.join();
  eventSync_.notify_all();
}

Client::Client(const std::string& socketPath) {
  socket_ = socket(AF_UNIX, SOCK_SEQPACKET, 0);

  sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  RT_LOG(INFO) << "Connecting to socket " << socketPath;
  strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);

  if (int val = 1; setsockopt(socket_, SOL_SOCKET, SO_PASSCRED, &val, sizeof(val)) == -1) {
    throw NetworkException(std::string{"unable to set local per credentials: "} + strerror(errno));
  }

  if (connect(socket_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
    throw NetworkException(std::string{"connect error: "} + strerror(errno));
  }
  ucred ucred;
  socklen_t len = sizeof(ucred);
  if (getsockopt(socket_, SOL_SOCKET, SO_PEERCRED, &ucred, &len) == -1) {
    throw NetworkException(std::string{"getsockopt error: "} + strerror(errno));
  }

  RT_LOG(INFO) << "Runtime client connection: (PID:  " << getpid()
               << ") ===> Credentials from SO_PEERCRED (server credentials): pid=" << ucred.pid
               << ", euid=" << ucred.uid << ", egid=" << ucred.gid;

  listener_ = std::thread(&Client::responseProcessor, this);
  handShake();
}

void Client::responseProcessor() {
  auto requestBuffer = std::vector<char>(kMaxMessageSize);
  try {
    while (running_) {
      if (auto res = read(socket_, requestBuffer.data(), requestBuffer.size()); running_) {
        if (res < 0) {
          RT_LOG(WARNING) << "Read socket error: " << strerror(errno);
          break;
        } else if (res > 0) {
          auto ms = MemStream{requestBuffer.data(), static_cast<size_t>(res)};
          std::istream is(&ms);
          cereal::PortableBinaryInputArchive archive{is};
          resp::Response response;
          archive >> response;
          RT_VLOG(HIGH) << "Got response, size: " << res << ". Type: " << static_cast<uint32_t>(response.type_)
                        << " id: " << response.id_;
          processResponse(response);
        } else {
          RT_LOG(INFO) << "Socket closed, ending client listener thread.";
          running_ = false;
        }
      }
    }
  } catch (const std::exception& e) {
    RT_LOG(WARNING) << std::string{"Exception happened: "} + e.what();
    running_ = false;
  }
  RT_LOG(INFO) << "End listener thread.";
}

bool Client::waitForEvent(EventId event, std::chrono::seconds timeout) {
  return waitForEventWithoutProfiling(event, timeout);
}

bool Client::waitForStream(StreamId stream, std::chrono::seconds timeout) {
  return waitForStreamWithoutProfiling(stream, timeout);
}

bool Client::waitForEventWithoutProfiling(EventId event, std::chrono::seconds timeout) {
  SpinLock lock(mutex_);
  if (eventToStream_.find(event) == end(eventToStream_)) {
    return true;
  }
  return eventSync_.wait_for(lock, timeout,
                             [this, event] { return eventToStream_.find(event) == end(eventToStream_); });
}

bool Client::waitForStreamWithoutProfiling(StreamId stream, std::chrono::seconds timeout) {
  auto start = std::chrono::steady_clock::now();
  std::unique_lock lock(mutex_);
  auto events = find(streamToEvents_, stream, "Stream does not exist")->second;
  lock.unlock();
  if (events.empty()) {
    return true;
  }
  for (auto e : events) {
    auto now = std::chrono::steady_clock::now();
    if (!waitForEventWithoutProfiling(e, timeout - std::chrono::duration_cast<std::chrono::seconds>(now - start))) {
      return false;
    }
  }
  return true;
}

req::Id Client::getNextId() {
  while (!req::IsRegularId(++nextId_))
    ;
  return nextId_;
}

void Client::sendRequest(const req::Request& request) {
  RT_VLOG(MID) << "Sending request " << static_cast<uint32_t>(request.type_) << " with id: " << request.id_;
  SpinLock lock(mutex_);
  std::stringstream sreq;
  cereal::PortableBinaryOutputArchive archive(sreq);
  archive(request);
  auto str = sreq.str();
  if (responseWaiters_.find(request.id_) != end(responseWaiters_)) {
    RT_LOG(WARNING) << "There was a previous response structure (Waiter) for request ID: " << request.id_
                    << ". New request ID will erase the previous one. This is likely a BUG.";
  }
  responseWaiters_[request.id_] = std::make_unique<Waiter>();

  if (auto res = write(socket_, str.data(), str.size()); res < static_cast<long>(str.size())) {
    auto errorMsg = std::string{strerror(errno)};
    RT_VLOG(LOW) << "Write socket error: " << errorMsg;
    throw NetworkException("Write socket error: " + errorMsg);
  }
}
resp::Response::Payload_t Client::waitForResponse(req::Id req) {
  SpinLock lock(mutex_);
  auto it = find(responseWaiters_, req, "Request not registered");
  auto& waiter = it->second;
  RT_VLOG(MID) << "Waiting for response with id: " << req;
  waiter->wait(lock);
  RT_VLOG(MID) << "Wait ended";
  auto payload = std::move(waiter->payload_);
  responseWaiters_.erase(it);
  if (std::holds_alternative<resp::RuntimeException>(payload)) {
    auto exception = std::get<resp::RuntimeException>(payload);
    throw exception.exception_;
  }
  return payload;
}

void Client::dispatch(EventId event) {
  auto st = find(eventToStream_, event, "Event does not exist")->second;
  eventToStream_.erase(event);
  auto& events = find(streamToEvents_, st, "Stream not found")->second;
  std::remove(begin(events), end(events), event);
  eventSync_.notify_all();
}

void Client::processResponse(const resp::Response& response) {
  RT_VLOG(MID) << "Processing response";
  SpinLock lock(mutex_);
  switch (response.type_) {
  case resp::Type::EVENT_DISPATCHED: {
    CHECK(response.id_ == req::ASYNC_RUNTIME_EVENT) << "Event dispatched should have request type ASYNC_RUNTIME_EVENT";
    auto evt = std::get<resp::Event>(response.payload_).event_;
    dispatch(evt);
    break;
  }
  case resp::Type::STREAM_ERROR:
    CHECK(response.id_ == req::ASYNC_RUNTIME_EVENT) << "Stream error should have request type ASYNC_RUNTIME_EVENT";
    // execute callback or send through the threadpool
    break;
  default:
    RT_VLOG(MID) << "Got response type: " << static_cast<uint32_t>(response.type_) << " wake up waiter.";
    auto it = find(responseWaiters_, response.id_, "Couldn't found the request id" + std::to_string(response.id_));
    RT_VLOG(MID) << "Waiter found; notifying";
    it->second->notify(response.payload_);
    break;
  }
}

EventId Client::registerEvent(const resp::Response::Payload_t& payload, StreamId st) {
  auto evt = std::get<resp::Event>(payload).event_;
  SpinLock lock(mutex_);
  find(streamToEvents_, st, "Stream does not exist");
  eventToStream_[evt] = st;
  streamToEvents_[st].emplace_back(evt);
  return evt;
}

EventId Client::abortStream(StreamId st) {
  auto payload = sendRequestAndWait(req::Type::ABORT_STREAM, req::AbortStream{st});
  return registerEvent(payload, st);
}

void Client::handShake() {
  auto payload = sendRequestAndWait(req::Type::VERSION, std::monostate{});
  if (std::get<resp::Version>(payload).version_ != 1) {
    throw Exception("Unsupported version. Current client version only supports version: 1. Please update the runtime "
                    "client library.");
  }
}

EventId Client::memcpyDeviceToHost(StreamId st, std::byte const* src, std::byte* dst, unsigned long size,
                                   bool barrier) {
  auto payload = sendRequestAndWait(req::Type::MEMCPY_D2H, req::Memcpy{st, reinterpret_cast<AddressT>(src),
                                                                       reinterpret_cast<AddressT>(dst), size, barrier});
  return registerEvent(payload, st);
}
StreamId Client::createStream(DeviceId deviceId) {
  auto payload = sendRequestAndWait(req::Type::CREATE_STREAM, req::CreateStream{deviceId});
  auto st = std::get<resp::CreateStream>(payload).stream_;
  streamToEvents_[st] = {};
  return st;
}
std::byte* Client::mallocDevice(DeviceId device, unsigned long size, unsigned int alignment) {
  auto payload = sendRequestAndWait(req::Type::MALLOC, req::Malloc{size, device, alignment});
  return reinterpret_cast<std::byte*>(std::get<resp::Malloc>(payload).address_);
}
EventId Client::memcpyDeviceToHost(StreamId st, MemcpyList memcpyList, bool barrier) {
  auto payload = sendRequestAndWait(req::Type::MEMCPY_LIST_D2H, req::MemcpyList{memcpyList, st, barrier});
  return registerEvent(payload, st);
}
EventId Client::memcpyHostToDevice(StreamId st, std::byte const* src, std::byte* dst, unsigned long size,
                                   bool barrier) {
  auto payload = sendRequestAndWait(req::Type::MEMCPY_H2D, req::Memcpy{st, reinterpret_cast<AddressT>(src),
                                                                       reinterpret_cast<AddressT>(dst), size, barrier});
  return registerEvent(payload, st);
}
void Client::freeDevice(DeviceId device, std::byte* ptr) {
  sendRequestAndWait(req::Type::FREE, req::Free{device, reinterpret_cast<AddressT>(ptr)});
}
EventId Client::kernelLaunch(StreamId, KernelId, std::byte const*, unsigned long, unsigned long, bool, bool,
                             std::optional<UserTrace>) {
  throw Exception("UNIMPLEMENTED, YET");
}
EventId Client::abortCommand(EventId, std::chrono::milliseconds) {
  throw Exception("UNIMPLEMENTED, YET");
}
std::vector<StreamError> Client::retrieveStreamErrors(StreamId) {
  throw Exception("UNIMPLEMENTED, YET");
}
IProfiler* Client::getProfiler() {
  throw Exception("UNIMPLEMENTED, YET");
}
LoadCodeResult Client::loadCode(StreamId, std::byte const*, unsigned long) {
  throw Exception("UNIMPLEMENTED, YET");
}
std::vector<DeviceId> Client::getDevices() {
  auto payload = sendRequestAndWait(req::Type::GET_DEVICES, std::monostate{});
  return std::get<resp::GetDevices>(payload).devices_;
}
void Client::unloadCode(KernelId) {
  throw Exception("UNIMPLEMENTED, YET");
}
EventId Client::memcpyHostToDevice(StreamId st, MemcpyList memcpyList, bool barrier) {
  auto payload = sendRequestAndWait(req::Type::MEMCPY_LIST_H2D, req::MemcpyList{memcpyList, st, barrier});
  return registerEvent(payload, st);
}
void Client::destroyStream(StreamId stream) {
  find(streamToEvents_, stream, "Stream does not exist");
  sendRequestAndWait(req::Type::DESTROY_STREAM, req::DestroyStream{stream});
  streamToEvents_.erase(stream);
}
