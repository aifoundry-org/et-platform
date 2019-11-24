//******************************************************************************
// Copyright (C) 2018,2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "device-fw-fixture.h"

#include "esperanto/runtime/Support/Logging.h"

// Wait for device-fw to raise an interrupt from device to host
TEST_F(DeviceFWTest, waitForHostInterrupt) {
  // Do nothing the test fixture should do the above
  auto *target_device_ptr = &dev_->getTargetDevice();
  auto *target_device = dynamic_cast<device::RPCTarget *>(target_device_ptr);
  ASSERT_TRUE(target_device != nullptr);

  auto res = target_device->init();
  ASSERT_TRUE(res);

  res = memManager()->init();
  ASSERT_TRUE(res);

  auto success = dev_->loadFirmwareOnDevice();
  assert(success == etrtSuccess);

  target_device->boot(0x8000001000);
  res = target_device->waitForHostInterrupt(std::chrono::seconds(30));
  ASSERT_TRUE(res);
}

TEST_F(DeviceFWTest, waitForMailboxReady) {
  auto *target_device_ptr = &dev_->getTargetDevice();
  auto *target_device = dynamic_cast<device::RPCTarget *>(target_device_ptr);
  EXPECT_TRUE(target_device != nullptr);

  auto res = target_device->init();
  ASSERT_TRUE(res);

  res = memManager()->init();
  ASSERT_TRUE(res);

  auto success = dev_->loadFirmwareOnDevice();
  assert(success == etrtSuccess);

  target_device->boot(0x8000001000);
  res = target_device->waitForHostInterrupt(std::chrono::seconds(30));

  auto &mb_emu = target_device->mailboxDev();
  auto ready = mb_emu.ready(std::chrono::seconds(20));
  EXPECT_TRUE(ready);
}

TEST_F(DeviceFWTest, resetMailBox) {

  auto *target_device_ptr = &dev_->getTargetDevice();
  auto *target_device = dynamic_cast<device::RPCTarget *>(target_device_ptr);
  EXPECT_TRUE(target_device != nullptr);
  auto res = target_device->init();
  ASSERT_TRUE(res);

  res = memManager()->init();
  ASSERT_TRUE(res);

  auto fw_load = dev_->loadFirmwareOnDevice();
  assert(fw_load == etrtSuccess);

  target_device->boot(0x8000001000);
  res = target_device->waitForHostInterrupt(std::chrono::seconds(30));

  auto &mb_emu = target_device->mailboxDev();
  auto success = mb_emu.ready(std::chrono::seconds(20));
  EXPECT_TRUE(success);

  success = mb_emu.reset();
  EXPECT_TRUE(success);

  success = mb_emu.ready(std::chrono::seconds(20));
  EXPECT_TRUE(success);
}

TEST_F(DeviceFWTest, reflectTest) {
  auto *target_device_ptr = &dev_->getTargetDevice();
  auto *target_device = dynamic_cast<device::RPCTarget *>(target_device_ptr);
  EXPECT_TRUE(target_device != nullptr);

  ASSERT_EQ(dev_->init(), etrtSuccess);

  // Construct the reflect message and write it
  device_fw::host_message_t msg = {0};
  int active_shires = 1;
  msg.message_id = device_fw::MBOX_MESSAGE_ID_REFLECT_TEST;

  auto success = target_device->mb_write(&msg, sizeof(msg));
  EXPECT_TRUE(success);

  std::vector<uint8_t> message(target_device->mboxMsgMaxSize(), 0);
  auto size = target_device->mb_read(message.data(), message.size(),
                                     std::chrono::seconds(20));
  EXPECT_EQ(size, sizeof(device_fw::host_message_t));
  auto response = reinterpret_cast<device_fw::host_message_t *>(message.data());
  RTDEBUG << "MessageID: " << response->message_id << "\n";
}

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  FLAGS_minloglevel = 0;
  FLAGS_logtostderr = 1;
  testing::InitGoogleTest(&argc, argv);
  et_runtime::ParseCommandLineOptions(argc, argv);
  return RUN_ALL_TESTS();
}
