#include "EsperantoRuntime.h"

#include "Core/CommandLineOptions.h"
#include "Support/Logging.h"

#include <absl/flags/flag.h>
#include <absl/strings/numbers.h>
#include <absl/strings/str_cat.h>
#include <absl/strings/str_split.h>
#include <absl/strings/string_view.h>
#include <cstdlib>
#include <experimental/filesystem>
#include <iostream>
#include <string_view>
#include <system_error>

namespace fs = std::experimental::filesystem;

ABSL_FLAG(bool, list_devices, false,
          "List all devices we can find in the system");
ABSL_FLAG(int32_t, device, 0, "Specify the device to execute the command on");
ABSL_FLAG(std::string, write_memory, "",
          "Specifiy address and buffer to write to memory, the format is "
          "<ADDRESS>:<HEX_DATA>, "
          "e.g. 0xdeadbeef:abcdef123456789");
ABSL_FLAG(std::string, read_memory, "",
          "Specifiy address and size in bytes to read data from, the format is "
          "<ADDRESS>:<SIZE>  "
          "e.g. 0xdeadbeef:12, the output should be data in hex characters");

static std::vector<et_runtime::DeviceInformation> getDeviceList() {
  auto device_manager = et_runtime::getDeviceManager();
  auto devices_list_ret = device_manager->enumerateDevices();
  if (!devices_list_ret) {
    RTERROR << "Failed to enumerate devices \n";
    std::terminate();
  }
  auto &device_list = devices_list_ret.get();
  return device_list;
}

static void listDevices() {
  auto device_list = getDeviceList();
  for (auto &device : device_list) {
    std::cout << "Found Device: " << device.name << "\n";
  }
}

static std::vector<char> hexToBytes(const std::string &hex) {
  std::vector<char> bytes;

  for (unsigned int i = 0; i < hex.length(); i += 2) {
    std::string_view hex_str = hex.substr(i, 2);
    uint32_t byte = 0;
    if (absl::SimpleAtoi(hex_str, &byte)) {
      bytes.push_back(static_cast<char>(byte));
    }
  }

  return bytes;
}

static void write_memory(const std::string &opt_val) {
  auto device_ist = getDeviceList();
  auto device_manager = et_runtime::getDeviceManager();
  auto device_list = getDeviceList();
  size_t device = absl::GetFlag(FLAGS_device);
  if (device > device_list.size()) {
    RTERROR << "Device index " << device
            << " larger than expected number of devices " << device_list.size();
    std::terminate();
  }
  auto pcie_dev_res = device_manager->registerDevice(device);
  if (!pcie_dev_res) {
    RTERROR << "Failed to create device " << device;
    std::terminate();
  }
  auto pcie_dev = pcie_dev_res.get();
  // FIXME should perform minimal init of some short ?
  // auto init_res = pcie_dev->init();
  // assert(init_res == etrtSuccess);
  // Access the underlying implementation
  auto &dev_target = pcie_dev->getTargetDevice();
  auto write_cmd = absl::GetFlag(FLAGS_write_memory);
  std::pair<std::string, std::string> res = absl::StrSplit(write_cmd, ":");
  uint64_t addr = 0;
  if (!absl::numbers_internal::safe_strtou64_base(res.first, &addr, 16)) {
    RTERROR << "Error converting address: " << res.first;
    std::terminate();
  }
  auto data = res.second;
  auto bytes = hexToBytes(data);
  RTINFO << "Writting address: " << std::hex << addr << " with data: " << data;
  auto ret = dev_target.writeDevMem(addr, bytes.size(), bytes.data());
  assert(ret);
}

static void read_memory(const std::string &opt) {}

void run() {
  auto list_devices_opt = absl::GetFlag(FLAGS_list_devices);
  if (list_devices_opt) {
    listDevices();
    return;
  }
  auto write_memory_opt = absl::GetFlag(FLAGS_write_memory);
  if (!write_memory_opt.empty()) {
    write_memory(write_memory_opt);
    return;
  }
  auto read_memory_opt = absl::GetFlag(FLAGS_read_memory);
  if (!read_memory_opt.empty()) {
    read_memory(read_memory_opt);
    return;
  }
}

int main(int argc, char *argv[]) {
  et_runtime::ParseCommandLineOptions(argc, argv);
  run();

  return 0;
}
