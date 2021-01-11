//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "EmuVirtQueueDev.h"

#include "Core/DeviceFwTypes.h"
#include "RPCTarget_MM.h"
#include "Tracing/Tracing.h"
#include "esperanto/runtime/Support/Logging.h"

#include <cassert>
#include <unistd.h>
#include <thread>

namespace et_runtime {
namespace device {

namespace {
  TimeDuration kPollingInterval = std::chrono::milliseconds(10);
}

CircQueue::CircQueue(const std::shared_ptr<RPCGenerator> &rpcGen, uint64_t queueBaseAddr,
                     uint64_t queueHeaderAddr, uint16_t queueBufCount, uint16_t queueBufSize)
    : rpcGen_(rpcGen),
      queueBaseAddr_(queueBaseAddr),
      queueHeadAddr_(queueHeaderAddr + offsetof(device_fw::circ_buf_header, head)),
      queueTailAddr_(queueHeaderAddr + offsetof(device_fw::circ_buf_header, tail)),
      queueBufCount_(queueBufCount),
      queueBufSize_(queueBufSize),
      head_(0),
      tail_(0),
      headBufPos_(0),
      tailBufPos_(0) {}

std::unique_ptr<CircQueue> CircQueue::queueFactory(const std::shared_ptr<RPCGenerator> &rpcGen,
                                                   uint64_t queueBaseAddr,
                                                   uint64_t queueHeaderAddr,
                                                   uint16_t queueBufCount,
                                                   uint16_t queueBufSize) {
  if (queueBufCount & (queueBufCount - 1)) {
    RTERROR << "queueBufCount must be in 2's power";
    assert(false);
    return nullptr;
  }

  if (queueBufSize & (queueBufSize - 1)) {
    RTERROR << "queueBufSize must be in 2's power";
    assert(false);
    return nullptr;
  }

  return std::make_unique<CircQueue>(rpcGen, queueBaseAddr, queueHeaderAddr, queueBufCount,
                                     queueBufSize);
}

uint16_t CircQueue::queueUsedBufCount() {
  return ((head_ - tail_) & (queueBufCount_ - 1));
}

uint16_t CircQueue::queueFreeBufCount() {
  return ((tail_ - head_ - 1) & (queueBufCount_ - 1));
}

int64_t CircQueue::write(const void *const buffer, uint32_t length, bool setHeadBufWritten) {
  // NOTE: EXPECT THAT THE VIRTQUEUE HAS UPDATED THE STATE FROM
  // THE SIMULATOR AND WILL WRITE IT BACK.
  const uint8_t *const data_ptr = reinterpret_cast<const uint8_t *const>(buffer);
  bool res;

  if (!queueFreeBufCount()) {
    return CIRCBUFFER_ERROR_FULL;
  }

  if (length > (uint16_t)(queueBufSize_ - headBufPos_)) {
    return CIRCBUFFER_ERROR_BAD_LENGTH;
  }

  res = rpcGen_->rpcVirtQueueWrite(queueBaseAddr_ + head_ * queueBufSize_ + headBufPos_,
                                   length, data_ptr);

  if (!res) {
    return 0;
  }

  if (setHeadBufWritten) {
    head_ = (head_ + 1) % queueBufCount_;
    headBufPos_ = 0;
  } else {
    headBufPos_ += length;
  }

  return length;
}

int64_t CircQueue::read(void *const buffer, uint32_t length, bool setTailBufRead) {
  // NOTE: EXPECT THAT THE VIRTQUEUE HAS UPDATED THE STATE FROM
  // THE SIMULATOR AND WILL WRITE IT BACK.
  // Update the state form the remote target
  uint8_t *const data_ptr = reinterpret_cast<uint8_t *const>(buffer);
  bool res;

  if (!queueUsedBufCount()) {
    return CIRCBUFFER_ERROR_EMPTY;
  }

  if (length > (uint16_t)(queueBufSize_ - tailBufPos_)) {
    return RINGBUFFER_ERROR_BAD_LENGTH;
  }

  if (data_ptr == nullptr) {
    if (setTailBufRead) {
      // Throw away this tail buffer
      tail_ = (tail_ + 1) % queueBufCount_;
    } else {
      // Throw away length bytes from tail buffer
      tailBufPos_ += length;
    }
    return CIRCBUFFER_ERROR_DATA_DROPPED;
  }

  res = rpcGen_->rpcVirtQueueRead(queueBaseAddr_ + tail_ * queueBufSize_ + tailBufPos_,
                                  length, data_ptr);
  if (!res) {
    return 0;
  }

  if (setTailBufRead) {
    tail_ = (tail_ + 1) % queueBufCount_;
    tailBufPos_ = 0;
  } else {
    tailBufPos_ += length;
  }

  return length;
}

bool CircQueue::init() {
  head_ = tail_ = 0;
  return true;
}

bool CircQueue::empty() {
  return (head_ == tail_);
}

bool CircQueue::full() {
  return (((head_ + 1U) % queueBufCount_) == tail_);
}

bool CircQueue::readCircQueueIndices() {
  bool res;

  res = rpcGen_->rpcVirtQueueRead(queueHeadAddr_, sizeof(head_), &head_);
  if (!res) {
    return false;
  }

  res = rpcGen_->rpcVirtQueueRead(queueTailAddr_, sizeof(tail_), &tail_);
  if (!res) {
    return false;
  }

  return true;
}

bool CircQueue::writeCircQueueHeadIndex() {
  return rpcGen_->rpcVirtQueueWrite(queueHeadAddr_, sizeof(head_), &head_);
}

bool CircQueue::writeCircQueueTailIndex() {
  return rpcGen_->rpcVirtQueueWrite(queueTailAddr_, sizeof(tail_), &tail_);
}

EmuVirtQueueDev::EmuVirtQueueDev(const std::shared_ptr<RPCGenerator> &rpcGen, uint8_t queueId,
                                 uint64_t virtQueueAddr, uint64_t virtQueueInfoAddr,
                                 uint16_t queueBufCount, uint16_t queueBufSize, bool isSP)
    : rpcGen_(rpcGen),
      queueId_(queueId),
      masterStatusAddr_(virtQueueInfoAddr + offsetof(device_fw::vqueue_info, master_status)),
      slaveStatusAddr_(virtQueueInfoAddr + offsetof(device_fw::vqueue_info, slave_status)),
      submissionQueue_(CircQueue::queueFactory(rpcGen,
                                               virtQueueAddr,
                                               virtQueueInfoAddr + offsetof(device_fw::vqueue_info, sq_header),
                                               queueBufCount,
                                               queueBufSize)),
      completionQueue_(CircQueue::queueFactory(rpcGen,
                                               virtQueueAddr + queueBufCount * queueBufSize,
                                               virtQueueInfoAddr + offsetof(device_fw::vqueue_info, cq_header),
                                               queueBufCount,
                                               queueBufSize)),
      queueBufSize_(queueBufSize),
      isSP_(isSP),
      isReady_(false) {}

bool EmuVirtQueueDev::readRemoteStatus(uint8_t &master_status, uint8_t &slave_status) {
  bool res;

  res = rpcGen_->rpcVirtQueueRead(masterStatusAddr_, sizeof(master_status), &master_status);
  if (!res) {
    return false;
  }

  res = rpcGen_->rpcVirtQueueRead(slaveStatusAddr_, sizeof(slave_status), &slave_status);
  if (!res) {
    return false;
  }

  return true;
}
bool EmuVirtQueueDev::writeRemoteSlaveStatus(uint8_t slave_status) {
  // The host is always the vqueue slave, so only write slave status
  return rpcGen_->rpcVirtQueueWrite(slaveStatusAddr_, sizeof(slave_status), &slave_status);
}

bool EmuVirtQueueDev::raiseTargetDeviceInterrupt() {
  if (isSP_) {
    return rpcGen_->rpcRaiseDeviceSpioPlicPcieMessageInterrupt();
  } else {
    return rpcGen_->rpcRaiseDevicePuPlicPcieMessageInterrupt();
  }
}

bool EmuVirtQueueDev::virtQueueDestroy() {
  isReady_ = false;
  return writeRemoteSlaveStatus(device_fw::VQ_STATUS_NOT_READY);
}

bool EmuVirtQueueDev::virtQueueReady() {
  return isReady_;
}

bool EmuVirtQueueDev::virtQueueStatusHandShake() {
  uint8_t master_status, slave_status;

  // first update status, the status from the remote
  auto res = readRemoteStatus(master_status, slave_status);
  if (!res) {
    return false;
  }
  // The host is always the virtqueue slave
  switch (master_status) {
  case device_fw::VQ_STATUS_NOT_READY:
    break;
  case device_fw::VQ_STATUS_READY:
    if (slave_status != device_fw::VQ_STATUS_READY &&
        slave_status != device_fw::VQ_STATUS_WAITING) {
      TRACE_RPCDevice_virtqueue_master_ready_slave_ready();
      slave_status = device_fw::VQ_STATUS_READY;
      // Write back the status to the remote simulator
      res = writeRemoteSlaveStatus(slave_status);
      assert(res);
      res = raiseTargetDeviceInterrupt();
      assert(res);
    }
    break;
  case device_fw::VQ_STATUS_WAITING:
    if (slave_status != device_fw::VQ_STATUS_READY &&
        slave_status != device_fw::VQ_STATUS_WAITING) {
      TRACE_RPCDevice_virtqueue_master_waiting_slave_ready();
      slave_status = device_fw::VQ_STATUS_READY;
      // Write back the status to the remote simulator
      res = writeRemoteSlaveStatus(slave_status);
      assert(res);
      res = raiseTargetDeviceInterrupt();
      assert(res);
    }
    break;
  case device_fw::VQ_STATUS_ERROR:
    break;
  }

  return isReady_ = master_status == device_fw::VQ_STATUS_READY &&
                      slave_status == device_fw::VQ_STATUS_READY;
}

bool EmuVirtQueueDev::virtQueueReset() {
  TRACE_RPCDevice_virtqueue_reset();

  RTDEBUG << "Resetting slave, transitioning to WAITING\n";

  isReady_ = false;

  // Write back the status to the remote simulator
  auto res = writeRemoteSlaveStatus(device_fw::VQ_STATUS_WAITING);
  assert(res);

  res = raiseTargetDeviceInterrupt();
  assert(res);

  // Clean up the circular queue state, discard all messages
  submissionQueue_->init();
  completionQueue_->init();

  return true;
}

bool EmuVirtQueueDev::checkForEventEPOLLIN() {
  // EPOLLIN indicates the availability of read event i.e. completion queue
  // is available for reads

  auto res = virtQueueReady();
  if (!res) {
    RTINFO << "Virt Queue is not ready\n";
    return false;
  }
  // Pull the latest state from the simulator
  res = completionQueue_->readCircQueueIndices();
  assert(res);

  return completionQueue_->queueUsedBufCount() > 0;
}

bool EmuVirtQueueDev::checkForEventEPOLLOUT() {
  // EPOLLOUT indicates the availability of write event i.e. submission queue
  // is available for writes

  auto res = virtQueueReady();
  if (!res) {
    RTINFO << "Virt Queue is not ready\n";
    return false;
  }
  // Pull the latest state from the simulator
  res = submissionQueue_->readCircQueueIndices();
  assert(res);

  //TODO: specify user defined buffer threshold here instead of 0
  return submissionQueue_->queueFreeBufCount() > 0;
}

bool EmuVirtQueueDev::write(const void *data, ssize_t size) {
  TRACE_RPCDevice_virtqueue_write(size);

  int64_t res;

  if (!virtQueueReady()) {
    RTINFO << "VirtQueue: Not ready\n";
    return false;
  }

  if (size < 1 || size > std::numeric_limits<uint16_t>::max()) {
    return false;
  }

  // Write data to the submission queue
  // Pull the latest state from the simulator
  res = submissionQueue_->readCircQueueIndices();
  assert(res);

  if (submissionQueue_->queueFreeBufCount() < 1) {
    RTINFO << "VirtQueue: No free buffer available!\n";
    return false;
  }

  const device_fw::vqueue_buf_header header = {.length = (uint16_t)size,
                                                  .magic = VQUEUE_MAGIC};

  // Write header and don't mark the virtqueue buffer as full
  res = submissionQueue_->write(&header, sizeof(header), false);
  assert(res == sizeof(header));

  // Write body and mark the virtqueue buffer as full
  res = submissionQueue_->write(data, size, true);
  assert(res == size);

  // Update the state of the submission queue back in the simulator
  res = submissionQueue_->writeCircQueueHeadIndex();
  assert(res);

  // Raise interrupt to the target device to consume the new virtqueue message
  res = raiseTargetDeviceInterrupt();
  assert(res);
  return true;
}

ssize_t EmuVirtQueueDev::read(void *data, ssize_t size, TimeDuration wait_time) {
  TRACE_RPCDevice_virtqueue_read(size);

  int64_t res;

  // Read data from the completion queue
  if (!virtQueueReady()) {
    RTINFO << "VirtQueue: Not ready\n";
    return 0;
  }

  // Pull the latest state from the simulator
  res = completionQueue_->readCircQueueIndices();
  if (!res) {
    return 0;
  }

  device_fw::vqueue_buf_header header = {};

  // Wait for there to be a message in the virtqueue before reading
  if (completionQueue_->queueUsedBufCount() < 1) {
    RTDEBUG << "VirtQueue: No buffer available to read\n";
    return 0;
  }

  // Read header and don't mark the virtqueue buffer to be read completely
  res = completionQueue_->read(&header, sizeof(header), false);
  assert(res == sizeof(header));

  // Check if the message is valid
  if (header.length < 1 ||
      header.length > (queueBufSize_ - sizeof(header)) ||
      header.magic != VQUEUE_MAGIC) {
    RTERROR << "VirtQueue: Invalid header\n";
    std::terminate();
    return false;
  }

  // Check if the buffer is big enough to store the message body
  if (size < header.length) {
    RTERROR << "VirtQueue: Insufficient buffer to store virtqueue buffer message\n";
    std::terminate();
    return false;
  }

  // Read message body and mark the virtqueue buffer to be read completely
  res = completionQueue_->read(data, header.length, true);
  assert(res);

  // Write back to the simulator that we consumed the message
  completionQueue_->writeCircQueueTailIndex();

  RTDEBUG << "VirtQueue READ, read size: " << header.length << "\n";
  return header.length;
}

bool EmuVirtQueueDev::ready(TimeDuration wait_time) {
  auto start = Clock::now();
  auto end = start + wait_time; // TODO: Fix wait_time (TimeDuration::max()) + value can cause issue

  auto ready = virtQueueReady();
  while (!ready) {
    ready = virtQueueStatusHandShake();
    if (ready) {
      return ready;
    }
    if (end < Clock::now()) {
      RTERROR << "VirtQueue not ready in time\n";
      return false;
    }

    // Clean up the circular queues state, consume all messages similar to reset
    completionQueue_->init();
    auto res = completionQueue_->writeCircQueueTailIndex();
    if(!res) {
      RTERROR << "Completion Queue write to tail index failed\n";
      return false;
    }
    submissionQueue_->init();
    res = submissionQueue_->writeCircQueueHeadIndex();
    if(!res) {
      RTERROR << "Submission Queue write to head index failed\n";
      return false;
    }

    rpcGen_->rpcWaitForHostInterrupt(queueId_);
  }
  return ready;
}

bool EmuVirtQueueDev::reset(TimeDuration wait_time) {
  auto reset = virtQueueReset();
  assert(reset);

  rpcGen_->rpcWaitForHostInterrupt(queueId_);

  return ready(wait_time);
}

} // namespace device
} // namespace et_runtime
