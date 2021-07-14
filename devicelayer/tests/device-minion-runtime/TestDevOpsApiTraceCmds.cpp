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

using namespace dev::dl_tests;

/***********************************************************
 *                                                         *
 *                   Functional Tests                      *
 *                                                         *
 **********************************************************/

void TestDevOpsApiTraceCmds::traceCtrlAndExtractMMFwData_5_1() {
  std::vector<std::vector<uint8_t>> readBufs;
  std::vector<std::vector<uint8_t>> compBufs;
  int deviceCount = getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    std::vector<CmdTag> stream;
    uint64_t size = 1024 * 1024;
    uint16_t queueIdx = 0; // Default SQ for dmaLoopback
    std::vector<uint8_t> readBuf(size, 0);
    std::vector<uint8_t> compBuf(size, 0);
    uint32_t trace_control = device_ops_api::TRACE_RT_CONTROL_ENABLE_TRACE |
                             device_ops_api::TRACE_RT_CONTROL_LOG_TO_TRACE;

    // Trace Config for MM
    stream.push_back(IDevOpsApiCmd::createCmd<TraceRtConfigCmd>(device_ops_api::CMD_FLAGS_BARRIER_DISABLE,
                                    MM_SHIRE_MASK, HART_ID, TRACE_STRING_FILTER, TRACE_STRING_LOG_INFO,
                                    device_ops_api::DEV_OPS_TRACE_RT_CONFIG_RESPONSE_SUCCESS));

    // start MM trace and dump logs to trace buffer
    stream.push_back(IDevOpsApiCmd::createCmd<TraceRtControlCmd>(device_ops_api::CMD_FLAGS_BARRIER_DISABLE,
                                    device_ops_api::TRACE_RT_TYPE_MM, trace_control,
                                    device_ops_api::DEV_OPS_TRACE_RT_CONTROL_RESPONSE_SUCCESS));

    // Read Trace data from MM
    auto devPhysAddr = 0;
    auto hostVirtAddr = templ::bit_cast<uint64_t>(readBuf.data());
    stream.push_back(IDevOpsApiCmd::createCmd<DataReadCmd>(device_ops_api::CMD_FLAGS_MMFW_TRACEBUF, devPhysAddr,
                                                           hostVirtAddr, readBuf.size(),
                                                           device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

    // stop MM trace and redirect logs to UART
    trace_control |= device_ops_api::TRACE_RT_CONTROL_LOG_TO_UART;
    stream.push_back(IDevOpsApiCmd::createCmd<TraceRtControlCmd>(device_ops_api::CMD_FLAGS_BARRIER_ENABLE,
                                    device_ops_api::TRACE_RT_TYPE_MM, trace_control,
                                    device_ops_api::DEV_OPS_TRACE_RT_CONTROL_RESPONSE_SUCCESS));

    readBufs.push_back(std::move(readBuf));
    compBufs.push_back(std::move(compBuf));
    insertStream(deviceIdx, queueIdx, std::move(stream));
    stream.clear();
  }
  execute(false);

  for (size_t i = 0; i < readBufs.size(); ++i) {
    ASSERT_NE(readBufs[i], compBufs[i]);
  }

  for (size_t i = 0; i < readBufs.size(); ++i) {
    EXPECT_TRUE(printMMTraceData(readBufs[i].data(), readBufs[i].size()))
      << "No Trace String event found!" << std::endl;
      deleteStreams();
  }
  deleteStreams();
}

void TestDevOpsApiTraceCmds::traceCtrlAndExtractCMFwData_5_2() {
  std::vector<std::vector<uint8_t>> readBufs;
  std::vector<std::vector<uint8_t>> compBufs;
  int deviceCount = getDevicesCount();
  for (int deviceIdx = 0; deviceIdx < deviceCount; deviceIdx++) {
    std::vector<CmdTag> stream;
    uint64_t size = CM_SIZE_PER_HART * WORKER_HART_COUNT;
    uint16_t queueIdx = 0; // Default SQ for dmaLoopback
    std::vector<uint8_t> readBuf(size, 0);
    std::vector<uint8_t> compBuf(size, 0);
    uint32_t trace_control = device_ops_api::TRACE_RT_CONTROL_ENABLE_TRACE |
                             device_ops_api::TRACE_RT_CONTROL_LOG_TO_TRACE;

    // Trace Config the first Hart of CM
    stream.push_back(IDevOpsApiCmd::createCmd<TraceRtConfigCmd>(device_ops_api::CMD_FLAGS_BARRIER_DISABLE,
                                    CM_SHIRE_MASK, HART_ID, TRACE_STRING_FILTER, TRACE_STRING_LOG_INFO,
                                    device_ops_api::DEV_OPS_TRACE_RT_CONFIG_RESPONSE_SUCCESS));

    // start CM trace and dump logs to trace buffer
    stream.push_back(IDevOpsApiCmd::createCmd<TraceRtControlCmd>(device_ops_api::CMD_FLAGS_BARRIER_DISABLE,
                                    device_ops_api::TRACE_RT_TYPE_CM, trace_control,
                                    device_ops_api::DEV_OPS_TRACE_RT_CONTROL_RESPONSE_SUCCESS));

    // Read Trace data from CM
    auto devPhysAddr = 0;
    auto hostVirtAddr = templ::bit_cast<uint64_t>(readBuf.data());
    stream.push_back(IDevOpsApiCmd::createCmd<DataReadCmd>(device_ops_api::CMD_FLAGS_CMFW_TRACEBUF, devPhysAddr,
                                                           hostVirtAddr, readBuf.size(),
                                                           device_ops_api::DEV_OPS_API_DMA_RESPONSE_COMPLETE));

    readBufs.push_back(std::move(readBuf));
    compBufs.push_back(std::move(compBuf));
    insertStream(deviceIdx, queueIdx, std::move(stream));
    stream.clear();
  }
  execute(false);

  for (size_t i = 0; i < readBufs.size(); ++i) {
    ASSERT_NE(readBufs[i], compBufs[i]);
  }

  for (size_t i = 0; i < readBufs.size(); ++i) {
    EXPECT_TRUE(printCMTraceData(readBufs[i].data(), readBufs[i].size()))
      << "No Trace String event found!" << std::endl;
      deleteStreams();
  }
  deleteStreams();
}
