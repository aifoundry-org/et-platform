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

#include "Core/DeviceFwTypes.h"
#include "TargetRPC.h"
#include "Tracing/Tracing.h"
#include "esperanto/runtime/Support/Logging.h"

#include <cassert>
#include <unistd.h>

namespace et_runtime {
namespace device {

RingBuffer::RingBuffer(size_t offset_from_mb, RPCTarget &target)
    : offset_from_mb_(offset_from_mb),
      head_index_(0),
      tail_index_(0),
      rpcDev_(target) {}

uint32_t RingBuffer::free() {
  return (head_index_ >= tail_index_)
             ? (RINGBUFFER_LENGTH - 1U) - (head_index_ - tail_index_)
             : tail_index_ - head_index_ - 1U;
}

uint32_t RingBuffer::used() {
  return (head_index_ >= tail_index_)
             ? head_index_ - tail_index_
             : (RINGBUFFER_LENGTH + head_index_ - tail_index_);
}

int64_t RingBuffer::write(const void *const buffer, uint32_t length) {
  // NOTE: EXPECT THAT THE MAILBOX HAS UPDATED THE STATE FROM
  // THE SIMULATOR AND WILL WRITE IT BACK.
  const uint8_t *const data_ptr = reinterpret_cast<const uint8_t *const>(buffer);
  bool res;
  size_t written = 0;

  if (length > free()) {
    return RINGBUFFER_ERROR_BAD_LENGTH;
  }

  if (head_index_ + length > RINGBUFFER_LENGTH) {
    size_t count = RINGBUFFER_LENGTH - head_index_;
    res = rpcDev_.rpcMailboxWrite(MailboxTarget::MAILBOX_TARGET_MM,
                                  offset_from_mb_ + rb_queue_offset_ + head_index_,
                                  count, &data_ptr[0]);
    if (!res) {
      return 0;
    }

    head_index_ = (head_index_ + count) % RINGBUFFER_LENGTH;
    written += count;
    length -= count;
  }

  res = rpcDev_.rpcMailboxWrite(MailboxTarget::MAILBOX_TARGET_MM,
                                offset_from_mb_ + rb_queue_offset_ + head_index_,
                                length, &data_ptr[written]);
  if (!res) {
    return written;
  }

  head_index_ = (head_index_ + length) % RINGBUFFER_LENGTH;
  written += length;

  return written;
}

int64_t RingBuffer::read(void *const buffer, uint32_t length) {
  // NOTE: EXPECT THAT THE MAILBOX HAS UPDATED THE STATE FROM
  // THE SIMULATOR AND WILL WRITE IT BACK.
  // Update the state form the remote target
  uint8_t *const data_ptr = reinterpret_cast<uint8_t *const>(buffer);
  bool res;
  size_t read = 0;

  if (length > used()) {
    return RINGBUFFER_ERROR_BAD_LENGTH;
  }

  if (data_ptr == nullptr) {
    // Throw away length bytes
    tail_index_ = (tail_index_ + length) % RINGBUFFER_LENGTH;
    return RINGBUFFER_ERROR_DATA_DROPPED;
  }

  if (tail_index_ + length > RINGBUFFER_LENGTH) {
    size_t count = RINGBUFFER_LENGTH - tail_index_;
    res = rpcDev_.rpcMailboxRead(MailboxTarget::MAILBOX_TARGET_MM,
                                 offset_from_mb_ + rb_queue_offset_ + tail_index_,
                                 count, &data_ptr[0]);
    if (!res) {
      return 0;
    }

    tail_index_ = (tail_index_ + count) % RINGBUFFER_LENGTH;
    read += count;
    length -= count;
  }

  res = rpcDev_.rpcMailboxRead(MailboxTarget::MAILBOX_TARGET_MM,
                               offset_from_mb_ + rb_queue_offset_ + tail_index_,
                               length, &data_ptr[read]);
  if (!res) {
    return read;
  }

  tail_index_ = (tail_index_ + length) % RINGBUFFER_LENGTH;
  read += length;

  return read;
}

bool RingBuffer::init() {
  head_index_ = tail_index_ = 0;
  return true;
}

bool RingBuffer::empty() {
  return (head_index_ == tail_index_);
}

bool RingBuffer::full() {
  return (((head_index_ + 1U) % RINGBUFFER_LENGTH) == tail_index_);
}

bool RingBuffer::readRingBufferIndices() {
  bool res;

  res = rpcDev_.rpcMailboxRead(MailboxTarget::MAILBOX_TARGET_MM,
                               offset_from_mb_ + rb_head_index_off_,
                               sizeof(head_index_), &head_index_);
  if (!res) {
    return false;
  }

  res = rpcDev_.rpcMailboxRead(MailboxTarget::MAILBOX_TARGET_MM,
                               offset_from_mb_ + rb_tail_index_off_,
                               sizeof(tail_index_), &tail_index_);
  if (!res) {
    return false;
  }

  return true;
}

bool RingBuffer::writeRingBufferHeadIndex() {
  return rpcDev_.rpcMailboxWrite(MailboxTarget::MAILBOX_TARGET_MM,
                                 offset_from_mb_ + rb_head_index_off_,
                                 sizeof(head_index_), &head_index_);
}

bool RingBuffer::writeRingBufferTailIndex() {
  return rpcDev_.rpcMailboxWrite(MailboxTarget::MAILBOX_TARGET_MM,
                                 offset_from_mb_ + rb_tail_index_off_,
                                 sizeof(tail_index_), &tail_index_);
}

EmuMailBoxDev::EmuMailBoxDev(RPCTarget &dev)
    : tx_ring_buffer_(tx_ring_buffer_off_, dev),
      rx_ring_buffer_(rx_ring_buffer_off_, dev),
      slave_status_(device_fw::MBOX_STATUS_NOT_READY),
      rpcDev_(dev)
{}

bool EmuMailBoxDev::readRemoteStatus() {
  const size_t mb_master_status_off = offsetof(device_fw::mbox_t, master_status);
  const size_t mb_slave_status_off = offsetof(device_fw::mbox_t, slave_status);
  bool res;

  res = rpcDev_.rpcMailboxRead(MailboxTarget::MAILBOX_TARGET_MM, mb_master_status_off,
                               sizeof(master_status_), &master_status_);
  if (!res) {
    return false;
  }

  res = rpcDev_.rpcMailboxRead(MailboxTarget::MAILBOX_TARGET_MM, mb_slave_status_off,
                               sizeof(slave_status_), &slave_status_);
  if (!res) {
    return false;
  }

  return true;
}
bool EmuMailBoxDev::writeRemoteStatus() {
  const size_t mb_slave_status_off = offsetof(device_fw::mbox_t, slave_status);
  // The host is always the mbox slave, so only write slave status
  return rpcDev_.rpcMailboxWrite(MailboxTarget::MAILBOX_TARGET_MM, mb_slave_status_off,
                                    sizeof(slave_status_), &slave_status_);
}

bool EmuMailBoxDev::mboxDestroy() {
  slave_status_ = device_fw::MBOX_STATUS_NOT_READY;
  return writeRemoteStatus();
}

bool EmuMailBoxDev::mboxReady() {
  // first update status the status from the remote
  auto res = readRemoteStatus();
  if (!res) {
    return false;
  }
  // The host is always the mbox slave
  switch (master_status_) {
  case device_fw::MBOX_STATUS_NOT_READY:
    break;
  case device_fw::MBOX_STATUS_READY:
    if (slave_status_ != device_fw::MBOX_STATUS_READY &&
        slave_status_ != device_fw::MBOX_STATUS_WAITING) {
      TRACE_RPCDevice_mbox_master_ready_slave_ready();
      slave_status_ = device_fw::MBOX_STATUS_READY;
    }
    break;
  case device_fw::MBOX_STATUS_WAITING:
    if (slave_status_ != device_fw::MBOX_STATUS_READY &&
        slave_status_ != device_fw::MBOX_STATUS_WAITING) {
      TRACE_RPCDevice_mbox_master_waiting_slave_ready();
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
  TRACE_RPCDevice_mbox_reset();

  if (!rpcDev_.alive()) {
    return false;
  }
  // first update status the status from the remote
  auto res = readRemoteStatus();
  assert(res);

  RTDEBUG << "Resetting slave, transitioning to WAITING\n";
  slave_status_ = device_fw::MBOX_STATUS_WAITING;

  // Write back the status to the remote simulator
  res = writeRemoteStatus();
  assert(res);

  // Clean up the ring buffer state, consume all messages similar to reset
  tx_ring_buffer_.init();
  res = tx_ring_buffer_.writeRingBufferTailIndex();
  assert(res);
  rx_ring_buffer_.init();
  res = rx_ring_buffer_.writeRingBufferHeadIndex();
  assert(res);

  return true;
}

bool EmuMailBoxDev::write(const void *data, ssize_t size) {

  TRACE_RPCDevice_write(size);

  if (!rpcDev_.alive()) {
    return false;
  }

  const device_fw::mbox_header_t header = {.length = (uint16_t)size,
                                           .magic = MBOX_MAGIC};

  if (!mboxReady()) {
    RTINFO << "Mailbox: Not ready\n";
    return false;
  }

  if (size < 1 || size > std::numeric_limits<uint16_t>::max()) {
    return false;
  }

  // Write data to the mailbox RX ringbuffer (from master perspective)
  RingBuffer &rb = rx_ring_buffer_;

  // Pull the latest state from the simulator
  auto res = rb.readRingBufferIndices();
  assert(res);

  if (rb.free() < sizeof(header) + size) {
    RTERROR << "Mailbox: No room for message (" << rb.free() << " free, "
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
  res = rb.writeRingBufferHeadIndex();
  assert(res);

  // Raise interrupt to M&M to consume the new mailbox message
  res = rpcDev_.rpcRaiseDevicePuPlicPcieMessageInterrupt();
  assert(res);
  return true;
}

ssize_t EmuMailBoxDev::read(void *data, ssize_t size, TimeDuration wait_time) {
  TRACE_RPCDevice_read(size);

  if (!rpcDev_.alive()) {
    return 0;
  }

  // Read the mailbox from the simulator currently is not blocking like in the PCIe driver
  auto received = rpcDev_.rpcWaitForHostInterrupt(wait_time);
  if (!received) {
    return 0;
  }

  // Read data from the mailbox TX ringbuffer (from master perspective)
  RingBuffer &rb = tx_ring_buffer_;

  if (!mboxReady()) {
    RTERROR << "Mailbox: Not ready\n";
    return 0;
  }

  // Pull the latest state from the simulator
  auto res = rb.readRingBufferIndices();
  if (!res) {
    return 0;
  }

  device_fw::mbox_header_t header = {};

  // Wait for there to be a message in the mailbox before reading.
  if (rb.used() < sizeof(header)) {
    RTDEBUG << "Mailbox: No new message available\n";
    return 0;
  }

  res = rb.read(&header, sizeof(header));
  assert(res);

  // Check if the message is valid
  if (header.length < 1 || header.length > MBOX_MAX_LENGTH ||
      header.magic != MBOX_MAGIC) {
    RTERROR << "Mailbox: Invalid header\n";
    std::terminate();
    return false;
  }

  // Check if the buffer is big enough to store the message body
  if (size < header.length) {
    RTERROR << "Mailbox: Insufficient buffer to store mailbox message\n";
    std::terminate();
    return false;
  }

  // Read message body
  res = rb.read(data, header.length);
  assert(res);

  // Write back to the simulator that we consumed the message
  rb.writeRingBufferTailIndex();

  RTDEBUG << "Mailbox READ, read size: " << header.length << "\n";
  return header.length;
}

ssize_t EmuMailBoxDev::mboxMaxMsgSize() const { return MBOX_MAX_LENGTH; }

bool EmuMailBoxDev::ready(TimeDuration wait_time) {
  auto start = Clock::now();
  auto end = start + wait_time;
  static const TimeDuration polling_interval = std::chrono::milliseconds(100);

  auto ready = mboxReady();
  while (!ready) {
    ready = mboxReady();
    if (ready) {
      return ready;
    }
    if (end < Clock::now()) {
      RTERROR << "Mailbox not ready in time\n";
      return false;
    }

    // Clean up the ring buffer state, consume all messages similar to reset
    tx_ring_buffer_.init();
    auto res = tx_ring_buffer_.writeRingBufferTailIndex();
    if(!res) {
      RTERROR << "tx_ring_buffer write to tail index failed\n";
      return false;
    }
    rx_ring_buffer_.init();
    res = rx_ring_buffer_.writeRingBufferHeadIndex();
    if(!res) {
      RTERROR << "rx_ring_buffer write to head index failed\n";
      return false;
    }

    rpcDev_.rpcRaiseDevicePuPlicPcieMessageInterrupt();
    rpcDev_.rpcWaitForHostInterrupt(polling_interval);
  }
  return ready;
}

bool EmuMailBoxDev::reset(TimeDuration wait_time) {
  auto start = Clock::now();
  auto end = start + wait_time;
  static const TimeDuration polling_interval = std::chrono::milliseconds(100);

  auto reset = mboxReset();
  while (!reset) {
    reset = mboxReset();
    if (reset) {
      return reset;
    }
    if (end < Clock::now()) {
      RTERROR << "Mailbox not reset in time\n";
      return false;
    }
    rpcDev_.rpcRaiseDevicePuPlicPcieMessageInterrupt();
    rpcDev_.rpcWaitForHostInterrupt(polling_interval);
  }
  return reset;
}

} // namespace device
} // namespace et_runtime
