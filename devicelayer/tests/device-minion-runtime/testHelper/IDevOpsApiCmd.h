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
#include <atomic>
#include <cstddef>
#include <cstring>
#include <esperanto/device-apis/operations-api/device_ops_api_cxx.h>
#include <glog/logging.h>
#include <hostUtils/debug/StackException.h>
#include <memory>
#include <tuple>
#include <unordered_map>

namespace dev::dl_tests {

// test logging define
// severity levels: INFO, WARNING, ERROR, and FATAL which are 0, 1, 2, and 3, respectively
#define TEST_DLOG(severity) DLOG(severity) << "ET [DEVICE_LAYER_TEST]: "

// uses user specific verbose levels
#define TEST_VLOG(level) VLOG(level) << "ET [DEVICE_LAYER_TEST]: "

// A namespace containing template for `bit_cast`. To be removed when `bit_cast` will be available
namespace templ {
template <class To, class From>
typename std::enable_if_t<
  sizeof(To) == sizeof(From) && std::is_trivially_copyable_v<From> && std::is_trivially_copyable_v<To>, To>
// constexpr support needs compiler magic
bit_cast(const From& src) noexcept {
  static_assert(std::is_trivially_constructible_v<To>,
                "This implementation additionally requires destination type to be trivially constructible");
  To dst;
  std::memcpy(&dst, &src, sizeof(To));
  return dst;
}
} // namespace templ

#define PRINTABLE_ENUM(name, ...)                                                                                      \
  enum class name { __VA_ARGS__, __COUNT };                                                                            \
  inline std::ostream& operator<<(std::ostream& os, const name& value) {                                               \
    std::string enumName = #name;                                                                                      \
    std::string str = #__VA_ARGS__;                                                                                    \
    int len = str.length();                                                                                            \
    std::vector<std::string> strings;                                                                                  \
    std::ostringstream temp;                                                                                           \
    for (int i = 0; i < len; i++) {                                                                                    \
      if (isspace(str[i]))                                                                                             \
        continue;                                                                                                      \
      else if (str[i] == ',') {                                                                                        \
        strings.push_back(temp.str());                                                                                 \
        temp.str(std::string());                                                                                       \
      } else                                                                                                           \
        temp << str[i];                                                                                                \
    }                                                                                                                  \
    strings.push_back(temp.str());                                                                                     \
    os << enumName << "::" << strings[static_cast<int>(value)];                                                        \
    return os;                                                                                                         \
  }

PRINTABLE_ENUM(CmdType, ECHO_CMD, API_COMPATIBILITY_CMD, FW_VERSION_CMD, DATA_WRITE_CMD, DATA_READ_CMD,
               DMA_WRITELIST_CMD, DMA_READLIST_CMD, KERNEL_LAUNCH_CMD, KERNEL_ABORT_CMD, TRACE_CONFIG_CMD,
               TRACE_CONTROL_CMD, CUSTOM_CMD);

PRINTABLE_ENUM(CmdStatus, CMD_RSP_NOT_RECEIVED, CMD_TIMED_OUT, CMD_FAILED, CMD_RSP_DUPLICATE, CMD_SUCCESSFUL);

using CmdTag = device_ops_api::tag_id_t;

class Exception : public dbg::StackException {
  using dbg::StackException::StackException;
};

class IDevOpsApiCmd {
public:
  template <typename cmdClass, typename... Args> static CmdTag createCmd(Args&&... args) {
    auto tagId = tagId_++;
    cmds_.emplace(tagId, std::make_unique<cmdClass>(tagId, std::tuple<Args&&...>(std::forward<Args>(args)...)));
    return tagId;
  }

  static IDevOpsApiCmd* getDevOpsApiCmd(CmdTag tagId);
  static void deleteDevOpsApiCmds();

  virtual std::byte* getCmdPtr() = 0;
  virtual size_t getCmdSize() = 0;
  virtual CmdTag getCmdTagId() = 0;
  virtual CmdType whoAmI() = 0;
  virtual bool isDma() const {
    return false;
  }
  virtual CmdStatus setRsp(const std::vector<std::byte>& rsp) = 0;
  virtual CmdStatus getCmdStatus() const = 0;
  virtual uint32_t getRspStatusCode() const {
    return 0;
  }
  virtual std::string printSummary() = 0;
  virtual ~IDevOpsApiCmd() = default;

private:
  static std::unordered_map<CmdTag, std::unique_ptr<IDevOpsApiCmd>> cmds_;
  static std::atomic<CmdTag> tagId_;
};

class EchoCmd : public IDevOpsApiCmd {
public:
  std::byte* getCmdPtr() override;
  size_t getCmdSize() override;
  CmdTag getCmdTagId() override;
  CmdType whoAmI() override;
  CmdStatus setRsp(const std::vector<std::byte>& rsp) override;
  CmdStatus getCmdStatus() const override {
    return cmdStatus_;
  };
  std::string printSummary() override;
  explicit EchoCmd(CmdTag tagId, const std::tuple<device_ops_api::cmd_flags_e /* flags */>& args);
  ~EchoCmd() final = default;

private:
  device_ops_api::device_ops_echo_cmd_t cmd_;
  device_ops_api::device_ops_echo_rsp_t rsp_;
  CmdStatus cmdStatus_ = CmdStatus::CMD_RSP_NOT_RECEIVED;
};

class ApiCompatibilityCmd : public IDevOpsApiCmd {
public:
  std::byte* getCmdPtr() override;
  size_t getCmdSize() override;
  CmdTag getCmdTagId() override;
  CmdType whoAmI() override;
  CmdStatus setRsp(const std::vector<std::byte>& rsp) override;
  CmdStatus getCmdStatus() const override {
    return cmdStatus_;
  };
  std::string printSummary() override;
  explicit ApiCompatibilityCmd(
    CmdTag tagId,
    const std::tuple<device_ops_api::cmd_flags_e /* flags */, uint8_t /*major*/, uint8_t /*minor*/, uint8_t /*patch*/>&
      args);
  ~ApiCompatibilityCmd() final = default;

private:
  device_ops_api::check_device_ops_api_compatibility_cmd_t cmd_;
  device_ops_api::device_ops_api_compatibility_rsp_t rsp_;
  CmdStatus cmdStatus_ = CmdStatus::CMD_RSP_NOT_RECEIVED;
};

class FwVersionCmd : public IDevOpsApiCmd {
public:
  std::byte* getCmdPtr() override;
  size_t getCmdSize() override;
  CmdTag getCmdTagId() override;
  CmdType whoAmI() override;
  CmdStatus setRsp(const std::vector<std::byte>& rsp) override;
  CmdStatus getCmdStatus() const override {
    return cmdStatus_;
  };
  std::string printSummary() override;
  explicit FwVersionCmd(CmdTag tagId,
                        const std::tuple<device_ops_api::cmd_flags_e /*flags*/, uint8_t /*firmwareType*/>& args);
  ~FwVersionCmd() final = default;

private:
  device_ops_api::device_ops_device_fw_version_cmd_t cmd_;
  device_ops_api::device_ops_fw_version_rsp_t rsp_;
  CmdStatus cmdStatus_ = CmdStatus::CMD_RSP_NOT_RECEIVED;
};

class DataWriteCmd : public IDevOpsApiCmd {
public:
  std::byte* getCmdPtr() override;
  size_t getCmdSize() override;
  CmdTag getCmdTagId() override;
  CmdType whoAmI() override;
  CmdStatus setRsp(const std::vector<std::byte>& rsp) override;
  CmdStatus getCmdStatus() const override {
    return cmdStatus_;
  };
  uint32_t getRspStatusCode() const override;
  std::string printSummary() override;
  explicit DataWriteCmd(CmdTag tagId,
                        const std::tuple<device_ops_api::cmd_flags_e /*flags*/, uint64_t /*devPhysAddr*/,
                                         uint64_t /*hostVirtAddr*/, uint64_t /*hostPhysAddr*/, uint64_t /*dataSize*/,
                                         device_ops_api::dev_ops_api_dma_response_e /*expStatus*/>&
                          args);
  ~DataWriteCmd() final = default;
  bool isDma() const override {
    return true;
  }

private:
  device_ops_api::device_ops_data_write_cmd_t cmd_;
  device_ops_api::device_ops_data_write_rsp_t rsp_;
  uint32_t expStatus_;
  CmdStatus cmdStatus_ = CmdStatus::CMD_RSP_NOT_RECEIVED;
};

class DataReadCmd : public IDevOpsApiCmd {
public:
  std::byte* getCmdPtr() override;
  size_t getCmdSize() override;
  CmdTag getCmdTagId() override;
  CmdType whoAmI() override;
  CmdStatus setRsp(const std::vector<std::byte>& rsp) override;
  CmdStatus getCmdStatus() const override {
    return cmdStatus_;
  };
  uint32_t getRspStatusCode() const override;
  std::string printSummary() override;
  explicit DataReadCmd(CmdTag tagId,
                       const std::tuple<device_ops_api::cmd_flags_e /*flags*/, uint64_t /*devPhysAddr*/,
                                        uint64_t /*hostVirtAddr*/, uint64_t /*hostPhysAddr*/, uint64_t /*dataSize*/,
                                        device_ops_api::dev_ops_api_dma_response_e /*expStatus*/>&
                         args);
  ~DataReadCmd() final = default;
  bool isDma() const override {
    return true;
  }

private:
  device_ops_api::device_ops_data_read_cmd_t cmd_;
  device_ops_api::device_ops_data_read_rsp_t rsp_;
  uint32_t expStatus_;
  CmdStatus cmdStatus_ = CmdStatus::CMD_RSP_NOT_RECEIVED;
};

class DmaWriteListCmd : public IDevOpsApiCmd {
public:
  std::byte* getCmdPtr() override;
  size_t getCmdSize() override;
  CmdTag getCmdTagId() override;
  CmdType whoAmI() override;
  CmdStatus setRsp(const std::vector<std::byte>& rsp) override;
  CmdStatus getCmdStatus() const override {
    return cmdStatus_;
  };
  uint32_t getRspStatusCode() const override;
  std::string printSummary() override;
  explicit DmaWriteListCmd(
    CmdTag tagId,
    const std::tuple<device_ops_api::cmd_flags_e /*flags*/, const device_ops_api::dma_write_node* /*list*/,
                     uint32_t /*nodeCount*/, device_ops_api::dev_ops_api_dma_response_e /*expStatus*/>&
      args);
  ~DmaWriteListCmd() final = default;
  bool isDma() const override {
    return true;
  }

private:
  device_ops_api::device_ops_dma_writelist_cmd_t* cmd_;
  std::vector<std::byte> cmdMem_;
  device_ops_api::device_ops_dma_writelist_rsp_t rsp_;
  uint32_t expStatus_;
  CmdStatus cmdStatus_ = CmdStatus::CMD_RSP_NOT_RECEIVED;
};

class DmaReadListCmd : public IDevOpsApiCmd {
public:
  std::byte* getCmdPtr() override;
  size_t getCmdSize() override;
  CmdTag getCmdTagId() override;
  CmdType whoAmI() override;
  CmdStatus setRsp(const std::vector<std::byte>& rsp) override;
  CmdStatus getCmdStatus() const override {
    return cmdStatus_;
  };
  uint32_t getRspStatusCode() const override;
  std::string printSummary() override;
  explicit DmaReadListCmd(
    CmdTag tagId, const std::tuple<device_ops_api::cmd_flags_e /*flags*/, const device_ops_api::dma_read_node* /*list*/,
                                   uint32_t /*nodeCount*/, device_ops_api::dev_ops_api_dma_response_e /*expStatus*/>&
                    args);
  ~DmaReadListCmd() final = default;
  bool isDma() const override {
    return true;
  }

private:
  device_ops_api::device_ops_dma_readlist_cmd_t* cmd_;
  std::vector<std::byte> cmdMem_;
  device_ops_api::device_ops_dma_readlist_rsp_t rsp_;
  uint32_t expStatus_;
  CmdStatus cmdStatus_ = CmdStatus::CMD_RSP_NOT_RECEIVED;
};

class KernelLaunchCmd : public IDevOpsApiCmd {
public:
  std::byte* getCmdPtr() override;
  size_t getCmdSize() override;
  CmdTag getCmdTagId() override;
  CmdType whoAmI() override;
  CmdStatus setRsp(const std::vector<std::byte>& rsp) override;
  CmdStatus getCmdStatus() const override {
    return cmdStatus_;
  };
  uint32_t getRspStatusCode() const override;
  std::string printSummary() override;
  explicit KernelLaunchCmd(
    CmdTag tagId,
    const std::tuple<device_ops_api::cmd_flags_e /*flags*/, uint64_t /*codeStartAddr*/, uint64_t /*ptrToArgs*/,
                     uint64_t /*exceptionBuffer*/, uint64_t /*shireMask*/, uint64_t /*traceBuffer*/,
                     const uint64_t* /*argumentPayload*/, uint32_t /*sizeOfArgPayload*/, std::string /*kernelName*/,
                     device_ops_api::dev_ops_api_kernel_launch_response_e /*expStatus*/>&
      args);
  ~KernelLaunchCmd() final = default;

private:
  device_ops_api::device_ops_kernel_launch_cmd_t* cmd_;
  std::vector<std::byte> cmdMem_;
  device_ops_api::device_ops_kernel_launch_rsp_t rsp_;
  std::string kernelName_;
  uint32_t expStatus_;
  CmdStatus cmdStatus_ = CmdStatus::CMD_RSP_NOT_RECEIVED;
};

class KernelAbortCmd : public IDevOpsApiCmd {
public:
  std::byte* getCmdPtr() override;
  size_t getCmdSize() override;
  CmdTag getCmdTagId() override;
  CmdType whoAmI() override;
  CmdStatus setRsp(const std::vector<std::byte>& rsp) override;
  CmdStatus getCmdStatus() const override {
    return cmdStatus_;
  };
  uint32_t getRspStatusCode() const override;
  std::string printSummary() override;
  explicit KernelAbortCmd(CmdTag tagId,
                          const std::tuple<device_ops_api::cmd_flags_e /*flags*/, CmdTag /*kernelToAbortTagId*/,
                                           device_ops_api::dev_ops_api_kernel_abort_response_e /*expStatus*/>&
                            args);
  ~KernelAbortCmd() final = default;

private:
  device_ops_api::device_ops_kernel_abort_cmd_t cmd_;
  device_ops_api::device_ops_kernel_abort_rsp_t rsp_;
  uint32_t expStatus_;
  CmdStatus cmdStatus_ = CmdStatus::CMD_RSP_NOT_RECEIVED;
};

class TraceRtConfigCmd : public IDevOpsApiCmd {
public:
  std::byte* getCmdPtr() override;
  size_t getCmdSize() override;
  CmdTag getCmdTagId() override;
  CmdType whoAmI() override;
  CmdStatus setRsp(const std::vector<std::byte>& rsp) override;
  CmdStatus getCmdStatus() const override {
    return cmdStatus_;
  };
  uint32_t getRspStatusCode() const override;
  std::string printSummary() override;
  explicit TraceRtConfigCmd(
    CmdTag tagId, const std::tuple<device_ops_api::cmd_flags_e /*flags*/, uint32_t /*shire_mask*/,
                                   uint32_t /*thread_mask*/, uint32_t /*event_mask*/, uint32_t /*filter_mask*/,
                                   device_ops_api::dev_ops_trace_rt_config_response_e /*expStatus*/>&
                    args);
  ~TraceRtConfigCmd() final = default;

private:
  device_ops_api::device_ops_trace_rt_config_cmd_t cmd_;
  device_ops_api::device_ops_trace_rt_config_rsp_t rsp_;
  uint32_t expStatus_;
  CmdStatus cmdStatus_ = CmdStatus::CMD_RSP_NOT_RECEIVED;
};

class TraceRtControlCmd : public IDevOpsApiCmd {
public:
  std::byte* getCmdPtr() override;
  size_t getCmdSize() override;
  CmdTag getCmdTagId() override;
  CmdType whoAmI() override;
  CmdStatus setRsp(const std::vector<std::byte>& rsp) override;
  CmdStatus getCmdStatus() const override {
    return cmdStatus_;
  };
  uint32_t getRspStatusCode() const override;
  std::string printSummary() override;
  explicit TraceRtControlCmd(
    CmdTag tagId, const std::tuple<device_ops_api::cmd_flags_e /*flags*/, device_ops_api::trace_rt_type_e /*rt_type*/,
                    uint32_t /*control*/, device_ops_api::dev_ops_trace_rt_control_response_e /*expStatus*/>&
                    args);
  ~TraceRtControlCmd() final = default;

private:
  device_ops_api::device_ops_trace_rt_control_cmd_t cmd_;
  device_ops_api::device_ops_trace_rt_control_rsp_t rsp_;
  uint32_t expStatus_;
  CmdStatus cmdStatus_ = CmdStatus::CMD_RSP_NOT_RECEIVED;
};

class CustomCmd : public IDevOpsApiCmd {
public:
  std::byte* getCmdPtr() override;
  size_t getCmdSize() override;
  CmdTag getCmdTagId() override;
  CmdType whoAmI() override;
  CmdStatus setRsp(const std::vector<std::byte>& rsp) override;
  CmdStatus getCmdStatus() const override {
    return cmdStatus_;
  };
  std::string printSummary() override;
  explicit CustomCmd(CmdTag tagId, const std::tuple<const std::byte* /*cmdPtr*/, size_t /*cmdSize*/>& args);
  ~CustomCmd() final = default;
  bool isDma() const override {
    auto* customCmd = reinterpret_cast<device_ops_api::cmd_header_t*>(cmd_);
    if (customCmd != nullptr) {
      return customCmd->cmd_hdr.msg_id == device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_CMD ||
             customCmd->cmd_hdr.msg_id == device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_CMD ||
             customCmd->cmd_hdr.msg_id == device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DMA_WRITELIST_CMD ||
             customCmd->cmd_hdr.msg_id == device_ops_api::DEV_OPS_API_MID_DEVICE_OPS_DMA_READLIST_CMD;
    }
  }

private:
  std::byte* cmd_;
  std::vector<std::byte> cmdMem_;
  std::byte* rsp_;
  std::vector<std::byte> rspMem_;
  CmdStatus cmdStatus_ = CmdStatus::CMD_RSP_NOT_RECEIVED;
};

} // namespace dev::dl_tests
