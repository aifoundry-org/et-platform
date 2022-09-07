# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

[[_TOC_]]
## [Unreleased]
### Added
- [SW-13832] Add support for SP2MM_CMD_GET_MM_STATS and start sampling without a delay.
- Adding support for ET_TRACE_READ_MEM and ET_TRACE_WRITE_MEM in MasterMinion.
### Changed
- [SW-13964] Aborting the execution of kernel on a hart if bus error is received.
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
