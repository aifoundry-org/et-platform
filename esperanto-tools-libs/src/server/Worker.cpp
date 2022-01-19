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
using ReqHeader = Request::Header;
using RespHeader = Response::Header;

struct MemStream : public std::streambuf {
  MemStream(char* s, std::size_t n) {
    setg(s, s, s + n);
  }
};

template <typename T> bool Worker::deserializeRequest(T& out, size_t size) {
  std::vector<char> buffer(size);
  if (auto res = read(socket_, buffer.data(), size - sizeof(ReqHeader)); res < static_cast<long>(size)) {
    RT_VLOG(LOW) << "Read socket error: " << strerror(static_cast<int>(errno));
    RT_LOG(INFO) << "Closing connection.";
    return false;
  }
  try {
    auto ms = MemStream{buffer.data(), buffer.size()};
    std::istream is(&ms);
    cereal::PortableBinaryInputArchive archive(is);
    archive(out);
    return true;
  } catch (...) {
    return false;
  }
}

template <typename T> bool Worker::sendResponse(Response::Type type, T payload) {
  std::stringstream response;
  cereal::PortableBinaryOutputArchive archive(response);
  archive(payload);
  auto str = response.str();
  RespHeader rspHeader;
  rspHeader.size_ = static_cast<uint32_t>(sizeof(rspHeader) + str.size());
  rspHeader.type_ = type;
  if (auto res = write(socket_, &rspHeader, sizeof(rspHeader)); res < static_cast<long>(sizeof(rspHeader))) {
    RT_VLOG(LOW) << "Write socket error: " << strerror(static_cast<int>(errno));
    RT_LOG(INFO) << "Closing connection.";
    return false;
  }
  if (auto res = write(socket_, str.data(), str.size()); res < static_cast<long>(str.size())) {
    RT_VLOG(LOW) << "Write socket error: " << strerror(static_cast<int>(errno));
    RT_LOG(INFO) << "Closing connection.";
    return false;
  }
  return true;
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

  while (running_) {
    ReqHeader reqHeader;
    if (auto res = read(socket_, &reqHeader, sizeof(reqHeader)); res != static_cast<long>(sizeof(reqHeader))) {
      RT_VLOG(LOW) << "Read socket error: " << strerror(static_cast<int>(res));
      RT_LOG(INFO) << "Closing connection.";
      break;
    }
    if (!processRequest(reqHeader)) {
      break;
    }
  }
  server_.removeWorker(this);
}

bool Worker::processRequest(const ReqHeader& header) {
  switch (header.type_) {

  case Request::Type::FREE: {
    Request::Free req;
    deserializeRequest(req, header.size_);
    auto addr = reinterpret_cast<std::byte*>(req.address_);
    if (allocations_.erase(Allocation{req.device_, addr}) != 1) {
      RT_LOG(WARNING) << "Trying to deallocate a non previous allocated buffer.";
      return false;
    }
    runtime_.freeDevice(DeviceId{req.device_}, addr);
    return true;
  }

  case Request::Type::ABORT_STREAM: {
    Request::AbortStream req;
    deserializeRequest(req, header.size_);
    auto evt = runtime_.abortStream(StreamId{req.streamId_});
    return sendResponse(Response::Type::ABORT_STREAM, evt);
  }

  case Request::Type::CREATE_STREAM: {
    Request::CreateStream req;
    deserializeRequest(req, header.size_);
    auto st = runtime_.createStream(req.device_);
    return sendResponse(Response::Type::CREATE_STREAM, st);
  }
  default:
    RT_LOG(WARNING) << "Unknown request: " << static_cast<int>(header.type_);
    return false;
  }
}

void Worker::freeResources() {
  for (auto alloc : allocations_) {
    runtime_.freeDevice(alloc.device_, alloc.ptr_);
  }
  allocations_.clear();
}