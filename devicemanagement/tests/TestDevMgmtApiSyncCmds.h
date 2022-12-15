//******************************************************************************
// Copyright (C) 2020 Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef TEST_DEVICE_M_H
#define TEST_DEVICE_M_H

#include "deviceManagement/DeviceManagement.h"
#include "utils.h"

#include <cstring>
#include <dlfcn.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace dev;
using namespace device_management;
using Clock = std::chrono::system_clock;
using Timepoint = Clock::time_point;
using TimeDuration = Clock::duration;

#define MAX_DEVICE_NODE (250)
#define INPUT_SIZE_TEST (1)
#define OUTPUT_SIZE_TEST (24)
#define DM_CMD_INVALID (256)
#define INVALID_INPUT_SIZE (0)
#define INVALID_OUTPUT_SIZE (0)
#define ECID_LOT_ID_LENGTH 6

DECLARE_bool(enable_trace_dump);
DECLARE_string(trace_base_dir);
DECLARE_string(trace_txt_dir);
DECLARE_string(trace_bin_dir);

typedef struct {
  uint64_t lot_id;
  uint8_t wafer_id;
  uint8_t x_coordinate;
  uint8_t y_coordinate;
  char lot_id_str[ECID_LOT_ID_LENGTH + 1];
} ecid_t;

enum class Target { Silicon, Bemu, FullBoot, FullChip, SysEMU, Loopback };

class TestDevMgmtApiSyncCmds : public ::testing::Test {
protected:
  device_management::getDM_t getInstance();

  // Functional tests asset tracking service
  void getModuleManufactureName(bool singleDevice);
  void getModulePartNumber(bool singleDevice);
  void setAndGetModulePartNumber(bool singleDevice);
  void getModuleSerialNumber(bool singleDevice);
  void getASICChipRevision(bool singleDevice);
  void getModulePCIEPortsMaxSpeed(bool singleDevice);
  void getModuleMemorySizeMB(bool singleDevice);
  void getModuleRevision(bool singleDevice);
  void getModuleFormFactor(bool singleDevice);
  void getModuleMemoryVendorPartNumber(bool singleDevice);
  void getModuleMemoryType(bool singleDevice);

  // Functional tests thermal and power management service
  void getModulePowerState(bool singleDevice);
  void setAndGetModuleStaticTDPLevel(bool singleDevice);
  void setAndGetModuleTemperatureThreshold(bool singleDevice);
  void getModuleResidencyThrottleState(bool singleDevice);
  void getModuleUptime(bool singleDevice);
  void getModulePower(bool singleDevice);
  void getAsicVoltage(bool singleDevice);
  void getModuleVoltage(bool singleDevice);
  void setAndGetModuleVoltage(bool singleDevice);
  void getModuleCurrentTemperature(bool singleDevice);
  void getModuleResidencyPowerState(bool singleDevice);
  void setModuleActivePowerManagement(bool singleDevice);
  void setThrottlePowerStatus(bool singleDevice);
  void setAndGetModuleFrequency(bool singleDevice);
  void dmStatsRunControl(bool singleDevice);

  // Functional tests historical extreme value service
  void getModuleMaxTemperature(bool singleDevice);
  void getModuleMaxMemoryErrors(bool singleDevice);
  void getModuleMaxDDRBW(bool singleDevice);

  // Functional tests error control service
  void setAndGetDDRECCThresholdCount(bool singleDevice);
  void setAndGetSRAMECCThresholdCount(bool singleDevice);
  void setAndGetPCIEECCThresholdCount(bool singleDevice);

  // Functional tests link management service
  void getPCIEECCUECCCount(bool singleDevice);
  void getDDRECCUECCCount(bool singleDevice);
  void getSRAMECCUECCCount(bool singleDevice);
  void getDDRBWCounter(bool singleDevice);
  void setPCIELinkSpeed(bool singleDevice);
  void setPCIELaneWidth(bool singleDevice);
  void setPCIERetrainPhy(bool singleDevice);

  // Functional tests performance management service
  void getASICFrequencies(bool singleDevice);
  void getDRAMBW(bool singleDevice);
  void getDRAMCapacityUtilization(bool singleDevice);
  void getASICPerCoreDatapathUtilization(bool singleDevice);
  void getASICUtilization(bool singleDevice);
  void getASICStalls(bool singleDevice);
  void getASICLatency(bool singleDevice);
  void getMMErrorCount(bool singleDevice);
  void getFWBootstatus(bool singleDevice);
  void getModuleFWRevision(bool singleDevice);
  void setSpRootCertificate(bool singleDevice);
  void updateFirmwareImage(bool singleDevice);

  // Integration tests for SP tracing and error events
  void initTestTrace();
  bool decodeTraceEvents(int deviceIdx, const std::vector<std::byte>& traceBuf, TraceBufferType bufferType) const;
  void dumpRawTraceBuffer(int deviceIdx, const std::vector<std::byte>& traceBuf, TraceBufferType bufferType) const;

  void controlTraceLogging(void);
  bool extractAndPrintTraceData(bool singleDevice, TraceBufferType bufferType);
  void serializeAccessMgmtNode(bool singleDevice);
  void getDeviceErrorEvents(bool singleDevice);
  void setTraceControl(bool singleDevice, uint32_t control_bitmap);
  void setTraceConfigure(bool singleDevice, uint32_t event_type, uint32_t filter_level);
  void getTraceBuffer(bool singleDevice, TraceBufferType bufferType);

  // Security tests general checks
  void isUnsupportedService(bool singleDevice);
  void testInvalidCmdCode(bool singleDevice);

  // Security tests asset tracking service
  void getModuleManufactureNameInvalidOutputSize(bool singleDevice);
  void getModuleManufactureNameInvalidDeviceNode(bool singleDevice);
  void getModuleManufactureNameInvalidHostLatency(bool singleDevice);
  void getModuleManufactureNameInvalidDeviceLatency(bool singleDevice);
  void getModuleManufactureNameInvalidOutputBuffer(bool singleDevice);

  // Security tests thermal and power management service
  void setModuleActivePowerManagementRange(bool singleDevice);
  void setModuleSetTemperatureThresholdRange(bool singleDevice);
  void setModuleStaticTDPLevelRange(bool singleDevice);
  void setModuleActivePowerManagementRangeInvalidInputSize(bool singleDevice);
  void setModuleActivePowerManagementRangeInvalidInputBuffer(bool singleDevice);

  // Security tests historical extreme value service
  void getHistoricalExtremeWithInvalidDeviceNode(bool singleDevice);
  void getHistoricalExtremeWithInvalidHostLatency(bool singleDevice);
  void getHistoricalExtremeWithInvalidDeviceLatency(bool singleDevice);
  void getHistoricalExtremeWithInvalidOutputBuffer(bool singleDevice);
  void getHistoricalExtremeWithInvalidOutputSize(bool singleDevice);

  // Security tests error control service
  void setDDRECCCountInvalidInputBuffer(bool singleDevice);
  void setDDRECCCountInvalidInputSize(bool singleDevice);
  void setDDRECCCountInvalidOutputSize(bool singleDevice);
  void setDDRECCCountInvalidDeviceNode(bool singleDevice);
  void setDDRECCCountInvalidHostLatency(bool singleDevice);
  void setDDRECCCountInvalidDeviceLatency(bool singleDevice);
  void setDDRECCCountInvalidOutputBuffer(bool singleDevice);
  void setPCIEECCCountInvalidInputBuffer(bool singleDevice);
  void setPCIEECCountInvalidInputSize(bool singleDevice);
  void setPCIEECCountInvalidOutputSize(bool singleDevice);
  void setPCIEECCountInvalidDeviceNode(bool singleDevice);
  void setPCIEECCountInvalidHostLatency(bool singleDevice);
  void setPCIEECCountInvalidDeviceLatency(bool singleDevice);
  void setPCIEECCountInvalidOutputBuffer(bool singleDevice);
  void setSRAMECCCountInvalidInputBuffer(bool singleDevice);
  void setSRAMECCCountInvalidInputSize(bool singleDevice);
  void setSRAMECCountInvalidOutputSize(bool singleDevice);
  void setSRAMECCountInvalidDeviceNode(bool singleDevice);
  void setSRAMECCountInvalidHostLatency(bool singleDevice);
  void setSRAMECCountInvalidDeviceLatency(bool singleDevice);
  void setSRAMECCountInvalidOutputBuffer(bool singleDevice);

  // Security tests link management service
  void setPCIELinkSpeedToInvalidLinkSpeed(bool singleDevice);
  void setPCIELaneWidthToInvalidLaneWidth(bool singleDevice);

  // Security tests performance management
  void testInvalidOutputSize(int32_t dmCmdType, bool singleDevice);
  void testInvalidDeviceNode(int32_t dmCmdType, bool singleDevice);
  void testInvalidHostLatency(int32_t dmCmdType, bool singleDevice);
  void testInvalidDeviceLatency(int32_t dmCmdType, bool singleDevice);
  void testInvalidOutputBuffer(int32_t dmCmdType, bool singleDevice);
  void testInvalidInputBuffer(int32_t dmCmdType, bool singleDevice);
  void testInvalidInputSize(int32_t dmCmdType, bool singleDevice);
  void getASICFrequenciesInvalidOutputSize(bool singleDevice);
  void getASICFrequenciesInvalidDeviceNode(bool singleDevice);
  void getASICFrequenciesInvalidHostLatency(bool singleDevice);
  void getASICFrequenciesInvalidDeviceLatency(bool singleDevice);
  void getASICFrequenciesInvalidOutputBuffer(bool singleDevice);
  void getDRAMBandwidthInvalidOutputSize(bool singleDevice);
  void getDRAMBandwidthInvalidDeviceNode(bool singleDevice);
  void getDRAMBandwidthInvalidHostLatency(bool singleDevice);
  void getDRAMBandwidthInvalidDeviceLatency(bool singleDevice);
  void getDRAMBandwidthInvalidOutputBuffer(bool singleDevice);
  void getDRAMCapacityUtilizationInvalidOutputSize(bool singleDevice);
  void getDRAMCapacityUtilizationInvalidDeviceNode(bool singleDevice);
  void getDRAMCapacityUtilizationInvalidHostLatency(bool singleDevice);
  void getDRAMCapacityUtilizationInvalidDeviceLatency(bool singleDevice);
  void getDRAMCapacityUtilizationInvalidOutputBuffer(bool singleDevice);
  void getASICPerCoreDatapathUtilizationInvalidOutputSize(bool singleDevice);
  void getASICPerCoreDatapathUtilizationInvalidDeviceNode(bool singleDevice);
  void getASICPerCoreDatapathUtilizationInvalidHostLatency(bool singleDevice);
  void getASICPerCoreDatapathUtilizationInvalidDeviceLatency(bool singleDevice);
  void getASICPerCoreDatapathUtilizationInvalidOutputBuffer(bool singleDevice);
  void getASICUtilizationInvalidOutputSize(bool singleDevice);
  void getASICUtilizationInvalidDeviceNode(bool singleDevice);
  void getASICUtilizationInvalidHostLatency(bool singleDevice);
  void getASICUtilizationInvalidDeviceLatency(bool singleDevice);
  void getASICUtilizationInvalidOutputBuffer(bool singleDevice);
  void getASICStallsInvalidOutputSize(bool singleDevice);
  void getASICStallsInvalidDeviceNode(bool singleDevice);
  void getASICStallsInvalidHostLatency(bool singleDevice);
  void getASICStallsInvalidDeviceLatency(bool singleDevice);
  void getASICStallsInvalidOutputBuffer(bool singleDevice);
  void getASICLatencyInvalidOutputSize(bool singleDevice);
  void getASICLatencyInvalidDeviceNode(bool singleDevice);
  void getASICLatencyInvalidHostLatency(bool singleDevice);
  void getASICLatencyInvalidDeviceLatency(bool singleDevice);
  void getASICLatencyInvalidOutputBuffer(bool singleDevice);
  void resetMM(bool singleDevice);
  void resetMMWithOpsInUse(bool singleDevice);
  void resetSOC(bool singleDevice);
  void resetSOCWithOpsInUse(bool singleDevice);

  // Functional tests for  MDI Run control/State Inspection APIs
  void testRunControlCmdsSetandUnsetBreakpoint(uint64_t shireID, uint64_t threadMask, uint64_t hartID, uint64_t bpAddr);
  void testRunControlCmdsGetHartStatus(uint64_t shireID, uint64_t threadMask, uint64_t hartID);
  void testStateInspectionReadGPR(uint64_t shireID, uint64_t threadMask, uint64_t hartID);
  void testStateInspectionWriteGPR(uint64_t shireID, uint64_t threadMask, uint64_t hartID, uint64_t writeTestData);
  void testStateInspectionReadCSR(uint64_t shireID, uint64_t threadMask, uint64_t hartID, uint64_t csrName);
  void testStateInspectionWriteCSR(uint64_t shireID, uint64_t threadMask, uint64_t hartID, uint64_t csrName,
                                   uint64_t csrData);
  void readMem(uint64_t readAddr);
  void writeMem(uint64_t testInputData, uint64_t writeAddr);

  inline Target getTestTarget(void) const {
    auto envTarget = getenv("TARGET");
    auto currentTarget = Target::Silicon;
    if (envTarget != nullptr) {
      if (std::strncmp(envTarget, "silicon", 7) == 0) {
        currentTarget = Target::Silicon;
      } else if (std::strncmp(envTarget, "bemu", 4) == 0) {
        currentTarget = Target::Bemu;
      } else if (std::strncmp(envTarget, "fullboot", 8) == 0) {
        currentTarget = Target::FullBoot;
      } else if (std::strncmp(envTarget, "fullchip", 8) == 0) {
        currentTarget = Target::FullChip;
      } else if (std::strncmp(envTarget, "sysemu", 6) == 0) {
        currentTarget = Target::SysEMU;
      } else if (std::strncmp(envTarget, "loopback", 8) == 0) {
        currentTarget = Target::Loopback;
      } else {
        DV_LOG(INFO) << "Unknown target: " << envTarget << ", using default target (silicon)";
      }
    }
    return currentTarget;
  }

  inline bool targetInList(std::initializer_list<Target> list) {
    return std::find(list.begin(), list.end(), getTestTarget()) != list.end();
  }

  inline bool isParallelRun(void) const {
    auto envParallel = getenv("PARALLEL");
    if (envParallel != nullptr && envParallel[0] != '\0') {
      return true;
    }
    return false;
  }

  void* handle_ = nullptr;
  std::unique_ptr<IDeviceLayer> devLayer_;
  logging::LoggerDefault logger_;
};

#endif // TEST_DEVICE_M_H
