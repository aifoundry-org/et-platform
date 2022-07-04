# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

[[_TOC_]]
## [Unreleased]
### Added
- Enforce updation of CHANGELOG.md and patch version in merge request pipelines
### Changed
### Deprecated
### Removed
### Fixed
### Security

## [0.8.0]  - 2022-7-4
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
