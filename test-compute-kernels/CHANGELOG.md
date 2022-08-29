# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

[[_TOC_]]

## [Unreleased]

### Added
### Changed
- [SW-13248] Modifying bus error kernel to access non-existent ESR in U-mode.
- [SW-14007] Modifying bus error kernel to generate bus error interrupt on every hart.
- [SW-14007] Increasing the artifacts expiry from 1h to 1d in Gitlab CI.
### Deprecated
### Removed
### Fixed
### Security

## [1.2.0] - 2022-8-16
### Added
- [SW-13005] Workaround for MLP kernel.
- Added new Virus Kernels for Power measurement
### Changed
### Deprecated
### Removed
### Fixed
- [SW-13233] Coherency hazards fixed in MLP kernel and removed workaround.
- [SW-13685] FLB checker issues fixed in MLP kernel.
- Fix bug to remove cobbler which was causing registers to be backed up
### Security

## [1.1.0] - 2022-7-4
### Added
- Fixed bus error kernel, setting stride to 0
- Enforce updation of CHANGELOG.md and patch version in merge request pipelines
### Changed
### Deprecated
### Removed
### Fixed
### Security

## [1.0.0] - 2022-6-23
### Added
- Initial version of changelog file.
### Changed
### Deprecated
### Removed
### Fixed
### Security
