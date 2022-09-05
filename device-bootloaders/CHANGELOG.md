# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

[[_TOC_]]

## [Unreleased]
### Added
- [SW-13953]: Adding eviction of SP stats and trace buffer to avoid trace header update delay
- [SW-14044]: Addition of op stats init function and refactoring update temperature stats function
### Changed
- [SW-13850] Disabled power throttling code in update_module_soc_power() until SW-13951 is resolved.
- Updating NOC, DDR and Maxion boot voltages
- [SW-13954] Add handling for DM_CMD_SET_MODULE_PART_NUMBER
- [SW-13954] Add handling for DM_CMD_SET_MODULE_VOLTAGE
- [SW-13954] Using PMIC voltage values for Maxion, PCIe Logic, VDDQ and VDDQLP.
### Deprecated
### Removed
### Fixed
- [SW-13571] Fixed watchdog timeout calculations with respect to pclk
- [SW-13974] Updating the PCIe driver to poll on bar addresses until assigned with delays.
- [SW-13978]: Refactored set frequency function
### Security

## [0.4.0] - 2022-08-16
### Added
- Added locks while configure/reset SP trace
- [SW-13253] Added support to read Minion, NOC and SRAM current from PMIC
- [SW-13035] Enforce updation of CHANGELOG.md and patch version in merge request pipelines
- [SW-13284] Adding locks to I2C driver and minor fixes in soc power
- [SW-13253] Adding functionality to read PMB stats from PMIC
- [SW-13253] Workaround for flash init for fast boot.
- [SW-13466] Porting over changes from silicon branch.
- [SW-6113] Add doxygen support for sp_host_iface.h and mm_iface.h.
- Extend support to read instantenous power, and added delay to PMIC reg access
- Updated DDR frequency from 933 Mhz to 1066 Mhz
- [SW-13620] Add implementation for DM_CMD_GET_SP_STATS
- [SW-13788] Forcing all shires to be initialized for silicon. Workaround for VPURF bug.
- [SW-13788] Clearing RF F0-F31 in VPU RF init.
### Changed
- [SW-11649] Separate out Trace and Serial log string max size.
- Conanfile now declares dependencies with version-ranges
- Changed DDR Frequency to 933MHz.
### Deprecated
### Removed
- [SW-12973] Auto reset of MM from MM heartbeat watchdog expiry.
- Removed Thermal_Pwr_Mgmt_Update_Sample_counter() since its no longer required.
### Fixed
- [SW-11649] Fixed usage of Trace string length to be not fixed size.
- [SW-13218] Reading memory size from flash in get_memory_size().
  * Reading dummy vendor ID until reading from chip register is fixed.
- [SW-13220] Updates to ensure the correct value is returned for PCIe max link speed. It was returning value for one older GEN.
- Override MNN, SRM, NOC voltage from SP to enable optimized operation point
- Throttle power state test to update the trace header.
- [SW-13253] updated get module voltage to use values from pvt
- [FV-296, SW-13251, SW-13273] SW WA for FV-296
- [SW-13273] Fix for PLL frequency calculation
- [SW-13253] Fixed delays for sysemu by introducing delay macro
- [FV-296] Fix of SW WA for FV-296
- [SW-13479] Fix for Zebu FAST_BOOT
- [SW-12973] Initialized the MM heartbeat timer after the PCIe initialization is complete.
- [SW-12853] Updated sequence changes in MM reset flow
- [SW-13583] Fixed updating completed boot counters
### Security

## [0.3.0] - 2022-6-25
### Added
### Changed
### Deprecated
### Removed
### Fixed
### Security

## [0.2.3] - 2022-6-24
### Added
- Initial version of changelog file.
- Logs for TDP get.
### Changed
### Deprecated
### Removed
### Fixed
### Security
