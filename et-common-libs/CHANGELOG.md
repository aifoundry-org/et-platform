# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

[[_TOC_]]

## [Unreleased]
### Added
### Changed
- Adding version ranges for Conan package requirements.
- Provide __assert_func() handler to support libc asserts
### Deprecated
### Removed
### Fixed
### Security

## [0.12.0] - 2022-11-22
### Added
- Adding new error codes for SP recoverable errors.
### Changed
- [SW-12171] Large buffer to be evicted per cache line
### Deprecated
### Removed
- [SW-14349] Removed deprecated ET_TRACE_MEM_CPY config option from trace.
### Fixed
- [SW-14768] fix et_assert expr stringification
- [SW-15097] Fixing the SC PMC config value for L2 reads.
### Security

## [0.11.0] - 2022-10-20
### Added
- [SW-13851] Add SP2MM_CMD_MM_STATS_RUN_CONTROL command and response.
- [SW-14508] Adding get memory size command from MM to SP interface and replace HOST_MANAGED_DRAM_SIZE with ddr_get_memory_size
- [SW-14508] Removing hardcoded DRAM size and adding MAX size define to validate DDR size
- Added definitions for ET_TRACE_READ_MEM and ET_TRACE_WRITE_MEM.
### Changed
- [SW-14154]: Removing typecast for get current cycles
### Deprecated
### Removed
### Fixed
### Security

## [0.10.0] - 2022-09-02
### Added
- [SW-13832] Add SP2MM GET_MM_STATS command and response. Also fixed some typos.
- [SW-13832] Update SP2MM GET_MM_STATS command response to include a status field.
### Changed
### Deprecated
### Removed
### Fixed
### Security

## [0.9.0] - 2022-8-15
### Added
- Enforce updation of CHANGELOG.md and patch version in merge request pipelines
[SW-13750] New APIs in PMU to simultaneously sample all PMCs.
### Changed
- Optimized et_printf by removing extra vsnprintf call.
- Require esperantoTrace/0.6.0
### Deprecated
### Removed
### Fixed
[SW-13750] Invalid shire ID bug in PMU.
### Security

## [0.8.0] - 2022-7-4
### Added
### Changed
### Deprecated
### Removed
### Fixed
- Code smells.
### Security

## [0.7.0] - 2022-6-23
### Added
### Changed
### Deprecated
### Removed
### Fixed
### Security

## [0.0.7] - 2022-6-14
### Added
- New APIs in PMU to reset each type of PMCs.
### Changed
- configure_ms_pmcs() signature.
### Deprecated
### Removed
- Deprecated APIs from PMU.
- Deprecated Syscalls.
### Fixed
- (Conan) Recipe compatible with Conan >= 1.46.0
### Security

## [0.0.6] - 2022-5-30
### Added
### Changed
- Changed the interface of PMC sampling syscall
### Deprecated
### Removed
### Fixed
### Security

## [0.4.0] - 2022-3-06
### Added
- Adding RISC-V helper macros
### Changed
- Improved MM2SP error codes
### Deprecated
### Removed
### Fixed
### Security

## [0.3.0] - 2022-03-04
### Added
- Added SC PMC reset clear
- Added CM shires boot mask in L2 SCP
### Changed
- Clang format
- (Conan) RISC-V recipes compatible with x86_64 builds
- (Conan) Exposed bootloader components to consumers
### Deprecated
### Removed
- Removing IO reads to generate MSI
### Fixed
### Security

## [0.2.0] - 2022-02-17
### Added
- Added MM to CM Sync/Async command support
### Changed
### Deprecated
### Removed
### Fixed
### Security

## [0.1.0] - 2022-02-16
Initial version
### Added
- This CHANGELOG file.
- Conan recipe to package the project.
### Changed
### Deprecated
### Removed
### Fixed
### Security

[Unreleased]: https://gitlab.esperanto.ai/software/et-common-libs/-/compare/v0.4.0...master
[0.4.0]: https://gitlab.esperanto.ai/software/et-common-libs/-/compare/v0.3.0...v0.4.0
[0.3.0]: https://gitlab.esperanto.ai/software/et-common-libs/-/compare/v0.2.0...v0.3.0
[0.2.0]: https://gitlab.esperanto.ai/software/et-common-libs/-/compare/v0.1.0...v0.2.0
[0.1.0]: https://gitlab.esperanto.ai/software/et-common-libs/-/tags/v0.1.0
