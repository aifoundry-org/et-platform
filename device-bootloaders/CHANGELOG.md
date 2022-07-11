# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

[[_TOC_]]

## [Unreleased]
### Added
- Added locks while configure/reset SP trace
- [SW-13253] Added support to read Minion, NOC and SRAM current from PMIC
- [SW-13035] Enforce updation of CHANGELOG.md and patch version in merge request pipelines
- [SW-13284] Adding locks to I2C driver and minor fixes in soc power
### Changed
- Conanfile now declares dependencies with version-ranges
### Deprecated
### Removed
### Fixed
- [SW-13218] Reading memory size from flash in get_memory_size().
  * Reading dummy vendor ID until reading from chip register is fixed.
- [SW-13220] Updates to ensure the correct value is returned for PCIe max link speed. It was returning value for one older GEN.
- Override MNN, SRM, NOC voltage from SP to enable optimized operation point
- Throttle power state test to update the trace header.
- [SW-13253]: updated get module voltage to use values from pvt
- [FV-296, SW-13251, SW-13273] SW WA for FV-296
- [SW-13273] Fix for PLL frequency calculation
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
