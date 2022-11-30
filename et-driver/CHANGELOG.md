# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

[[_TOC_]]

## [Unreleased]
### Added
### Changed
- [SW-15181] `enum dev_state` will not be used internally for maintaining states. The usage is now limited to GET_DEVICE_STATE IOCTL only.
### Deprecated
### Removed
### Fixed
- [SW-15181] Fixed the following:
  * MM reset workflow when previous MM reset has failed
  * Race condition in restoring device state when reset command submission to SQ has failed
### Security

## [0.11.0] - 2022-11-23
### Added
### Changed
- [SW-14614] et_device_api: Changed wait time from u32 to u64
- [SW-9608b] et_device_api: Removed reserved IDs before DMA list commands
- [SW-14868] et_device_api: Removed dependency from device-ops-api message IDs.
- [SW-15075] Reducing device discovery timeouts to 100s
- [SW-15100] et_vma: avoid across device usage of VMA mapping.
- [SW-14867] Saving and restoring the entire PCIe config space for hot reset to remove potential performance degradation.
### Deprecated
### Removed
- [SW-9608] Removed legacy DMA read/write support
### Fixed
### Security

## [0.10.0] - 2022-10-20
### Added
- [SW-14288] Adding support for DM get asic voltage and set module voltage commands in loopback driver.
- [SW-14306] Add ETSOC reset support.
- Updated conanfile to get version from VERSION file.
### Changed
- [SW-14061] Updating the device mgmt api command codes and Get Module Voltage response name.
- (CI) Fetch latest conan docker image
- [SW-14154] Updating DMA response structures to match API structures
- [FV-317] Updating asic_voltage elements to 16bits
### Deprecated
### Removed
### Fixed
- [SW-14453] Fixed crash during driver loading when some msg was already available in the CQ. The spinlock had to be initialized before its usage.
- [SW-10058] Fixed crash during MM/CM trace pull using IO accesses.
### Security

## [0.9.0] - 2022-8-15
### Added
- Enforce updation of CHANGELOG.md and patch version in merge request pipelines
### Changed
- Shift from Jenkins to Gitlab CI
### Deprecated
### Removed
- mem_stats/clear sysfs attribute file
### Fixed
### Security

## [0.8.0] - 2022-6-23
### Added
### Changed
### Deprecated
### Removed
### Fixed
- Passing of LINUX_KMOD_LOCAL_BUILD argument to CI jobs
### Security

## [0.7.1] - 2022-6-23
### Added
- Initial version of changelog file.
### Changed
### Deprecated
### Removed
### Fixed
### Security
