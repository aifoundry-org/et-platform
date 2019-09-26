//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "EmuMailBoxDev.h"

#include "DeviceFwTypes.h"
#include "Support/Logging.h"
#include "TargetRPC.h"

#include <cassert>
#include <unistd.h>

namespace et_runtime {
namespace device {

#if ENABLE_DEVICE_FW
RingBuffer::RingBuffer(RingBufferType type, RPCTarget &target)
    : type_(type),   //
      ringbuffer_(), //
      rpcDev_(target) {}

int64_t RingBuffer::write(const void *const buffer, uint32_t length) {
  // NOTE: EXPECT THAT THE MAILBOX HAS UPDATED THE STATE FROM
  // THE SIMULATOR AND WILL WRITE IT BACK.
  uint32_t head_index = ringbuffer_.head_index;
  const uint8_t *const data_ptr =
      reinterpret_cast<const uint8_t *const>(buffer);

  int64_t ret_val = 0;
  if (length < free()) {
    for (uint16_t i = 0; i < length; i++) {
      ringbuffer_.queue[head_index] = data_ptr[i];
      head_index = (head_index + 1U) % RINGBUFFER_LENGTH;
    }

    ringbuffer_.head_index = head_index;

    ret_val = (int64_t)length;
  } else {
    ret_val = RINGBUFFER_ERROR_BAD_LENGTH;
  }
  return ret_val;
}

int64_t RingBuffer::read(void *const buffer, uint32_t length) {
  // NOTE: EXPECT THAT THE MAILBOX HAS UPDATED THE STATE FROM
  // THE SIMULATOR AND WILL WRITE IT BACK.
  // Update the state form the remote target
  uint32_t tail_index = ringbuffer_.tail_index;
  uint8_t *const data_ptr = reinterpret_cast<uint8_t *const>(buffer);

  if (length > used()) {
    return RINGBUFFER_ERROR_BAD_LENGTH;
  }
  int64_t ret_value = 0;
  if (data_ptr == NULL) {
    // Throw away length bytes
    ringbuffer_.tail_index = (tail_index + length) % RINGBUFFER_LENGTH;
    ret_value = RINGBUFFER_ERROR_DATA_DROPPED;
  }
  // Read length bytes to data_ptr
  for (uint16_t i = 0; i < length; i++) {
    data_ptr[i] = ringbuffer_.queue[tail_index];
    tail_index = (tail_index + 1U) % RINGBUFFER_LENGTH;
  }

  ringbuffer_.tail_index = tail_index;
  ret_value = length;
  return ret_value;
}
uint64_t RingBuffer::free() {
  const uint32_t head_index = ringbuffer_.head_index;
  const uint32_t tail_index = ringbuffer_.tail_index;

  return (head_index >= tail_index)
             ? (RINGBUFFER_LENGTH - 1U) - (head_index - tail_index)
             : tail_index - head_index - 1U;
}
uint64_t RingBuffer::used() {
  const uint32_t head_index = ringbuffer_.head_index;
  const uint32_t tail_index = ringbuffer_.tail_index;

  return (head_index >= tail_index)
             ? head_index - tail_index
             : (RINGBUFFER_LENGTH + head_index - tail_index);
}

bool RingBuffer::init() {
  ringbuffer_.head_index = ringbuffer_.tail_index = 0;
  return true;
}

bool RingBuffer::empty() {
  return (ringbuffer_.head_index == ringbuffer_.tail_index);
}

bool RingBuffer::full() {
  return (((ringbuffer_.head_index + 1U) % RINGBUFFER_LENGTH) ==
          ringbuffer_.tail_index);
}

bool RingBuffer::readRingBufferState() {
  if (type_ == RingBufferType::RX) {
    auto [res, rb] = rpcDev_.readRxRb();
    if (!res) {
      return false;
    }
    ringbuffer_ = rb;
  } else {
    assert(type_ == RingBufferType::TX);
    auto [res, rb] = rpcDev_.readTxRb();
    if (!res) {
      return false;
    }
    ringbuffer_ = rb;
  }

  return true;
}

bool RingBuffer::writeRingBufferState() {
  bool res = false;
  if (type_ == RingBufferType::RX) {
    res = rpcDev_.writeRxRb(ringbuffer_);
  } else {
    assert(type_ == RingBufferType::TX);
    res = rpcDev_.writeTxRb(ringbuffer_);
  }
  return res;
}

#endif // ENABLE_DEVICE_FW

#if ENABLE_DEVICE_FW

EmuMailBoxDev::EmuMailBoxDev(RPCTarget &dev)
    : tx_ring_buffer_(RingBufferType::TX, dev),        //
      rx_ring_buffer_(RingBufferType::RX, dev),        //
      slave_status_(device_fw::MBOX_STATUS_NOT_READY), //
      rpcDev_(dev)                                     //
{}

bool EmuMailBoxDev::readRemoteStatus() {
  auto [res, status] = rpcDev_.readMBStatus();
  if (!res) {
    return false;
  }
  master_status_ = std::get<0>(status);
  slave_status_ = std::get<1>(status);
  return true;
}
bool EmuMailBoxDev::writeRemoteStatus() {
  return rpcDev_.writeMBStatus(master_status_, slave_status_);
}

bool EmuMailBoxDev::mboxDestroy() {
  slave_status_ = device_fw::MBOX_STATUS_NOT_READY;
  return writeRemoteStatus();
}

bool EmuMailBoxDev::mboxReady() {
  // first update status the status from the remote
  auto res = readRemoteStatus();
  assert(res);
  // The host is always the mbox slave
  switch (master_status_) {
  case device_fw::MBOX_STATUS_NOT_READY:
    break;
  case device_fw::MBOX_STATUS_READY:
    if (slave_status_ != device_fw::MBOX_STATUS_READY &&
        slave_status_ != device_fw::MBOX_STATUS_WAITING) {
      RTDEBUG << "Received master ready, going slave ready \n";
      slave_status_ = device_fw::MBOX_STATUS_READY;
    }
    break;
  case device_fw::MBOX_STATUS_WAITING:
    if (slave_status_ != device_fw::MBOX_STATUS_READY &&
        slave_status_ != device_fw::MBOX_STATUS_WAITING) {
      RTDEBUG << "Received master waiting, going slave ready \n";
      slave_status_ = device_fw::MBOX_STATUS_READY;
    }
    break;
  case device_fw::MBOX_STATUS_ERROR:
    break;
  }

  // Write back the status to the remote simulator
  res = writeRemoteStatus();
  assert(res);

  return master_status_ == device_fw::MBOX_STATUS_READY &&
         slave_status_ == device_fw::MBOX_STATUS_READY;
}

bool EmuMailBoxDev::mboxReset() {
  RTDEBUG << "Reset Mailbox \n";
  // first update status the status from the remote
  auto res = readRemoteStatus();
  assert(res);

  RTDEBUG << "Reseting slave, transitioning to WAITING \n";
  slave_status_ = device_fw::MBOX_STATUS_WAITING;

  // Write back the status to the remote simulator
  res = writeRemoteStatus();
  assert(res);

  // Clean up the ring buffer state, consume all messages similar to reset
  tx_ring_buffer_.init();
  res = tx_ring_buffer_.writeRingBufferState();
  assert(res);
  rx_ring_buffer_.init();
  res = rx_ring_buffer_.writeRingBufferState();
  assert(res);

  return true;
}

bool EmuMailBoxDev::write(const void *data, ssize_t size) {
  RTDEBUG << "Mailbox Write, size: " << size << "\n";

  const device_fw::mbox_header_t header = {.length = (uint16_t)size,
                                           .magic = MBOX_MAGIC};

  if (!mboxReady()) {
    RTINFO << "Mbox not ready\n";
    return false;
  }

  if (size < 1 || size > std::numeric_limits<uint16_t>::max()) {
    return false;
  }

  RingBuffer &rb = rx_ring_buffer_;

  // pull the latest state from the simulator
  auto res = rb.readRingBufferState();
  assert(res);

  if (rb.free() < sizeof(header) + size) {
    RTERROR << "No room for message (" << rb.free() << " free, "
            << sizeof(header) + size << " needed)\n";
    return false;
  }

  // Write header
  res = rb.write(&header, sizeof(header));
  assert(res);

  // Write body
  res = rb.write(data, size);
  assert(res);

  // Update the state of the ring buffer back in the simulator
  res = rb.writeRingBufferState();
  assert(res);

  // Raise interrupt to M&M to consume the new mailbox message
  res = raiseDevicePuPlicPcieMessageInterrupt();
  assert(res);
  return true;
}

ssize_t EmuMailBoxDev::read(void *data, ssize_t size, TimeDuration wait_time) {
  RTDEBUG << "Mailbox READ, req buf size: " << size << " \n";
  // read the mailbox from the simulator currently is not blocking like in the
  // pcie driver
  auto received = waitForHostInterrupt(wait_time);
  if (!received) {
    return 0;
  }

  RingBuffer &rb = tx_ring_buffer_;

  if (!mboxReady()) {
    RTERROR << "MBox not ready\n";
    return 0;
  }

  // pull the latest state from the simulator
  auto res = rb.readRingBufferState();
  assert(res);

  device_fw::mbox_header_t header = {};

  // Wait for there to be a message in the mailbox before reading.
  if (rb.used() < sizeof(header)) {
    RTDEBUG << "MB: No new message available \n";
    return 0;
  }

  res = rb.read(&header, sizeof(header));
  assert(res);

  // Check if the message is valid
  if (header.length < 1 || header.length > MBOX_MAX_LENGTH ||
      header.magic != MBOX_MAGIC) {
    RTERROR << "Invalid header \n";
    std::terminate();
    return false;
  }

  // Check if the buffer is big enough to store the message body
  if (size < header.length) {
    RTERROR << "Insufficient buffer to store mailbox message \n";
    std::terminate();
    return false;
  }

  // Read message body
  res = rb.read(data, header.length);
  assert(res);

  // write back to the simulator that we consumed the message
  rb.writeRingBufferState();
  RTDEBUG << "Mailbox READ, read size: " << header.length << " \n";
  return header.length;
}

ssize_t EmuMailBoxDev::mboxMaxMsgSize() const { return MBOX_MAX_LENGTH; }

#else

EmuMailBoxDev::EmuMailBoxDev(RPCTarget &dev)
    : tx_ring_buffer_(RingBufferType::TX, dev), //
      rx_ring_buffer_(RingBufferType::RX, dev), //
      slave_status_(0),                         //
      rpcDev_(dev)                              //
{}

bool EmuMailBoxDev::readRemoteStatus() {
  std::terminate();
  return false;
}
bool EmuMailBoxDev::writeRemoteStatus() {
  std::terminate();
  return false;
}

bool EmuMailBoxDev::mboxDestroy() {
  std::terminate();
  return false;
}

bool EmuMailBoxDev::mboxReady() {
  std::terminate();
  return false;
}

bool EmuMailBoxDev::mboxReset() {
  std::terminate();
  return false;
}

bool EmuMailBoxDev::write(const void *data, ssize_t size) {
  std::terminate();
  return false;
}

ssize_t EmuMailBoxDev::read(void *data, ssize_t size, TimeDuration wait_time) {
  std::terminate();
  return 0;
}

ssize_t EmuMailBoxDev::mboxMaxMsgSize() const { return 0; }

#endif // ENABLE_DEVICE_FW

bool EmuMailBoxDev::waitForHostInterrupt(TimeDuration wait_time) {
  return rpcDev_.waitForHostInterrupt(wait_time);
}

bool EmuMailBoxDev::raiseDevicePuPlicPcieMessageInterrupt() {
  return rpcDev_.raiseDevicePuPlicPcieMessageInterrupt();
}

bool EmuMailBoxDev::ready(TimeDuration wait_time) {
  auto start = Clock::now();
  auto end = start + wait_time;
  static const TimeDuration polling_interval = std::chrono::seconds(1);

  auto ready = mboxReady();
  while (!ready) {
    ready = mboxReady();
    if (ready) {
      return ready;
    }
    if (end < Clock::now()) {
      RTERROR << "Mailbox not ready in time \n";
      return false;
    }
#if ENABLE_DEVICE_FW
    // Clean up the ring buffer state, consume all messages similar to reset
    tx_ring_buffer_.init();
    auto res = tx_ring_buffer_.writeRingBufferState();
    assert(res);
    rx_ring_buffer_.init();
    res = rx_ring_buffer_.writeRingBufferState();
    assert(res);

    rpcDev_.raiseDevicePuPlicPcieMessageInterrupt();
    rpcDev_.waitForHostInterrupt(polling_interval);
#endif // ENABLE_DEVICE_FW
  }
  return ready;
}

bool EmuMailBoxDev::reset(TimeDuration wait_time) {
  auto start = Clock::now();
  auto end = start + wait_time;
  static const TimeDuration polling_interval = std::chrono::seconds(1);

  auto reset = mboxReset();
  while (!reset) {
    reset = mboxReset();
    if (reset) {
      return reset;
    }
    if (end < Clock::now()) {
      RTERROR << "Mailbox not reset in time \n";
      return false;
    }
    rpcDev_.raiseDevicePuPlicPcieMessageInterrupt();
    rpcDev_.waitForHostInterrupt(polling_interval);
  }
  return reset;
}

} // namespace device
} // namespace et_runtime
