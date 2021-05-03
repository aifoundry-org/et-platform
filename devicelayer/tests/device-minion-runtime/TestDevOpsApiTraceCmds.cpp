//******************************************************************************
// Copyright (C) 2020 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#include "TestDevOpsApiTraceCmds.h"
#include "Autogen.h"

/***********************************************************
 *                                                         *
 *                   Functional Tests                      *
 *                                                         *
 **********************************************************/

namespace {

/* TODO: All trace packet information should be in common files 
         for both Host and Device usage. */
constexpr uint32_t CM_SIZE_PER_HART  = 4096;
constexpr uint32_t WORKER_HART_COUNT = 2080;

enum trace_type_e {
  TRACE_TYPE_STRING,
  TRACE_TYPE_PMC_COUNTER,
  TRACE_TYPE_PMC_ALL_COUNTERS,
  TRACE_TYPE_VALUE_U64,
  TRACE_TYPE_VALUE_U32,
  TRACE_TYPE_VALUE_U16,
  TRACE_TYPE_VALUE_U8,
  TRACE_TYPE_VALUE_FLOAT
};

struct trace_entry_header_t {
  uint64_t cycle;   // Current cycle
  uint32_t hart_id; // Hart ID of the Hart which is logging Trace
  uint16_t type;    // One of enum trace_type_e
  uint8_t pad[2];
} __attribute__((packed));

struct trace_string_t {
  struct trace_entry_header_t header;
  char dataString[64];
} __attribute__((packed));
} // namespace

void TestDevOpsApiTraceCmds::traceCtrlAndExtractMMFwData_5_1() {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  initTagId(0x51);
  uint64_t size = 1024 * 1024;
  uint16_t sqIdx = 0; // Default SQ for dmaLoopback
  std::vector<uint8_t> readBuf(size, 0);
  std::vector<uint8_t> compBuf(size, 0);

  // Read Trace data from MM
  auto devPhysAddr = 0;
  auto hostVirtAddr = reinterpret_cast<uint64_t>(readBuf.data());
  auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataReadCmd(getNextTagId(), device_ops_api::CMD_FLAGS_MMFW_TRACEBUF,
                                                    devPhysAddr, hostVirtAddr, hostPhysAddr, readBuf.size(),
                                                    device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  streams_.emplace(sqIdx, std::move(stream));
  executeSync();

  ASSERT_NE(readBuf, compBuf);

  printMMTraceStringData(readBuf);
}

void TestDevOpsApiTraceCmds::traceCtrlAndExtractCMFwData_5_2() {
  std::vector<std::unique_ptr<IDevOpsApiCmd>> stream;
  uint64_t size = CM_SIZE_PER_HART * WORKER_HART_COUNT;
  uint16_t sqIdx = 0; // Default SQ for dmaLoopback
  std::vector<uint8_t> readBuf(size, 0);
  std::vector<uint8_t> compBuf(size, 0);

  // Read Trace data from CM
  auto devPhysAddr = 0;
  auto hostVirtAddr = reinterpret_cast<uint64_t>(readBuf.data());
  auto hostPhysAddr = hostVirtAddr; // Should be handled in SysEmu, userspace should not fill this value
  stream.push_back(IDevOpsApiCmd::createDataReadCmd(getNextTagId(), device_ops_api::CMD_FLAGS_CMFW_TRACEBUF,
                                                    devPhysAddr, hostVirtAddr, hostPhysAddr, readBuf.size(),
                                                    device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

  streams_.emplace(sqIdx, std::move(stream));

  executeSync();

  ASSERT_NE(readBuf, compBuf);

  printCMTraceStringData(readBuf);
}

void TestDevOpsApiTraceCmds::printMMTraceStringData(std::vector<uint8_t>& traceBuf) {
  auto dataPtr = reinterpret_cast<trace_string_t*>(traceBuf.data());
  while (1) {
    if (dataPtr->header.type == TRACE_TYPE_STRING) {
      std::cout << "H:" <<dataPtr->header.hart_id << ":" << dataPtr->dataString;
      dataPtr++;
    } else {
      break;
    }
  }
}

void TestDevOpsApiTraceCmds::printCMTraceStringData(std::vector<uint8_t>& traceBuf) {
  uint64_t size = CM_SIZE_PER_HART / sizeof(trace_string_t);
  unsigned char* bytePtr = reinterpret_cast<unsigned char*>(traceBuf.data());
  
  for (int i=0; i < WORKER_HART_COUNT; ++i)
  {
    auto dataPtr = reinterpret_cast<trace_string_t*>(bytePtr);
    auto start = dataPtr;
    while ((dataPtr - start) < size) {
      if ((dataPtr->header.type == TRACE_TYPE_STRING) && (dataPtr->dataString[0] != '\0')){
        std::cout << "H:" <<dataPtr->header.hart_id << ":" << dataPtr->dataString;
      } 
      dataPtr++;
    }
    bytePtr += CM_SIZE_PER_HART;
  }
}
