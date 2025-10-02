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
  ss << "Device Id: " << static_cast<int>(device_);
  if (stream_) {
    ss << "Stream Id: " << static_cast<int>(*stream_);
  }
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
      case 5:
        ss << "kernel execution tensor error";
        break;
      case 6:
        ss << "kernel execution bus error";
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
      for (auto j = 0UL; j < (ctx.gpr_.size() - 1); ++j) {
        ss << "\n\t\t0x" << ctx.gpr_[j];
      }
      ss << "]\n}";
    }
  }
  if (cmShireMask_) {
    ss << "\n Shire mask: 0x" << *cmShireMask_;
  }
  return ss.str();
}

#define STR_DEVICE_ERROR_CODE(CODE)                                                                                    \
  case DeviceErrorCode::CODE: {                                                                                        \
    return #CODE;                                                                                                      \
  }
std::string std::to_string(rt::DeviceErrorCode e) {
  using namespace rt;
  switch (e) {
    STR_DEVICE_ERROR_CODE(KernelLaunchUnexpectedError)
    STR_DEVICE_ERROR_CODE(KernelLaunchException)
    STR_DEVICE_ERROR_CODE(KernelLaunchShiresNotReady)
    STR_DEVICE_ERROR_CODE(KernelLaunchHostAborted)
    STR_DEVICE_ERROR_CODE(KernelLaunchInvalidAddress)
    STR_DEVICE_ERROR_CODE(KernelLaunchTimeoutHang)
    STR_DEVICE_ERROR_CODE(KernelLaunchInvalidArgsPayloadSize)
    STR_DEVICE_ERROR_CODE(KernelLaunchCmIfaceMulticastFailed)
    STR_DEVICE_ERROR_CODE(KernelLaunchCmIfaceUnicastFailed)
    STR_DEVICE_ERROR_CODE(KernelLaunchSpIfaceResetFailed)
    STR_DEVICE_ERROR_CODE(KernelLaunchCwMinionsBootFailed)
    STR_DEVICE_ERROR_CODE(KernelLaunchInvalidArgsInvalidShireMask)
    STR_DEVICE_ERROR_CODE(KernelLaunchResponseUserError)

    STR_DEVICE_ERROR_CODE(AbortUnexpectedError)
    STR_DEVICE_ERROR_CODE(AbortInvalidTagId)

    STR_DEVICE_ERROR_CODE(CmResetUnexpectedError)
    STR_DEVICE_ERROR_CODE(CmResetInvalidShireMask)
    STR_DEVICE_ERROR_CODE(CmResetFailed)

    STR_DEVICE_ERROR_CODE(KernelAbortError)
    STR_DEVICE_ERROR_CODE(KernelAbortInvalidTagId)
    STR_DEVICE_ERROR_CODE(KernelAbortTimeoutHang)
    STR_DEVICE_ERROR_CODE(KernelAbortHostAborted)

    STR_DEVICE_ERROR_CODE(DmaUnexpectedError)
    STR_DEVICE_ERROR_CODE(DmaHostAborted)
    STR_DEVICE_ERROR_CODE(DmaErrorAborted)
    STR_DEVICE_ERROR_CODE(DmaInvalidAddress)
    STR_DEVICE_ERROR_CODE(DmaInvalidSize)
    STR_DEVICE_ERROR_CODE(DmaCmIfaceMulticastFailed)
    STR_DEVICE_ERROR_CODE(DmaDriverDataConfigFailed)
    STR_DEVICE_ERROR_CODE(DmaDriverLinkConfigFailed)
    STR_DEVICE_ERROR_CODE(DmaDriverChanStartFailed)
    STR_DEVICE_ERROR_CODE(DmaDriverAbortFailed)

    STR_DEVICE_ERROR_CODE(TraceConfigUnexpectedError)
    STR_DEVICE_ERROR_CODE(TraceConfigBadShireMask)
    STR_DEVICE_ERROR_CODE(TraceConfigBadThreadMask)
    STR_DEVICE_ERROR_CODE(TraceConfigBadEventMask)
    STR_DEVICE_ERROR_CODE(TraceConfigBadFilterMask)
    STR_DEVICE_ERROR_CODE(TraceConfigHostAborted)
    STR_DEVICE_ERROR_CODE(TraceConfigCmFailed)
    STR_DEVICE_ERROR_CODE(TraceConfigMmFailed)
    STR_DEVICE_ERROR_CODE(TraceConfigInvalidConfig)

    STR_DEVICE_ERROR_CODE(TraceControlUnexpectedError)
    STR_DEVICE_ERROR_CODE(TraceControlBadRtType)
    STR_DEVICE_ERROR_CODE(TraceControlBadControlMask)
    STR_DEVICE_ERROR_CODE(TraceControlComputeMinionRtCtrlError)
    STR_DEVICE_ERROR_CODE(TraceControlMasterMinionRtCtrlError)
    STR_DEVICE_ERROR_CODE(TraceControlHostAborted)
    STR_DEVICE_ERROR_CODE(TraceControlCmIfaceMulticastFailed)

    STR_DEVICE_ERROR_CODE(ApiCompatibilityUnexpectedError)
    STR_DEVICE_ERROR_CODE(ApiCompatibilityIncompatibleMajor)
    STR_DEVICE_ERROR_CODE(ApiCompatibilityIncompatibleMinor)
    STR_DEVICE_ERROR_CODE(ApiCompatibilityIncompatiblePatch)
    STR_DEVICE_ERROR_CODE(ApiCompatibilityBadFirmwareType)
    STR_DEVICE_ERROR_CODE(ApiCompatibilityHostAborted)

    STR_DEVICE_ERROR_CODE(FirmwareVersionUnexpectedError)
    STR_DEVICE_ERROR_CODE(FirmwareVersionBadFwType)
    STR_DEVICE_ERROR_CODE(FirmwareVersionNotAvailable)
    STR_DEVICE_ERROR_CODE(FirmwareVersionHostAborted)

    STR_DEVICE_ERROR_CODE(EchoHostAborted)

    STR_DEVICE_ERROR_CODE(ErrorTypeUnsupportedCommand)
    STR_DEVICE_ERROR_CODE(ErrorTypeCmSmodeRtException)
    STR_DEVICE_ERROR_CODE(ErrorTypeCmSmodeRtHang)

    STR_DEVICE_ERROR_CODE(Unknown)

  default:
    RT_LOG(WARNING) << "Not stringized error code. Consider adding it to " __FILE__;
    return "Not stringized error code: " + std::to_string(static_cast<int>(e));
  }
}
