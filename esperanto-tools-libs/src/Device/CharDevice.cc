//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "CharDevice.h"

#include "Support/Logging.h"

#include <sys/types.h>

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;

namespace et_runtime {
namespace device {

CharacterDevice::CharacterDevice(
    const std::experimental::filesystem::path &char_dev)
    : path_(char_dev) {
  fd_ = open(path_.string().c_str(), O_RDWR);
  if (fd_ == 0) {
    std::terminate();
  }
}

CharacterDevice::CharacterDevice(CharacterDevice &&other)
    : fd_(std::move(other.fd_)) {}

CharacterDevice::~CharacterDevice() {
  auto res = close(fd_);
  auto err = errno;
  if (res < 0) {
    RTERROR << "Failed to close file, error : " << std::strerror(err) << "\n";
    std::terminate();
  }
}

bool CharacterDevice::write(uintptr_t addr, const void *data, ssize_t size) {

  off_t target_offset = static_cast<off_t>(addr);
  auto offset = lseek(fd_, target_offset, SEEK_SET);
  auto err = errno;
  if (offset != target_offset) {
    RTERROR << "Failed to seek to device offset " << hex << addr;
    RTERROR << " Error: " << std::strerror(err) << "\n";
    return false;
  }

  auto rc = ::write(fd_, data, size);
  err = errno;
  if (rc != size) {
    RTERROR << "Failed to write full data, wrote: " << rc;
    RTERROR << " Error: " << std::strerror(err) << "\n";
    return false;
  }
  return true;
}

bool CharacterDevice::read(uintptr_t addr, void *data, ssize_t size) {

  off_t target_offset = static_cast<off_t>(addr);
  auto offset = lseek(fd_, target_offset, SEEK_SET);
  auto err = errno;
  if (offset != target_offset) {
    RTERROR << "Failed to seek to device offset " << hex << addr;
    RTERROR << " Error: " << std::strerror(err) << "\n";
    return false;
  }

  auto rc = ::read(fd_, data, size);
  err = errno;
  if (rc != size) {
    RTERROR << "Failed to write full data, wrote: " << rc;
    RTERROR << " Error: " << std::strerror(err) << "\n";
    return false;
  }
  return true;
}

} // namespace device
} // namespace et_runtime
