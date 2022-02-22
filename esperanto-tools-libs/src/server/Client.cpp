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
#include <cereal/archives/portable_binary.hpp>
#include <chrono>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

using namespace rt;

namespace {
constexpr size_t kMaxMessageSize = 4096;
constexpr auto kRecevTimeoutMs = 10; // 10 ms
struct MemStream : public std::streambuf {
  MemStream(char* s, std::size_t n) {
    setg(s, s, s + n);
  }
};
} // namespace

Client::~Client() {
  running_ = false;
  listener_.join();
}

Client::Client(const std::string& socketPath) {
  socket_ = socket(AF_UNIX, SOCK_STREAM, 0);

  sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);
  int val = 1;

  if (setsockopt(socket_, SOL_SOCKET, SO_PASSCRED, &val, sizeof(val)) < 0) {
    throw Exception("unable to set local per credentials");
  }

  if (connect(socket_, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    throw Exception("connect error");
  }
  ucred ucred;
  socklen_t len = sizeof(ucred);
  if (getsockopt(socket_, SOL_SOCKET, SO_PEERCRED, &ucred, &len) == -1) {
    throw Exception("getsockopt error");
  }

  RT_LOG(INFO) << "Runtime client connection: (PID:  " << getpid()
               << ") ===> Credentials from SO_PEERCRED (server credentials): pid=" << ucred.pid
               << ", euid=" << ucred.uid << ", egid=" << ucred.gid;

  listener_ = std::thread(&Client::responseProcessor, this);
}

void Client::responseProcessor() {
  auto requestBuffer = std::vector<char>(kMaxMessageSize);
  auto ms = MemStream{requestBuffer.data(), requestBuffer.size()};

  pollfd fd;
  fd.fd = socket_;
  fd.events = POLLIN;
  fd.revents = 0;
  try {
    while (running_) {
      if (auto rpoll = poll(&fd, 1, kRecevTimeoutMs); rpoll > 0) {
        if (fd.revents & POLLIN) {
          if (auto res = read(socket_, requestBuffer.data(), requestBuffer.size()); res < 0) {
            RT_LOG(WARNING) << "Read socket error: " << strerror(errno);
            throw Exception(std::string{"Read socket error: "} + strerror(errno));
          }
          std::istream is(&ms);
          cereal::PortableBinaryInputArchive archive{is};
          resp::Response response;
          archive >> response;
          processResponse(response);
        } else {
          RT_LOG(WARNING) << "Unexpected poll events result: " << fd.revents;
          throw Exception("Unexpected poll events result: " + std::to_string(fd.revents));
        }
      } else if (rpoll < 0) { // some error happened
        RT_LOG(WARNING) << "Poll error: " << strerror(errno);
        throw Exception(std::string{"Poll error: "} + strerror(errno));
      }
      // rpoll==0 means timeout, so run the bucle again
    }
  } catch (...) {
    // some error happened ... let's TODO: fill this when error handling is implemented
  }
}

bool Client::waitForEvent(EventId event, std::chrono::seconds timeout) {
  return waitForEventWithoutProfiling(event, timeout);
}

bool Client::waitForStream(StreamId stream, std::chrono::seconds timeout) {
  return waitForStreamWithoutProfiling(stream, timeout);
}

bool Client::waitForEventWithoutProfiling(EventId event, std::chrono::seconds timeout) {
  std::unique_lock lock(mutex_);
  if (eventToStream_.find(event) == end(eventToStream_)) {
    return true;
  }

  return waiters_[event].wait_for(lock, timeout,
                                  [this, event] { return eventToStream_.find(event) == end(eventToStream_); });
}

bool Client::waitForStreamWithoutProfiling(StreamId stream, std::chrono::seconds timeout) {
  auto start = std::chrono::steady_clock::now();
  std::unique_lock lock(mutex_);
  auto events = find(streamToEvents_, stream, "Stream does not exist")->second;
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