/*-------------------------------------------------------------------------
 * Copyright (C) 2021, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/
#include "Utils.h"
#include "runtime/Types.h"
#include <esperanto/device-apis/operations-api/device_ops_api_spec.h>
rt::DeviceErrorCode convert(int responseType, uint32_t responseCode) {
  switch (responseType) {
  case DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_RSP:
    switch (responseCode) {
    case DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_ERROR:
      return rt::DeviceErrorCode::KernelLaunchError;
    case DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_EXCEPTION:
      return rt::DeviceErrorCode::KernelLaunchException;
    case DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_SHIRES_NOT_READY:
      return rt::DeviceErrorCode::KernelLaunchShiresNotReady;
    case DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_HOST_ABORTED:
      return rt::DeviceErrorCode::KernelLaunchHostAborted;
    case DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_INVALID_ADDRESS:
      return rt::DeviceErrorCode::KernelLaunchInvalidAddress;
    case DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_TIMEOUT_HANG:
      return rt::DeviceErrorCode::KernelLaunchTimeoutHang;
    case DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_INVALID_ARGS_PAYLOAD_SIZE:
      return rt::DeviceErrorCode::KernelLaunchInvalidArgsPayloadSize;
    default:
      RT_LOG(WARNING) << "Unknown DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_RSP response code: " << responseCode;
      return rt::DeviceErrorCode::Unknown;
    }
  case DEV_OPS_API_MID_DEVICE_OPS_ABORT_RSP:
    switch (responseCode) {
    case DEV_OPS_API_ABORT_RESPONSE_ERROR:
      return rt::DeviceErrorCode::AbortError;
    case DEV_OPS_API_ABORT_RESPONSE_INVALID_TAG_ID:
      return rt::DeviceErrorCode::AbortInvalidTagId;
    default:
      RT_LOG(WARNING) << "Unknown DEV_OPS_API_MID_DEVICE_OPS_ABORT_RSP response code: " << responseCode;
      return rt::DeviceErrorCode::Unknown;
    }
  case DEV_OPS_API_MID_DEVICE_OPS_KERNEL_ABORT_RSP:
    switch (responseCode) {
    case DEV_OPS_API_KERNEL_ABORT_RESPONSE_ERROR:
      return rt::DeviceErrorCode::KernelAbortError;
    case DEV_OPS_API_KERNEL_ABORT_RESPONSE_INVALID_TAG_ID:
      return rt::DeviceErrorCode::KernelAbortInvalidTagId;
    case DEV_OPS_API_KERNEL_ABORT_RESPONSE_TIMEOUT_HANG:
      return rt::DeviceErrorCode::KernelAbortTimeoutHang;
    case DEV_OPS_API_KERNEL_ABORT_RESPONSE_HOST_ABORTED:
      return rt::DeviceErrorCode::KernelAbortHostAborted;
    default:
      RT_LOG(WARNING) << "Unknown DEV_OPS_API_MID_DEVICE_OPS_KERNEL_ABORT_RSP response code: " << responseCode;
      return rt::DeviceErrorCode::Unknown;
    }
  case DEV_OPS_API_MID_DEVICE_OPS_DATA_READ_RSP:
  case DEV_OPS_API_MID_DEVICE_OPS_DMA_READLIST_RSP:
  case DEV_OPS_API_MID_DEVICE_OPS_DATA_WRITE_RSP:
  case DEV_OPS_API_MID_DEVICE_OPS_DMA_WRITELIST_RSP:
    switch (responseCode) {
    case DEV_OPS_API_DMA_RESPONSE_UNKNOWN_ERROR:
      return rt::DeviceErrorCode::DmaUnknownError;
    case DEV_OPS_API_DMA_RESPONSE_TIMEOUT_IDLE_CHANNEL_UNAVAILABLE:
      return rt::DeviceErrorCode::DmaTimeoutIdleChannelUnavailable;
    /* todo: For now, DMA abort is being combined into one error type "DmaAborted".
    Shift to DmaHostAborted and DmaErrorAborted once glow uses these types.
    case DEV_OPS_API_DMA_RESPONSE_HOST_ABORTED:
      return rt::DeviceErrorCode::DmaHostAborted;
    case DEV_OPS_API_DMA_RESPONSE_ERROR_ABORTED:
      return rt::DeviceErrorCode::DmaErrorAborted; */
    case DEV_OPS_API_DMA_RESPONSE_HOST_ABORTED:
    case DEV_OPS_API_DMA_RESPONSE_ERROR_ABORTED:
      return rt::DeviceErrorCode::DmaAborted;
    case DEV_OPS_API_DMA_RESPONSE_TIMEOUT_HANG:
      return rt::DeviceErrorCode::DmaTimeoutHang;
    case DEV_OPS_API_DMA_RESPONSE_INVALID_ADDRESS:
      return rt::DeviceErrorCode::DmaInvalidAddress;
    case DEV_OPS_API_DMA_RESPONSE_INVALID_SIZE:
      return rt::DeviceErrorCode::DmaInvalidSize;
    default:
      RT_LOG(WARNING) << "Unknown DEV_OPS_API_DMA_RESPONSE response code: " << responseCode;
      return rt::DeviceErrorCode::Unknown;
    }
  case DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONFIG_RSP:
    switch (responseCode) {
    case DEV_OPS_TRACE_RT_CONFIG_RESPONSE_BAD_SHIRE_MASK:
      return rt::DeviceErrorCode::TraceConfigBadShireMask;
    case DEV_OPS_TRACE_RT_CONFIG_RESPONSE_BAD_THREAD_MASK:
      return rt::DeviceErrorCode::TraceConfigBadThreadMask;
    case DEV_OPS_TRACE_RT_CONFIG_RESPONSE_BAD_EVENT_MASK:
      return rt::DeviceErrorCode::TraceConfigBadEventMask;
    case DEV_OPS_TRACE_RT_CONFIG_RESPONSE_BAD_FILTER_MASK:
      return rt::DeviceErrorCode::TraceConfigBadFilterMask;
    case DEV_OPS_TRACE_RT_CONFIG_RESPONSE_RT_CONFIG_ERROR:
      return rt::DeviceErrorCode::TraceConfigError;
    default:
      RT_LOG(WARNING) << "Unknown DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONFIG_RSP response code: " << responseCode;
      return rt::DeviceErrorCode::Unknown;
    }
  case DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONTROL_RSP:
    switch (responseCode) {
    case DEV_OPS_TRACE_RT_CONTROL_RESPONSE_BAD_RT_TYPE:
      return rt::DeviceErrorCode::TraceControlBadRtType;
    case DEV_OPS_TRACE_RT_CONTROL_RESPONSE_BAD_CONTROL_MASK:
      return rt::DeviceErrorCode::TraceControlBadControlMask;
    case DEV_OPS_TRACE_RT_CONTROL_RESPONSE_CM_RT_CTRL_ERROR:
      return rt::DeviceErrorCode::TraceControlComputeMinionRtCtrlError;
    case DEV_OPS_TRACE_RT_CONTROL_RESPONSE_MM_RT_CTRL_ERROR:
      return rt::DeviceErrorCode::TraceControlMasterMinionRtCtrlError;
    default:
      RT_LOG(WARNING) << "Unknown DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONTROL_RSP response code: " << responseCode;
      return rt::DeviceErrorCode::Unknown;
    }
  case DEV_OPS_API_MID_DEVICE_OPS_API_COMPATIBILITY_RSP:
  case DEV_OPS_API_MID_DEVICE_OPS_FW_VERSION_RSP:
  case DEV_OPS_API_MID_DEVICE_OPS_ECHO_RSP:
  default:
    RT_LOG(WARNING) << "Unknown errorcodes for this response: " << responseCode;
    return rt::DeviceErrorCode::Unknown;
  }
}