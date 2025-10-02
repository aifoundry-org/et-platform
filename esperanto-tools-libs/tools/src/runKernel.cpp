#include "RuntimeImp.h"
#include <chrono>
#include <device-layer/IDeviceLayer.h>
#include <fstream>
#include <hostUtils/logging/Logging.h>
#include <thread>

inline std::vector<std::byte> readFile(const std::string& path) {
  auto file = std::ifstream(path, std::ios_base::binary);
  if (!file.is_open()) {
    throw std::runtime_error("can't open file" + path);
  }
  auto iniF = file.tellg();
  file.seekg(0, std::ios::end);
  auto endF = file.tellg();
  auto size = endF - iniF;
  file.seekg(0, std::ios::beg);

  std::vector<std::byte> fileContent(static_cast<uint32_t>(size));
  file.read(reinterpret_cast<char*>(fileContent.data()), size);
  return fileContent;
}

int main(int argc, char* argv[]) {
  using namespace std::chrono_literals;

  std::shared_ptr<dev::IDeviceLayer> deviceLayer = dev::IDeviceLayer::createPcieDeviceLayer();
  auto runtime = rt::IRuntime::create(deviceLayer);
  auto devices = runtime->getDevices();
  auto st = runtime->createStream(devices[0]);
  auto fileContents = readFile(argv[1]);
  auto kernel = runtime->loadCode(st, fileContents.data(), fileContents.size());
  runtime->waitForStream(st);
  auto rimp = static_cast<rt::RuntimeImp*>(runtime.get());
  volatile bool done = false;
  rimp->setSentCommandCallback(devices[0], [&done](rt::Command const* cmd) { done = true; });
  std::array<std::byte, 4> junk;
  runtime->kernelLaunch(st, kernel.kernel_, junk.data(), junk.size(), 0x1FFFFFFFFUL);
  LOG(INFO) << "Waiting for command to be sent...";
  while (!done) {
    std::this_thread::sleep_for(1ms);
  }
  LOG(INFO) << "Kernel launched.";
}
