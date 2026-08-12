/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/

#include <nanobind/nanobind.h>
#include <nanobind/stl/array.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include <runtime/Types.h>

namespace nb = nanobind;

void bind_types(nb::module_& m) {
  // Register exception
  nb::exception<rt::Exception>(m, "RuntimeError");

  // Handle types - use automatic conversion via underlying type
  // EventId, StreamId, DeviceId, KernelId are enum classes that nanobind
  // will automatically convert to/from their underlying integer types

  // Options struct
  nb::class_<rt::Options>(m, "Options", "Runtime instantiation options")
    .def(nb::init<>())
    .def_rw("check_memcpy_device_operations", &rt::Options::checkMemcpyDeviceOperations_,
            "Validate memcpy operations")
    .def_rw("check_device_api_version", &rt::Options::checkDeviceApiVersion_,
            "Check device API compatibility");

  m.def("get_default_options", &rt::getDefaultOptions, "Get default runtime options");

  // DeviceErrorCode enum
  nb::enum_<rt::DeviceErrorCode>(m, "DeviceErrorCode", "Device error codes")
    .value("KERNEL_LAUNCH_UNEXPECTED_ERROR", rt::DeviceErrorCode::KernelLaunchUnexpectedError)
    .value("KERNEL_LAUNCH_EXCEPTION", rt::DeviceErrorCode::KernelLaunchException)
    .value("KERNEL_LAUNCH_SHIRES_NOT_READY", rt::DeviceErrorCode::KernelLaunchShiresNotReady)
    .value("KERNEL_LAUNCH_HOST_ABORTED", rt::DeviceErrorCode::KernelLaunchHostAborted)
    .value("KERNEL_LAUNCH_INVALID_ADDRESS", rt::DeviceErrorCode::KernelLaunchInvalidAddress)
    .value("KERNEL_LAUNCH_TIMEOUT_HANG", rt::DeviceErrorCode::KernelLaunchTimeoutHang)
    .value("KERNEL_LAUNCH_INVALID_ARGS_PAYLOAD_SIZE", rt::DeviceErrorCode::KernelLaunchInvalidArgsPayloadSize)
    .value("KERNEL_LAUNCH_CM_IFACE_MULTICAST_FAILED", rt::DeviceErrorCode::KernelLaunchCmIfaceMulticastFailed)
    .value("KERNEL_LAUNCH_CM_IFACE_UNICAST_FAILED", rt::DeviceErrorCode::KernelLaunchCmIfaceUnicastFailed)
    .value("KERNEL_LAUNCH_SP_IFACE_RESET_FAILED", rt::DeviceErrorCode::KernelLaunchSpIfaceResetFailed)
    .value("KERNEL_LAUNCH_CW_MINIONS_BOOT_FAILED", rt::DeviceErrorCode::KernelLaunchCwMinionsBootFailed)
    .value("KERNEL_LAUNCH_INVALID_ARGS_INVALID_SHIRE_MASK", rt::DeviceErrorCode::KernelLaunchInvalidArgsInvalidShireMask)
    .value("KERNEL_LAUNCH_RESPONSE_USER_ERROR", rt::DeviceErrorCode::KernelLaunchResponseUserError)
    .value("KERNEL_ABORT_ERROR", rt::DeviceErrorCode::KernelAbortError)
    .value("KERNEL_ABORT_INVALID_TAG_ID", rt::DeviceErrorCode::KernelAbortInvalidTagId)
    .value("KERNEL_ABORT_TIMEOUT_HANG", rt::DeviceErrorCode::KernelAbortTimeoutHang)
    .value("KERNEL_ABORT_HOST_ABORTED", rt::DeviceErrorCode::KernelAbortHostAborted)
    .value("ABORT_UNEXPECTED_ERROR", rt::DeviceErrorCode::AbortUnexpectedError)
    .value("ABORT_INVALID_TAG_ID", rt::DeviceErrorCode::AbortInvalidTagId)
    .value("DMA_UNEXPECTED_ERROR", rt::DeviceErrorCode::DmaUnexpectedError)
    .value("DMA_HOST_ABORTED", rt::DeviceErrorCode::DmaHostAborted)
    .value("DMA_ERROR_ABORTED", rt::DeviceErrorCode::DmaErrorAborted)
    .value("DMA_INVALID_ADDRESS", rt::DeviceErrorCode::DmaInvalidAddress)
    .value("DMA_INVALID_SIZE", rt::DeviceErrorCode::DmaInvalidSize)
    .value("DMA_CM_IFACE_MULTICAST_FAILED", rt::DeviceErrorCode::DmaCmIfaceMulticastFailed)
    .value("DMA_DRIVER_DATA_CONFIG_FAILED", rt::DeviceErrorCode::DmaDriverDataConfigFailed)
    .value("DMA_DRIVER_LINK_CONFIG_FAILED", rt::DeviceErrorCode::DmaDriverLinkConfigFailed)
    .value("DMA_DRIVER_CHAN_START_FAILED", rt::DeviceErrorCode::DmaDriverChanStartFailed)
    .value("DMA_DRIVER_ABORT_FAILED", rt::DeviceErrorCode::DmaDriverAbortFailed)
    .value("TRACE_CONFIG_UNEXPECTED_ERROR", rt::DeviceErrorCode::TraceConfigUnexpectedError)
    .value("TRACE_CONFIG_BAD_SHIRE_MASK", rt::DeviceErrorCode::TraceConfigBadShireMask)
    .value("TRACE_CONFIG_BAD_THREAD_MASK", rt::DeviceErrorCode::TraceConfigBadThreadMask)
    .value("TRACE_CONFIG_BAD_EVENT_MASK", rt::DeviceErrorCode::TraceConfigBadEventMask)
    .value("TRACE_CONFIG_BAD_FILTER_MASK", rt::DeviceErrorCode::TraceConfigBadFilterMask)
    .value("TRACE_CONFIG_HOST_ABORTED", rt::DeviceErrorCode::TraceConfigHostAborted)
    .value("TRACE_CONFIG_CM_FAILED", rt::DeviceErrorCode::TraceConfigCmFailed)
    .value("TRACE_CONFIG_MM_FAILED", rt::DeviceErrorCode::TraceConfigMmFailed)
    .value("TRACE_CONFIG_INVALID_CONFIG", rt::DeviceErrorCode::TraceConfigInvalidConfig)
    .value("TRACE_CONTROL_UNEXPECTED_ERROR", rt::DeviceErrorCode::TraceControlUnexpectedError)
    .value("TRACE_CONTROL_BAD_RT_TYPE", rt::DeviceErrorCode::TraceControlBadRtType)
    .value("TRACE_CONTROL_BAD_CONTROL_MASK", rt::DeviceErrorCode::TraceControlBadControlMask)
    .value("TRACE_CONTROL_COMPUTE_MINION_RT_CTRL_ERROR", rt::DeviceErrorCode::TraceControlComputeMinionRtCtrlError)
    .value("TRACE_CONTROL_MASTER_MINION_RT_CTRL_ERROR", rt::DeviceErrorCode::TraceControlMasterMinionRtCtrlError)
    .value("TRACE_CONTROL_HOST_ABORTED", rt::DeviceErrorCode::TraceControlHostAborted)
    .value("TRACE_CONTROL_CM_IFACE_MULTICAST_FAILED", rt::DeviceErrorCode::TraceControlCmIfaceMulticastFailed)
    .value("API_COMPATIBILITY_UNEXPECTED_ERROR", rt::DeviceErrorCode::ApiCompatibilityUnexpectedError)
    .value("API_COMPATIBILITY_INCOMPATIBLE_MAJOR", rt::DeviceErrorCode::ApiCompatibilityIncompatibleMajor)
    .value("API_COMPATIBILITY_INCOMPATIBLE_MINOR", rt::DeviceErrorCode::ApiCompatibilityIncompatibleMinor)
    .value("API_COMPATIBILITY_INCOMPATIBLE_PATCH", rt::DeviceErrorCode::ApiCompatibilityIncompatiblePatch)
    .value("API_COMPATIBILITY_BAD_FIRMWARE_TYPE", rt::DeviceErrorCode::ApiCompatibilityBadFirmwareType)
    .value("API_COMPATIBILITY_HOST_ABORTED", rt::DeviceErrorCode::ApiCompatibilityHostAborted)
    .value("FIRMWARE_VERSION_UNEXPECTED_ERROR", rt::DeviceErrorCode::FirmwareVersionUnexpectedError)
    .value("FIRMWARE_VERSION_BAD_FW_TYPE", rt::DeviceErrorCode::FirmwareVersionBadFwType)
    .value("FIRMWARE_VERSION_NOT_AVAILABLE", rt::DeviceErrorCode::FirmwareVersionNotAvailable)
    .value("FIRMWARE_VERSION_HOST_ABORTED", rt::DeviceErrorCode::FirmwareVersionHostAborted)
    .value("ECHO_HOST_ABORTED", rt::DeviceErrorCode::EchoHostAborted)
    .value("CM_RESET_UNEXPECTED_ERROR", rt::DeviceErrorCode::CmResetUnexpectedError)
    .value("CM_RESET_INVALID_SHIRE_MASK", rt::DeviceErrorCode::CmResetInvalidShireMask)
    .value("CM_RESET_FAILED", rt::DeviceErrorCode::CmResetFailed)
    .value("ERROR_TYPE_UNSUPPORTED_COMMAND", rt::DeviceErrorCode::ErrorTypeUnsupportedCommand)
    .value("ERROR_TYPE_CM_SMODE_RT_EXCEPTION", rt::DeviceErrorCode::ErrorTypeCmSmodeRtException)
    .value("ERROR_TYPE_CM_SMODE_RT_HANG", rt::DeviceErrorCode::ErrorTypeCmSmodeRtHang)
    .value("UNKNOWN", rt::DeviceErrorCode::Unknown);

  // ErrorContext struct
  nb::class_<rt::ErrorContext>(m, "ErrorContext", "Device kernel error context")
    .def(nb::init<>())
    .def_rw("type", &rt::ErrorContext::type_, "Error type")
    .def_rw("cycle", &rt::ErrorContext::cycle_, "Cycle count when error occurred")
    .def_rw("hart_id", &rt::ErrorContext::hartId_, "Hart thread ID")
    .def_rw("mepc", &rt::ErrorContext::mepc_, "Exception program counter")
    .def_rw("mstatus", &rt::ErrorContext::mstatus_, "Status register")
    .def_rw("mtval", &rt::ErrorContext::mtval_, "Bad address or instruction")
    .def_rw("mcause", &rt::ErrorContext::mcause_, "Trap cause")
    .def_rw("user_defined_error", &rt::ErrorContext::userDefinedError_, "User-defined error code")
    .def_rw("gpr", &rt::ErrorContext::gpr_, "General purpose registers x1-x31");

  // StreamError struct
  nb::class_<rt::StreamError>(m, "StreamError", "Stream error information")
    .def(nb::init<>())
    // Use __init__ method with lambda to handle default enum class argument safely
    .def("__init__", [](rt::StreamError* self, rt::DeviceErrorCode error_code, std::optional<int> device) {
           new (self) rt::StreamError(error_code, rt::DeviceId{device.value_or(-1)});
         },
         nb::arg("error_code"),
         nb::arg("device") = nb::none())
    .def_rw("error_code", &rt::StreamError::errorCode_, "Device error code")
    .def_rw("device", &rt::StreamError::device_, "Device where error originated")
    .def_rw("stream", &rt::StreamError::stream_, "Stream where error occurred")
    .def_rw("cm_shire_mask", &rt::StreamError::cmShireMask_, "Offending shire mask (if applicable)")
    .def_rw("error_context", &rt::StreamError::errorContext_, "Error context details")
    .def("get_string", &rt::StreamError::getString, "Get string representation")
    .def("__str__", &rt::StreamError::getString)
    .def("__repr__", [](const rt::StreamError& e) {
      return "StreamError(" + e.getString() + ")";
    });

  // LoadCodeResult struct (NOTE: load_code() returns tuple instead, this is for completeness)
  nb::class_<rt::LoadCodeResult>(m, "LoadCodeResult", "Result of loading kernel code")
    .def(nb::init<>())
    .def_prop_rw("event",
                 [](const rt::LoadCodeResult& self) -> int {
                   return static_cast<int>(self.event_);
                 },
                 [](rt::LoadCodeResult& self, int event) {
                   self.event_ = rt::EventId{static_cast<uint16_t>(event)};
                 },
                 "Event to wait for load completion")
    .def_prop_rw("kernel",
                 [](const rt::LoadCodeResult& self) -> int {
                   return static_cast<int>(self.kernel_);
                 },
                 [](rt::LoadCodeResult& self, int kernel) {
                   self.kernel_ = rt::KernelId{kernel};
                 },
                 "Kernel ID for launching")
    .def_prop_rw("load_address",
                 [](const rt::LoadCodeResult& self) -> uintptr_t {
                   return reinterpret_cast<uintptr_t>(self.loadAddress_);
                 },
                 [](rt::LoadCodeResult& self, uintptr_t addr) {
                   self.loadAddress_ = reinterpret_cast<std::byte*>(addr);
                 },
                 "Device physical load address");

  // DeviceProperties struct
  nb::class_<rt::DeviceProperties>(m, "DeviceProperties", "Device hardware properties")
    .def(nb::init<>())
    .def_rw("frequency", &rt::DeviceProperties::frequency_, "Boot frequency in MHz")
    .def_rw("available_shires", &rt::DeviceProperties::availableShires_, "Shire count")
    .def_rw("memory_bandwidth", &rt::DeviceProperties::memoryBandwidth_, "Memory bandwidth in MB/s")
    .def_rw("memory_size", &rt::DeviceProperties::memorySize_, "Memory size in bytes")
    .def_rw("l3_size", &rt::DeviceProperties::l3Size_, "L3 size in bytes")
    .def_rw("l2_shire_size", &rt::DeviceProperties::l2shireSize_, "L2 shire size in bytes")
    .def_rw("l2_scratchpad_size", &rt::DeviceProperties::l2scratchpadSize_, "L2 scratchpad size in bytes")
    .def_rw("cache_line_size", &rt::DeviceProperties::cacheLineSize_, "Cache line size in bytes")
    .def_rw("l2_cache_banks", &rt::DeviceProperties::l2CacheBanks_, "L2 cache banks count")
    .def_rw("compute_minion_shire_mask", &rt::DeviceProperties::computeMinionShireMask_, "Compute minion shire mask")
    .def_rw("spare_compute_minion_shire_id", &rt::DeviceProperties::spareComputeMinionShireId_, "Spare compute minion shire ID")
    .def_rw("device_arch", &rt::DeviceProperties::deviceArch_, "Architecture revision")
    .def_rw("form_factor", &rt::DeviceProperties::formFactor_, "Form factor")
    .def_rw("tdp", &rt::DeviceProperties::tdp_, "TDP in watts")
    .def_rw("p2p_bitmap", &rt::DeviceProperties::p2pBitmap_, "P2P capabilities bitmap")
    .def_rw("local_scp_format0_base_address", &rt::DeviceProperties::localScpFormat0BaseAddress_)
    .def_rw("local_scp_format1_base_address", &rt::DeviceProperties::localScpFormat1BaseAddress_)
    .def_rw("local_dram_base_address", &rt::DeviceProperties::localDRAMBaseAddress_)
    .def_rw("on_pkg_scp_format2_base_address", &rt::DeviceProperties::onPkgScpFormat2BaseAddress_)
    .def_rw("on_pkg_dram_base_address", &rt::DeviceProperties::onPkgDRAMBaseAddress_)
    .def_rw("on_pkg_dram_interleaved_base_address", &rt::DeviceProperties::onPkgDRAMInterleavedBaseAddress_)
    .def_rw("local_dram_size", &rt::DeviceProperties::localDRAMSize_)
    .def_rw("minimum_address_alignment_bits", &rt::DeviceProperties::minimumAddressAlignmentBits_)
    .def_rw("num_chiplets", &rt::DeviceProperties::numChiplets_)
    .def_rw("local_scp_format0_shire_lsb", &rt::DeviceProperties::localScpFormat0ShireLSb_)
    .def_rw("local_scp_format0_shire_bits", &rt::DeviceProperties::localScpFormat0ShireBits_)
    .def_rw("local_scp_format0_local_shire", &rt::DeviceProperties::localScpFormat0LocalShire_)
    .def_rw("local_scp_format1_shire_lsb", &rt::DeviceProperties::localScpFormat1ShireLSb_)
    .def_rw("local_scp_format1_shire_bits", &rt::DeviceProperties::localScpFormat1ShireBits_)
    .def_rw("on_pkg_scp_format2_shire_lsb", &rt::DeviceProperties::onPkgScpFormat2ShireLSb_)
    .def_rw("on_pkg_scp_format2_shire_bits", &rt::DeviceProperties::onPkgScpFormat2ShireBits_)
    .def_rw("on_pkg_scp_format2_chiplet_lsb", &rt::DeviceProperties::onPkgScpFormat2ChipletLSb_)
    .def_rw("on_pkg_scp_format2_chiplet_bits", &rt::DeviceProperties::onPkgScpFormat2ChipletBits_)
    .def_rw("on_pkg_dram_chiplet_lsb", &rt::DeviceProperties::onPkgDRAMChipletLSb_)
    .def_rw("on_pkg_dram_chiplet_bits", &rt::DeviceProperties::onPkgDRAMChipletBits_)
    .def_rw("on_pkg_dram_interleaved_chiplet_lsb", &rt::DeviceProperties::onPkgDRAMInterleavedChipletLSb_)
    .def_rw("on_pkg_dram_interleaved_chiplet_bits", &rt::DeviceProperties::onPkgDRAMInterleavedChipletBits_);

  // DeviceProperties enums
  nb::enum_<rt::DeviceProperties::FormFactor>(m, "FormFactor", "Device form factor")
    .value("PCIE", rt::DeviceProperties::FormFactor::PCIE)
    .value("M2", rt::DeviceProperties::FormFactor::M2);

  nb::enum_<rt::DeviceProperties::ArchRevision>(m, "ArchRevision", "Architecture revision")
    .value("ETSOC1", rt::DeviceProperties::ArchRevision::ETSOC1)
    .value("PANTERO", rt::DeviceProperties::ArchRevision::PANTERO)
    .value("GEPARDO", rt::DeviceProperties::ArchRevision::GEPARDO)
    .value("UNKNOWN", rt::DeviceProperties::ArchRevision::UNKNOWN);

  // UserTrace struct (buffer_ is already uint64_t in the API)
  nb::class_<rt::UserTrace>(m, "UserTrace", "User trace configuration")
    .def(nb::init<>())
    .def_rw("buffer", &rt::UserTrace::buffer_, "Trace buffer base address")
    .def_rw("buffer_size", &rt::UserTrace::buffer_size_, "Trace buffer size")
    .def_rw("threshold", &rt::UserTrace::threshold_, "Free memory threshold")
    .def_rw("shire_mask", &rt::UserTrace::shireMask_, "Shire mask for trace capture")
    .def_rw("thread_mask", &rt::UserTrace::threadMask_, "Thread mask for trace capture")
    .def_rw("event_mask", &rt::UserTrace::eventMask_, "Event mask")
    .def_rw("filter_mask", &rt::UserTrace::filterMask_, "Filter mask");

  // DmaInfo struct
  nb::class_<rt::DmaInfo>(m, "DmaInfo", "DMA constraints information")
    .def(nb::init<>())
    .def_rw("max_element_size", &rt::DmaInfo::maxElementSize_, "Max bytes per DMA entry")
    .def_rw("max_element_count", &rt::DmaInfo::maxElementCount_, "Max DMA entries per command");

  // MemcpyList - NOT EXPOSED
  // The C++ MemcpyList API has buffer lifetime issues for host memory operations
  // and is not commonly used. We may add proper support later with automatic
  // lifetime management. For now, use single-operation memcpy functions.
  // If needed in the future, the binding would need to:
  // - Accept buffer objects (not integer pointers) for host memory
  // - Keep references to all buffers until the batch operation completes

  // KernelLaunchOptions
  nb::class_<rt::KernelLaunchOptions>(m, "KernelLaunchOptions", "Kernel launch configuration")
    .def(nb::init<>())
    .def("set_shire_mask", &rt::KernelLaunchOptions::setShireMask, nb::arg("shire_mask"),
         "Set which shires to execute on")
    .def("set_barrier", &rt::KernelLaunchOptions::setBarrier, nb::arg("barrier"),
         "Set barrier flag (wait for previous operations)")
    .def("set_flush_l3", &rt::KernelLaunchOptions::setFlushL3, nb::arg("flush_l3"),
         "Set L3 flush flag")
    .def("set_user_tracing", &rt::KernelLaunchOptions::setUserTracing,
         nb::arg("buffer"), nb::arg("buffer_size"), nb::arg("threshold"),
         nb::arg("shire_mask"), nb::arg("thread_mask"), nb::arg("event_mask"), nb::arg("filter_mask"),
         "Configure user tracing")
    .def("set_stack_config", [](rt::KernelLaunchOptions& self, uintptr_t base_address, size_t size) {
           self.setStackConfig(reinterpret_cast<std::byte*>(base_address), size);
         },
         nb::arg("base_address"), nb::arg("size"),
         "Configure kernel stack")
    .def("set_core_dump_file_path", &rt::KernelLaunchOptions::setCoreDumpFilePath,
         nb::arg("path"),
         "Set core dump file path");

}
