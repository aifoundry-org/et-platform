#include "SysEmuHostListener.h"
#include "utils.h"
#include <cassert>
using namespace dev;

void SysEmuHostListener::pcieReady() {
  pcieReady_.set_value(); 
}

void SysEmuHostListener::memoryReadFromHost(uint64_t address, size_t size, std::byte* dst) {
  DV_VLOG(HIGH) << "Device requested host memory read: addr: " << std::hex << address << ", size: " << size
                << ", dst: " << dst;
  IHostListener::memoryReadFromHost(address, size, dst);
}

void SysEmuHostListener::memoryWriteFromHost(uint64_t address, size_t size, const std::byte* src) {
  DV_VLOG(HIGH) << "Device requested host memory write: addr: " << std::hex << address << ", size: " << size
                << ", src: " << src;
  IHostListener::memoryWriteFromHost(address, size, src);
}

std::future<void> SysEmuHostListener::getPcieReadyFuture() {
  return pcieReady_.get_future();
}
