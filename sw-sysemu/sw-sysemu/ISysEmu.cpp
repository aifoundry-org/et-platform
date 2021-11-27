#include "sw-sysemu/ISysEmu.h"
#include "SysEmuImp.h"

using namespace emu;

void ISysEmu::IHostListener::memoryReadFromHost(uint64_t address, size_t size, std::byte* dst) {
  auto src = reinterpret_cast<std::byte*>(address);
  std::copy(src, src + size, dst);
}

void ISysEmu::IHostListener::memoryWriteFromHost(uint64_t address, size_t size, const std::byte* src) {
  auto dst = reinterpret_cast<std::byte*>(address);
  std::copy(src, src + size, dst);
}

std::unique_ptr<ISysEmu> ISysEmu::create(const SysEmuOptions& options, const std::array<uint64_t, 8>& barAddresses,
                                         IHostListener* hostListener) {
  return std::make_unique<SysEmuImp>(options, barAddresses, hostListener);
}