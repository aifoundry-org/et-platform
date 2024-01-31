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

## [0.22.0] - 2024-01-30
### Added
- [SW-19066] Add system calls for flushing entire L1 and L2
- [SW-19606] released sync.h
### Changed
### Deprecated
### Removed
### Fixed
- [SW-19739] Fixed the et_memcmp utility function
### Security

## [0.21.0] - 2023-12-15
### Added
### Changed
### Deprecated
### Removed
### Fixed
### Security

## [0.20.0] - 2023-11-07
### Added
- [SW-18547] Extending kernel launch message to add stack base and size.
### Changed
- [SW-17491] Run integration-tests on tag push and pass-on related options for docs generation
- [SW-18547] Change kernel launch msg args to hold stack base address and per hart stack size.
- Changes setting for SC PMC to make it more accurate
### Deprecated
### Removed
### Fixed
- [SW-18928] Fix clang compilation issues
- [SW-18522] Stop using non-portable function attributes for clang
### Security

## [0.19.0] - 2023-09-01
### Added
### Changed
- (Conan) Define compatibility of binaries form gcc --> clang
### Deprecated
### Removed
### Fixed
### Security

## [0.18.0] - 2023-08-03
### Added
- [SW-13951] reporting MM state with heartbeat
### Changed
- (CI) Update gitlab-ci-common poointer for Gitlab-CI 15 compatibility
- [SW-17876] Update conanfile.py & CI to gitlab.com
- Updating the conan dependencies versions to released components
### Deprecated
### Removed
### Fixed
### Security

## [0.17.0] - 2023-06-05
### Added
- [SW-17172] Quote arbitrary strings instead of func-name:code in profiling.
### Changed
### Deprecated
### Removed
### Fixed
- [CI] Fix gitlab-ci default branch
### Security

## [0.16.0] - 2023-04-19
### Added
- Add round up and round down help macros
### Changed
- Remove target level flags and add project level C flags
### Deprecated
### Removed
### Fixed
### Security

## [0.15.0] - 2023-03-15
### Added
- [SW-15710] Baseline implementation for Accelerator lib docs.
- [SW-15710] Adding a docs prototype with examples
- [SW-15805] Adding doxygen docs support for memory operations API with examples
- [SW-15803] Adding doxygen docs support for tensor operations API
- [SW-15806] Adding doxygen docs support for sync operations APIs
- [SW-16238] Added ABI header and kernel environment related datastructures.
- [SW-15063] Moving out non-generic C flags (-fno-zero-initialized-in-bss -ffunction-sections -fdata-sections) from toolchain config to consumer projects
### Changed
- [SW-16451] Generate pre-releases by default and full SemVer versions on tag pipelines
### Deprecated
### Removed
### Fixed
- [SW-15924] Support simultaneous profiling from the 2 threads of the minion w/o corruptions (using default API entries)
### Security

## [0.14.0] - 2023-01-10
### Added
- Added Trac_User_Profile_Event() for code instrumentation
- [SW-5479] Adding Trace Buffer Full message for CM-MM interface.
### Changed
- Changed the CI common repo hash to allow triggerring pipeline without MR.
- Updated DMA Max Element from 4 to 8
### Deprecated
### Removed
- Removed temporary define to pass the build.
- Removed temporary Conan job disables.
### Fixed
### Security

## [0.13.0] - 2022-12-15
### Added
- Added temporary define to pass the build.
### Changed
- Adding version ranges for Conan package requirements.
- Provide __assert_func() handler to support libc asserts
- [SW-13350] Replaced the usage of Trace_String() with Trace_Format_String().
### Deprecated
### Removed
- Removed old exec_cycles_t structure.
### Fixed
### Security

## [0.12.0] - 2022-11-22
### Added
- Adding new error codes for SP recoverable errors.
### Changed
- [SW-12171] Large buffer to be evicted per cache line
- [SW-10308] separate out SC and MS APIs to log PMC events
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
