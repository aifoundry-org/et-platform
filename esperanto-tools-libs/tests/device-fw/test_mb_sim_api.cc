#include "Core/CommandLineOptions.h"
#include "Device/TargetRPC.h"

// Enable logging on the server side
#define ENABLE_LOGGING 1
#include <atomic>
#include <cstdint>
#include <esperanto/simulator_server.h>
#include <experimental/filesystem>
#include <future>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>
#include <thread>
#include <vector>

using namespace std;
using namespace simulator_api;
namespace fs = std::experimental::filesystem;

class DummySimulator final : public AbstractSimulator {

public:
  DummySimulator() : AbstractSimulator(createSocketFile()), done_(false) {}

  std::string createSocketFile() const {
    // Create a temporary file for the socket and overwrite the
    // one passed by the user. The following function opens the file and
    // immediately deletes as log as it is open by the process it is still
    // accessible.
    auto socket_file = std::tmpfile();
    // Get filename from the socket number
    auto file_path =
        fs::path("/proc/self/fd") / std::to_string(fileno(socket_file));
    cout << "FilePath : " + file_path.string() << "\n";
    auto socket_name = fs::read_symlink(file_path).string();
    // Remove the following suffix  and replace it with -sysemu.sock
    auto pos = socket_name.find_last_not_of(" (deleted)");
    socket_name = socket_name.substr(0, pos);
    socket_name += "-sysemu.sock";
    return string("unix://") + socket_name;
  }

  bool done() override { return done_.load(); }
  int active_threads() override { return 0; }
  bool boot() override { return true; }
  bool sync() override { return true; }
  bool read(uint64_t ad, size_t size, void *data) override {
    memcpy(data, data_.data(), size);
    return true;
  }
  bool write(uint64_t ad, size_t size, const void *data) override {
    data_.clear();
    data_.resize(size);
    memcpy(data_.data(), data, size);
    return true;
  }
  bool mb_read(struct mbox_t* mbox) override {
    return true;
  }
  bool mb_write(const struct mbox_t& mbox) override {
    return true;
  }
  bool raise_device_interrupt() override {
    return true;
  }
  bool new_region(uint64_t base, uint64_t size, int flags = 0) override {
    return true;
  }
  bool execute(const rt_host_kernel_launch_info_t &launch_info) override {
    return true;
  }
  bool continue_exec() override { return true; }
  void set_done(bool val) override { done_.store(val); }

private:
  // memorize the last write
  vector<uint8_t> data_;
  std::atomic<bool> done_;
};

class MBSimAPITest : public ::testing::Test {

protected:
  MBSimAPITest() : sim_(), sim_api_(&sim_), rpc_(0, sim_.communicationPath()) {}

  void SetUp() override {
    auto res = sim_api_.init();
    ASSERT_TRUE(res);
    res = rpc_.init();
    ASSERT_TRUE(res);

    // Start simulator thread
    sim_thread_ = std::thread([this]() { execution_loop(); });
  }

  void TearDown() override { sim_thread_.join(); }

  // Simulator Execution loop
  void execution_loop() {
    while (!sim_.done()) {
      sim_api_.nextCmd(false);
    }
  }

  DummySimulator sim_;
  SimAPIServer<DummySimulator> sim_api_;
  et_runtime::device::RPCTarget rpc_;
  std::thread sim_thread_;
};

// Start the simulator thread and send simple RPC commands
TEST_F(MBSimAPITest, StartThread) { rpc_.shutdown(); }

// Send a device Interrupt
TEST_F(MBSimAPITest, RaiseDeviceInterrupt) {
  rpc_.raiseDeviceInterrupt();
  rpc_.shutdown();
}

// Receive a Device Interrupt

TEST_F(MBSimAPITest, BlockOnDeviceInterrupt) {
  // Thread that waits to receive an interrupt from the device.
  auto rpc_thread = std::thread([this]() {
    rpc_.waitForDeviceInterrupt();
    return true;
  });
  // send interrupt to host
  std::this_thread::sleep_for(3s);
  sim_api_.raiseHostInterrupt();
  rpc_thread.join();
  rpc_.shutdown();
}

// Prepopulate the mailbox message and send it.

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  et_runtime::ParseCommandLineOptions(argc, argv);
  return RUN_ALL_TESTS();
}
