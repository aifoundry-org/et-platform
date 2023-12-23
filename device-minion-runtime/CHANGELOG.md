# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

[[_TOC_]]


## [Unreleased]
### Added
### Changed
- Updated Minion Runtime documentation
### Deprecated
### Removed
### Fixed
- [SW-19533] Added support for device-minion-runtime to be clang compatible
### Security

## [0.20.0] - 2023-11-07
### Added
- Added a new SysCall to fliush all L1 and L2 with a single thread
- [SW-18547] Added support for custom U-mode kernel stack configuration.
### Changed
- [SW-18472] Reduced log message length for bus error
- Fixed handling of overflow for Stats worker
### Deprecated
### Removed
### Fixed
- [SW-18427] Fixed the stats worker to handle trace disabled case and fixed the null pointer evict issue.
- [SW-18522] Fixed minor build issues seen with clang
- [SW-18522] More minor fixes for clang builds
- [SW-18881] Fixing build warning.
### Security

## [0.19.0] - 2023-08-03
### Added
- [SW-13951] Reporting MM state with heartbeat
### Changed
- (CI) Update gitlab-ci-common pointer for Gitlab v15 compatibility
- [SW-17528] Use GCC primitives fir alignment in DIRs.
- Updating the conan dependencies versions to released components
### Deprecated
### Removed
### Fixed
- [CI] Adapt pipeline and conanfile.py to gitlab.com
### Security

## [0.18.0] - 2023-06-02
### Added
### Changed
### Deprecated
### Removed
### Fixed
- [SW-17162] Fixed SonarQube MAJOR and MINOR code smells.
### Security

## [0.17.0] - 2023-04-19
### Added
- [SW-16695] Added support for P2P node accessibility and host managed DRAM alignment requirements.
### Changed
- [SW-16847] Unifying the code for P2P and normal DMA commands.
### Deprecated
### Removed
### Fixed
### Security

## [0.16.0] - 2023-03-15
### Added
- [SW-16238] Adding support for kernel environment variables.
### Changed
- Updating the gitlab CI repo pointer to latest.
- [SW-15697] Replacing threshold_data with threshold value in trace buffer full notification.
- [SW-16430] Optimizing the kernel launch path to process command fields before waiting on idle slot search.
- [SW-16452] Generate pre-release packages by default and full versions in tags pipelines
- [SW-15063] Moving out non-generic C flags (-fno-zero-initialized-in-bss -ffunction-sections -fdata-sections) from toolchain config to consumer projects
### Deprecated
### Removed
### Fixed
- [SW-16370] Fixed the kernel launch assembly code to pass correct function attributes.
### Security

## [0.15.0] - 2023-01-10
### Added
- [SW-5479] Adding support for sending trace buffer full event to host.
### Changed
- Updating Conan package requirements.
- [SW-13350] Replaced the usage of Trace_String() with Trace_Format_String().
- [SW-13350] Replaced the usage of Trace_Format_String() with Trace_Format_String_V() to reduce code size in Master Minion.
- Update the lower limit of esperantoTrace in conan.
- Improved some logs.
- [SW-15614] Report threshold data in trace buffer full event.
### Deprecated
### Removed
### Fixed
- [SW-15464] Fixed the DDR and SC Bank bandwidth min max values.
- Fix compiler maybe-uninitialized warning.
- [SW-15576] Multiple fixes to U-mode tracing support.
    * Fixed buffer partioning for U-mode harts. Now there is no more holes.
    * Fixed base addresses for U-mode harts, specifically for last hart in a shire.
    * Fixed addressing of Master shire U-mode harts trace.
    * Added sanity checks on the trace config data.
    * Reorganized some utils in the FW.
### Security

## [0.14.0] - 2022-12-13
### Added
### Changed
### Deprecated
### Removed
### Fixed
- [SW-12171] Evict U-Mode trace only if it is enabled
- [SW-15282] Fixed the wait duration type from uint32_t to uint64_t.
### Security

## [0.13.0] - 2022-11-22
### Added
- [SW-14813]: Adding hart ID in MM exception log.
### Changed
- Conan recipe to use version ranges
### Deprecated
### Removed
- [SW-9022] Removed legacy DMA commands.
- [SW-14349] Removed deprecated ET_TRACE_MEM_CPY config option from trace.
- [SW-14813] Removing the MPROT config from Machine FW and removing the syscall for MPROT.
- Remove WA for CM reset on Zebu.
- [SW-13796] Halting PMU sampling while CM reset and adding sampling states
### Fixed
- [SW-14813] Fixed coherency issue in PCIe DMA driver and set the DRAM size to 16 GB default.
- Making all instance of Shire Cache PMU generic
### Security

## [0.12.0] - 2022-10-20
### Added
- [SW-13832] Add support for SP2MM_CMD_GET_MM_STATS and start sampling without a delay.
- Adding support for ET_TRACE_READ_MEM and ET_TRACE_WRITE_MEM in MasterMinion.
- [SW-13851] add handling for SP2MM_CMD_MM_STATS_RUN_CONTROL.
- [SW-14508] Adding get memory size command from MM to SP interface and replace HOST_MANAGED_DRAM_SIZE with ddr_get_memory_size
- [SW-14508] Adding support for dynamically configuring the DDR size in mprot.
### Changed
### Deprecated
### Removed
### Fixed
- [SW-14084] Fixing the CMA calculation to ceil and floor the values correctly.
- [SW-14290] Fix to address stats in et_powertop becoming zero
- [SW-14154]: Fix utilization calculation algorithm to handle execution scenarios beyond or before sampling interval
- [SW-14290] Adding the missing check for minimum stats value.
- [SW-14508] More fixes to DMA and KW utilization
### Security

## [0.11.0] - 2022-09-02
### Added
### Changed
- [SW-13964] Aborting the execution of kernel on a hart if bus error is received.
- Bumping the version of etsoc_hal in conanfile.
### Deprecated
### Removed
### Fixed
### Security

## [0.10.0] - 2022-08-12
### Added
- [SW-13352] DMA and Kernel utilization stats support.
### Changed
- [SW-13750] Using the new APIs from PMU to sample SC and MS PMCs.
### Deprecated
### Removed
### Fixed
- [SW-11649] Separate out Trace and Serial log string max size.
- [SW-13241] Read coherence issue in KW abort path.
- Fixed MM Stat buffer eviction.
- [SW-11649] Fixed usage of Trace string length to be not fixed size.
- [SW-13631] Coherency write hazard in stats worker while writing to L3.
- [SW-13640] Updating STAT worker to query for freq Vs hardcoding it.
- [SW-13825] Making sure that accesses to L3 are complete before evicting the lines before a kernel launch.
- [SW-13738]: Fixed algorithm to calculate DAMW & KW utilization
### Security

## [0.9.0] - 2022-7-7
### Added
- Enforce updation of CHANGELOG.md and patch version in merge request pipelines
### Changed
### Deprecated
### Removed
### Fixed
- Fixed software timer reload count at the time of expiry.
- Fixed MM Stat delta initialization.
### Security

## [0.8.0] - 2022-7-4
### Added
### Changed
- PU timer load count to 1 MHz.
- Updated MS and SC PMU counters sampling algorithm to log delta across sampling interval.
### Deprecated
### Removed
### Fixed
### Security

## [0.7.0]  - 2022-6-23
### Added
### Changed
### Deprecated
### Removed
### Fixed
### Security

## [0.6.5]  - 2022-6-23
### Added
- Enabled MS PMU Events for all Minions, IOShire, and PShire.
### Changed
- STAT worker Macro defines to unsighned long data type.
### Deprecated
### Removed
### Fixed
- Fixed STAT worker minimum comparison
### Security

## [0.6.4]  - 2022-6-20
### Added
### Changed
### Deprecated
### Removed
### Fixed
- Updated power mm rt management to use atomics while loading data
### Security

## [0.6.3]  - 2022-6-17
### Added
### Changed
### Deprecated
### Removed
### Fixed
- PU timer control register value was being overwritten. Fixed it.
### Security

## [0.6.2] - 2022-6-17
### Added
### Changed
### Deprecated
### Removed
### Fixed
- Fixed device stats minimum value initialization.
### Security

## [0.6.1] - 2022-6-16
### Added
### Changed
### Deprecated
### Removed
### Fixed
- [SW-12973] PU timer initialization and callback check for SW timer.
### Security

## [0.6.0] - 2022-6-15
### Added
- Initial version of changelog file.
### Changed
- Changed project version to 0.6.0
### Deprecated
### Removed
### Fixed
### Security
