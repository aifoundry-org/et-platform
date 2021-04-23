/*-------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#pragma once
#include <algorithm>
#include <cstddef>
#include <esperanto/device-apis/operations-api/device_ops_api_cxx.h>
#include <glog/logging.h>
#include <memory>
#include <unordered_map>

// test logging define
// severity levels: INFO, WARNING, ERROR, and FATAL which are 0, 1, 2, and 3, respectively
#define TEST_DLOG(severity) DLOG(severity) << "ET [DEVICE_LAYER_TEST]: "

// uses user specific verbose levels
#define TEST_VLOG(level) VLOG(level) << "ET [DEVICE_LAYER_TEST]: "

class IDevOpsApiCmd {
public:
  enum class CmdType {
    ECHO_CMD,
    API_COMPATIBILITY_CMD,
    FW_VERSION_CMD,
    DATA_WRITE_CMD,
    DATA_READ_CMD,
    KERNEL_LAUNCH_CMD,
    KERNEL_ABORT_CMD,
    TRACE_CONFIG_CMD,
    TRACE_CONTROL_CMD,
    CUSTOM_CMD
  };

  virtual std::byte* getCmdPtr() = 0;
  virtual size_t getCmdSize() = 0;
  virtual device_ops_api::tag_id_t getCmdTagId() = 0;
  virtual CmdType whoAmI() = 0;
  virtual ~IDevOpsApiCmd() = default;

  static std::unique_ptr<IDevOpsApiCmd> createEchoCmd(device_ops_api::tag_id_t tagId, device_ops_api::cmd_flags_e flag,
                                                      int32_t echoPayload);
  static std::unique_ptr<IDevOpsApiCmd> createApiCompatibilityCmd(device_ops_api::tag_id_t tagId, device_ops_api::cmd_flags_e flag,
                                                                  uint8_t major, uint8_t minor, uint8_t patch);
  static std::unique_ptr<IDevOpsApiCmd> createFwVersionCmd(device_ops_api::tag_id_t tagId, device_ops_api::cmd_flags_e flag,
                                                           uint8_t firmwareType);
  static std::unique_ptr<IDevOpsApiCmd> createDataWriteCmd(device_ops_api::tag_id_t tagId, device_ops_api::cmd_flags_e flag,
                                                           uint64_t devPhysAddr, uint64_t hostVirtAddr,
                                                           uint64_t hostPhysAddr, uint64_t dataSize,
                                                           device_ops_api::dev_ops_api_dma_response_e status);
  static std::unique_ptr<IDevOpsApiCmd> createDataReadCmd(device_ops_api::tag_id_t tagId, device_ops_api::cmd_flags_e flag,
                                                          uint64_t devPhysAddr, uint64_t hostVirtAddr,
                                                          uint64_t hostPhysAddr, uint64_t dataSize,
                                                          device_ops_api::dev_ops_api_dma_response_e status);
  static std::unique_ptr<IDevOpsApiCmd>
  createKernelLaunchCmd(device_ops_api::tag_id_t tagId, device_ops_api::cmd_flags_e flag, uint64_t codeStartAddr, uint64_t ptrToArgs,
                        uint64_t shireMask, device_ops_api::dev_ops_api_kernel_launch_response_e status);
  static std::unique_ptr<IDevOpsApiCmd>
  createKernelAbortCmd(device_ops_api::tag_id_t tagId, device_ops_api::cmd_flags_e flag, device_ops_api::tag_id_t kernelToAbortTagId,
                       device_ops_api::dev_ops_api_kernel_abort_response_e status);
  static std::unique_ptr<IDevOpsApiCmd> createTraceRtConfigCmd(device_ops_api::tag_id_t, device_ops_api::cmd_flags_e flag,
                                                               uint32_t  shire_mask, uint32_t  thread_mask,
                                                               uint32_t  event_mask, uint32_t  filter_mask,
                                                               device_ops_api::dev_ops_trace_rt_config_response_e status);
  static std::unique_ptr<IDevOpsApiCmd> createTraceRtControlCmd(device_ops_api::tag_id_t tagId, device_ops_api::cmd_flags_e flag,
                                                                uint32_t  rt_type, uint32_t  control,
                                                                device_ops_api::dev_ops_trace_rt_control_response_e status);
  static std::unique_ptr<IDevOpsApiCmd> createCustomCmd(std::byte* cmdPtr, size_t cmdSize, uint32_t status);

  static uint32_t getExpectedRsp(device_ops_api::tag_id_t tagId);

protected:
  static bool addRspEntry(device_ops_api::tag_id_t tagId, uint32_t expectedRsp);
  static void deleteRspEntry(device_ops_api::tag_id_t tagId);

  static std::unordered_map<device_ops_api::tag_id_t, uint32_t> rspStorage_;
};

class EchoCmd : public IDevOpsApiCmd {
public:
  std::byte* getCmdPtr() override;
  size_t getCmdSize() override;
  device_ops_api::tag_id_t getCmdTagId() override;
  CmdType whoAmI() override;
  explicit EchoCmd(device_ops_api::tag_id_t tagId, device_ops_api::cmd_flags_e flag, int32_t echoPayload);
  ~EchoCmd();

private:
  device_ops_api::device_ops_echo_cmd_t cmd_;
};

class ApiCompatibilityCmd : public IDevOpsApiCmd {
public:
  std::byte* getCmdPtr() override;
  size_t getCmdSize() override;
  device_ops_api::tag_id_t getCmdTagId() override;
  CmdType whoAmI() override;
  explicit ApiCompatibilityCmd(device_ops_api::tag_id_t tagId, device_ops_api::cmd_flags_e flag, uint8_t major, uint8_t minor,
                               uint8_t patch);
  ~ApiCompatibilityCmd();

private:
  device_ops_api::check_device_ops_api_compatibility_cmd_t cmd_;
};

class FwVersionCmd : public IDevOpsApiCmd {
public:
  std::byte* getCmdPtr() override;
  size_t getCmdSize() override;
  device_ops_api::tag_id_t getCmdTagId() override;
  CmdType whoAmI() override;
  explicit FwVersionCmd(device_ops_api::tag_id_t tagId, device_ops_api::cmd_flags_e flag, uint8_t firmwareType);
  ~FwVersionCmd();

private:
  device_ops_api::device_ops_device_fw_version_cmd_t cmd_;
};

class DataWriteCmd : public IDevOpsApiCmd {
public:
  std::byte* getCmdPtr() override;
  size_t getCmdSize() override;
  device_ops_api::tag_id_t getCmdTagId() override;
  CmdType whoAmI() override;
  explicit DataWriteCmd(device_ops_api::tag_id_t tagId, device_ops_api::cmd_flags_e flag, uint64_t devPhysAddr, uint64_t hostVirtAddr,
                        uint64_t hostPhysAddr, uint64_t dataSize);
  ~DataWriteCmd();

private:
  device_ops_api::device_ops_data_write_cmd_t cmd_;
};

class DataReadCmd : public IDevOpsApiCmd {
public:
  std::byte* getCmdPtr() override;
  size_t getCmdSize() override;
  device_ops_api::tag_id_t getCmdTagId() override;
  CmdType whoAmI() override;
  explicit DataReadCmd(device_ops_api::tag_id_t tagId, device_ops_api::cmd_flags_e flag, uint64_t devPhysAddr, uint64_t hostVirtAddr,
                       uint64_t hostPhysAddr, uint64_t dataSize);
  ~DataReadCmd();

private:
  device_ops_api::device_ops_data_read_cmd_t cmd_;
};

class KernelLaunchCmd : public IDevOpsApiCmd {
public:
  std::byte* getCmdPtr() override;
  size_t getCmdSize() override;
  device_ops_api::tag_id_t getCmdTagId() override;
  CmdType whoAmI() override;
  explicit KernelLaunchCmd(device_ops_api::tag_id_t tagId, device_ops_api::cmd_flags_e flag, uint64_t codeStartAddr, uint64_t ptrToArgs,
                           uint64_t shireMask);
  ~KernelLaunchCmd();

private:
  device_ops_api::device_ops_kernel_launch_cmd_t cmd_;
};

class KernelAbortCmd : public IDevOpsApiCmd {
public:
  std::byte* getCmdPtr() override;
  size_t getCmdSize() override;
  device_ops_api::tag_id_t getCmdTagId() override;
  CmdType whoAmI() override;
  explicit KernelAbortCmd(device_ops_api::tag_id_t tagId, device_ops_api::cmd_flags_e flag, device_ops_api::tag_id_t kernelToAbortTagId);
  ~KernelAbortCmd();

private:
  device_ops_api::device_ops_kernel_abort_cmd_t cmd_;
};

class CustomCmd : public IDevOpsApiCmd {
public:
  std::byte* getCmdPtr() override;
  size_t getCmdSize() override;
  device_ops_api::tag_id_t getCmdTagId() override;
  CmdType whoAmI() override;
  explicit CustomCmd(std::byte* cmdPtr, size_t cmdSize);
  ~CustomCmd();

private:
  std::byte* cmd_;
};

class TraceRtConfigCmd : public IDevOpsApiCmd {
public:
  std::byte* getCmdPtr() override;
  size_t getCmdSize() override;
  device_ops_api::tag_id_t getCmdTagId() override;
  CmdType whoAmI() override;
  explicit TraceRtConfigCmd(device_ops_api::tag_id_t tagId, device_ops_api::cmd_flags_e flag, uint32_t  shire_mask, uint32_t  thread_mask,
                            uint32_t  event_mask, uint32_t  filter_mask);
  ~TraceRtConfigCmd();

private:
  device_ops_api::device_ops_trace_rt_config_cmd_t cmd_;
};

class TraceRtControlCmd : public IDevOpsApiCmd {
public:
  std::byte* getCmdPtr() override;
  size_t getCmdSize() override;
  device_ops_api::tag_id_t getCmdTagId() override;
  CmdType whoAmI() override;
  explicit TraceRtControlCmd(device_ops_api::tag_id_t tagId, device_ops_api::cmd_flags_e flag,
                             uint32_t  rt_type, uint32_t  control);
  ~TraceRtControlCmd();

private:
  device_ops_api::device_ops_trace_rt_control_cmd_t cmd_;
};