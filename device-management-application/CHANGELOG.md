# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

[[_TOC_]]

## [Unreleased]
### Added
- [SW-15117] et-powertop: add option to switch between devices
### Changed
### Deprecated
### Removed
### Fixed
### Security

## [1.7.0] - 2022-11-22
### Added
### Changed
- Replaced label PCIE_LOGIC with IOSHIRE(p075)
- Fixed conan recipe with VirtualRunEnv usage + modernize recipe to conan 2.0
### Deprecated
### Removed
### Fixed
- Fixed bug in handling SET_MODULE_VOTLAGE
- [SW-14679] dev_mngt_service: Fix argument for setting L2CACHE voltage
- Fixed device id ranges from 0-5 to 0-63
- [SW-15097] Fixing the display msg for L2 BW in et-power-top.
- Changing et_powertop to display SC Bank Vs PMC specific counter
### Security

## [1.6.0] - 2022-10-21
### Added
- [SW-13851] et-powertop: Add support to reset device stats.
- [SW-14061] dev_mngt_service: Added support for DM_CMD_GET_ASIC_VOLTAGE service.
- [SW-14061] et-powertop: Added support for displaying ASIC voltages.
- [SW-14337] dev_mngt_service: Added support for DM_CMD_RESET_ETSOC.
- [SW-14586] merge all trace operations into one option.
- [SW-14616] dev_mngt_service: Added option to print PCI_SLOT_NAME.
### Changed
- [SW-13832] Use DM_CMD_GET_MM_STATS instead of getTraceBufferServiceProcessor
- Changed other Power from 5.75W to 3.75W
- [FV-317] Updated asic_voltage element to be 16bits
- [SW-14409] update deviceLayer version for Conan
### Deprecated
### Removed
### Fixed
- Fix et-powertop regression of bar graph display of watts info via the 'w' command
- [SW-14061] Fixing the conversion of binary to mV and mV to binary voltage values.
- PCIE SS Voltage should use PCIe_LOGIC value
- [SW-14633] dev_mngt_service: Behavior should be same regardless of the arguments ordering.
### Security

## [1.5.0] - 2022-09-05
### Added
- [SW-13954] dev_mngt_service: Add options to set module partid and voltage.
- [SW-14096] Add parsing for SERIAL and PART numbers before printing them.
### Changed
- [SW-14107] update trace bin file names to be unique w.r.t time.
- Renaming PCIE and PCIE Logic voltage labels in et-power-top.
- [SW-14115] Converting module power from 10mW to W in dev_mgmt_service.
- [SW-14115] Converting module power from 10mW to mW in et-power-top.
### Deprecated
### Removed
### Fixed
- [SW-13978] dev_mngt_service: Intermittent failures while setting MINION,NOC frequencies.
- [SW-13978] Fixed get sp trace buffer to return error in case of buffer not is not retrieved.
### Security

## [1.4.0] - 2022-8-15
### Added
- ET-Top: Add 'w' interactive command to toggle display of watts in horizonal bar form.
- ET-Top: Add labels to the display of watts in horizonal bar form.
- ET-Top: Add MISC power at a constant 5.75 watts under the ETSOC power category.
- Conanfile & Conan CI
- [SW-13480]: Adding memshire frequency log print
- et-power-top: Printing compute utilization stats.
- ET-TOP: Print Device Id, FW version, voltages and frequencies.
- ET-TOP: Collect frequency and voltage statistics in every iteration if their display is enabled
- [SW-14044]: Updating power value to remove hex conversion
### Changed
- ET-TOP: Change the DELAY argument to take milliseconds instead of seconds (default: 100ms)
- ET-TOP: Print PCI DMA BW in MB/s instead of GB/s.
- [SW-13620] Use new DM_CMD_GET_SP_STATS instead of getTraceBufferServiceProcessor
- [SW-13736] Add batch and number of iterations options to et-powertop
### Deprecated
### Removed
### Fixed
- Typo in conanfile url metadata
- Warnings in device management service.
- Warnings in et-power-top.
### Security

## [1.3.0] - 2022-7-8
### Added
### Changed
- ET-Top: Remove -f dump trace stats buffer option and add interactive dump command.
- ET-Top: Add displayOpStats helper and add conversion from milliwatts to watts.
- Rename execute-able from et-top to et-powertop
### Deprecated
### Removed
### Fixed

## [1.2.0] - 2022-7-7
### Added
- ET-Top: Add -f dump trace stats buffer option and version info.
### Changed
### Deprecated
### Removed
- ET-Top: Remove use of routines from dlfcn.h.
### Fixed
- ET-Top: Decode until you find a valid packet.
### Security

## [1.1.0] - 2022-7-4
### Added
- Gitlab CI with clang format job along with sw-platform regressions
- Added -v (version) argument to get DM application version
- Clang formatter rules.
- Added support to extract SP and MM stats trace buffer
- Enforce updation of CHANGELOG.md and patch version in merge request pipelines
### Changed
- Changed back the sw-platform branch for CI back to develop/system-sw
- Ran clang formatter.
### Deprecated
### Removed
### Fixed
- Clang format to include .cc file type.
### Security

## [1.0.0] - 2022-6-24
### Added
- This CHANGELOG file.
### Changed
### Deprecated
### Removed
- Redundant readme file.
### Fixed
### Security
