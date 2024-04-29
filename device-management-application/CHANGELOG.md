# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

[[_TOC_]]

## [Unreleased]
### Added
### Changed
### Deprecated
### Removed
### Fixed
### Security

## [1.18.0] - 2024-04-29
- [SW-19030] Add new commands DM_CMD_SET_VMIN_LUT and DM_CMD_GET_VMIN_LUT
### Added
### Changed
### Deprecated
### Removed
### Fixed
- [SW-19030] Fix mempcy size
### Security

## [1.17.0] - 2024-03-27
### Added
- [CS-226] Add support for FRU
- add DM_CMD_GET_FRU output file
### Changed
- Make FRU raw data
- Initialize fruData
- [SW-20572]: split DDR SC and PCI BW to separate plots
- [SW-20572]: rename SC Bank BW -> SC BW
- [SW-20572]: Made voltage graph display voltage items on y axis
### Deprecated
### Removed
### Fixed
- Fixed bug in read FRU bin file
### Security

## [1.16.0] - 2024-01-30
### Added
### Changed
- Increased timeout for FW_UPDATE command.
### Deprecated
### Removed
### Fixed
### Security

## [1.15.0] - 2023-12-15
### Added
### Changed
- Generate packages with linux-ubuntu22.04-x86_64-gcc11-release and fix gcc11 build errors
- Updated FTXUI to use version 5.0.0 instead of 3.0.0
### Deprecated
### Removed
### Fixed
- Stop using FetchContent when importing ftxui dependency in Conan builds
- Fix case where perfMeasure array elements are acessesed before being populated
### Security

## [1.14.0] - 2023-11-07
### Added
- Added -g option to the et_powertop command to allow for graphical display of statistics.
- Added new files which include rendering related code for graphical display.
- [SW-15748] Implemented check DM events at service startup
### Changed
- Increased the FW update service timeout to facilitate PMIC firmware update.
- Enabled parallel collection of all performance metrics irrespective of the view being displayed
- [SW-18309] Removed TDP level check from DM service
- [SW-19093] Changing the FW update timeout to warning instead of error.
### Deprecated
### Removed
### Fixed
- Fixed bug in resizing Window
### Security

## [1.13.0] - 2023-09-01
### Added
### Changed
- [SW-18395] Maintaining a count of unparsed events in textual trace instead of logging each event separately.
### Deprecated
### Removed
### Fixed
- [SW-18234] Fixed TDP level range in DM service
### Security

## [1.12.0] - 2023-08-03
### Added
[SW-SW-17724] Implemented a check to ensure that the firmware update is completed within the specified time limit.
### Changed
- (CI) Update gitlab-ci-common pointer for Gitlab v15 compatibility
- [SW-17593] et-powertop: updated to include min/max/avg of new op_stats
- Updating the conan dependencies versions to released components
### Deprecated
### Removed
### Fixed
- [SW-13951] Fixed argument parsing for active power management cmd
- [CI] Adapt pipeline and conanfile.py to gitlab.com
### Security

## [1.11.0] - 2023-06-02
### Added
### Changed
### Deprecated
### Removed
### Fixed
- [SW-17348] et-powertop: Fix statistics display after statistics reset (-r)
### Security

## [1.10.0] - 2023-04-19
### Added
### Changed
### Deprecated
### Removed
### Fixed
- [SW-16770] Remove return true for SET/GET{SHIRE_CACHE_CONFIG}
### Security

## [1.9.0] - 2023-03-15
### Added
- [SW-16234] Adding shire cache config command
    - Shire cache configuration command to accept scp, l2 and l3 sizes in MB
### Changed
- [SW-15581] Define a default verbosity level for DM_VLOG.
- [SW-16446] (Conan) Generate full SemVer versions only when tagging the project. Generate pre-release otherwise.
             (CI) Enable tags pipelines and filter out MR "development/testing" jobs from release pipeline
- (Conan) Update required packages versions
### Deprecated
### Removed
### Fixed
### Security

## [1.8.0] - 2023-01-10
### Added
### Changed
- [SW-15175] Updating the asset output prints for assest tracking svcs
### Deprecated
### Removed
### Fixed
### Security

## [1.7.0] - 2022-11-22
### Added
- [SW-15117] et-powertop: add option to switch between devices
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
