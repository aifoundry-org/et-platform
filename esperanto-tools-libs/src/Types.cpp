/*-------------------------------------------------------------------------
 * Copyright (C) 2021, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/
#include "runtime/Types.h"
#include "Utils.h"
#include <assert.h>
#include <sstream>
using namespace rt;

std::string StreamError::getString() const {
  std::stringstream ss;
  ss << "Error code: " << std::to_string(errorCode_);
  if (errorContext_) {
    const auto& values = errorContext_.value();
    ss << "\n Num contexts: " << values.size();
    ss << std::hex;
    for (auto i = 0UL, count = values.size(); i < count; ++i) {
      const auto& ctx = values[i];
      ss << "{\n\ttype: ";
      switch (ctx.type_) {
      case 0:
        ss << "compute hang";
        break;
      case 1:
        ss << "U-mode exception";
        break;
      case 2:
        ss << "system abort";
        break;
      case 3:
        ss << "self abort";
        break;
      case 4:
        ss << "kernel execution error";
        break;
      default:
        ss << " unknown";
        break;
      }
      ss << "\n\tcycle: 0x" << ctx.cycle_;
      ss << "\n\thartId: 0x" << ctx.hartId_;
      ss << "\n\texception program counter: 0x" << ctx.mepc_;
      ss << "\n\tstatus register: 0x" << ctx.mstatus_;
      ss << "\n\tbad address or instruction: 0x" << ctx.mtval_;
      ss << "\n\ttrap cause: 0x" << ctx.mcause_;
      ss << "\n\tuser defined error: 0x" << ctx.userDefinedError_;
      ss << "\n\tGPR registers (x1-x31): [";
      for (auto j = 0UL; j < ctx.gpr_.size(); ++j) {
        ss << "\n\t\t0x" << ctx.gpr_[j];
      }
      ss << "]\n}";
    }
  }
  return ss.str();
}

std::string std::to_string(rt::DeviceErrorCode e) {
  using namespace rt;
  switch (e) {
  case DeviceErrorCode::KernelLaunchError:
    return "KernelLaunchError";
  case DeviceErrorCode::KernelLaunchException:
    return "KernelLaunchException";
  case DeviceErrorCode::KernelLaunchShiresNotReady:
    return "KernelLaunchShiresNotReady";
  case DeviceErrorCode::KernelLaunchHostAborted:
    return "KernelLaunchHostAborted";
  case DeviceErrorCode::KernelLaunchInvalidAddress:
    return "KernelLaunchInvalidAddress";
  case DeviceErrorCode::KernelLaunchTimeoutHang:
    return "KernelLaunchTimeoutHang";
  case DeviceErrorCode::KernelLaunchInvalidArgsPayloadSize:
    return "KernelLaunchInvalidArgsPayloadSize";
  case DeviceErrorCode::KernelAbortError:
    return "KernelAbortError";
  case DeviceErrorCode::KernelAbortInvalidTagId:
    return "KernelAbortInvalidTagId";
  case DeviceErrorCode::KernelAbortTimeoutHang:
    return "KernelAbortTimeoutHang";
  case DeviceErrorCode::DmaError:
    return "DmaError";
  case DeviceErrorCode::DmaTimeoutIdleChannelUnavailable:
    return "DmaTimeoutIdleChannelUnavailable";
  case DeviceErrorCode::DmaAborted:
    return "DmaAborted";
  case DeviceErrorCode::DmaTimeoutHang:
    return "DmaTimeoutHang";
  case DeviceErrorCode::DmaInvalidAddress:
    return "DmaInvalidAddress";
  case DeviceErrorCode::DmaInvalidSize:
    return "DmaInvalidSize";
  case DeviceErrorCode::TraceConfigError:
    return "TraceConfigError";
  case DeviceErrorCode::TraceConfigBadShireMask:
    return "TraceConfigBadShireMask";
  case DeviceErrorCode::TraceConfigBadThreadMask:
    return "TraceConfigBadThreadMask";
  case DeviceErrorCode::TraceConfigBadEventMask:
    return "TraceConfigBadEventMask";
  case DeviceErrorCode::TraceConfigBadFilterMask:
    return "TraceConfigBadFilterMask";
  case DeviceErrorCode::TraceControlBadRtType:
    return "TraceControlBadRtType";
  case DeviceErrorCode::TraceControlBadControlMask:
    return "TraceControlBadControlMask";
  case DeviceErrorCode::TraceControlComputeMinionRtCtrlError:
    return "TraceControlComputeMinionRtCtrlError";
  case DeviceErrorCode::TraceControlMasterMinionRtCtrlError:
    return "TraceControlMasterMinionRtCtrlError";
  case rt::DeviceErrorCode::Unknown:
    RT_LOG(WARNING) << "Unknown error code";
    return "Unknown error code";
  default:
    RT_LOG(WARNING) << "Not stringized error code. Consider adding it to " __FILE__;
    return "Not stringized error code: " + std::to_string(static_cast<int>(e));
  }
}