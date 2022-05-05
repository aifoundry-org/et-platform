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
    case DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_UNEXPECTED_ERROR:
      return rt::DeviceErrorCode::KernelLaunchUnexpectedError;
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
    case DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_CM_IFACE_MULTICAST_FAILED:
      return rt::DeviceErrorCode::KernelLaunchCmIfaceMulticastFailed;
    case DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_CM_IFACE_UNICAST_FAILED:
      return rt::DeviceErrorCode::KernelLaunchCmIfaceUnicastFailed;
    case DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_SP_IFACE_RESET_FAILED:
      return rt::DeviceErrorCode::KernelLaunchSpIfaceResetFailed;
    case DEV_OPS_API_KERNEL_LAUNCH_RESPONSE_CW_MINIONS_BOOT_FAILED:
      return rt::DeviceErrorCode::KernelLaunchCwMinionsBootFailed;
    default:
      RT_LOG(WARNING) << "Unknown DEV_OPS_API_MID_DEVICE_OPS_KERNEL_LAUNCH_RSP response code: " << responseCode;
      return rt::DeviceErrorCode::Unknown;
    }
  case DEV_OPS_API_MID_DEVICE_OPS_ABORT_RSP:
    switch (responseCode) {
    case DEV_OPS_API_ABORT_RESPONSE_UNEXPECTED_ERROR:
      return rt::DeviceErrorCode::AbortUnexpectedError;
    case DEV_OPS_API_ABORT_RESPONSE_INVALID_TAG_ID:
      return rt::DeviceErrorCode::AbortInvalidTagId;
    default:
      RT_LOG(WARNING) << "Unknown DEV_OPS_API_MID_DEVICE_OPS_ABORT_RSP response code: " << responseCode;
      return rt::DeviceErrorCode::Unknown;
    }
  case DEV_OPS_API_MID_DEVICE_OPS_CM_RESET_RSP:
    switch (responseCode) {
    case DEV_OPS_API_CM_RESET_RESPONSE_UNEXPECTED_ERROR:
      return rt::DeviceErrorCode::CmResetUnexpectedError;
    case DEV_OPS_API_CM_RESET_RESPONSE_INVALID_SHIRE_MASK:
      return rt::DeviceErrorCode::CmResetInvalidShireMask;
    case DEV_OPS_API_CM_RESET_RESPONSE_CM_RESET_FAILED:
      return rt::DeviceErrorCode::CmResetFailed;
    default:
      RT_LOG(WARNING) << "Unknown DEV_OPS_API_MID_DEVICE_OPS_CM_RESET_RSP response code: " << responseCode;
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
    case DEV_OPS_API_DMA_RESPONSE_UNEXPECTED_ERROR:
      return rt::DeviceErrorCode::DmaUnexpectedError;
    case DEV_OPS_API_DMA_RESPONSE_HOST_ABORTED:
      return rt::DeviceErrorCode::DmaHostAborted;
    case DEV_OPS_API_DMA_RESPONSE_ERROR_ABORTED:
      return rt::DeviceErrorCode::DmaErrorAborted;
    case DEV_OPS_API_DMA_RESPONSE_INVALID_ADDRESS:
      return rt::DeviceErrorCode::DmaInvalidAddress;
    case DEV_OPS_API_DMA_RESPONSE_INVALID_SIZE:
      return rt::DeviceErrorCode::DmaInvalidSize;
    case DEV_OPS_API_DMA_RESPONSE_CM_IFACE_MULTICAST_FAILED:
      return rt::DeviceErrorCode::DmaCmIfaceMulticastFailed;
    case DEV_OPS_API_DMA_RESPONSE_DRIVER_DATA_CONFIG_FAILED:
      return rt::DeviceErrorCode::DmaDriverDataConfigFailed;
    case DEV_OPS_API_DMA_RESPONSE_DRIVER_LINK_CONFIG_FAILED:
      return rt::DeviceErrorCode::DmaDriverLinkConfigFailed;
    case DEV_OPS_API_DMA_RESPONSE_DRIVER_CHAN_START_FAILED:
      return rt::DeviceErrorCode::DmaDriverChanStartFailed;
    case DEV_OPS_API_DMA_RESPONSE_DRIVER_ABORT_FAILED:
      return rt::DeviceErrorCode::DmaDriverAbortFailed;

    default:
      RT_LOG(WARNING) << "Unknown DEV_OPS_API_DMA_RESPONSE response code: " << responseCode;
      return rt::DeviceErrorCode::Unknown;
    }
  case DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONFIG_RSP:
    switch (responseCode) {
    case DEV_OPS_TRACE_RT_CONFIG_RESPONSE_UNEXPECTED_ERROR:
      return rt::DeviceErrorCode::TraceConfigUnexpectedError;
    case DEV_OPS_TRACE_RT_CONFIG_RESPONSE_BAD_SHIRE_MASK:
      return rt::DeviceErrorCode::TraceConfigBadShireMask;
    case DEV_OPS_TRACE_RT_CONFIG_RESPONSE_BAD_THREAD_MASK:
      return rt::DeviceErrorCode::TraceConfigBadThreadMask;
    case DEV_OPS_TRACE_RT_CONFIG_RESPONSE_BAD_EVENT_MASK:
      return rt::DeviceErrorCode::TraceConfigBadEventMask;
    case DEV_OPS_TRACE_RT_CONFIG_RESPONSE_BAD_FILTER_MASK:
      return rt::DeviceErrorCode::TraceConfigBadFilterMask;
    case DEV_OPS_TRACE_RT_CONFIG_RESPONSE_HOST_ABORTED:
      return rt::DeviceErrorCode::TraceConfigHostAborted;
    case DEV_OPS_TRACE_RT_CONFIG_RESPONSE_CM_TRACE_CONFIG_FAILED:
      return rt::DeviceErrorCode::TraceConfigCmFailed;
    case DEV_OPS_TRACE_RT_CONFIG_RESPONSE_MM_TRACE_CONFIG_FAILED:
      return rt::DeviceErrorCode::TraceConfigMmFailed;
    case DEV_OPS_TRACE_RT_CONFIG_RESPONSE_INVALID_TRACE_CONFIG_INFO:
      return rt::DeviceErrorCode::TraceConfigInvalidConfig;

    default:
      RT_LOG(WARNING) << "Unknown DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONFIG_RSP response code: " << responseCode;
      return rt::DeviceErrorCode::Unknown;
    }
  case DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONTROL_RSP:
    switch (responseCode) {
    case DEV_OPS_TRACE_RT_CONTROL_RESPONSE_UNEXPECTED_ERROR:
      return rt::DeviceErrorCode::TraceControlUnexpectedError;
    // uncomment this when https://esperantotech.atlassian.net/browse/SW-11009 is fixed
    /*case DEV_OPS_TRACE_RT_CONTROL_RESPONSE_BAD_RT_TYPE:
      return rt::DeviceErrorCode::TraceControlBadRtType;*/
    case DEV_OPS_TRACE_RT_CONTROL_RESPONSE_BAD_CONTROL_MASK:
      return rt::DeviceErrorCode::TraceControlBadControlMask;
    case DEV_OPS_TRACE_RT_CONTROL_RESPONSE_CM_RT_CTRL_ERROR:
      return rt::DeviceErrorCode::TraceControlComputeMinionRtCtrlError;
    case DEV_OPS_TRACE_RT_CONTROL_RESPONSE_MM_RT_CTRL_ERROR:
      return rt::DeviceErrorCode::TraceControlMasterMinionRtCtrlError;
    case DEV_OPS_TRACE_RT_CONTROL_RESPONSE_HOST_ABORTED:
      return rt::DeviceErrorCode::TraceControlHostAborted;
    case DEV_OPS_TRACE_RT_CONTROL_RESPONSE_CM_IFACE_MULTICAST_FAILED:
      return rt::DeviceErrorCode::TraceControlCmIfaceMulticastFailed;
    default:
      RT_LOG(WARNING) << "Unknown DEV_OPS_API_MID_DEVICE_OPS_TRACE_RT_CONTROL_RSP response code: " << responseCode;
      return rt::DeviceErrorCode::Unknown;
    }
  case DEV_OPS_API_MID_DEVICE_OPS_API_COMPATIBILITY_RSP:
    switch (responseCode) {
    case DEV_OPS_API_COMPATIBILITY_RESPONSE_UNEXPECTED_ERROR:
      return rt::DeviceErrorCode::ApiCompatibilityUnexpectedError;
    case DEV_OPS_API_COMPATIBILITY_RESPONSE_HOST_ABORTED:
      return rt::DeviceErrorCode::ApiCompatibilityHostAborted;
    default:
      RT_LOG(WARNING) << "Unknown DEV_OPS_API_MID_DEVICE_OPS_API_COMPATIBILITY_RSP response code: " << responseCode;
      return rt::DeviceErrorCode::Unknown;
    }
  case DEV_OPS_API_MID_DEVICE_OPS_FW_VERSION_RSP:
    switch (responseCode) {
    case DEV_OPS_API_FW_VERSION_RESPONSE_UNEXPECTED_ERROR:
      return rt::DeviceErrorCode::FirmwareVersionUnexpectedError;
    case DEV_OPS_API_FW_VERSION_RESPONSE_BAD_FW_TYPE:
      return rt::DeviceErrorCode::FirmwareVersionBadFwType;
    case DEV_OPS_API_FW_VERSION_RESPONSE_NOT_AVAILABLE:
      return rt::DeviceErrorCode::FirmwareVersionNotAvailable;
    case DEV_OPS_API_FW_VERSION_RESPONSE_HOST_ABORTED:
      return rt::DeviceErrorCode::FirmwareVersionHostAborted;
    default:
      RT_LOG(WARNING) << "Unknown DEV_OPS_API_MID_DEVICE_OPS_API_COMPATIBILITY_RSP response code: " << responseCode;
      return rt::DeviceErrorCode::Unknown;
    }
  case DEV_OPS_API_MID_DEVICE_OPS_ECHO_RSP:
    switch (responseCode) {
    case DEV_OPS_API_ECHO_RESPONSE_HOST_ABORTED:
      return rt::DeviceErrorCode::EchoHostAborted;
    default:
      RT_LOG(WARNING) << "Unknown DEV_OPS_API_MID_DEVICE_OPS_API_COMPATIBILITY_RSP response code: " << responseCode;
      return rt::DeviceErrorCode::Unknown;
    }
  case DEV_OPS_API_MID_DEVICE_OPS_DEVICE_FW_ERROR:
    switch (responseCode) {
    case DEV_OPS_API_ERROR_TYPE_UNSUPPORTED_COMMAND:
      return rt::DeviceErrorCode::ErrorTypeUnsupportedCommand;
    case DEV_OPS_API_ERROR_TYPE_CM_SMODE_RT_EXCEPTION:
      return rt::DeviceErrorCode::ErrorTypeCmSmodeRtException;
    case DEV_OPS_API_ERROR_TYPE_CM_SMODE_RT_HANG:
      return rt::DeviceErrorCode::ErrorTypeCmSmodeRtHang;
    default:
      RT_LOG(WARNING) << "Unknown DEV_OPS_API_MID_DEVICE_OPS_DEVICE_FW_ERROR response code: " << responseCode;
      return rt::DeviceErrorCode::Unknown;
    }
  default:
    RT_LOG(WARNING) << "Unknown errorcodes for this response: " << responseCode;
    return rt::DeviceErrorCode::Unknown;
  }
}
