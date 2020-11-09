#include "RPCDevice/TargetRPC.h"
#include "esperanto/runtime/Core/CommandLineOptions.h"

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
    auto file_path = fs::path("/proc/self/fd") / std::to_string(fileno(socket_file));
    cout << "FilePath : " + file_path.string() << "\n";
    auto socket_name = fs::read_symlink(file_path).string();
    // Remove the following suffix  and replace it with -sysemu.sock
    auto pos = socket_name.find_last_not_of(" (deleted)");
    socket_name = socket_name.substr(0, pos);
    socket_name += "-sysemu.sock";
    return string("unix://") + socket_name;
  }

  bool boot(uint32_t shire_id, uint32_t thread0_enable, uint32_t thread1_enable) override {
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
  bool memory_read(uint64_t ad, size_t size, void *data) override {
    return true;
  }
  bool memory_write(uint64_t ad, size_t size, const void *data) override {
    return true;
  }
  bool mailbox_read(simulator_api::MailboxTarget target, uint32_t offset, size_t size, void *data) override {
    const uint8_t *const mbox_ptr = reinterpret_cast<const uint8_t *const>(&mbox_);
    memcpy(data, mbox_ptr + offset, size);
    return true;
  }
  bool mailbox_write(simulator_api::MailboxTarget target, uint32_t offset, size_t size, const void *data) override {
    uint8_t *const mbox_ptr = reinterpret_cast<uint8_t *const>(&mbox_);
    memcpy(mbox_ptr + offset, data, size);
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
  MBSimAPITest() : sim_(), sim_api_{}, rpc_(0, sim_.communicationPath()) {}

  void SetUp() override {
    sim_api_ = std::make_unique<SimAPIServer<DummySimulator>>(&sim_);
    auto res = sim_api_->init();
    ASSERT_TRUE(res);
    res = rpc_.init();
    ASSERT_TRUE(res);

    // Start simulator thread
    sim_thread_ = std::thread([this]() { execution_loop(); });
  }

  void TearDown() override {
    sim_thread_.join();
    sim_api_.reset();
  }

  // Simulator Execution loop
  void execution_loop() {
    while (!sim_.is_done()) {
      sim_api_->nextCmd(false);
    }
  }

  static constexpr size_t tx_ring_buffer_off_ = offsetof(device_fw::mbox_t, tx_ring_buffer);
  static constexpr size_t rx_ring_buffer_off_ = offsetof(device_fw::mbox_t, rx_ring_buffer);

  DummySimulator sim_;
  std::unique_ptr<SimAPIServer<DummySimulator>> sim_api_;
  et_runtime::device::RPCTarget rpc_;
  std::thread sim_thread_;
};

// Start the simulator thread and send simple RPC commands
TEST_F(MBSimAPITest, StartThread) { rpc_.shutdown(); }

// Send a device PU PLIC PCIe Message Interrupt
TEST_F(MBSimAPITest, RaiseDevicePuPlicPcieMessageInterrupt) {
  rpc_.rpcRaiseDevicePuPlicPcieMessageInterrupt();
  rpc_.shutdown();
}

// Send a device SPIO PLIC PCIe Message Interrupt
TEST_F(MBSimAPITest, RaiseDeviceSpioPlicPcieMessageInterrupt) {
  rpc_.rpcRaiseDeviceSpioPlicPcieMessageInterrupt();
  rpc_.shutdown();
}

// Receive a Device Interrupt
TEST_F(MBSimAPITest, BlockOnDeviceInterrupt) {
  // Thread that waits to receive an interrupt from the device.
  auto rpc_thread = std::thread([this]() {
    rpc_.rpcWaitForHostInterrupt();
    return true;
  });
  // send interrupt to host
  std::this_thread::sleep_for(3s);
  // Raise a zeroth bit host interrupt
  sim_api_->raiseHostInterrupt(0x1);
  rpc_thread.join();
  rpc_.shutdown();
}

// Read a mailbox message cases
TEST_F(MBSimAPITest, ReadEmptyMailBoxMessage) {
  // Prepare the mailbox status
  sim_.mbox_.master_status = et_runtime::device_fw::MBOX_STATUS_READY;
  // Raise a zeroth bit host interrupt on the device
  sim_api_->raiseHostInterrupt(0x1);
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
  device::RingBuffer rb(rpc_, MailboxTarget::MAILBOX_TARGET_MM, tx_ring_buffer_off_);
  bool res;
  int64_t wr_res;

  // Reference data
  auto elem_num = 20;
  std::vector<uint16_t> data(elem_num, 0xbeef);
  uint16_t data_size = data.size() * sizeof(typename decltype(data)::value_type);
  const device_fw::mbox_header_t header = {.length = (uint16_t)data_size,
                                           .magic = MBOX_MAGIC};

  // Pull the latest state from the simulator
  res = rb.readRingBufferIndices();
  EXPECT_TRUE(res);

  // Write header
  wr_res = rb.write(&header, sizeof(header));
  EXPECT_EQ(wr_res, sizeof(header));

  // Write body
  wr_res = rb.write(data.data(), data_size);
  EXPECT_EQ(wr_res, data_size);

  // Update the state of the ring buffer back in the simulator
  res = rb.writeRingBufferHeadIndex();
  EXPECT_TRUE(res);

  // Raise a zeroth bit host interrupt on the device, to mark the device ready
  sim_api_->raiseHostInterrupt(0x1);
  std::vector<uint16_t> res_data(elem_num, 0);

  // Read the mailbox message
  res = rpc_.mb_read(res_data.data(), data_size);
  EXPECT_TRUE(res);
  EXPECT_THAT(res_data, ::testing::ElementsAreArray(data));
  rpc_.shutdown();
}

// Write a valid mailbox message
TEST_F(MBSimAPITest, WriteMailBoxMessage) {
  // Prepare the mailbox status
  sim_.mbox_.master_status = et_runtime::device_fw::MBOX_STATUS_READY;
  device::RingBuffer rb(rpc_, MailboxTarget::MAILBOX_TARGET_MM, rx_ring_buffer_off_);

  // Reference data
  auto elem_num = 20;
  std::vector<uint16_t> data(elem_num, 0xbeef);
  uint16_t data_size = data.size() * sizeof(typename decltype(data)::value_type);

  // Raise a zeroth bit host interrupt on the device so that the host can proceed
  sim_api_->raiseHostInterrupt(0x1);

  // Write the mailbox message
  auto res = rpc_.mb_write(data.data(), data_size);
  EXPECT_TRUE(res);

  // Validate the writen results in the simulator

  // Pull the latest state from the simulator
  res = rb.readRingBufferIndices();
  EXPECT_TRUE(res);

  // Read header
  device_fw::mbox_header_t header;
  res = rb.read(&header, sizeof(header));
  EXPECT_TRUE(res);
  EXPECT_EQ(header.length, data_size);

  // Read body
  std::vector<uint16_t> res_data(elem_num, 0);
  res = rb.read(res_data.data(), data_size);
  EXPECT_TRUE(res);

  // Update the state of the ring buffer back in the simulator
  res = rb.writeRingBufferTailIndex();
  EXPECT_TRUE(res);

  EXPECT_THAT(res_data, ::testing::ElementsAreArray(data));
  rpc_.shutdown();
}

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  et_runtime::ParseCommandLineOptions(argc, argv);
  return RUN_ALL_TESTS();
}
