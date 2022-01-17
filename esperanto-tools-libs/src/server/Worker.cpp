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
#include "RuntimeServer.h"
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
    RT_VLOG(LOW) << "Read socket error: " << strerror(static_cast<int>(res));
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

Worker::Worker(int socket, IRuntime& runtime, RuntimeServer& server)
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
    if (auto res = read(socket_, &reqHeader, sizeof(reqHeader)); res != sizeof(reqHeader)) {
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
    runtime_.freeDevice(DeviceId{req.device_}, req.address_);
    break;
  }

  case Request::Type::ABORT_STREAM: {
    Request::AbortStream req;
    deserializeRequest(req, header.size_);
    auto evt = runtime_.abortStream(StreamId{req.streamId_});
    sendResponse(Response::Type::ABORT_STREAM, evt);
    break;
  }
  }
}

void Worker::freeResources() {
  for (auto alloc : allocations_) {
    runtime_.freeDevice(DeviceId{alloc.device_}, alloc.ptr_);
  }
  allocations_.clear();
}