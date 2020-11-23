//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "PCIeDevice.h"

#include "Core/DeviceFwTypes.h"
#include "esperanto/runtime/Core/CommandLineOptions.h"
#include "esperanto/runtime/Support/Logging.h"

#include <absl/strings/str_format.h>
#include <asm-generic/errno.h>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <regex>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <et_ioctl.h>
#include <thread>
#include <experimental/filesystem>
#include "Tracing/Tracing.h"
#include "esperanto/runtime/Support/TimeHelpers.h"

namespace fs = std::experimental::filesystem;

namespace et_runtime {
namespace device {
namespace {

  //FIX magic numbers
  TimeDuration kPollingInterval = std::chrono::milliseconds(5);
  auto kWaitTime = std::chrono::seconds(5 * 60);

  //helper to make sure a trace is recorded when creating the object and when exiting the basic block
  struct ScopedTrace {
    ScopedTrace(std::function<void()> start, std::function<void()> end)
      : end_(move(end)) {
        start();
    }
    ~ScopedTrace() {
      end_();
    }
    std::function<void()> end_;
  };

  struct IoctlResult {
    int rc_;
    operator bool() const {return rc_ >= 0;}
  };


  template<typename ...Types>
  IoctlResult wrap_ioctl(int fd, unsigned long int request, Timepoint retryEnd, Types... args) {
    auto res = ::ioctl(fd, request, args...);
    if (res < 0) {
      auto error = errno;
      RTERROR << "Failed to execute IOCTL: " << std::strerror(error) << "\n";
      //check if ioctl failed due to timeout, check the retryEnd time limit and retry after a while
      if (errno == ETIMEDOUT && Clock::now() < retryEnd) {
        RTERROR << "IOCTL not ready in time, retrying after " << std::chrono::duration_cast<std::chrono::milliseconds>(kPollingInterval).count() << "ms \n";
        std::this_thread::sleep_for(kPollingInterval);
        return wrap_ioctl(fd, request, retryEnd, args...);
      }
    }
    return {res};
  }

  void enableMMIO(int fd) {
    if (!wrap_ioctl(fd, ETSOC1_IOCTL_SET_BULK_CFG, Clock::now(), static_cast<uint32_t>(BULK_CFG_MMIO))) {
      RTERROR << "Failed to enable MMIO\n";
      std::terminate();
    }
    TRACE_PCIeDevice_enable_mmio();
  }
  void enableDMA(int fd) {
    if (!wrap_ioctl(fd, ETSOC1_IOCTL_SET_BULK_CFG, Clock::now(), static_cast<uint32_t>(BULK_CFG_DMA))) {
      RTERROR << "Failed to enable DMA\n";
      std::terminate();
    }
    TRACE_PCIeDevice_enable_dma();
  }
}

PCIeDevice::PCIeDevice(int index, bool mgmtNode)
  : DeviceTarget(index)
  , path_(absl::StrFormat("/dev/et%d_%s", index, (mgmtNode) ? "mgmt" : "ops" )) {
    fd_ = open(path_.c_str(), O_RDWR);
    if (fd_ < 0) {
      RTERROR << "Error opening driver: " << std::strerror(errno) << "\n";
      //#TODO deal with errors in the same way across all the code
      std::terminate();
    }
    RTINFO << "PCIe target opened: \"" << path_ << "\"\n";

    auto res = wrap_ioctl(fd_, ETSOC1_IOCTL_GET_MBOX_MAX_MSG, Clock::now(), &mboxMaxMsgSize_);
    if (!res) {
      RTERROR << "Failed to get maximum mailbox message size\n";
      std::terminate();
    }
    RTINFO << "Maximum mbox message size: " << mboxMaxMsgSize_ << "\n";

    if (mgmtNode) {
      res = wrap_ioctl(fd_, ETSOC1_IOCTL_GET_FW_UPDATE_REG_BASE, Clock::now(), &firmwareBase_);
      if (!res) {
        RTERROR << "Failed to get FIRMWARE base\n";
        std::terminate();
      }
      RTINFO << "FIRMWARE base: 0x" << std::hex << firmwareBase_ << "\n";

      res = wrap_ioctl(fd_, ETSOC1_IOCTL_GET_FW_UPDATE_REG_SIZE, Clock::now(), &firmwareSize_);
      if (!res) {
        RTERROR << "Failed to get FIRMWARE size\n";
        std::terminate();
      }
      RTINFO << "FIRMWARE size: 0x" << std::hex << firmwareSize_ << "\n";
    } else {
      res = wrap_ioctl(fd_, ETSOC1_IOCTL_GET_DRAM_BASE, Clock::now(), &dramBase_);
      if (!res) {
        RTERROR << "Failed to get DRAM base\n";
        std::terminate();
      }
      RTINFO << "DRAM base: 0x" << std::hex << dramBase_ << "\n";

      res = wrap_ioctl(fd_, ETSOC1_IOCTL_GET_DRAM_SIZE, Clock::now(), &dramSize_);
      if (!res) {
        RTERROR << "Failed to get DRAM size\n";
        std::terminate();
      }
      RTINFO << "DRAM size: 0x" << std::hex << dramSize_ << "\n";
    }
}

PCIeDevice::~PCIeDevice() {
  auto res = close(fd_);
  if (res < 0) {
    RTERROR << "Failed to close file, error: " << std::strerror(errno) << "\n";
    std::terminate();
  }
}

bool PCIeDevice::read(uintptr_t addr, void *data, ssize_t size) {
  RTINFO << "Dev: " << path_ << "PCIE Read: 0x" << std::hex << addr << " size "
          << std::dec << size << "\n";

    off_t target_offset = static_cast<off_t>(addr);
    auto offset = lseek(fd_, target_offset, SEEK_SET);
    auto err = errno;
    if (offset != target_offset) {
      RTERROR << "Failed to seek to device offset " << std::hex << addr;
      RTERROR << " Error: " << std::strerror(err) << "\n";
      return false;
    }

  auto rc = ::read(fd_, data, size);
  err = errno;
  if (rc != size) {
    RTERROR << "Failed to read full data, wrote: " << rc;
    RTERROR << " Error: " << std::strerror(err) << "\n";
    return false;
  }
  return true;
}

bool PCIeDevice::write(uintptr_t addr, const void* data, ssize_t size) {
  RTINFO << "Dev: " << path_ << "PCIE Write: 0x" << std::hex << addr << " size "
         << std::dec << size << "\n";
  off_t target_offset = static_cast<off_t>(addr);
  auto offset = lseek(fd_, target_offset, SEEK_SET);
  auto err = errno;
  if (offset != target_offset) {
    RTERROR << "Failed to seek to device offset " << std::hex << addr;
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

void PCIeDevice::resetMbox(TimeDuration wait_time) {
  auto valid = wrap_ioctl(fd_, ETSOC1_IOCTL_RESET_MBOX, Clock::now() + wait_time);
  if (!valid) {
    RTERROR << "Failed to g \n";
    std::terminate();
  }
  TRACE_PCIeDevice_mailbox_reset();
}

bool PCIeDevice::readyMbox(TimeDuration wait_time) {
  uint64_t ready = 0;
  auto res = wrap_ioctl(fd_, ETSOC1_IOCTL_GET_MBOX_READY, Clock::now() + wait_time, &ready);

  TRACE_PCIeDevice_mailbox_ready(static_cast<bool>(ready));
  if (!res) {
    if (errno == ETIMEDOUT) {
      RTERROR << "Mailbox not ready in time \n";
    } else {
      RTERROR << "Failed to get the status of the mailbox. Errno: " << strerror(errno) << std::endl;
      std::terminate();
    }
  }
  return static_cast<bool>(ready);
}

bool PCIeDevice::init() {
  // FIXME current we perform no initialization action that will not apply in
  // the future.
  RTINFO << "PCIeDevice: Initialization \n";
  resetMbox(kWaitTime);
  RTINFO << "PCIEDevice: Reset MM mailbox \n";
  // Wait for the device to be ready
  auto mb_ready = readyMbox(kWaitTime);
  RTINFO << "PCIEDevice: MM mailbox ready " << mb_ready << "\n";
  device_alive_ = mb_ready;
  return device_alive_;
}

bool PCIeDevice::deinit() { return true; }

bool PCIeDevice::getStatus() {
  assert(false);
  return false;
}

DeviceInformation PCIeDevice::getStaticConfiguration() {
  assert(false);
  return {};
}

bool PCIeDevice::submitCommand() {
  assert(false);
  return false;
}

bool PCIeDevice::registerResponseCallback() {
  assert(false);
  return false;
}

bool PCIeDevice::registerDeviceEventCallback() {
  assert(false);
  return false;
}

bool PCIeDevice::virtQueuesDiscover(TimeDuration wait_time) {
  assert(false);
  return false;
}

bool PCIeDevice::virtQueueWrite(const void *data, ssize_t size, uint8_t queueId) {
  assert(false);
  return false;
}

ssize_t PCIeDevice::virtQueueRead(void *data, ssize_t size, uint8_t queueId, TimeDuration wait_time) {
  assert(false);
  return false;
}

bool PCIeDevice::waitForEpollEvents(uint32_t &sq_bitmap, uint32_t &cq_bitmap) {
  assert(false);
  return false;
}

#define MMIO_LIMIT 1024

bool PCIeDevice::readDevMemMMIO(uintptr_t dev_addr, size_t size, void *buf) {
  enableMMIO(fd_);
  TRACE_PCIeDevice_mmio_read_start(dev_addr, (uint64_t)buf, size);
  size_t tcount = 0;
  bool res = true;
  for ( ; tcount+MMIO_LIMIT <= size; tcount += MMIO_LIMIT) {
    res = read(dev_addr+tcount, (void*)(static_cast<char*>(buf)+tcount), MMIO_LIMIT);

    if (!res) {
      break;
    }
  }
  if (res && (tcount < size)) {
    res = read(dev_addr+tcount, (void*)(static_cast<char*>(buf)+tcount), size-tcount);
  }
  TRACE_PCIeDevice_mmio_read_end();
  return res;
}

bool PCIeDevice::writeDevMemMMIO(uintptr_t dev_addr, size_t size, const void *buf) {
  enableMMIO(fd_);
  TRACE_PCIeDevice_mmio_write_start(dev_addr, (uint64_t)buf, size);
  size_t tcount = 0;
  bool res = true;
  for ( ; tcount+MMIO_LIMIT <= size; tcount += MMIO_LIMIT) {
    res = write(dev_addr+tcount, (void*)(static_cast<const char*>(buf)+tcount), MMIO_LIMIT);

    if (!res) {
      break;
    }
  }
  if (res && (tcount < size)) {
    res = write(dev_addr+tcount, (void*)(static_cast<const char*>(buf)+tcount), size-tcount);
  }
  TRACE_PCIeDevice_mmio_write_end();
  return res;
}

bool PCIeDevice::readDevMemDMA(uintptr_t dev_addr, size_t size, void *buf) {
  enableDMA(fd_);
  TRACE_PCIeDevice_dma_read_start(dev_addr, (uint64_t)buf, size);
  auto res = read(dev_addr, buf, size);
  TRACE_PCIeDevice_dma_read_end();
  return res;
}

bool PCIeDevice::writeDevMemDMA(uintptr_t dev_addr, size_t size,
                                const void *buf) {
  enableDMA(fd_);
  TRACE_PCIeDevice_dma_write_start(dev_addr, (uint64_t)buf, size);
  auto res = write(dev_addr, buf, size);
  TRACE_PCIeDevice_dma_write_end();
  return res;
}

bool PCIeDevice::mb_write(const void *data, ssize_t size) {
  TRACE_PCIeDevice_mailbox_write_start((uint64_t)data, size);
  auto res = wrap_ioctl(fd_, ETSOC1_IOCTL_PUSH_MBOX(size), Clock::now(), data);
  TRACE_PCIeDevice_mailbox_write_end();
  return res.rc_ == size;
}

ssize_t PCIeDevice::mb_read(void *data, ssize_t size, TimeDuration wait_time) {
  int dataRead = 0;
  auto trace = ScopedTrace(
    [&](){TRACE_PCIeDevice_mailbox_read_start((uint64_t)data, size);},
    [&](){TRACE_PCIeDevice_mailbox_read_end(dataRead);}
  );
  auto start = Clock::now();
  auto end = start + wait_time;
  while (auto res = wrap_ioctl(fd_, ETSOC1_IOCTL_POP_MBOX(size), Clock::now(), data)) {
    if (res.rc_ > 0) {
      dataRead = res.rc_;
      break;
    }
    std::this_thread::sleep_for(kPollingInterval);
    if (end < Clock::now()) {
      break;
    }
  }
  return dataRead;
}

bool PCIeDevice::shutdown() {
  assert(false);
  return true;
}

uintptr_t PCIeDevice::dramBaseAddr() const { return dramBase_; }

uintptr_t PCIeDevice::dramSize() const { return dramSize_; }

uintptr_t PCIeDevice::FWBaseAddr() const { return firmwareBase_; }

uintptr_t PCIeDevice::FWSize()const { return firmwareSize_; }

ssize_t PCIeDevice::mboxMsgMaxSize() const { return mboxMaxMsgSize_; }

std::vector<DeviceInformation> PCIeDevice::enumerateDevices() {
  std::vector<DeviceInformation> infos;
  /// We are going to match one of the character devices if it exists we are
  /// assuing the rest have been created
  const std::regex dev_name_re("et[[:digit:]]+_ops");
  for (auto &file : fs::directory_iterator("/dev")) {
    auto &path = file.path();
    if (fs::is_character_file(path)) {
      std::smatch match;
      auto path_str = path.string();
      if (std::regex_search(path_str, match, dev_name_re)) {
        // FIXME for now add empty information in the future
        // we should add real information
        DeviceInformation info;
        strncpy(info.name, path_str.c_str(), sizeof(info.name) - 1);
        infos.emplace_back(info);
      }
    }
  }
  return infos;
}

} // namespace device
} // namespace et_runtime
