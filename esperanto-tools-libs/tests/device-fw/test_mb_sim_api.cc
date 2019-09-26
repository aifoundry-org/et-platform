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
using namespace et_runtime;

class DummySimulator final : public AbstractSimulator {

public:
  DummySimulator() : AbstractSimulator(createSocketFile()), done_(false), mbox_{0} {}

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

  bool boot(uint64_t pc) override {
    return true;
  }
  bool shutdown() override {
    done_.store(true);
    return true;
  }
  bool is_done() override {
    return done_.load();
  }
  int active_threads() override {
    return 0;
  }
  bool read(uint64_t ad, size_t size, void *data) override {
    return true;
  }
  bool write(uint64_t ad, size_t size, const void *data) override {
    return true;
  }
  bool mb_read(struct mbox_t* mbox) override {
    *mbox = mbox_;
    return true;
  }
  bool mb_write(const struct mbox_t& mbox) override {
    mbox_ = mbox;
    return true;
  }
  bool raise_device_interrupt(simulator_api::DeviceInterruptType type) override {
    return true;
  }

  std::atomic<bool> done_;
  struct mbox_t mbox_;
};

class MBSimAPITest : public ::testing::Test {

protected:
  MBSimAPITest()
      : sim_(), sim_api_(&sim_, true), rpc_(0, sim_.communicationPath()) {}

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
    while (!sim_.is_done()) {
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

// Send a device PU PLIC PCIe Message Interrupt
TEST_F(MBSimAPITest, RaiseDevicePuPlicPcieMessageInterrupt) {
  rpc_.raiseDevicePuPlicPcieMessageInterrupt();
  rpc_.shutdown();
}

// Send an IPI to the Master Shire
TEST_F(MBSimAPITest, RaiseDeviceMasterShireIpiInterrupt) {
  rpc_.raiseDeviceMasterShireIpiInterrupt();
  rpc_.shutdown();
}

// Receive a Device Interrupt

TEST_F(MBSimAPITest, BlockOnDeviceInterrupt) {
  // Thread that waits to receive an interrupt from the device.
  auto rpc_thread = std::thread([this]() {
    rpc_.waitForHostInterrupt();
    return true;
  });
  // send interrupt to host
  std::this_thread::sleep_for(3s);
  sim_api_.raiseHostInterrupt();
  rpc_thread.join();
  rpc_.shutdown();
}

// Read a mailbox message cases
TEST_F(MBSimAPITest, ReadEmptyMailBoxMessage) {
  // Prepare the mailbox status
  sim_.mbox_.master_status = et_runtime::device_fw::MBOX_STATUS_READY;
  // Raise a host interrupt on the device
  sim_api_.raiseHostInterrupt();
  uint8_t data[MBOX_MAX_LENGTH];
  // Read the mailbox message it should be empty
  auto res = rpc_.mb_read(data, sizeof(data));
  EXPECT_TRUE(!res);
  rpc_.shutdown();
}

// Read a valid mailbox message
TEST_F(MBSimAPITest, ReadMailBoxMessage) {
  // Prepare the mailbox status
  sim_.mbox_.master_status = et_runtime::device_fw::MBOX_STATUS_READY;
  device::RingBuffer rb(device::RingBufferType::RX, rpc_);
  auto elem_num = 20;
  std::vector<uint16_t> data(elem_num, 0xbeef);
  uint16_t data_size =
      data.size() * sizeof(typename decltype(data)::value_type);
  const device_fw::mbox_header_t header = {.length = (uint16_t)data_size,
                                           .magic = MBOX_MAGIC};
  rb.write(&header, sizeof(header));
  rb.write(data.data(), data_size);
  auto &state = rb.state();
  sim_.mbox_.tx_ring_buffer.head_index = state.head_index;
  sim_.mbox_.tx_ring_buffer.tail_index = state.tail_index;
  memcpy(sim_.mbox_.tx_ring_buffer.queue, state.queue, sizeof(state.queue));

  // Raise a host interrupt on the device, to mark the device ready
  sim_api_.raiseHostInterrupt();
  std::vector<uint16_t> res_data(elem_num, 0);

  // Read the mailbox message
  auto res = rpc_.mb_read(res_data.data(), data_size);
  EXPECT_EQ(res, data_size);
  EXPECT_THAT(res_data, ::testing::ElementsAreArray(data));
  rpc_.shutdown();
}

// Write a valid mailbox message
TEST_F(MBSimAPITest, WriteMailBoxMessage) {
  // Prepare the mailbox status
  sim_.mbox_.master_status = et_runtime::device_fw::MBOX_STATUS_READY;

  // reference data
  auto elem_num = 20;
  std::vector<uint16_t> data(elem_num, 0xbeef);
  uint16_t data_size =
      data.size() * sizeof(typename decltype(data)::value_type);

  // Raise a host interrupt on the device so that the host can proceed
  sim_api_.raiseHostInterrupt();

  // Write the mailbox message
  auto res = rpc_.mb_write(data.data(), data_size);
  EXPECT_TRUE(res);

  // Validate the writen results in the simulator

  device::RingBuffer rb(device::RingBufferType::RX, rpc_);
  device_fw::ringbuffer_s irb;
  irb.head_index = sim_.mbox_.rx_ring_buffer.head_index;
  irb.tail_index = sim_.mbox_.rx_ring_buffer.tail_index;
  memcpy(irb.queue, sim_.mbox_.rx_ring_buffer.queue, sizeof(irb.queue));

  rb.setState(irb);
  device_fw::mbox_header_t header;

  rb.read(&header, sizeof(header));
  EXPECT_EQ(header.length, data_size);

  std::vector<uint16_t> res_data(elem_num, 0);
  rb.read(res_data.data(), data_size);

  EXPECT_THAT(res_data, ::testing::ElementsAreArray(data));
  rpc_.shutdown();
}

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  et_runtime::ParseCommandLineOptions(argc, argv);
  return RUN_ALL_TESTS();
}
