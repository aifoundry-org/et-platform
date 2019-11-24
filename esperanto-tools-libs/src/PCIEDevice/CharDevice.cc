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

#include "esperanto/runtime/Support/Logging.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

namespace et_runtime {
namespace device {

CharacterDevice::CharacterDevice(
    const std::experimental::filesystem::path &char_dev, uintptr_t base_addr)
    : path_(char_dev), base_addr_(base_addr) {
  fd_ = open(path_.string().c_str(), O_RDWR);
  if (fd_ == 0) {
    std::terminate();
  }
}

CharacterDevice::CharacterDevice(CharacterDevice &&other)
    : path_(std::move(other.path_)), fd_(std::move(other.fd_)),
      base_addr_(other.base_addr_) {}

CharacterDevice::~CharacterDevice() {
  auto res = close(fd_);
  auto err = errno;
  if (res < 0) {
    RTERROR << "Failed to close file, error : " << std::strerror(err) << "\n";
    std::terminate();
  }
}

bool CharacterDevice::write(uintptr_t addr, const void *data, ssize_t size) {
  RTINFO << "Dev: " << path_ << "PCIE Write: 0x" << std::hex << addr << " size "
         << std::dec << size << "\n";
  // If we know the base address then substrasct that because for now we are
  // expecting SOC absolute addresses
  if (base_addr_ > 0) {
    addr -= base_addr_;
  }
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
  RTINFO << "Dev: " << path_ << "PCIE Read: 0x" << std::hex << addr << " size "
         << std::dec << size << "\n";
  // If we know the base address then substrasct that because for now we are
  // expecting SOC absolute addresses
  if (base_addr_ > 0) {
    addr -= base_addr_;
  }
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
