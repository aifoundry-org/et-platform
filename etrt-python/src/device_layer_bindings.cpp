/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/

#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/unique_ptr.h>
#include <nanobind/stl/vector.h>

#include <device-layer/IDeviceLayer.h>
#include <sw-sysemu/SysEmuOptions.h>

namespace nb = nanobind;

void bind_device_layer(nb::module_& m) {
  // SysEmuOptions struct
  nb::class_<emu::SysEmuOptions>(m, "SysEmuOptions", "System emulator configuration options")
    .def(nb::init<>())
    .def_rw("bootrom_trampoline_to_bl2_elf_path", &emu::SysEmuOptions::bootromTrampolineToBL2ElfPath,
            "Bootrom trampoline to BL2 ELF path")
    .def_rw("sp_bl2_elf_path", &emu::SysEmuOptions::spBL2ElfPath,
            "Service Processor BL2 ELF path")
    .def_rw("machine_minion_elf_path", &emu::SysEmuOptions::machineMinionElfPath,
            "Machine minion ELF path")
    .def_rw("master_minion_elf_path", &emu::SysEmuOptions::masterMinionElfPath,
            "Master minion ELF path")
    .def_rw("worker_minion_elf_path", &emu::SysEmuOptions::workerMinionElfPath,
            "Worker minion ELF path")
    .def_rw("executable_path", &emu::SysEmuOptions::executablePath,
            "Absolute path to sysemu executable")
    .def_rw("run_dir", &emu::SysEmuOptions::runDir,
            "Working directory for sysemu")
    .def_rw("max_cycles", &emu::SysEmuOptions::maxCycles,
            "Maximum cycles before finishing simulation")
    .def_rw("minion_shires_mask", &emu::SysEmuOptions::minionShiresMask,
            "Minion shire mask (enabled shires)")
    .def_rw("pu_uart0_path", &emu::SysEmuOptions::puUart0Path,
            "SysEmu PU UART0 TX log file path");

  // IDeviceLayer class (opaque pointer, not directly constructible)
  nb::class_<dev::IDeviceLayer>(m, "IDeviceLayer", "Device abstraction layer interface")
    .def("get_devices_count", &dev::IDeviceLayer::getDevicesCount,
         "Get number of available devices");

  // Factory functions for creating device layers
  m.def("create_pcie_device_layer",
        [](bool enable_master_minion, bool enable_service_processor) -> std::shared_ptr<dev::IDeviceLayer> {
          return dev::IDeviceLayer::createPcieDeviceLayer(enable_master_minion, enable_service_processor);
        },
        nb::arg("enable_master_minion") = true,
        nb::arg("enable_service_processor") = false,
        "Create PCIe device layer backend");

  m.def("create_sysemu_device_layer",
        [](const emu::SysEmuOptions& options, uint8_t num_devices) -> std::shared_ptr<dev::IDeviceLayer> {
          return dev::IDeviceLayer::createSysEmuDeviceLayer(options, num_devices);
        },
        nb::arg("options"),
        nb::arg("num_devices") = 1,
        "Create system emulator device layer backend (single config)");

  m.def("create_sysemu_device_layer",
        [](const std::vector<emu::SysEmuOptions>& options) -> std::shared_ptr<dev::IDeviceLayer> {
          return dev::IDeviceLayer::createSysEmuDeviceLayer(options);
        },
        nb::arg("options"),
        "Create system emulator device layer backend (multi-device config)");
}
