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
#include "Protocol.h"
#include "Server.h"
#include "Utils.h"
#include <cereal/archives/portable_binary.hpp>
#include <cstring>
#include <unistd.h>

using namespace rt;

struct MemStream : public std::streambuf {
  MemStream(char* s, std::size_t n) {
    setg(s, s, s + n);
  }
};

void Worker::sendResponse(const resp::Response& resp) {
  std::stringstream response;
  cereal::PortableBinaryOutputArchive archive(response);
  archive(resp);
  auto str = response.str();
  if (auto res = write(socket_, str.data(), str.size()); res < static_cast<long>(str.size())) {
    auto errorMsg = std::string{strerror(errno)};
    RT_VLOG(LOW) << "Write socket error: " << errorMsg;
    RT_LOG(INFO) << "Closing connection.";
    throw Exception("Write socket error: " + errorMsg);
  }
}

Worker::Worker(int socket, IRuntime& runtime, Server& server)
  : runtime_(runtime)
  , server_(server)
  , socket_(socket) {
  runner_ = std::thread(&Worker::requestProcessor, this);
}

Worker::~Worker() {
  running_ = false;
  runner_.join();
  freeResources();
}

void Worker::requestProcessor() {
  constexpr size_t kMaxRequestSize = 4096;
  auto requestBuffer = std::vector<char>(kMaxRequestSize);
  auto ms = MemStream{requestBuffer.data(), requestBuffer.size()};

  while (running_) {
    if (auto res = read(socket_, requestBuffer.data(), requestBuffer.size()); res < 0) {
      RT_VLOG(LOW) << "Read socket error: " << strerror(errno);
      RT_LOG(INFO) << "Closing connection.";
      break;
    }
    std::istream is(&ms);
    try {
      cereal::PortableBinaryInputArchive archive{is};
      req::Request request;
      archive >> request;
      processRequest(request);
    } catch (...) {
      RT_VLOG(LOW) << "Error processing the request or sending the response. Closing connection.";
      break;
    }
  }
  server_.removeWorker(this);
}

void Worker::processRequest(const req::Request& request) {
  switch (request.type_) {
  case req::Type::FREE: {
    auto& req = std::get<req::Free>(request.payload_);
    auto addr = reinterpret_cast<std::byte*>(req.address_);
    if (allocations_.erase(Allocation{req.device_, addr}) != 1) {
      RT_LOG(WARNING) << "Trying to deallocate a non previous allocated buffer.";
      throw Exception("Trying to deallocate a non previous allocated buffer.");
    }
    runtime_.freeDevice(DeviceId{req.device_}, addr);
    break;
  }

  case req::Type::ABORT_STREAM: {
    auto& req = std::get<req::AbortStream>(request.payload_);
    auto evt = runtime_.abortStream(req.streamId_);
    sendResponse({resp::Type::ABORT_STREAM, resp::Event{evt}});
    break;
  }

  case req::Type::CREATE_STREAM: {
    auto& req = std::get<req::CreateStream>(request.payload_);
    auto st = runtime_.createStream(req.device_);
    sendResponse({resp::Type::CREATE_STREAM, resp::CreateStream{st}});
    break;
  }
  default:
    RT_LOG(WARNING) << "Unknown request: " << static_cast<int>(request.type_);
    throw Exception("Unknown request: " + std::to_string(static_cast<int>(request.type_)));
  }
}

void Worker::freeResources() {
  for (auto alloc : allocations_) {
    runtime_.freeDevice(alloc.device_, alloc.ptr_);
  }
  allocations_.clear();
}