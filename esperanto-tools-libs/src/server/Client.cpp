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
#include "Constants.h"
#include "NetworkException.h"
#include "ProfilerImp.h"
#include "Protocol.h"
#include "Utils.h"
#include "runtime/Types.h"

#include <easy/arbitrary_value.h>
#include <easy/details/profiler_colors.h>
#include <easy/profiler.h>
#include <poll.h>
#include <sstream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

using namespace rt;

namespace {
constexpr auto kSocketNeededSize = static_cast<int>(sizeof(ErrorContext) * kNumErrorContexts);
constexpr size_t kMaxMessageSize = kSocketNeededSize * 2;
constexpr auto kProcessResponseTries = 100;
struct MemStream : public std::streambuf {
  MemStream(char* s, std::size_t n) {
    setg(s, s, s + n);
  }
};
} // namespace
void Client::doSetOnStreamErrorsCallback(std::function<void(EventId, StreamError const&)> callback) {
  SpinLock lock(mutex_);
  streamErrorCallback_ = std::move(callback);
}
Client::~Client() {
  RT_LOG(INFO) << "Destroying client.";
  running_ = false;
  close(socket_);
  listener_.join();
  RT_VLOG(LOW) << "Listener joined.";
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
  if (socklen_t len = sizeof(ucred); getsockopt(socket_, SOL_SOCKET, SO_PEERCRED, &ucred, &len) == -1) {
    throw NetworkException(std::string{"getsockopt error: "} + strerror(errno));
  }

  RT_LOG(INFO) << "Runtime client connection: (PID:  " << getpid()
               << ") ===> Credentials from SO_PEERCRED (server credentials): pid=" << ucred.pid
               << ", euid=" << ucred.uid << ", egid=" << ucred.gid;

  listener_ = std::thread(&Client::responseProcessor, this);
  handShake();
}

void Client::responseProcessor() {
  EASY_THREAD_SCOPE("Client::responseProcessor")
  auto requestBuffer = std::vector<char>(kMaxMessageSize);
  try {
    while (running_) {
      EASY_BLOCK("Client::responseProcessor::loop")
      RT_VLOG(LOW) << "Reading response ...";

      pollfd pfd;
      pfd.events = POLLIN;
      pfd.fd = socket_;
      if (poll(&pfd, 1, 5) == 0) {
        continue;
      }

      if (auto res = read(socket_, requestBuffer.data(), requestBuffer.size()); running_) {
        EASY_BLOCK("Client::responseProcessor::read")
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
          bool done = false;
          for (int i = 0; i < kProcessResponseTries && !done; ++i) {
            try {
              processResponse(response);
              done = true;
            } catch (const Exception& e) {
              EASY_EVENT("Response arrived before request, sometimes happens. Will retry.")
              RT_LOG(WARNING) << "Exception happened (response arrived before request): " << e.what();
              using namespace std::literals;
              std::this_thread::sleep_for(1us);
            }
          }
          if (!done) {
            RT_LOG(FATAL) << "Failed to process response after " << kProcessResponseTries << " tries. Aborting.";
          }
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

bool Client::doWaitForEvent(EventId event, std::chrono::seconds timeout) {
  EASY_FUNCTION(profiler::colors::Red300)
  SpinLock lock(mutex_);
  if (eventToStream_.find(event) == end(eventToStream_)) {
    return true;
  }
  return eventSync_.wait_for(lock, timeout,
                             [this, event] { return eventToStream_.find(event) == end(eventToStream_); });
}

bool Client::doWaitForStream(StreamId stream, std::chrono::seconds timeout) {
  auto start = std::chrono::steady_clock::now();
  SpinLock lock(mutex_);
  auto events = find(streamToEvents_, stream, "Stream does not exist")->second;
  lock.unlock();
  if (events.empty()) {
    return true;
  }
  for (auto e : events) {
    auto now = std::chrono::steady_clock::now();
    if (!doWaitForEvent(e, timeout - std::chrono::duration_cast<std::chrono::seconds>(now - start))) {
      return false;
    }
  }
  return true;
}

req::Id Client::getNextId() {
  auto res = ++nextId_;
  return req::IsRegularId(res) ? res : getNextId();
}

void Client::sendRequest(const req::Request& request) {
  EASY_FUNCTION()
  EASY_BLOCK("Serialize request")
  RT_VLOG(MID) << "Sending request " << static_cast<uint32_t>(request.type_) << " with id: " << request.id_;
  std::stringstream sreq;
  cereal::PortableBinaryOutputArchive archive(sreq);
  archive(request);
  EASY_END_BLOCK
  auto str = sreq.str();
  SpinLock lock(mutex_);
  if (responseWaiters_.find(request.id_) != end(responseWaiters_)) {
    RT_LOG(WARNING) << "There was a previous response structure (Waiter) for request ID: " << request.id_
                    << ". New request ID will erase the previous one. This is likely a BUG.";
  }
  responseWaiters_[request.id_] = std::make_unique<Waiter>();
  EASY_BLOCK("Write socket")
  if (auto res = write(socket_, str.data(), str.size()); res < static_cast<long>(str.size())) {
    auto errorMsg = std::string{strerror(errno)};
    RT_VLOG(LOW) << "Write socket error: " << errorMsg;
    throw NetworkException("Write socket error: " + errorMsg);
  }
}
resp::Response::Payload_t Client::waitForResponse(req::Id req) {
  EASY_FUNCTION()
  SpinLock lock(mutex_);
  auto it = find(responseWaiters_, req, "Request not registered");
  auto& waiter = *it->second;
  RT_VLOG(MID) << "Waiting for response with id: " << req;
  waiter.wait(lock);
  RT_VLOG(MID) << "Wait ended";
  auto payload = std::move(waiter.payload_);
  responseWaiters_.erase(it);
  if (std::holds_alternative<resp::RuntimeException>(payload)) {
    auto exception = std::get<resp::RuntimeException>(payload);
    throw exception.exception_;
  }
  return payload;
}

void Client::dispatch(EventId event) {
  EASY_FUNCTION()
  EASY_VALUE("Event", static_cast<int>(event))
  auto st = find(eventToStream_, event, "Event does not exist")->second;
  eventToStream_.erase(event);
  auto& events = find(streamToEvents_, st, "Stream not found")->second;
  auto it = std::remove(begin(events), end(events), event);
  events.erase(it, end(events));
  eventSync_.notify_all();
}

void Client::processResponse(const resp::Response& response) {
  EASY_FUNCTION()
  RT_VLOG(MID) << "Processing response";
  SpinLock lock(mutex_);
  switch (response.type_) {
  case resp::Type::KERNEL_ABORTED: {
    CHECK(response.id_ == req::ASYNC_RUNTIME_EVENT)
      << "Kernel aborted message should have request type ASYNC_RUNTIME_EVENT";
    auto& payload = std::get<resp::KernelAborted>(response.payload_);
    auto evt = payload.event_;
    auto buffer = reinterpret_cast<std::byte*>(payload.buffer_);
    if (kernelAbortCallback_) {
      tp_.pushTask([this, cb = kernelAbortCallback_, evt, buffer, size = payload.size_] {
        cb(evt, buffer, size, [this, evt] { sendRequestAndWait(req::Type::KERNEL_ABORT_RELEASE_RESOURCES, evt); });
      });
    } else {
      lock.unlock();
      sendRequestAndWait(req::Type::KERNEL_ABORT_RELEASE_RESOURCES, evt);
    }
    break;
  }
  case resp::Type::EVENT_DISPATCHED: {
    CHECK(response.id_ == req::ASYNC_RUNTIME_EVENT) << "Event dispatched should have request type ASYNC_RUNTIME_EVENT";
    auto evt = std::get<resp::Event>(response.payload_).event_;
    dispatch(evt);
    break;
  }
  case resp::Type::STREAM_ERROR: {
    CHECK(response.id_ == req::ASYNC_RUNTIME_EVENT) << "Stream error should have request type ASYNC_RUNTIME_EVENT";
    auto& payload = std::get<resp::StreamError>(response.payload_);
    auto evt = payload.event_;
    auto& error = payload.error_;
    if (streamErrorCallback_) {
      tp_.pushTask([cb = streamErrorCallback_, evt, error = std::move(error)] { cb(evt, error); });
    } else {
      auto it = eventToStream_.find(evt);
      if (it == end(eventToStream_)) {
        RT_LOG(WARNING) << "Got an stream error for an unkown event. Ignoring this error";
      } else {
        streamErrors_[it->second].emplace_back(std::move(payload.error_));
      }
    }
    break;
  }
  default:
    RT_VLOG(MID) << "Got response type: " << static_cast<uint32_t>(response.type_) << " wake up waiter.";
    auto it = find(responseWaiters_, response.id_, "Couldn't found the request id" + std::to_string(response.id_));
    RT_VLOG(MID) << "Waiter found; notifying";
    it->second->notify(response.payload_);
    break;
  }
}

EventId Client::registerEvent(const resp::Response::Payload_t& payload, StreamId st) {
  EASY_FUNCTION(profiler::colors::Purple)
  auto evt = std::get<resp::Event>(payload).event_;
  EASY_VALUE("Event", static_cast<int>(evt))
  registerEvent(evt, st);
  return evt;
}

void Client::registerEvent(EventId evt, StreamId st) {
  EASY_FUNCTION(profiler::colors::Purple)
  SpinLock lock(mutex_);
  find(streamToEvents_, st, "Stream does not exist");
  eventToStream_[evt] = st;
  streamToEvents_[st].emplace_back(evt);
  EASY_VALUE("Event", static_cast<int>(evt))
  EASY_VALUE("Stream", static_cast<int>(st))
}

EventId Client::doAbortStream(StreamId st) {
  auto payload = sendRequestAndWait(req::Type::ABORT_STREAM, req::AbortStream{st});
  return registerEvent(payload, st);
}

void Client::handShake() {
  EASY_FUNCTION()
  auto payload = sendRequestAndWait(req::Type::VERSION, std::monostate{});
  auto major = std::get<resp::Version>(payload).major_;
  auto minor = std::get<resp::Version>(payload).minor_;
  RT_LOG_IF(FATAL, major == resp::Version::INVALID_VERSION || minor == resp::Version::INVALID_VERSION)
    << "Invalid version received from server. Major: " << major << " Minor: " << minor;

  RT_LOG(INFO) << "Server protocol version: " << major << "." << minor;

  if (std::get<resp::Version>(payload).major_ != 3) {
    throw Exception("Unsupported version. Current client version only supports version: 2.X. Please update the runtime "
                    "client library or runtime daemon server.");
  }
  // get deviceLayerProperties now
  auto devices = getDevices();
  for (auto d : devices) {
    DeviceLayerProperties dlp;
    auto pl = sendRequestAndWait(req::Type::DEVICE_PROPERTIES, d);
    dlp.deviceProperties_ = std::get<DeviceProperties>(pl);
    pl = sendRequestAndWait(req::Type::DMA_INFO, d);
    dlp.dmaInfo_ = std::get<DmaInfo>(pl);
    deviceLayerProperties_.emplace_back(dlp);
  }
}

EventId Client::doMemcpyDeviceToHost(StreamId st, std::byte const* src, std::byte* dst, unsigned long size,
                                     bool barrier, const CmaCopyFunction&) {
  auto payload = sendRequestAndWait(req::Type::MEMCPY_D2H, req::Memcpy{st, reinterpret_cast<AddressT>(src),
                                                                       reinterpret_cast<AddressT>(dst), size, barrier});
  return registerEvent(payload, st);
}
StreamId Client::doCreateStream(DeviceId deviceId) {
  auto payload = sendRequestAndWait(req::Type::CREATE_STREAM, req::CreateStream{deviceId});
  auto st = std::get<resp::CreateStream>(payload).stream_;
  SpinLock lock(mutex_);
  streamToEvents_[st] = {};
  return st;
}
std::byte* Client::doMallocDevice(DeviceId device, unsigned long size, unsigned int alignment) {
  auto payload = sendRequestAndWait(req::Type::MALLOC, req::Malloc{size, device, alignment});
  return reinterpret_cast<std::byte*>(std::get<resp::Malloc>(payload).address_);
}
EventId Client::doMemcpyDeviceToHost(StreamId st, MemcpyList memcpyList, bool barrier, const CmaCopyFunction&) {
  auto payload = sendRequestAndWait(req::Type::MEMCPY_LIST_D2H, req::MemcpyList{memcpyList, st, barrier});
  return registerEvent(payload, st);
}
EventId Client::doMemcpyHostToDevice(StreamId st, std::byte const* src, std::byte* dst, unsigned long size,
                                     bool barrier, const CmaCopyFunction&) {
  auto payload = sendRequestAndWait(req::Type::MEMCPY_H2D, req::Memcpy{st, reinterpret_cast<AddressT>(src),
                                                                       reinterpret_cast<AddressT>(dst), size, barrier});
  return registerEvent(payload, st);
}
void Client::doFreeDevice(DeviceId device, std::byte* ptr) {
  sendRequestAndWait(req::Type::FREE, req::Free{device, reinterpret_cast<AddressT>(ptr)});
}
EventId Client::doKernelLaunch(StreamId stream, KernelId kernel, const std::byte* kernel_args, size_t kernel_args_size,
                               uint64_t shire_mask, bool barrier, bool flushL3,
                               std::optional<UserTrace> userTraceConfig) {
  std::vector<std::byte> kernelArgs;
  std::copy(kernel_args, kernel_args + kernel_args_size, std::back_inserter(kernelArgs));
  auto payload = sendRequestAndWait(req::Type::KERNEL_LAUNCH, req::KernelLaunch{stream, kernel, shire_mask, kernelArgs,
                                                                                barrier, flushL3, userTraceConfig});
  return registerEvent(payload, stream);
}

EventId Client::doAbortCommand(EventId evt, std::chrono::milliseconds timeout) {
  SpinLock lock(mutex_);
  auto st = find(eventToStream_, evt, "Trying to abort a non existing command.")->second;
  auto payload = sendRequestAndWait(req::Type::ABORT_COMMAND, req::AbortCommand{evt, timeout});
  return registerEvent(payload, st);
}

std::vector<StreamError> Client::doRetrieveStreamErrors(StreamId st) {
  SpinLock lock(mutex_);
  auto errors = std::move(streamErrors_[st]);
  return errors;
}

LoadCodeResult Client::doLoadCode(StreamId st, std::byte const* data, unsigned long size) {
  auto payload = sendRequestAndWait(req::Type::LOAD_CODE, req::LoadCode{st, data, size});
  LoadCodeResult r;
  auto resp = std::get<resp::LoadCode>(payload);
  r.loadAddress_ = reinterpret_cast<std::byte*>(resp.loadAddress_);
  r.event_ = resp.event_;
  r.kernel_ = resp.kernel_;
  registerEvent(r.event_, st);
  return r;
}

std::vector<DeviceId> Client::doGetDevices() {
  auto payload = sendRequestAndWait(req::Type::GET_DEVICES, std::monostate{});
  return std::get<resp::GetDevices>(payload).devices_;
}
void Client::doUnloadCode(KernelId kid) {
  sendRequestAndWait(req::Type::UNLOAD_CODE, req::UnloadCode{kid});
}
EventId Client::doMemcpyHostToDevice(StreamId st, MemcpyList memcpyList, bool barrier, const CmaCopyFunction&) {
  auto payload = sendRequestAndWait(req::Type::MEMCPY_LIST_H2D, req::MemcpyList{memcpyList, st, barrier});
  return registerEvent(payload, st);
}
void Client::doDestroyStream(StreamId stream) {
  SpinLock lock(mutex_);
  find(streamToEvents_, stream, "Stream does not exist");
  lock.unlock();
  sendRequestAndWait(req::Type::DESTROY_STREAM, req::DestroyStream{stream});
  lock.lock();
  streamToEvents_.erase(stream);
  if (!streamErrors_[stream].empty()) {
    RT_LOG(WARNING) << "Destroying stream with pending errors. These errors will be ignored.";
    streamErrors_[stream].clear();
  }
}

DmaInfo Client::doGetDmaInfo(DeviceId deviceId) const {
  auto idx = static_cast<uint64_t>(deviceId);
  if (idx >= deviceLayerProperties_.size()) {
    throw Exception("Invalid device");
  }
  return deviceLayerProperties_[idx].dmaInfo_;
}

DeviceProperties Client::doGetDeviceProperties(DeviceId device) const {
  auto idx = static_cast<uint64_t>(device);
  if (idx >= deviceLayerProperties_.size()) {
    throw Exception("Invalid device");
  }
  return deviceLayerProperties_[idx].deviceProperties_;
}

size_t Client::getCurrentClients() {
  auto payload = sendRequestAndWait(req::Type::GET_CURRENT_CLIENTS, std::monostate{});
  return std::get<resp::NumClients>(payload).numClients_;
}

std::unordered_map<DeviceId, uint64_t> Client::getFreeMemory() {
  auto payload = sendRequestAndWait(req::Type::GET_FREE_MEMORY, std::monostate{});
  return std::get<resp::FreeMemory>(payload).freeMemory_;
}

std::unordered_map<DeviceId, uint32_t> Client::getWaitingCommands() {
  auto payload = sendRequestAndWait(req::Type::GET_WAITING_COMMANDS, std::monostate{});
  return std::get<resp::WaitingCommands>(payload).waitingCommands_;
}

std::unordered_map<DeviceId, uint32_t> Client::getAliveEvents() {
  auto payload = sendRequestAndWait(req::Type::GET_ALIVE_EVENTS, std::monostate{});
  return std::get<resp::AliveEvents>(payload).aliveEvents_;
}
