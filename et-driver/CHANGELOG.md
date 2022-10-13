# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

[[_TOC_]]

## [Unreleased]
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
